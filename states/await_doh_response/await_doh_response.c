#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connections/connections.h"
#include "connections/connections_def.h"
#include "states/send_doh_request/send_doh_request.h"
#include "utils/net/net_utils.h"
#include "utils/doh/doh_utils.h"

static void
initDohState(struct selector_key *key);

void await_doh_response_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    initDohState(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_READ);
}

unsigned
await_doh_response_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    /*Significa que la función initDoh que se invoca en await_doh_response_on_arrival falló al intentar allocar memoria*/
    if (connection->error == INTERNAL_SERVER_ERROR) {
        return ERROR;
    }

    buffer *origin_buffer = &connection->origin_buffer;

    while (buffer_can_read(origin_buffer)) {
        if (connection->dohConnection->statusLineParser.state != status_line_done) {
            status_line_state statusLineState = status_line_parser_feed(&connection->dohConnection->statusLineParser,
                                                                        buffer_read(origin_buffer));
            if (statusLineState == status_line_error) {
                connection->error = BAD_GATEWAY;
                return ERROR;
            }
            continue;
        }

        if (connection->dohConnection->headersParser.state != headers_done) {
            headers_state headersState = headers_parser_feed(&connection->dohConnection->headersParser,
                                                             buffer_read(origin_buffer));
            if (headersState == headers_error) {
                connection->error = BAD_GATEWAY;
                return ERROR;
            }
            continue;
        }

        if (connection->dohConnection->dohParser.state != doh_response_done) {
            doh_response_state dohResponseState = doh_response_parser_feed(&connection->dohConnection->dohParser,
                                                                           buffer_read(origin_buffer));
            if (dohResponseState == doh_response_error) {
                connection->error = BAD_GATEWAY;
                return ERROR;
            } else if (dohResponseState == response_mem_alloc_error) {
                connection->error = INTERNAL_SERVER_ERROR;
                return ERROR;
            }else if(connection->dohConnection->dohParser.state == doh_response_done){
                buffer_reset(origin_buffer);
                break;
            }
        }
    }

    if (connection->origin_status == CLOSING_STATUS) {
        return try_next_dns_connection(key);
    }

    return stm_state(&connection->stm);
}

static void initDohState(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    doh_connection_t *doh = connection->dohConnection;

    if (doh == NULL) {
        connection->dohConnection = calloc(sizeof(doh_connection_t), 1);

        if (connection->dohConnection == NULL) {
            connection->error = INTERNAL_SERVER_ERROR;
            return;
        }

        doh = connection->dohConnection;
        doh->dohParser.response = &doh->dohResponse;
        doh->statusLineParser.status_line = &doh->statusLine;
        doh->currentType = ipv4_try;
    }
    connection->dohConnection->isActive = true;
    doh->currentTry = 0;
    headers_parser_init(&doh->headersParser);
    status_line_parser_init(&doh->statusLineParser);
    doh_response_parser_init(&doh->dohParser);
}

