#include "done.h"
#include <stdio.h>
#include "connections/connections_def.h"

void done_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    if (connection->origin_status != INACTIVE_STATUS) {
        selector_unregister_fd(key->s, connection->origin_fd);
    }
    selector_unregister_fd(key->s, connection->client_fd);
}