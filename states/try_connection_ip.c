#include "try_connection_ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/connections.h"
#include "utils/connections_def.h"
#include "utils/net_utils.h"

static int
check_origin_connection(int socketfd);

static unsigned
try_next_dns_connection(struct selector_key *key);

static unsigned
try_next_ipv4_connection(struct selector_key *key);

static unsigned
try_next_ipv6_connection(struct selector_key *key);

void try_connection_ip_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_WRITE);
}

unsigned
try_connection_ip_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (check_origin_connection(connection->origin_fd)) {
        /*Si dohConnection == NULL es que nunca mande el doh_request */
        if (connection->connectionRequest.host_type == domain && connection->dohConnection == NULL) {
            return SEND_DOH_REQUEST;
        }
        /*En este caso ya estoy conectado al origin*/
        return SEND_REQUEST_LINE;
    }

    if (connection->connectionRequest.host_type == domain && connection->dohConnection != NULL) {
        return try_next_dns_connection(key);
    }

    printf("ERROR AL CONECTARSE A LA IP O AL DOH SERVER");
    return ERROR;
}

static int
check_origin_connection(int socketfd) {
    int opt;
    socklen_t optlen = sizeof(opt);
    if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &opt, &optlen) < 0) {
        printf("Error getsockopt.\n\n");
        return false;
    }

    if (opt != 0) {
        printf("Connection could not be established, closing sockets.\n\n");
        return false;
    }

    return true;
}

static unsigned
try_next_dns_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (selector_unregister_fd(key->s, connection->origin_fd) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    doh_connection_t *doh = connection->dohConnection;

    if (doh->currentType == ipv4_try) {
        return try_next_ipv4_connection(key);
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
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
        }

        printf("registered origin!\n");

        if (register_origin_socket(key) != SELECTOR_SUCCESS) {
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
        }

        return TRY_CONNECTION_IP;
    }

    doh_response_parser_destroy(&connection->dohConnection->dohParser);
    connection->dohConnection->currentType = ipv6_try;

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

        connection->origin_fd = establish_origin_connection((struct sockaddr *)&ipv6, sizeof(ipv6),
                                                            ipv6.sin6_family);
        if (connection->origin_fd == -1) {
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
            printf("error creating socket dns\n");
        }

        printf("registered origin!\n");

        if (register_origin_socket(key) != SELECTOR_SUCCESS) {
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
        }

        return TRY_CONNECTION_IP;
    }

    //free_doh_connection(connection->dohConnection); MOVERLO AL ESTADO DE ERROR
    return ERROR;
}
