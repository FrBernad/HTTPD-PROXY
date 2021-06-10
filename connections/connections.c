#include "connections.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "connections_def.h"
#include "metrics/metrics.h"
#include "state_machine/stm_initializer.h"
#include "utils/selector/selector.h"

// STATIC FUNCTIONS
static proxyConnection *
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
free_doh_connection(doh_connection_t *dohConnection);

static void
free_connection_data(proxyConnection *connection);

static void
close_proxy_connection(struct selector_key *key);

enum conections_defaults {
    BUFFER_SIZE = 1024*8,
};

static fd_handler clientHandler;
static fd_handler originHandler;

void init_selector_handlers() {
    clientHandler.handle_read = proxy_client_read;
    clientHandler.handle_write = proxy_client_write;
    clientHandler.handle_close = proxy_client_close;  //DEFINE
    clientHandler.handle_block = NULL;

    originHandler.handle_read = proxy_origin_read;
    originHandler.handle_write = proxy_origin_write;
    originHandler.handle_close = proxy_origin_close;
    originHandler.handle_block = NULL;
}

/*
**     PROXY LISTENER SOCKET HANDLER FUNCTIONS
*/

void
accept_new_connection(struct selector_key *key) {
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    int clientSocket;

    if ((clientSocket = accept(key->fd, (struct sockaddr *)&addr, &addrlen)) < 0) {
        fprintf(stderr, "Error accept\n");
        return;
    }

    if (selector_fd_set_nio(clientSocket) < 0) {
        fprintf(stderr, "error setting new connection as NON-BLOCKIN\n");
        close(clientSocket);
        return;
    }

    proxyConnection *newConnection = create_new_connection(clientSocket);
    if (newConnection == NULL) {
        fprintf(stderr, "error creating new connection\n");
        close(clientSocket);
        return;
    }   

    newConnection->client_addr = addr;

    int status = selector_register(key->s, clientSocket, &clientHandler, OP_READ, newConnection);

    if (status != SELECTOR_SUCCESS) {
        fprintf(stderr, "error registering new fd\n");
        close(clientSocket);
        free_connection_data(newConnection);
        return;
    }


}

static proxyConnection *
create_new_connection(int clientFd) {
    proxyConnection *newConnection = calloc(1, sizeof(proxyConnection));

    if (newConnection == NULL) {
        return NULL;
    }
    uint8_t *readBuffer = malloc(BUFFER_SIZE * sizeof(uint8_t));

    if (readBuffer == NULL) {
        free(newConnection);
        return NULL;
    }

    uint8_t *writeBuffer = malloc(BUFFER_SIZE * sizeof(uint8_t));

    if (writeBuffer == NULL) {
        free(newConnection);
        free(readBuffer);
        return NULL;
    }

    buffer_init(&newConnection->origin_buffer, BUFFER_SIZE, readBuffer);
    buffer_init(&newConnection->client_buffer, BUFFER_SIZE, writeBuffer);

    init_connection_stm(&newConnection->stm);

    newConnection->client_fd = clientFd;
    newConnection->client_status = ACTIVE_STATUS;
    newConnection->origin_status = INACTIVE_STATUS;

    return newConnection;
}

/*
**     PROXY CLIENT HANDLER FUNCTIONS
*/
static void
proxy_client_read(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->client_buffer;

    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);

    ssize_t totalBytes = read(key->fd, data, maxBytes);

    if (totalBytes < 0) {
        close_proxy_connection(key);
    }
    /* Si el client no quiere mandar nada más, marco al client como que está cerrando y
            que envie los bytes que quedan en su buffer */
    else {
        if (totalBytes == 0) {
            connection->client_status = CLOSING_STATUS;
            //FIXME: cerrar la conexion (tener en cuenta lo que dijo Juan del CTRL+C)
        } else {
            unsigned currentState = stm_state(&connection->stm);
            if (currentState == PARSING_REQUEST_LINE || currentState == CONNECTED) {
                connection->sniffer.bytesToSniff = totalBytes;
            }
            buffer_write_adv(buffer, totalBytes);
        }
        stm_handler_read(&connection->stm, key);
    }
}

