#include "sniffer_utils.h"

#include <stdio.h>

#include "connections/connections_def.h"

void sniff_data(struct selector_key *key) {

    proxyConnection *connection = ATTACHMENT(key);

    int dataOwner;
    buffer * bufferToSniff;

    if (key->fd == connection->client_fd) {
        dataOwner = CLIENT_OWNED;
        bufferToSniff = &connection->client_buffer;
    } else {
        dataOwner = ORIGIN_OWNED;
        bufferToSniff = &connection->origin_buffer;
    }

    size_t maxBytes;
    uint8_t *data = buffer_read_ptr(bufferToSniff, &maxBytes);
    uint8_t *dataToParse = data + maxBytes - connection->sniffer.bytesToSniff;

    unsigned state;

    for (int i = 0; i < connection->sniffer.bytesToSniff; i++) {
        state = sniffer_parser_feed(dataToParse[i], &connection->sniffer.sniffer_parser, dataOwner);
        if (state == sniff_done) {
            connection->sniffer.isDone = true;
            printf("user: %s, password:%s\n", connection->sniffer.sniffer_parser.user,
                   connection->sniffer.sniffer_parser.password);
            break;
        } else if (state == sniff_error) {
            connection->sniffer.isDone = true;
            printf("error parsing!\n");
            break;
        }
    }

    connection->sniffer.bytesToSniff = 0;
}
