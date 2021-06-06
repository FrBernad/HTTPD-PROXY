#include "done.h"
#include <stdio.h>
#include "utils/connections_def.h"

void done_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    selector_unregister_fd(key->s,connection->origin_fd);
    selector_unregister_fd(key->s,connection->client_fd);
}