#include "done.h"
#include "connections_manager/connections_def.h"

void
done_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    if (connection->origin_status != INACTIVE_STATUS) {
        selector_unregister_fd(key->s, connection->origin_fd);
    }
    selector_unregister_fd(key->s, connection->client_fd);
}