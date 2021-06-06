#include <stdio.h>
#include <stdlib.h>

#include "../../utils/connections_def.h"
#include "send_doh_request.h"

static void initDohState(struct selector_key *key);

void await_doh_response_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    initDohState(key);

    if (selector_set_interest(key->s, connection->client_fd, OP_NOOP) != SELECTOR_SUCCESS) {
        printf("error set interest!");  //FIXME: VER QUE HACER
    }

    if (selector_set_interest(key->s, connection->origin_fd, OP_READ) != SELECTOR_SUCCESS) {
        printf("error set interest!");  //FIXME: VER QUE HACER
    }
}

unsigned
await_doh_response_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    buffer *origin_buffer = &connection->origin_buffer;

    while (buffer_can_read(origin_buffer)) {
        if (connection->dohConnection->statusLineParser.state != status_line_done) {
            status_line_state statusLineState = status_line_parser_feed(&connection->dohConnection->statusLineParser,
                                                                        buffer_read(origin_buffer));
            if (statusLineState == status_line_error) {
                printf("ERROR!!\n");
            }
            continue;
        }

        if (connection->dohConnection->headersParser.state != headers_done) {
            headers_state headersState = headers_parser_feed(&connection->dohConnection->headersParser,
                                                             buffer_read(origin_buffer));
            if (headersState == headers_error) {
                printf("ERROR!!\n");
            }
            continue;
        }

        if (connection->dohConnection->dohParser.state != doh_response_done) {
            doh_response_state dohResponseState = doh_response_parser_feed(&connection->dohConnection->dohParser,
                                                                           buffer_read(origin_buffer));
            if (dohResponseState == doh_response_error) {
                printf("ERROR!!\n");
            }
            continue;
        }
    }

    if (connection->origin_status == CLOSING_STATUS) {
        return TRY_CONNECTION_IP;
    }

    return connection->stm.current->state;
}

static void initDohState(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    connection->dohConnection = calloc(sizeof(doh_connection_t), 1);

    if (connection->dohConnection == NULL) {
        printf("error");  //FIXME:
    }

    doh_connection_t *doh = connection->dohConnection;

    doh->dohParser.response = &doh->dohResponse;
    doh->statusLineParser.status_line = &doh->statusLine;
    doh->currentTry = 0;
    doh->currentType = ipv4_try;
    headers_parser_init(&doh->headersParser);
    status_line_parser_init(&doh->statusLineParser);
    doh_response_parser_init(&doh->dohParser);
}
