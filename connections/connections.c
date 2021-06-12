#include "connections.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "connections_def.h"
#include "metrics/metrics.h"
#include "state_machine/stm_initializer.h"
#include "utils/selector/selector.h"
#include "logger/logger_utils.h"
#include "logger/logger.h"

// STATIC FUNCTIONS
static proxy_connection_t *
create_new_connection(int clientFd);

static void
proxy_client_write(struct selector_key *key);

static void
proxy_client_read(struct selector_key *key);

static void
proxy_client_close(struct selector_key *key);

static void
proxy_origin_write(struct selector_key *key);

static void
proxy_origin_read(struct selector_key *key);

static void
proxy_origin_close(struct selector_key *key);

static void
free_doh_connection(doh_connection_t *doh_connection);

static void
free_connection_data(proxy_connection_t *connection);

static void
close_proxy_connection(struct selector_key *key);

enum conections_defaults {
    BUFFER_SIZE = 1024 * 8,
};

static fd_handler client_handler;
static fd_handler origin_handler;

static double inactive_threshold;

void init_connections_manager(double threshold) {
    client_handler.handle_read = proxy_client_read;
    client_handler.handle_write = proxy_client_write;
    client_handler.handle_close = proxy_client_close;  //DEFINE
    client_handler.handle_block = NULL;

    origin_handler.handle_read = proxy_origin_read;
    origin_handler.handle_write = proxy_origin_write;
    origin_handler.handle_close = proxy_origin_close;
    origin_handler.handle_block = NULL;

    inactive_threshold = threshold;
}

/*
**     PROXY LISTENER SOCKET HANDLER FUNCTIONS
*/

void accept_new_connection(struct selector_key *key) {
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    int client_socket;

    if ((client_socket = accept(key->fd, (struct sockaddr *) &addr, &addrlen)) < 0) {
        fprintf(stderr, "Error accept\n");
        return;
    }

    if (selector_fd_set_nio(client_socket) < 0) {
        fprintf(stderr, "error setting new connection as NON-BLOCKIN\n");
        close(client_socket);
        return;
    }

    proxy_connection_t *new_connection = create_new_connection(client_socket);
    if (new_connection == NULL) {
        fprintf(stderr, "error creating new connection\n");
        close(client_socket);
        return;
    }

    new_connection->client_addr = addr;

    int status = selector_register(key->s, client_socket, &client_handler, OP_READ, new_connection);

    if (status != SELECTOR_SUCCESS) {
        fprintf(stderr, "error registering new fd\n");
        close(client_socket);
        free_connection_data(new_connection);
        return;
    }
}

static proxy_connection_t *
create_new_connection(int clientFd) {
    proxy_connection_t *new_connection = calloc(1, sizeof(proxy_connection_t));

    if (new_connection == NULL) {
        return NULL;
    }
    uint8_t *read_buffer = malloc(BUFFER_SIZE * sizeof(uint8_t));

    if (read_buffer == NULL) {
        free(new_connection);
        return NULL;
    }

    uint8_t *write_buffer = malloc(BUFFER_SIZE * sizeof(uint8_t));

    if (write_buffer == NULL) {
        free(new_connection);
        free(read_buffer);
        return NULL;
    }

    buffer_init(&new_connection->origin_buffer, BUFFER_SIZE, read_buffer);
    buffer_init(&new_connection->client_buffer, BUFFER_SIZE, write_buffer);

    init_connection_stm(&new_connection->stm);

    new_connection->last_action_time = time(NULL);

    new_connection->client_fd = clientFd;
    new_connection->client_status = ACTIVE_STATUS;
    new_connection->origin_status = INACTIVE_STATUS;

    return new_connection;
}

/*
**     PROXY CLIENT HANDLER FUNCTIONS
*/
static void
proxy_client_read(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    connection->last_action_time = time(NULL);

    buffer *buffer = &connection->client_buffer;

    size_t max_bytes;
    uint8_t *data = buffer_write_ptr(buffer, &max_bytes);

    ssize_t total_bytes = read(key->fd, data, max_bytes);

    if (total_bytes < 0) {
        close_proxy_connection(key);
    } else {
        /* Si el client no quiere mandar nada más, marco al client como que está cerrando y
                que envie los bytes que quedan en su buffer */
        if (total_bytes == 0) {
            if (stm_state(&connection->stm) < CONNECTED) {
                close_proxy_connection(key);
                return;
            }
            connection->client_status = CLOSING_STATUS;
        } else {
            unsigned currentState = stm_state(&connection->stm);
            if (currentState == PARSING_REQUEST_LINE || currentState == CONNECTED) {
                connection->bytes_to_analize = total_bytes;
            }
            buffer_write_adv(buffer, total_bytes);
        }
        stm_handler_read(&connection->stm, key);
    }
}

