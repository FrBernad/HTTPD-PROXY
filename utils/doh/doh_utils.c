#include "doh_utils.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "connections/connections.h"
#include "connections/connections_def.h"
#include "metrics/metrics.h"
#include "utils/net/net_utils.h"

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

static struct doh_utils {
    struct doh doh;
    union {
        struct in6_addr ipv6addr;
        struct in_addr ipv4addr;
    } ip;
    bool isipv4;
    in_port_t port;
} doh_utils;

void init_doh() {
    struct doh args_doh = get_httpd_args().doh;

    doh_utils.doh = args_doh;
    doh_utils.port = htons(args_doh.port);

    if (inet_pton(AF_INET, args_doh.ip, &doh_utils.ip.ipv4addr)) {
        doh_utils.isipv4 = true;
    } else if (inet_pton(AF_INET6, args_doh.ip, &doh_utils.ip.ipv6addr)) {
        doh_utils.isipv4 = false;
    }
}

size_t build_doh_request(uint8_t *dst, uint8_t *domain, uint8_t query_type) {
    struct dns_request_header dns_header;
    size_t dns_header_length = sizeof(dns_header);
    memset(&dns_header, 0, dns_header_length);
    dns_header.flags[0] = 0x01;
    dns_header.qdcount[1] = 0x01;

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
    uint8_t qtype[2] = {0x00, query_type};
    uint8_t qclass[2] = {0x00, 0x01};
    uint8_t content_length = dns_header_length + j + sizeof(uint16_t) * 2;

    int len = sprintf((char *) dst,
                      "POST %s HTTP/1.0\r\n"
                      "Host: %s\r\n"
                      "accept: application/dns-message\r\n"
                      "content-type: application/dns-message\r\n"
                      "content-length: %d\r\n\r\n",
                      doh_utils.doh.path, doh_utils.doh.host, content_length);

    size_t size = len;

    memcpy(dst + size, &dns_header, dns_header_length);

    size += dns_header_length;
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
    proxy_connection_t *connection = ATTACHMENT(key);

    if (doh_utils.isipv4) {
        struct sockaddr_in sockaddr = {
                .sin_addr = doh_utils.ip.ipv4addr,
                .sin_family = AF_INET,
                .sin_port = doh_utils.port,
        };
        connection->origin_fd = establish_origin_connection(
                (struct sockaddr *) &sockaddr,
                sizeof(sockaddr),
                AF_INET);
    } else {
        struct sockaddr_in6 sockaddr = {
                .sin6_addr = doh_utils.ip.ipv6addr,
                .sin6_family = AF_INET6,
                .sin6_port = doh_utils.port,
        };
        connection->origin_fd = establish_origin_connection(
                (struct sockaddr *) &sockaddr,
                sizeof(sockaddr),
                AF_INET6);
    }

    if (connection->origin_fd < 0) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    if (register_origin_socket(key) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    return TRY_CONNECTION_IP;
}

unsigned
try_next_dns_connection(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    // la primera vez se borra del selector el doh y las proximas veces los sockets con conexiones fallidas mediante una ip
    if (selector_unregister_fd(key->s, connection->origin_fd) != SELECTOR_SUCCESS) {
        connection->error = INTERNAL_SERVER_ERROR;
        return ERROR;
    }

    doh_connection_t *doh = connection->doh_connection;

    bool responseError = doh->doh_response.header.flags.rcode != 0;

    if (doh->current_type == ipv4_try) {
        if (!responseError) {
            return try_next_ipv4_connection(key);
        }
        doh_response_parser_destroy(&connection->doh_connection->doh_parser);
        connection->doh_connection->is_active = false;
        connection->doh_connection->current_type = ipv6_try;
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
    proxy_connection_t *connection = ATTACHMENT(key);

    doh_connection_t *doh = connection->doh_connection;
    struct answer currentAnswer;
    struct sockaddr_in ipv4;

    /*Significa que no es la primera vez que intento con una ip*/
    if (doh->current_try != 0) {
        increase_failed_connections();
    }

    while (doh->current_try < doh->doh_response.header.ancount) {
        currentAnswer = doh->doh_response.answers[doh->current_try++];
        if (currentAnswer.a_type != IPV4_TYPE) {
            continue;
        }

        memset(&ipv4, 0, sizeof(ipv4));
        ipv4.sin_addr = currentAnswer.aip.ipv4;
        ipv4.sin_family = AF_INET;
        ipv4.sin_port = connection->connection_parsers.request_line.request.request_target.port;

        connection->origin_fd = establish_origin_connection((struct sockaddr *) &ipv4, sizeof(ipv4),
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

    doh_response_parser_destroy(&connection->doh_connection->doh_parser);
    connection->doh_connection->is_active = false;
    connection->doh_connection->current_type = ipv6_try;
    return handle_origin_doh_connection(key);
}

static unsigned
try_next_ipv6_connection(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    doh_connection_t *doh = connection->doh_connection;
    struct answer currentAnswer;
    struct sockaddr_in6 ipv6;

    /*Significa que no es la primera vez que intento con una ip*/
    if (doh->current_try != 0) {
        increase_failed_connections();
    }

    while (doh->current_try < doh->doh_response.header.ancount) {
        currentAnswer = doh->doh_response.answers[doh->current_try++];
        if (currentAnswer.a_type != IPV6_TYPE) {
            continue;
        }

        memset(&ipv6, 0, sizeof(ipv6));
        ipv6.sin6_addr = currentAnswer.aip.ipv6;
        ipv6.sin6_family = AF_INET6;
        ipv6.sin6_port = connection->connection_parsers.request_line.request.request_target.port;

        connection->origin_fd = establish_origin_connection((struct sockaddr *) &ipv6, sizeof(ipv6), ipv6.sin6_family);
        if (connection->origin_fd == -1) {
            continue;
        }

        if (register_origin_socket(key) != SELECTOR_SUCCESS) {
            connection->error = INTERNAL_SERVER_ERROR;
            return ERROR;
        }

        return TRY_CONNECTION_IP;
    }

    connection->error = INTERNAL_SERVER_ERROR;
    return ERROR;
}
