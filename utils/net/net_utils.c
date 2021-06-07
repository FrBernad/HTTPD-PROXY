#include "net_utils.h"

#include <errno.h>
#include <unistd.h>

#include "utils/selector/selector.h"

int establish_origin_connection(struct sockaddr *addr, socklen_t addrlen, int protocol) {
    int originSocket = socket(protocol, SOCK_STREAM, IPPROTO_TCP);

    if (originSocket < 0)
        return -1;

    int opt = 1;

    if (selector_fd_set_nio(originSocket) < 0) {
        close(originSocket);
        return -1;
    }

    if (setsockopt(originSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(originSocket);
        return -1;
    }

    if (connect(originSocket, addr, addrlen) < 0) {
        if (errno != EINPROGRESS) {
            close(originSocket);
            return -1; 
        }
    }

    return originSocket;
}