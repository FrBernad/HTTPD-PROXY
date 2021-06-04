#include <arpa/inet.h>
#include <buffer.h>
#include <doh_parser.h>
#include <errno.h>
#include <request.h>
#include <selector.h>
#include <stdio.h>
#include <stdlib.h>
#include <stm.h>

#define ATTACHMENT(key) ((proxyConnection *)(key)->data)

enum conections_defaults {
    BUFFER_SIZE = 2048,
};

struct request_line_st {
    buffer *buffer;
    struct request_line request;
    struct request_parser request_parser;
};

struct doh_response_st {
    buffer *buffer;
    struct doh_response response;
    struct doh_response_parser dohParser;
};

typedef struct proxyConnection {
    /*Informacion del cliente*/
    struct sockaddr_storage client_addr;
    socklen_t client_addr_en;
    int client_fd;

    /* resolucion de la direccion del origin server*/
    struct addrinfo *origin_resolution;
    /*intento actual de la direccion del origin server*/
    struct addrinfo *origin_resolution_current;

    /*información del origin server*/
    struct sockaddr_storage origin_addr;
    socklen_t origin_addr_en;
    int origin_domain;
    int origin_fd;

    /*maquinas de estados*/
    struct state_machine stm;

    // estados para el cliente
    union client_state {
        struct request_line_st request_line;
        struct doh_response_st doh_response;
    } client;

    // estados para el origin
    // union{
    // estados
    // }

    buffer origin_buffer, client_buffer;

} proxyConnection;

enum connection_state {
    PARSING_HOST = 0,
    TRY_CONNECTION_IP,
    DOH_REQUEST,
    DOH_RESPONSE,
    TRY_CONNECTION_DOH_SERVER,
    DOH_RESOLVE_REQUEST_IPV4,
    DOH_RESOLVE_REQUEST_IPV6,

    CONNECTED,
    DONE,
    ERROR
};

static proxyConnection *
create_new_connection();
static void
init_proxy_connection_stm(struct state_machine * sm);
static fd_handler clientHandler;

void parsing_host_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    struct request_line_st *requestLine = &connection->client.request_line;

    requestLine->request_parser.request = &requestLine->request;
    requestLine->buffer = &connection->client_buffer;

    request_parser_init(&requestLine->request_parser);
}

unsigned
parsing_host_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct request_line_st *requestLine = &connection->client.request_line;
    /*Tengo que parsear la request del cliente para determinar a que host me conecto*/
    while (buffer_can_read(requestLine->buffer)) {
        uint8_t c = buffer_read(requestLine->buffer);
        putchar(c);  //FIXME: borrarlo
        request_state state = request_parser_feed(&requestLine->request_parser, c);
        if (state == request_error) {
            printf("BAD REQUEST\n");  //FIXME: DEVOLVER EN EL SOCKET AL CLIENTE BAD REQUEST
            break;
        } else if (state == request_done) {
            printf("REQUEST LINE PARSED!\nLine: %s\n", requestLine->request.method);
            return connection->stm.current->state;
            break;
        }
    }
    return connection->stm.current->state;
}

static const struct state_definition connection_states[] = {

    {
        .state = PARSING_HOST,
        .on_arrival = parsing_host_on_arrival,
        .on_read_ready = parsing_host_on_read_ready,
    },
    {
        .state = TRY_CONNECTION_IP,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_REQUEST,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_RESPONSE,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = TRY_CONNECTION_DOH_SERVER,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_RESOLVE_REQUEST_IPV4,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_RESOLVE_REQUEST_IPV6,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = CONNECTED,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DONE,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = ERROR,
        //  .on_arrival =,
        //  .on_write_ready =
    }};

static void
proxy_client_read(struct selector_key *key) {
    proxyConnection *client = ATTACHMENT(key);

    buffer *buffer = &client->client_buffer;

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
    stm_handler_read(&client->stm, key);
}

//FIXME: Listen IPV4 Listen IPV6
void accept_new_connection(struct selector_key *key) {
    printf("new connection!\n");

    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    int clientSocket;

    if ((clientSocket = accept(key->fd, (struct sockaddr *)&address, &addrlen)) < 0)
        printf("error accept\n");
    // ERROR_MANAGER("accept", clientSocket, errno);

    selector_fd_set_nio(clientSocket);

    proxyConnection *newConnection = create_new_connection();

    clientHandler.handle_read = proxy_client_read;
    clientHandler.handle_write = NULL;
    clientHandler.handle_close = NULL;
    clientHandler.handle_block = NULL;

    int status = selector_register(key->s, clientSocket, &clientHandler, OP_READ, newConnection);

    if (status != SELECTOR_SUCCESS) {
        //FIXME: END CONNECTION
    }
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

    init_proxy_connection_stm(&newConnection->stm);

    return newConnection;
}

static void
init_proxy_connection_stm(struct state_machine * stm) {
    stm_init(stm,PARSING_HOST,ERROR,connection_states);
}
