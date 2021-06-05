#include "connected.h"
#include "../../utils/connections_def.h"

void connected_on_arrival(const unsigned state, struct selector_key *key){
    proxyConnection *connection = ATTACHMENT(key);

    int clientFd = connection->client_fd;
    int serverFd = connection->origin_fd;

    //REVISAR
    selector_set_interest(key->s, clientFd, OP_READ | OP_WRITE);
    selector_set_interest(key->s, serverFd ,OP_READ | OP_WRITE);
}

unsigned
connected_on_read_ready(struct selector_key *key){
    return CONNECTED;
}
