#include "parsing_request_line.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "connections/connections.h"
#include "connections/connections_def.h"
#include "httpd.h"
#include "logger/logger_utils.h"
#include "metrics/metrics.h"
#include "utils/doh/doh_utils.h"
#include "utils/net/net_utils.h"
#include "utils/sniffer/sniffer_utils.h"

static unsigned
handle_origin_ip_connection(struct selector_key *key);

static void
build_connection_request(struct selector_key *key);

static int
check_request_line_error(proxy_connection_t *connection, request_state state);

void parsing_host_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    struct request_line_t *request_line = &connection->connection_parsers.request_line;

    // Init request line parser
    request_line->request_parser.request = &request_line->request;
    request_line->buffer = &connection->client_buffer;
    request_parser_init(&request_line->request_parser);

    // Init sniffer settings
    struct http_args args = get_httpd_args();
    connection->sniffer.sniff_enabled = args.disectors_enabled;
    connection->sniffer.is_done = false;
    sniffer_parser_init(&connection->sniffer.sniffer_parser);
}

unsigned
parsing_host_on_read_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    struct request_line_t *request_line = &connection->connection_parsers.request_line;

    while (buffer_can_read(request_line->buffer)) {
        request_state state = request_parser_feed(&request_line->request_parser, buffer_read(request_line->buffer));
        connection->bytes_to_analize--;

        if (check_request_line_error(connection, state)) {
            return ERROR;
        } else if (state == request_done) {
            unsigned next_state;

            if (request_line->request.request_target.host_type == domain) {
                next_state = handle_origin_doh_connection(key);
            } else {
                next_state = handle_origin_ip_connection(key);
            }

            if (next_state != ERROR) {
                build_connection_request(key);
                if (connection->connection_request.connect) {
                    buffer_reset(&connection->client_buffer);
                } else {
                    // already an http request, look directely for the authorization header
                    modify_sniffer_state(&connection->sniffer.sniffer_parser, sniff_http_authorization);
                    if (connection->sniffer.sniff_enabled && connection->bytes_to_analize > 0 &&
                        !connection->sniffer.is_done) {
                        sniff_data(key);
                    }
                }
            }

            return next_state;
        }
    }

    return stm_state(&connection->stm);
}

static int
check_request_line_error(proxy_connection_t *connection, request_state state) {
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
    proxy_connection_t *connection = ATTACHMENT(key);

    connection_request_t *connection_request = &connection->connection_request;

    struct request_line request_line = connection->connection_parsers.request_line.request;

    if (strcmp((char *) request_line.method, "OPTIONS") == 0 && request_line.request_target.origin_form[0] == 0) {
        sprintf((char *) connection_request->request_line, "%s * HTTP/1.0\r\n", request_line.method);
    } else {
        if (strcmp((char *) connection->connection_parsers.request_line.request.method, "CONNECT") == 0) {
            connection_request->connect = true;
        }
        sprintf((char *) connection_request->request_line, "%s %s HTTP/1.0\r\n", request_line.method,
                request_line.request_target.origin_form);
    }

    connection_request->host_type = request_line.request_target.host_type;
    connection_request->port = ntohs(request_line.request_target.port);
    memcpy(&connection_request->host, &request_line.request_target.host, sizeof(request_line.request_target.host));

    char origin_host[MAX_FQDN_LENGTH + 1] = {0};
    switch (connection_request->host_type) {
        case ipv4:
            inet_ntop(AF_INET, &request_line.request_target.host.ipv4, origin_host, MAX_FQDN_LENGTH + 1);
            break;
        case ipv6:
            inet_ntop(AF_INET6, &request_line.request_target.host.ipv6, origin_host, MAX_FQDN_LENGTH + 1);
            break;
        default:
            strcpy(origin_host, request_line.request_target.host.domain);
            break;
    }

    sprintf((char *) connection_request->target, "%s%s:%d%s",
            request_line.request_target.type == absolute_form ? "http://" : "",
            origin_host,
            (int) connection_request->port,
            connection_request->connect == true ? "" : (char *) request_line.request_target.origin_form);

    strcpy((char *) connection_request->method, (char *) request_line.method);
}

static unsigned
handle_origin_ip_connection(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    struct request_target *request_target = &connection->connection_parsers.request_line.request.request_target;

    if (request_target->host_type == ipv4) {
        connection->origin_fd = establish_origin_connection(
                (struct sockaddr *) &request_target->host.ipv4,
                sizeof(request_target->host.ipv4),
                AF_INET);
    } else {
        connection->origin_fd = establish_origin_connection(
                (struct sockaddr *) &request_target->host.ipv6,
                sizeof(request_target->host.ipv6),
                AF_INET6);
    }

    if (connection->origin_fd == -1) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    if (register_origin_socket(key) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    return TRY_CONNECTION_IP;
}
