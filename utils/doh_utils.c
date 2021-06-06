#include "doh_utils.h"

#include <stdio.h>
#include <string.h>

#define MAX_QUERY_LENGTH 300  //QNAME como MAXIMO PUEDE TENER 255 bytes + 2 de QTYPE Y QCLASS

struct dns_request_header {
    uint8_t id[2];
    uint8_t flags[2];
    uint8_t qdcount[2];
    uint8_t ancount[2];
    uint8_t nscount[2];
    uint8_t arcount[2];
};

int build_doh_request(uint8_t *dst, uint8_t *domain, uint8_t queryType) {
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
                      "POST https://dns.google/dns-query HTTP/1.0\r\n"
                      "Host: dns.google\r\n"
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
