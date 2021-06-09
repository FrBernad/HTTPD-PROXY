#include "parsing_request_line.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "connections/connections.h"
#include "connections/connections_def.h"
#include "metrics/metrics.h"
#include "utils/doh/doh_utils.h"
#include "httpd.h"
#include "utils/net/net_utils.h"
#include "utils/sniffer/sniffer_utils.h"

static unsigned
handle_origin_ip_connection(struct selector_key *key);

static void
build_connection_request(struct selector_key *key);

static int
check_request_line_error(proxyConnection *connection, request_state state);

void parsing_host_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    struct request_line_st *requestLine = &connection->client.request_line;

    requestLine->request_parser.request = &requestLine->request;
    requestLine->buffer = &connection->client_buffer;

    request_parser_init(&requestLine->request_parser);

    struct http_args args = get_httpd_args();
    connection->sniffer.sniffEnabled = args.disectors_enabled;

    sniffer_parser_init(&connection->sniffer.sniffer_parser);
}

unsigned
parsing_host_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct request_line_st *requestLine = &connection->client.request_line;

    while (buffer_can_read(requestLine->buffer)) {
        request_state state = request_parser_feed(&requestLine->request_parser, buffer_read(requestLine->buffer));
        connection->sniffer.bytesToSniff--;

        if (check_request_line_error(connection, state)) {
            return ERROR;
        } else if (state == request_done) {
            unsigned nextState;

            if (requestLine->request.request_target.host_type == domain) {
                nextState = handle_origin_doh_connection(key);
            } else {
                nextState = handle_origin_ip_connection(key);
            }

            if (nextState != ERROR) {
                build_connection_request(key);
                if (connection->connectionRequest.connect) {
                    buffer_reset(&connection->client_buffer);
                } else {
                    if (connection->sniffer.sniffEnabled && connection->sniffer.bytesToSniff > 0) {
                        modify_sniffer_state(&connection->sniffer.sniffer_parser, sniff_http_authorization);
                        sniff_data(key);
                    }
                }
            }

            return nextState;
        }
    }

    return connection->stm.current->state;
}

static int
check_request_line_error(proxyConnection *connection, request_state state) {
    int error;
    switch (state) {
        case request_error_unsupported_method:
            connection->error = NOT_IMPLEMENTED;
            error = 1;
            break;

        case request_error_unsupported_version:
            connection->error = HTTP_VERSION_NOT_SUPPORTED;
            error = 1;
            break;

        case request_error:
            connection->error = BAD_REQUEST;
            error = 1;
            break;
        default:
            error = 0;
            break;
    }

    return error;
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
    connectionRequest->connect = strcmp((char *)connection->client.request_line.request.method, "CONNECT") == 0;
}

static unsigned
handle_origin_ip_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct request_target *request_target = &connection->client.request_line.request.request_target;

    if (request_target->host_type == ipv4) {
        printf("Connecting to ipv4\n");
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
        connection->error = INTERNAL_SERVER_ERROR;
        increase_failed_connections();
        return ERROR;
    }

    printf("Registered origin!\n");

    if (register_origin_socket(key) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        increase_failed_connections();
        return ERROR;
    }

    return TRY_CONNECTION_IP;
}
