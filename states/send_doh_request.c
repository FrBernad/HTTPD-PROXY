#include "send_doh_request.h"

#include <stdbool.h>
#include <stdio.h>

#include "utils/connections_def.h"
#include "utils/doh_utils.h"

static void
write_doh_request(struct selector_key *key);

void send_doh_request_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    write_doh_request(key);

    if (selector_set_interest(key->s, connection->client_fd, OP_NOOP) != SELECTOR_SUCCESS) {
        printf("error set interest!");
        //FIXME: VER QUE HACER
    }
    if (selector_set_interest(key->s, connection->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        printf("error set interest!");
        //FIXME: VER QUE HACER
    }
}

unsigned
send_doh_request_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (!buffer_can_read(&connection->origin_buffer)) {
        return AWAIT_DOH_RESPONSE;
    }

    return connection->stm.current->state;
}

static void
write_doh_request(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;
    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);

    int requestDohSize = build_doh_request(data, (uint8_t *) connection->connectionRequest.host.domain,
                                           0x01);//FIXME cuando tenga la ip real del servidor doh

    buffer_write_adv(buffer, requestDohSize);
}
