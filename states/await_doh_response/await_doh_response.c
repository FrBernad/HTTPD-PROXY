#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connections/connections.h"
#include "connections/connections_def.h"
#include "states/send_doh_request/send_doh_request.h"
#include "utils/doh/doh_utils.h"
#include "utils/net/net_utils.h"

static void
init_doh_state(struct selector_key *key);

void await_doh_response_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    init_doh_state(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_READ);
}

unsigned
await_doh_response_on_read_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    /*Significa que la función initDoh que se invoca en await_doh_response_on_arrival falló al intentar allocar memoria*/
    if (connection->error == INTERNAL_SERVER_ERROR) {
        return ERROR;
    }

    buffer *origin_buffer = &connection->origin_buffer;

    while (buffer_can_read(origin_buffer)) {
        if (connection->doh_connection->status_line_parser.state != status_line_done) {
            status_line_state status_line_state = status_line_parser_feed(&connection->doh_connection->status_line_parser,
                                                                          buffer_read(origin_buffer));
            if (status_line_state == status_line_error) {
                connection->error = BAD_GATEWAY;
                return ERROR;
            }
            continue;
        }

        if (connection->doh_connection->headers_parser.state != headers_done) {
            headers_state headers_state = headers_parser_feed(&connection->doh_connection->headers_parser,
                                                             buffer_read(origin_buffer));
            if (headers_state == headers_error) {
                connection->error = BAD_GATEWAY;
                return ERROR;
            }
            continue;
        }

        if (connection->doh_connection->doh_parser.state != doh_response_done) {
            doh_response_state doh_response_state = doh_response_parser_feed(&connection->doh_connection->doh_parser,
                                                                           buffer_read(origin_buffer));
            if (doh_response_state == doh_response_error) {
                connection->error = BAD_GATEWAY;
                return ERROR;
            } else if (doh_response_state == response_mem_alloc_error) {
                connection->error = INTERNAL_SERVER_ERROR;
                return ERROR;
            } else if (doh_response_state == doh_no_answers) {
                if (connection->doh_connection->current_type == ipv6_try) {
                    connection->error = INTERNAL_SERVER_ERROR;
                    return ERROR;
                } else {
                    //try ipv6
                    buffer_reset(origin_buffer);
                    selector_unregister_fd(key->s, connection->origin_fd);
                    doh_response_parser_destroy(&connection->doh_connection->doh_parser);
                    connection->doh_connection->is_active = false;
                    connection->doh_connection->current_type = ipv6_try;
                    return handle_origin_doh_connection(key);
                }
            } else if (connection->doh_connection->doh_parser.state == doh_response_done) {
                buffer_reset(origin_buffer);
                break;
            }
        }
    }

    if (connection->origin_status == CLOSING_STATUS) {
        shutdown(connection->origin_fd,SHUT_WR);
        return try_next_dns_connection(key);
    }

    return stm_state(&connection->stm);
}

static void init_doh_state(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    doh_connection_t *doh = connection->doh_connection;

    if (doh == NULL) {
        connection->doh_connection = calloc(sizeof(doh_connection_t), 1);

        if (connection->doh_connection == NULL) {
            connection->error = INTERNAL_SERVER_ERROR;
            return;
        }

        doh = connection->doh_connection;
        doh->doh_parser.response = &doh->doh_response;
        doh->status_line_parser.status_line = &doh->status_line;
        doh->current_type = ipv4_try;
    }

    connection->doh_connection->is_active = true;
    doh->current_try = 0;
    headers_parser_init(&doh->headers_parser);
    status_line_parser_init(&doh->status_line_parser);
    doh_response_parser_init(&doh->doh_parser);
}
