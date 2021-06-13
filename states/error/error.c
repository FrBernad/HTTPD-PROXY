#include "error.h"

#include <stdio.h>

#include "connections_manager/connections_def.h"

static void
write_error(struct selector_key *key, errors_t error_code, char *reason_phrase);

void
error_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    switch (connection->error) {
        case BAD_REQUEST:
            write_error(key, connection->error, "BAD REQUEST");
            break;
        case UNAUTHORIZED:
            write_error(key, connection->error, "UNAUTHORIZED");
            break;
        case FORBIDDEN:
            write_error(key, connection->error, "FORBIDDEN");
            break;
        case NOT_FOUND:
            write_error(key, connection->error, "NOT_FOUND");
            break;
        case METHOD_NOT_ALLOWED:
            write_error(key, connection->error, "METHOD_NOT_ALLOWED");
            break;
        case PROXY_AUTHENTICATION_REQUIRED:
            write_error(key, connection->error, "PROXY AUTHENTICATION REQUIRED");
            break;
        case URI_TOO_LONG:
            write_error(key, connection->error, "URI TOO LONG");
            break;
        case INTERNAL_SERVER_ERROR:
            write_error(key, connection->error, "INTERNAL SERVER ERROR");
            break;
        case NOT_IMPLEMENTED:
            write_error(key, connection->error, "NOT IMPLEMENTED");
            break;
        case HTTP_VERSION_NOT_SUPPORTED:
            write_error(key, connection->error, "HTTP VERSION NOT SUPPORTED");
            break;

        case BAD_GATEWAY:
            write_error(key, connection->error, "BAD GATEWAY");
            break;

        default:
            write_error(key, connection->error, "INTERNAL SERVER ERROR");
            break;
    }
    
    selector_set_interest(key->s, connection->client_fd, OP_WRITE);

    if (connection->origin_status != INACTIVE_STATUS) {
        selector_set_interest(key->s, connection->origin_fd, OP_NOOP);  
    }
}

unsigned
error_on_write_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;

    if (!buffer_can_read(buffer)) {
        return DONE;
    }

    return stm_state(&connection->stm);
}

static void
write_error(struct selector_key *key, errors_t error_code, char *reason_phrase) {
    proxy_connection_t *connection = ATTACHMENT(key);

    buffer *buffer = &connection->origin_buffer;

    buffer_reset(buffer);

    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(buffer, &maxBytes);
    int len = sprintf((char *)data, "HTTP/1.0 %d %s\r\n\r\n", error_code, reason_phrase);
    buffer_write_adv(buffer, len);
}