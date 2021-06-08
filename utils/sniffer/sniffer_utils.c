#include "sniffer_utils.h"

#include <stdio.h>

void 
sniff_data(proxyConnection *connection) {
    size_t maxBytes;
    uint8_t *data = buffer_read_ptr(&connection->client_buffer, &maxBytes);
    uint8_t *dataToParse = data + maxBytes - connection->client_sniffer.bytesToSniff;
    unsigned state;

    for (int i = 0; i < connection->client_sniffer.bytesToSniff; i++) {
        state = sniffer_parser_feed(dataToParse[i], &connection->client_sniffer.sniffer_parser);
        if (state == sniff_done) {
            printf("user: %s, password:%s\n", connection->client_sniffer.sniffer_parser.user,
                   connection->client_sniffer.sniffer_parser.password);
            break;
        } else if (state == sniff_error) {
            printf("error parsing!\n");
            break;
        }
    }
}
