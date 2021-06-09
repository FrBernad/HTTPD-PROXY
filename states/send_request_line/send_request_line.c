#include "connections/connections_def.h"
#include "send_request_line.h"

#include <stdio.h>
#include <string.h>

static void 
write_request_line(struct selector_key *key);

void 
send_request_line_on_arrival(const unsigned state, struct selector_key *key){
    proxyConnection *connection = ATTACHMENT(key);

    write_request_line(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    
    selector_set_interest(key->s, connection->origin_fd, OP_WRITE);
}

unsigned
send_request_line_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;

    if (!buffer_can_read(buffer)) {
        return CONNECTED;
    }

    return stm_state(&connection->stm);
}

static void 
write_request_line(struct selector_key *key) {

    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;
    
    
    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);
    size_t requestLineSize = strlen((char*)connection->connectionRequest.requestLine);
    memcpy(data, connection->connectionRequest.requestLine, requestLineSize);
    buffer_write_adv(buffer, requestLineSize);

}