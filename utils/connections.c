#include <arpa/inet.h>
#include <buffer.h>
#include <errno.h>
#include <request.h>
#include <selector.h>
#include <stm.h>
#include <doh_parser.h>

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
    union {
        struct request_line_st request_line;
        struct doh_response_st doh_response;
    } client;

    // estados para el origin
    // union{
    // estados
    // }

    buffer origin_buffer, client_buffer;

} proxyConnection;

#define ATTACHMENT(key) ((proxyConnection *)(key)->data)

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

struct request_line_st {
    buffer *buffer;
    struct request_line request;
    struct request_parser request_parser;
};

void
parsing_host_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection * connection = ATTACHMENT(key);
    connection->client.request_line.request_parser = &connection->client.request_line.request;
    request_parser_init(&connection->client.request_line);

}

static const struct state_definition connection_states[] = {

    {.state = PARSING_HOST, 
        .on_arrival = ,
            request_line = 
        )
        .on_read_ready = 
        },
    {.state = TRY_CONNECTION_IP, 
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = DOH_REQUEST, 
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = DOH_RESPONSE, 
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = TRY_CONNECTION_DOH_SERVER, 
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = DOH_RESOLVE_REQUEST_IPV4, 
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = DOH_RESOLVE_REQUEST_IPV6, 
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = CONNECTED,
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = DONE,
        .on_arrival =, 
        .on_read_ready = 
        },
    {.state = ERROR,
        .on_arrival = , 
        .on_write_ready = 
        },

};

enum conections_defaults {
    BUFFER_SIZE = 2048,
};

struct request_line_st {
    buffer *buffer;

    struct request_line request;
    struct request_parser;
};

struct doh_response_st {
    buffer *buffer;
    struct doh_response response;
    struct doh_response_parser dohParser;
};

//FIXME: Listen IPV4 Listen IPV6
void accept_new_connection(struct selector_key *key) {
    printf("new connection!\n");

    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    int clientSocket;

    if ((clientSocket = accept(key->fd, (struct sockaddr *)&address, &addrlen)) < 0)
        ERROR_MANAGER("accept", clientSocket, errno);

    selector_fd_set_nio(clientSocket);

    proxyConnection *newConnection = create_new_connection();

    int status = selector_register(key->s, clientSocket, NULL, OP_READ, newConnection);

    if (status != SELECTOR_SUCCESS) {
        //FIXME: END CONNECTION
    }
}

static proxyConnection *
create_new_connection() {
    proxyConnection *newConnection = calloc(1, sizeof(proxyConnection));

    if (newConnection == NULL) {
        ERROR_MANAGER("calloc", -1, errno);
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

    return newConnection;
}