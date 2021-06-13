#include "send_doh_request.h"

#include <stdio.h>

#include "connections_manager/connections_def.h"
#include "utils/doh/doh_utils.h"

static void
write_doh_request(struct selector_key *key);

void 
send_doh_request_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    write_doh_request(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_WRITE);
}

unsigned
send_doh_request_on_write_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    if (!buffer_can_read(&connection->origin_buffer)) {
        return AWAIT_DOH_RESPONSE;
    }

    return stm_state(&connection->stm);
}

static void
write_doh_request(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;
    size_t max_bytes;
    uint8_t *data = buffer_write_ptr(buffer, &max_bytes);
    uint8_t query_type;
    if (connection->doh_connection == NULL) {
        query_type = IPV4_TYPE;
    } else {
        query_type = connection->doh_connection->current_type == ipv4_try ? IPV4_TYPE : IPV6_TYPE;
    }

    int request_doh_size = build_doh_request(data, (uint8_t *)connection->connection_request.host.domain, query_type);

    buffer_write_adv(buffer, request_doh_size);
}
