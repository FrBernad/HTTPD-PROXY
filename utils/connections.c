#include <arpa/inet.h>
#include <buffer.h>
#include <connections_def.h>
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
init_proxy_connection_stm(struct state_machine *sm);
static fd_handler clientHandler;

void parsing_host_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    struct request_line_st *requestLine = &connection->client.request_line;

    requestLine->request_parser.request = &requestLine->request;
    requestLine->buffer = &connection->client_buffer;

    request_parser_init(&requestLine->request_parser);
}

void parsing_host_on_departure(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    //ARREGLARIA EL BUFFER
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
            if (requestLine->request.request_target.host_type == domain) {
                printf("DOH!!");
            } else {
                return handle_origin_connection(key);
            }
        }
        return connection->stm.current->state;
    }
}

static connection_state handle_origin_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    struct request_target *request_target = &connection->requestline.request.request_target;

    if (request_target->host_type == ipv4) {
        connection->origin_fd = establish_origin_connection((struct sockaddr * addr) request_target->host.ipv4, sizeof(request_target->host.ipv4));
    } else {
        connection->origin_fd = establish_origin_connection((struct sockaddr * addr) request_target->host.ipv6, sizeof(request_target->host.ipv6))
    }

    if (connection->origin_fd==-1){
        return ERROR;
    }

    selector_register(key->s, connection->origin_fd, NULL, OP_WRITE, connection);

    return TRY_CONNECTION_IP;
}

static int establish_origin_connection(struct sockaddr *addr, socklen_t addrlen) {
    int originSocket = socket(domain, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    if (originSocket < 0)
        return -1;

    int opt = 1;

    if (setsockopt(originSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return -1;

    if (connect(originSocket, addr, addrlen) < 0) {
        if (errno != EINPROGRESS)
            return -1;  //FIXME: aca habria que ver que hacer
    }
    return originSocket;
}

void try_connection_ip_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (selector_set_interest(key->s, connection->client_fd, OP_NOOP) != SELECTOR_SUCCESS) {
        printf("error set interest!");
        //VER QUE HACER
    }
    if (selector_set_interest(key->s, connection->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        printf("error set interest!");
        //VER QUE HACER
    }

}

static int checkOriginConnection(int socketfd) {
    int opt;
    if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &opt, sizeof(opt)) < 0)
        return false;
    // printf("opt val: %d, fd: %ld\n\n", opt, i);
    // MATAR EL SOCKET Y AVISARLE AL CLIENT SI ES QUE NO HAY MAS IPS.SI NO PROBAR CON LA LISTA DE IPS 
    if (opt != 0) {
        printf("Connection could not be established, closing sockets.\n\n");
        return false;
    }

    return true;
}

unsigned
try_connection_ip_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if(checkOriginConnection(connection->origin_fd)_
        return CONNECTED;

}

static const struct state_definition connection_states[] = {

    {
        .state = PARSING_HOST,
        .on_arrival = parsing_host_on_arrival,
        .on_read_ready = parsing_host_on_read_ready,
    },
    {
        .state = TRY_CONNECTION_IP,
        .on_arrival = try_connection_ip_on_arrival,
        .on_write = try_connection_ip_on_write_ready,
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
init_proxy_connection_stm(struct state_machine *stm) {
    stm_init(stm, PARSING_HOST, ERROR, connection_states);
}
