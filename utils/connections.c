#include "connections.h"
#include "../state_machine/stm_initializer.h"
#include "../parsers/request.h"
#include "selector.h"
#include "connections_def.h"

#include <arpa/inet.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

// STATIC FUNCTIONS
static proxyConnection *
create_new_connection();

static void 
proxy_origin_write(struct selector_key *key);

static void
proxy_client_read(struct selector_key *key);

enum conections_defaults {
    BUFFER_SIZE = 2048,
};

static fd_handler clientHandler;
static fd_handler originHandler;

void 
init_selector_handlers() {
    clientHandler.handle_read = proxy_client_read;
    clientHandler.handle_write = NULL;
    clientHandler.handle_close = NULL;
    clientHandler.handle_block = NULL;

    originHandler.handle_read = NULL;
    originHandler.handle_write = proxy_origin_write;
    originHandler.handle_close = NULL;
    originHandler.handle_block = NULL;
}

//FIXME: Listen IPV4 Listen IPV6
void 
accept_new_connection(struct selector_key *key) {
    printf("new connection!\n");

    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    int clientSocket;

    if ((clientSocket = accept(key->fd, (struct sockaddr *)&address, &addrlen)) < 0)
        printf("error accept\n");
    // ERROR_MANAGER("accept", clientSocket, errno);

    selector_fd_set_nio(clientSocket);

    proxyConnection *newConnection = create_new_connection();

    int status = selector_register(key->s, clientSocket, &clientHandler, OP_READ, newConnection);

    if (status != SELECTOR_SUCCESS) {
        //FIXME: END CONNECTION
    }
}


int 
register_origin_socket(struct selector_key *key){
    proxyConnection * connection = ATTACHMENT(key);
    return selector_register(key->s, connection->origin_fd, &originHandler, OP_WRITE, connection);
}

static proxyConnection *
create_new_connection() {

    proxyConnection *newConnection = calloc(1, sizeof(proxyConnection));

    if (newConnection == NULL) {
        printf("error calloc\n");
        // ERROR_MANAGER("calloc", -1, errno);
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

    return newConnection;
}

static void
proxy_client_read(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->client_buffer;

    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);

    int totalBytes = read(key->fd, data, maxBytes);
    if (totalBytes < 0)
        printf("ERROR!!!.\n\n");
    if (totalBytes == 0) {
        //     //FIXME: cerrar la conexion (tener en cuenta lo que dijo Juan del CTRL+C)

        printf("Connection closed.\n\n");
    }

    buffer_write_adv(buffer, totalBytes);
    stm_handler_read(&connection->stm, key);
}

static void 
proxy_origin_write(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    buffer *buffer = &connection->origin_buffer;

    unsigned state;

    if (!buffer_can_read(buffer)) {
        if (state = stm_handler_write(&connection->stm, key), state == DONE) {
            printf("TERMINE!!\n");
        }
        printf("A VER SI ME CONECNTE??\n");
    }
}