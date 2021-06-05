#include "parsing_host.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "../../utils/connections.h"
#include "../../utils/connections_def.h"

static unsigned 
handle_origin_connection(struct selector_key *key);
static int 
establish_origin_connection(struct sockaddr *addr, socklen_t addrlen, int protocol);
static void 
build_request_line(struct selector_key *key);

void 
parsing_host_on_arrival(const unsigned state, struct selector_key *key) {
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
        request_state state = request_parser_feed(&requestLine->request_parser, c);
        if (state == request_error) {
            
            printf("BAD REQUEST\n");  //FIXME: DEVOLVER EN EL SOCKET AL CLIENTE BAD REQUEST
            
            break;
        } else if (state == request_done) {
            unsigned nextState;

            if (requestLine->request.request_target.host_type == domain) {
                printf("DOH!!");
                nextState = DOH_REQUEST;
            } else {
                nextState = handle_origin_connection(key);
            }

            build_request_line(key);

            return nextState;
        }
    }

    return connection->stm.current->state;
}

static void 
build_request_line(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    uint8_t *buffer = connection->requestLine;
    struct request_line requestLine = connection->client.request_line.request;
    sprintf((char *)buffer, "%s %s HTTP/1.0\r\n", requestLine.method, requestLine.request_target.origin_form);
    printf("FIRST LINE: %s", buffer);
}

static unsigned 
handle_origin_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct request_target *request_target = &connection->client.request_line.request.request_target;

    if (request_target->host_type == ipv4) {
        printf("connecting to ipv4\n");
        connection->origin_fd = establish_origin_connection(
            (struct sockaddr *)&request_target->host.ipv4,
            sizeof(request_target->host.ipv4),
            AF_INET);
    } else {
        connection->origin_fd = establish_origin_connection(
            (struct sockaddr *)&request_target->host.ipv6,
            sizeof(request_target->host.ipv6),
            AF_INET6);
    }

    if (connection->origin_fd == -1) {
        return ERROR;
    }

    printf("registered origin!\n");

    selector_status status = register_origin_socket(key);

    if (status != SELECTOR_SUCCESS) {
        //FIXME: ver que onda;
    }

    return TRY_CONNECTION_IP;
}

static int
establish_origin_connection(struct sockaddr *addr, socklen_t addrlen, int protocol) {
    int originSocket = socket(protocol, SOCK_STREAM, IPPROTO_TCP);

    if (originSocket < 0)
        return -1;

    int opt = 1;

    selector_fd_set_nio(originSocket);

    if (setsockopt(originSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return -1;

    if (connect(originSocket, addr, addrlen) < 0) {
        if (errno != EINPROGRESS)
            return -1;  //FIXME: aca habria que ver que hacer
    }
    return originSocket;
}
