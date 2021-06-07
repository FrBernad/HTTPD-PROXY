#include "send_doh_request.h"

#include <stdbool.h>
#include <stdio.h>

#include "connections/connections_def.h"
#include "utils/doh/doh_utils.h"

static void
write_doh_request(struct selector_key *key);

void 
send_doh_request_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    write_doh_request(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_WRITE);
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
    uint8_t queryType;
    if (connection->dohConnection == NULL) {
        queryType = IPV4_TYPE;
    } else {
        queryType = connection->dohConnection->currentType == ipv4_try ? IPV4_TYPE : IPV6_TYPE;
    }

    int requestDohSize = build_doh_request(data, (uint8_t *)connection->connectionRequest.host.domain,
                                           queryType);  //FIXME cuando tenga la ip real del servidor doh

    buffer_write_adv(buffer, requestDohSize);
}
