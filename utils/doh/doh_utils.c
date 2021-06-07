#include "doh_utils.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "connections/connections_def.h"
#include "utils/net/net_utils.h"
#include "connections/connections.h"

static unsigned
try_next_ipv4_connection(struct selector_key *key);

static unsigned
try_next_ipv6_connection(struct selector_key *key);


#define MAX_QUERY_LENGTH 300  //QNAME como MAXIMO PUEDE TENER 255 bytes + 2 de QTYPE Y QCLASS

struct dns_request_header {
    uint8_t id[2];
    uint8_t flags[2];
    uint8_t qdcount[2];
    uint8_t ancount[2];
    uint8_t nscount[2];
    uint8_t arcount[2];
};

int 
build_doh_request(uint8_t *dst, uint8_t *domain, uint8_t queryType) {
    struct dns_request_header dnsHeader;
    size_t dnsHeaderLength = sizeof(dnsHeader);
    memset(&dnsHeader, 0, dnsHeaderLength);
    dnsHeader.qdcount[1] = 0x01;

    uint8_t query[MAX_QUERY_LENGTH];
    uint8_t *label = query;
    uint8_t count = 0;
    int j = 1, i;
    for (i = 0; domain[i] != 0; i++) {
        if (domain[i] != '.') {
            count++;
            query[j++] = domain[i];
        } else {
            label[0] = count;
            count = 0;
            label = &query[j++];
        }
    }
    if (domain[i - 1] != '.') {
        query[j++] = 0;
    }
    label[0] = count;
    query[j] = 0;
    uint8_t qtype[2] = {0x00, queryType};
    uint8_t qclass[2] = {0x00, 0x01};
    uint8_t content_length = dnsHeaderLength + j + sizeof(uint16_t) * 2;

    int len = sprintf((char *)dst,
                      "POST https://cloudflare-dns.com/dns-query HTTP/1.0\r\n"
                      "Host: cloudflare-dns.com\r\n"
                      "accept: application/dns-message\r\n"
                      "content-type: application/dns-message\r\n"
                      "content-length: %d\r\n\r\n",
                      content_length);

    size_t size = len;

    memcpy(dst + size, &dnsHeader, dnsHeaderLength);

    size += dnsHeaderLength;
    memcpy(dst + size, &query, j);
    size += j;
    memcpy(dst + size, &qtype, sizeof(qtype));
    size += sizeof(qtype);
    memcpy(dst + size, &qclass, sizeof(qclass));
    size += sizeof(qclass);

    return size;
}

unsigned
handle_origin_doh_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct sockaddr_in ipv4;

    ipv4.sin_family = AF_INET;
    ipv4.sin_port = htons(8053);                                 //FIXME: Cambiar
    if (inet_pton(AF_INET, "127.0.0.1", &ipv4.sin_addr) <= 0) {  //FIXME: poner el doh servidor
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;  //FIXME: DOH SERVER ERROR
    }

    connection->origin_fd = establish_origin_connection(
        (struct sockaddr *)&ipv4,
        sizeof(ipv4),
        AF_INET);

    if (connection->origin_fd < 0) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    printf("registered doh origin!\n");

    if (register_origin_socket(key) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    return TRY_CONNECTION_IP;
}

unsigned
try_next_dns_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    // Unregister doh connection socket
    if (selector_unregister_fd(key->s, connection->origin_fd) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    doh_connection_t *doh = connection->dohConnection;

    bool responseError = doh->dohResponse.header.flags.rcode != 0;

    if (doh->currentType == ipv4_try) {
        if (!responseError) {
            return try_next_ipv4_connection(key);
        }
        doh_response_parser_destroy(&connection->dohConnection->dohParser);
        connection->dohConnection->currentType = ipv6_try;
        return handle_origin_doh_connection(key);
    }

    if (responseError) {
        connection->error = INTERNAL_SERVER_ERROR;  //FIXME: preguntar bien esto que pasa con el rcode
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
            continue;
        }

        if (register_origin_socket(key) != SELECTOR_SUCCESS) {
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
        }

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
            continue;
        }

        printf("registered origin!\n");

        if (register_origin_socket(key) != SELECTOR_SUCCESS) {
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
        }

        return TRY_CONNECTION_IP;
    }

    connection->error = INTERNAL_SERVER_ERROR;
    return ERROR;
}
