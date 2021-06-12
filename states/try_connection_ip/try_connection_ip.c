#include "try_connection_ip.h"

#include <stdio.h>
#include <string.h>

#include "connections/connections_def.h"
#include "logger/logger_utils.h"
#include "metrics/metrics.h"
#include "utils/doh/doh_utils.h"

static int
check_origin_connection(int socketfd);

void try_connection_ip_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_WRITE);
}

unsigned
try_connection_ip_on_write_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    if (check_origin_connection(connection->origin_fd)) {
        /*Si doh_connection == NULL es que nunca mande el doh_request, si no esta active tengo que probar ipv6 */
        if (connection->connection_request.host_type == domain &&
            (connection->doh_connection == NULL || !connection->doh_connection->is_active)) {
            return SEND_DOH_REQUEST;
        }
        /*En este caso ya estoy conectado al origin*/
        register_new_connection();
        if (connection->connection_request.connect) {
            connection->connection_request.status_code = 200;
            log_new_connection(key);

            buffer *origin_buffer = &connection->origin_buffer;
            buffer_reset(origin_buffer);

            size_t max_bytes;
            uint8_t *data = buffer_write_ptr(origin_buffer, &max_bytes);

            int len = sprintf((char *)data, "HTTP/1.0 %d %s\r\n\r\n", connection->connection_request.status_code, "OK");
            buffer_write_adv(origin_buffer, len);

            return CONNECTED;
        }
        return SEND_REQUEST_LINE;
    }

    // try next ip from doh response
    if (connection->connection_request.host_type == domain && connection->doh_connection != NULL) {
        return try_next_dns_connection(key);
    }

    if (connection->connection_request.host_type != domain) {
        // Connection with direct ip failed
        increase_failed_connections();
    }

    connection->error = INTERNAL_SERVER_ERROR;
    return ERROR;
}

static int
check_origin_connection(int socketfd) {
    int opt;
    socklen_t optlen = sizeof(opt);
    if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &opt, &optlen) < 0) {
        log_level_msg("Error getsockopt.",LEVEL_ERROR);
        return false;
    }

    if (opt != 0) {
        log_level_msg("Connection could not be established, closing sockets.",LEVEL_ERROR);
        return false;
    }

    return true;
}
