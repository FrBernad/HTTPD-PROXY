#include "try_connection_ip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connections/connections.h"
#include "connections/connections_def.h"
#include "logger/logger_utils.h"
#include "metrics/metrics.h"
#include "utils/doh/doh_utils.h"
#include "utils/net/net_utils.h"

static int
check_origin_connection(int socketfd);

void try_connection_ip_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    selector_set_interest(key->s, connection->client_fd, OP_NOOP);
    selector_set_interest(key->s, connection->origin_fd, OP_WRITE);
}

unsigned
try_connection_ip_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (check_origin_connection(connection->origin_fd)) {
        /*Si dohConnection == NULL es que nunca mande el doh_request, si no esta active tengo que probar ipv6 */
        if (connection->connectionRequest.host_type == domain &&
            (connection->dohConnection == NULL || !connection->dohConnection->isActive)) {
            return SEND_DOH_REQUEST;
        }
        /*En este caso ya estoy conectado al origin*/
        register_new_connection();
        if (connection->connectionRequest.connect) {
            connection->connectionRequest.status_code = 200;
            log_new_connection(key);

            buffer *originBuffer = &connection->origin_buffer;
            buffer_reset(originBuffer);

            size_t maxBytes;
            uint8_t *data = buffer_write_ptr(originBuffer, &maxBytes);

            int len = sprintf((char *)data, "HTTP/1.0 %d %s\r\n\r\n", connection->connectionRequest.status_code, "OK");
            buffer_write_adv(originBuffer, len);

            return CONNECTED;
        }
        return SEND_REQUEST_LINE;
    }

    // try next ip from doh response
    if (connection->connectionRequest.host_type == domain && connection->dohConnection != NULL) {
        return try_next_dns_connection(key);
    }

    if (connection->connectionRequest.host_type != domain) {
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
        printf("Error getsockopt.\n\n");
        return false;
    }

    if (opt != 0) {
        printf("Connection could not be established, closing sockets.\n\n");
        return false;
    }

    return true;
}
