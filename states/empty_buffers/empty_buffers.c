#include "empty_buffers.h"

#include <stdio.h>

#include "connections/connections_def.h"
#include "metrics/metrics.h"

static void
set_empty_buffers_interests(struct selector_key *key);

void empty_buffers_on_arrival(const unsigned state, struct selector_key *key) {
    set_empty_buffers_interests(key);
}

unsigned
empty_buffers_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;
    buffer *clientBuffer = &connection->client_buffer;

    if (!buffer_can_read(clientBuffer) && !buffer_can_read(originBuffer)) {
        close_connection();
        return DONE;
    }

    set_empty_buffers_interests(key);

    return connection->stm.current->state;
}

static void
set_empty_buffers_interests(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;
    buffer *clientBuffer = &connection->client_buffer;

    fd_interest clientInterest = OP_NOOP;
    fd_interest originInterest = OP_NOOP;

    if (buffer_can_read(clientBuffer)) {
        originInterest |= OP_WRITE;
    }

    if (buffer_can_read(originBuffer)) {
        clientInterest |= OP_WRITE;
    }

    selector_set_interest(key->s, connection->client_fd, clientInterest);
    selector_set_interest(key->s, connection->origin_fd, originInterest);
}
