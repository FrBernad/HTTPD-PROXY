#include <stdio.h>
#include "try_connection_ip.h"
#include "../../utils/connections_def.h"

static int check_origin_connection(int socketfd);

void 
try_connection_ip_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (selector_set_interest(key->s, connection->client_fd, OP_NOOP) != SELECTOR_SUCCESS) {
        printf("error set interest!");
        //VER QUE HACER
    }
    if (selector_set_interest(key->s, connection->origin_fd, OP_WRITE) != SELECTOR_SUCCESS) {
        printf("error set interest!");
        //VER QUE HACER
    }
}

unsigned
try_connection_ip_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (check_origin_connection(connection->origin_fd)){
        printf("me conecte bien!!\n");
        return CONNECTED;
    }

    return ERROR;
}

static int 
check_origin_connection(int socketfd) {
    int opt;
    socklen_t optlen = sizeof(opt);
    if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &opt, &optlen) < 0)
        return false;
    // printf("opt val: %d, fd: %ld\n\n", opt, i);
    // MATAR EL SOCKET Y AVISARLE AL CLIENT SI ES QUE NO HAY MAS IPS.SI NO PROBAR CON LA LISTA DE IPS
    if (opt != 0) {
        printf("Connection could not be established, closing sockets.\n\n");
        return false;
    }

    return true;
}
