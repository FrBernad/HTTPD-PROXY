#include "parsing_request_line.h"
#include "../../utils/net_utils.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "../../utils/connections.h"
#include "../../utils/connections_def.h"
#include "../../utils/doh_utils.h"

static unsigned
handle_origin_doh_connection(struct selector_key *key);

static unsigned
handle_origin_ip_connection(struct selector_key *key);

static void
build_connection_request(struct selector_key *key);

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
        request_state state = request_parser_feed(&requestLine->request_parser, c);
        if (state == request_error) {
            printf("BAD REQUEST\n");  //FIXME: DEVOLVER EN EL SOCKET AL CLIENTE BAD REQUEST
            break;
} else if (state == request_done) {
            unsigned nextState;
            if (requestLine->request.request_target.host_type == domain) {
                nextState = handle_origin_doh_connection(key);
            } else {
                nextState = handle_origin_ip_connection(key);
            }

            if (nextState != ERROR) {
                build_connection_request(key);
            }

            return nextState;
        }
    }

    return connection->stm.current->state;
}

static void
build_connection_request(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    connection_request_t *connectionRequest = &connection->connectionRequest;

    struct request_line requestLine = connection->client.request_line.request;

    sprintf((char *)connectionRequest->requestLine, "%s %s HTTP/1.0\r\n", requestLine.method, requestLine.request_target.origin_form);
    memcpy(&connectionRequest->host, &requestLine.request_target.host, sizeof(requestLine.request_target.host));
    connectionRequest->port = requestLine.request_target.port;
    connectionRequest->host_type = requestLine.request_target.host_type;
}

static unsigned
handle_origin_doh_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct sockaddr_in ipv4;

    ipv4.sin_family = AF_INET;
    ipv4.sin_port = htons(8053);                                 //FIXME: Cambiar
    if (inet_pton(AF_INET, "127.0.0.1", &ipv4.sin_addr) <= 0) {  //FIXME: poner el doh servidor
        return ERROR;                                            //FIXME: DOH SERVER ERROR
    }

    connection->origin_fd = establish_origin_connection(
        (struct sockaddr *)&ipv4,
        sizeof(ipv4),
        AF_INET);

    if (connection->origin_fd == -1) {
        return ERROR;
    }

    printf("registered doh origin!\n");

    selector_status status = register_origin_socket(key);

    if (status != SELECTOR_SUCCESS) {
        //FIXME: ver que onda;
    }

    return TRY_CONNECTION_IP;
}

static unsigned
handle_origin_ip_connection(struct selector_key *key) {
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


