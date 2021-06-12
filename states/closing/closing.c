
#include "closing.h"

#include <stdio.h>

#include "connections/connections_def.h"
#include "metrics/metrics.h"
#include "logger/logger_utils.h"

static void
set_closing_connection_interests(struct selector_key *key);

void closing_on_arrival(unsigned state, struct selector_key *key) {
    set_closing_connection_interests(key);
}

unsigned
closing_on_read_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    buffer *origin_buffer = &connection->origin_buffer;
    buffer *client_buffer = &connection->client_buffer;

    connection_status_t maybe_close;
    int already_closed_fd;

    if (key->fd == connection->client_fd) {
        already_closed_fd = connection->origin_fd;
        maybe_close = connection->client_status;
    } else {
        already_closed_fd = connection->client_fd;
        maybe_close = connection->origin_status;
    }

    if (maybe_close == CLOSING_STATUS) {
        if (!buffer_can_read(client_buffer) && !buffer_can_read(origin_buffer)) {
            shutdown(already_closed_fd, SHUT_WR);
            unregister_connection();
            return DONE;
        }
        return EMPTY_BUFFERS;
    }

    set_closing_connection_interests(key);

    return stm_state(&connection->stm);
}

unsigned
closing_on_write_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    set_closing_connection_interests(key);
    return stm_state(&connection->stm);
}

static void
set_closing_connection_interests(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    buffer *origin_buffer = &connection->origin_buffer;
    buffer *client_buffer = &connection->client_buffer;

    fd_interest client_interest = OP_NOOP;
    fd_interest origin_interest = OP_NOOP;

    if (connection->origin_status == CLOSING_STATUS || connection->origin_status == CLOSED_STATUS) {
        /*Si el origin esta cerrando la conexión significa que ya no va a leer nada mas de su socket 
        porque no le va a llegar mas nada
        Sin embargo hay que tener en cuenta que si el cliente quiere seguir mandando cosas, tengo que enviarlo a origin*/
        if (buffer_can_read(client_buffer)) {
            origin_interest |= OP_WRITE;
        }
        if (buffer_can_write(client_buffer)) {
            client_interest |= OP_READ;
        }

        if (buffer_can_read(origin_buffer)) {
            client_interest |= OP_WRITE;
        } else if (connection->origin_status == CLOSING_STATUS) {
            shutdown(connection->client_fd, SHUT_WR);
            connection->origin_status = CLOSED_STATUS;
        }
    }

    if (connection->client_status == CLOSING_STATUS || connection->client_status == CLOSED_STATUS) {
        /*Si el cliente esta cerrando la conexión significa que ya no va a leer nada mas de su socket 
        porque no le va a llegar mas nada
        Sin embargo hay que tener en cuenta que si el origin quiere seguir mandando cosas, tengo que enviarlo a cliente*/

        if (buffer_can_read(origin_buffer)) {
            client_interest |= OP_WRITE;
        }

        if (buffer_can_write(origin_buffer)) {
            origin_interest |= OP_READ;
        }

        if (buffer_can_read(client_buffer)) {
            origin_interest |= OP_WRITE;
        } else if (connection->client_status == CLOSING_STATUS) {
            shutdown(connection->origin_fd, SHUT_WR);
            connection->client_status = CLOSED_STATUS;
        }
    }

    selector_set_interest(key->s, connection->client_fd, client_interest);
    selector_set_interest(key->s, connection->origin_fd, origin_interest);
}