static void
proxy_client_write(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    connection->last_action_time = time(NULL);

    buffer *origin_buffer = &connection->origin_buffer;

    size_t max_bytes;
    uint8_t *data;

    data = buffer_read_ptr(origin_buffer, &max_bytes);

    ssize_t total_bytes = send(key->fd, data, max_bytes, MSG_NOSIGNAL);

    if (total_bytes > 0) {
        increase_bytes_received(total_bytes);
        buffer_read_adv(origin_buffer, total_bytes);
        stm_handler_write(&connection->stm, key);
    } else {
        close_proxy_connection(key);
    }
}

static void
proxy_client_close(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    close(connection->client_fd);
    free_connection_data(connection);
}

/*
**     PROXY ORIGIN HANDLER FUNCTIONS
*/

static void
proxy_origin_read(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    connection->last_action_time = time(NULL);

    buffer *buffer = &connection->origin_buffer;

    size_t max_bytes;
    uint8_t *data = buffer_write_ptr(buffer, &max_bytes);

    ssize_t total_bytes = read(key->fd, data, max_bytes);

    if (total_bytes < 0) {
        close_proxy_connection(key);
    }
        /* Si el origin no quiere mandar nada más, marco al origin como que está cerrando y
                    que envie los bytes que quedan en su buffer */
    else {
        if (total_bytes == 0) {
            connection->origin_status = CLOSING_STATUS;
        } else {
            unsigned currentState = stm_state(&connection->stm);
            if (currentState == PARSING_REQUEST_LINE || currentState == CONNECTED) {
                connection->bytes_to_analize = total_bytes;
            }
            buffer_write_adv(buffer, total_bytes);
        }
        stm_handler_read(&connection->stm, key);
    }
}

static void
proxy_origin_write(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    connection->last_action_time = time(NULL);

    buffer *origin_buffer = &connection->origin_buffer;
    buffer *client_buffer = &connection->client_buffer;

    unsigned state = stm_state(&connection->stm);

    if (state == TRY_CONNECTION_IP) {
        // ME FIJO SI LA CONEXION FUE EXITOSA
        stm_handler_write(&connection->stm, key);
        return;
    }

    size_t max_bytes;
    uint8_t *data;
    ssize_t total_bytes;

    if (state == SEND_REQUEST_LINE || state == SEND_DOH_REQUEST) {
        // LEO DEL BUFFER DEL SERVER PORQUE ES DONDE CARGAMOS LA REQUEST
        data = buffer_read_ptr(origin_buffer, &max_bytes);

        if ((total_bytes = send(key->fd, data, max_bytes, MSG_NOSIGNAL)) > 0) {
            increase_bytes_sent(total_bytes);
            buffer_read_adv(origin_buffer, total_bytes);
            stm_handler_write(&connection->stm, key);
        } else {
            close_proxy_connection(key);
        }

        return;
    }

    data = buffer_read_ptr(client_buffer, &max_bytes);

    if ((total_bytes = send(key->fd, data, max_bytes, MSG_NOSIGNAL)) > 0) {
        increase_bytes_sent(total_bytes);
        buffer_read_adv(client_buffer, total_bytes);
        stm_handler_write(&connection->stm, key);
    } else {
        close_proxy_connection(key);
    }
}

static void
proxy_origin_close(struct selector_key *key) {
    close(key->fd);
}

int register_origin_socket(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    connection->origin_status = ACTIVE_STATUS;
    return selector_register(key->s, connection->origin_fd, &origin_handler, OP_WRITE, connection);
}

/*
**     PROXY FREE RESOURCES FUNCTIONS
*/

static void
close_proxy_connection(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    if (connection->origin_status != INACTIVE_STATUS) {
        selector_unregister_fd(key->s, connection->origin_fd);
    }
    selector_unregister_fd(key->s, connection->client_fd);
}

static void
free_doh_connection(doh_connection_t *doh_connection) {
    if (doh_connection->is_active) {
        doh_response_parser_destroy(&doh_connection->doh_parser);
    }
    free(doh_connection);
}

static void
free_connection_data(proxy_connection_t *connection) {
    free(connection->origin_buffer.data);
    free(connection->client_buffer.data);
    if (connection->sniffer.sniffer_parser.parser_is_set) {
        free(connection->sniffer.sniffer_parser.string_parser);
    }
    if (connection->doh_connection != NULL) {
        free_doh_connection(connection->doh_connection);
    }
    free(connection);
}

uint64_t get_buffer_size() {
    return BUFFER_SIZE;
}

/*
**     PROXY GARBAGE COLLECTOR FUNCTION
*/
void
connection_garbage_collect(struct selector_key *key) {

    // proxy/management listenting socket or logger fd
    if (key->data == NULL || key->fd == STDERR_FILENO || key->fd == STDOUT_FILENO) {
        return;
    }

    proxy_connection_t *connection = ATTACHMENT(key);

    // if inactive close connection
    if (connection->client_fd == key->fd && difftime(time(NULL), connection->last_action_time) >= inactive_threshold) {
        close_proxy_connection(key);
    }
}