static void
proxy_client_write(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;

    if (!buffer_can_read(originBuffer)) {
        printf("Algo malo paso\n");
    }

    size_t maxBytes;
    uint8_t *data;

    data = buffer_read_ptr(originBuffer, &maxBytes);

    ssize_t totalBytes = send(key->fd, data, maxBytes, 0);

    if (totalBytes > 0) {
        increase_bytes_received(totalBytes);
        buffer_read_adv(originBuffer, totalBytes);
        stm_handler_write(&connection->stm, key);
    } else {
        close_proxy_connection(key);
    }
}

static void
proxy_client_close(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    close(connection->client_fd);
    free_connection_data(connection);
}

/*
**     PROXY ORIGIN HANDLER FUNCTIONS
*/

static void
proxy_origin_read(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;

    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);

    ssize_t totalBytes = read(key->fd, data, maxBytes);

    if (totalBytes < 0) {
        close_proxy_connection(key);
    }
    /* Si el origin no quiere mandar nada más, marco al origin como que está cerrando y
            que envie los bytes que quedan en su buffer */
    else {
        if (totalBytes == 0) {
            connection->origin_status = CLOSING_STATUS;
        } else {
            unsigned currentState = stm_state(&connection->stm);
            if (currentState == PARSING_REQUEST_LINE || currentState == CONNECTED) {
                connection->sniffer.bytesToSniff = totalBytes;
            }
            buffer_write_adv(buffer, totalBytes);
        }
        stm_handler_read(&connection->stm, key);
    }
}

static void
proxy_origin_write(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;
    buffer *clientBuffer = &connection->client_buffer;

    unsigned state = stm_state(&connection->stm);

    if (state == TRY_CONNECTION_IP) {
        // ME FIJO SI LA CONEXION FUE EXITOSA
        stm_handler_write(&connection->stm, key);
        return;
    }

    size_t maxBytes;
    uint8_t *data;
    ssize_t totalBytes;

    if (state == SEND_REQUEST_LINE || state == SEND_DOH_REQUEST) {
        // LEO DEL BUFFER DEL SERVER PORQUE ES DONDE CARGAMOS LA REQUEST
        data = buffer_read_ptr(originBuffer, &maxBytes);

        if ((totalBytes = send(key->fd, data, maxBytes, 0)) > 0) {
            increase_bytes_sent(totalBytes);
            buffer_read_adv(originBuffer, totalBytes);
            stm_handler_write(&connection->stm, key);
        } else {
            close_proxy_connection(key);
        }

        return;
    }

    data = buffer_read_ptr(clientBuffer, &maxBytes);

    if ((totalBytes = send(key->fd, data, maxBytes, 0)) > 0) {
        increase_bytes_sent(totalBytes);
        buffer_read_adv(clientBuffer, totalBytes);
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
    proxyConnection *connection = ATTACHMENT(key);
    connection->origin_status = ACTIVE_STATUS;
    return selector_register(key->s, connection->origin_fd, &originHandler, OP_WRITE, connection);
}

/*
**     PROXY FREE RESOURCES FUNCTIONS
*/

static void
close_proxy_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    proxy_client_close(key);
    if (connection->origin_status != INACTIVE_STATUS) {
        proxy_origin_close(key);
    }
}

static void
free_doh_connection(doh_connection_t *dohConnection) {
    if (dohConnection->isActive) {
        doh_response_parser_destroy(&dohConnection->dohParser);
    }
    free(dohConnection);
}

static void
free_connection_data(proxyConnection *connection) {
    free(connection->origin_buffer.data);
    free(connection->client_buffer.data);
    if (connection->sniffer.sniffer_parser.parserIsSet) {
        free(connection->sniffer.sniffer_parser.stringParser);
    }
    if (connection->dohConnection != NULL) {
        free_doh_connection(connection->dohConnection);
    }
    free(connection);
}