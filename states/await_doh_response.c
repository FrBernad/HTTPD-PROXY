#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "send_doh_request.h"
#include "utils/connections.h"
#include "utils/connections_def.h"
#include "utils/net_utils.h"

static void
initDohState(struct selector_key *key);

static unsigned
try_next_dns_connection(struct selector_key *key);

static unsigned
try_next_ipv4_connection(struct selector_key *key);

static unsigned
try_next_ipv6_connection(struct selector_key *key);

void 
await_doh_response_on_arrival(const unsigned state, struct selector_key *key) {
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
        }
    }

    if (connection->origin_status == CLOSING_STATUS) {
        return try_next_dns_connection(key);
    }

    return connection->stm.current->state;
}

static void initDohState(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    doh_connection_t *doh = connection->dohConnection;

    if (doh == NULL) {
        connection->dohConnection = calloc(sizeof(doh_connection_t), 1);
        doh = connection->dohConnection;
        doh->dohParser.response = &doh->dohResponse;
        doh->statusLineParser.status_line = &doh->statusLine;
        doh->currentType = ipv4_try;
    }
    
    doh->currentTry = 0;
    headers_parser_init(&doh->headersParser);
    status_line_parser_init(&doh->statusLineParser);
    doh_response_parser_init(&doh->dohParser);
}

static unsigned
try_next_dns_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    // Unregister doh connection socket 
    selector_unregister_fd(key->s, connection->origin_fd);

    doh_connection_t *doh = connection->dohConnection;

    bool responseError = doh->dohResponse.header.flags.rcode !=0;

    if (doh->currentType == ipv4_try) {
        if (responseError != 0){
            return try_next_ipv4_connection(key);
        }
        doh_response_parser_destroy(&connection->dohConnection->dohParser);
        connection->dohConnection->currentType = ipv6_try;
        return SEND_DOH_REQUEST;
    }

    if(responseError!=0){
        connection->error = INTERNAL_SERVER_ERROR;//CHEQUEAR
        return ERROR;
    }     

    return try_next_ipv6_connection(key);
}

static unsigned
try_next_ipv4_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    doh_connection_t *doh = connection->dohConnection;
    struct answer currentAnswer;
    struct sockaddr_in ipv4;

    while (doh->currentTry < doh->dohResponse.header.ancount) {
        currentAnswer = doh->dohResponse.answers[doh->currentTry++];
        if (currentAnswer.atype != IPV4_TYPE) {
            continue;
        }

        memset(&ipv4, 0, sizeof(ipv4));
        ipv4.sin_addr = currentAnswer.aip.ipv4;
        ipv4.sin_family = AF_INET;
        ipv4.sin_port = htons(80);

        connection->origin_fd = establish_origin_connection((struct sockaddr *)&ipv4, sizeof(ipv4),
                                                            ipv4.sin_family);
        if (connection->origin_fd == -1) {
            printf("error creating socket dns\n");
        }

        printf("registered origin!\n");

        register_origin_socket(key);

        return TRY_CONNECTION_IP;
    }

    return SEND_DOH_REQUEST;
}

static unsigned
try_next_ipv6_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    doh_connection_t *doh = connection->dohConnection;
    struct answer currentAnswer;
    struct sockaddr_in6 ipv6;

    while (doh->currentTry < doh->dohResponse.header.ancount) {
        currentAnswer = doh->dohResponse.answers[doh->currentTry++];
        if (currentAnswer.atype != IPV6_TYPE) {
            continue;
        }

        memset(&ipv6, 0, sizeof(ipv6));
        ipv6.sin6_addr = currentAnswer.aip.ipv6;
        ipv6.sin6_family = AF_INET6;
        ipv6.sin6_port = htons(80);

        connection->origin_fd = establish_origin_connection((struct sockaddr *)&ipv6, sizeof(ipv6), ipv6.sin6_family);
        if (connection->origin_fd == -1) {
            printf("error creating socket dns\n");
        }

        printf("registered origin!\n");

        register_origin_socket(key);

        return TRY_CONNECTION_IP;
    }

    return ERROR;
}
