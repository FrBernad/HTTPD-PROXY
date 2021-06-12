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
    set_empty_buffers_interests(key);

    proxyConnection *connection = ATTACHMENT(key);
    if(connection->client_status==CLOSED_STATUS && connection->origin_status==CLOSED_STATUS){
        unregister_connection();
        return DONE;
    }

    return stm_state(&connection->stm);
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
    } else {
        if (connection->client_status == CLOSING_STATUS) {
            shutdown(connection->origin_fd,SHUT_WR);
            connection->client_status=CLOSED_STATUS;
        }
    }

    if (buffer_can_read(originBuffer)) {
        clientInterest |= OP_WRITE;
    }else {
        if (connection->origin_status == CLOSING_STATUS) {
            shutdown(connection->client_fd,SHUT_WR);
            connection->origin_status=CLOSED_STATUS;
        }
    }

    selector_set_interest(key->s, connection->client_fd, clientInterest);
    selector_set_interest(key->s, connection->origin_fd, originInterest);
}
