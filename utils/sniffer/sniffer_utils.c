#include "sniffer_utils.h"

#include <stdio.h>

#include "connections_manager/connections_def.h"
#include "logger/logger_utils.h"

void
sniff_data(struct selector_key *key) {

    proxy_connection_t *connection = ATTACHMENT(key);

    int data_owner;
    buffer *buffer_to_sniff;

    if (key->fd == connection->client_fd) {
        data_owner = CLIENT_OWNED;
        buffer_to_sniff = &connection->client_buffer;
    } else {
        data_owner = ORIGIN_OWNED;
        buffer_to_sniff = &connection->origin_buffer;
    }

    size_t maxBytes;
    uint8_t *data = buffer_read_ptr(buffer_to_sniff, &maxBytes);
    if(connection->bytes_to_analize > (int)maxBytes){
        connection->bytes_to_analize = maxBytes;
    }
    unsigned state;

    for (ssize_t i = 0; i < connection->bytes_to_analize; i++) {
        state = sniffer_parser_feed(data[i], &connection->sniffer.sniffer_parser, data_owner,
                                    connection->connection_request.port);
        if (state == sniff_done) {
            connection->sniffer.is_done = true;
            log_user_and_password(key);
            break;
        } else if (state == sniff_error) {
            connection->sniffer.is_done = true;
            break;
        }
    }

    connection->bytes_to_analize = 0;
}
