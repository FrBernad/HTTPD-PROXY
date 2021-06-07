#include "connections.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "connections_def.h"
#include "parsers/request_line_parser.h"
#include "selector.h"
#include "state_machine/stm_initializer.h"

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
    BUFFER_SIZE = 2048,
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

//FIXME: Listen IPV4 Listen IPV6
void accept_new_connection(struct selector_key *key) {
    printf("new connection!\n");

    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    int clientSocket;

    if ((clientSocket = accept(key->fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Error accept\n");  //
        return;
    }

    if (selector_fd_set_nio(clientSocket) < 0) {
        perror("error setting new connection as NON-BLOCKIN\n");
        close(clientSocket);
        return;
    }

    proxyConnection *newConnection = create_new_connection(clientSocket);

    if (newConnection == NULL) {
        perror("error creating new connection\n");
        close(clientSocket);
        return;
    }

    int status = selector_register(key->s, clientSocket, &clientHandler, OP_READ, newConnection);

    if (status != SELECTOR_SUCCESS) {
        perror("error registering new fd\n");
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

    int totalBytes = read(key->fd, data, maxBytes);

    if (totalBytes < 0) {
        close_proxy_connection(key);
    }
    /* Si el client no quiere mandar nada más, marco al client como que está cerrando y 
    que envie los bytes que quedan en su buffer */
    else {
        if (totalBytes == 0) {
            connection->client_status = CLOSING_STATUS;
            //FIXME: cerrar la conexion (tener en cuenta lo que dijo Juan del CTRL+C)
        }
        buffer_write_adv(buffer, totalBytes);
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

    int totalBytes = send(key->fd, data, maxBytes, 0);

    if (totalBytes > 0) {
        buffer_read_adv(originBuffer, totalBytes);
        stm_handler_write(&connection->stm, key);
    } else {
        close_proxy_connection(key);
    }
}

static void
proxy_client_close(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    free_connection_data(connection);
    close(connection->client_fd);

    free(connection);
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

    int totalBytes = read(key->fd, data, maxBytes);

    if (totalBytes < 0) {
        close_proxy_connection(key);
    }
    /* Si el origin no quiere mandar nada más, marco al origin como que está cerrando y 
    que envie los bytes que quedan en su buffer */
    else {
        if (totalBytes == 0) {
            connection->origin_status = CLOSING_STATUS;
        }
        buffer_write_adv(buffer, totalBytes);
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
    int totalBytes;

    if (state == SEND_REQUEST_LINE || state == SEND_DOH_REQUEST) {
        // LEO DEL BUFFER DEL SERVER PORQUE ES DONDE CARGAMOS LA REQUEST
        data = buffer_read_ptr(originBuffer, &maxBytes);

        if ((totalBytes = send(key->fd, data, maxBytes, 0)) > 0) {
            buffer_read_adv(originBuffer, totalBytes);
            stm_handler_write(&connection->stm, key);
        } else {
            close_proxy_connection(key);
        }

        return;
    }

    data = buffer_read_ptr(clientBuffer, &maxBytes);

    if ((totalBytes = send(key->fd, data, maxBytes, 0)) > 0) {
        buffer_read_adv(clientBuffer, totalBytes);
        stm_handler_write(&connection->stm, key);
    } else {
        close_proxy_connection(key);
    }
}

static void
proxy_origin_close(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    close(connection->origin_fd);
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
    doh_response_parser_destroy(&dohConnection->dohParser);
    free(dohConnection);
}

static void
free_connection_data(proxyConnection *connection) {
    free(connection->origin_buffer.data);
    free(connection->client_buffer.data);
    if (connection->dohConnection != NULL) {
        free_doh_connection(connection->dohConnection);
    }
    free(connection);
}
