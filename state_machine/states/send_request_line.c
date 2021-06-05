#include "../../utils/connections_def.h"
#include "send_request_line.h"

#include <stdio.h>
#include <string.h>

static void 
writeRequestLine(struct selector_key *key);

void send_request_line_on_arrival(const unsigned state, struct selector_key *key){
    proxyConnection *connection = ATTACHMENT(key);


    writeRequestLine(key);

    if (selector_set_interest(key->s, connection->client_fd, OP_NOOP) != SELECTOR_SUCCESS) {
        printf("error set interest!");
    }
    
    if (selector_set_interest(key->s, connection->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        printf("error set interest!");
    }
}

static void 
writeRequestLine(struct selector_key *key) {

    proxyConnection *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;
    
    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);
    size_t requestLineSize = strlen((char*)connection->requestLine);
    memcpy(data, connection->requestLine, requestLineSize);
    buffer_write_adv(buffer, requestLineSize);

}
