#include "net_utils.h"

#include <errno.h>
#include <unistd.h>

#include "utils/selector/selector.h"

int establish_origin_connection(struct sockaddr *addr, socklen_t addrlen, int protocol) {
    int origin_socket = socket(protocol, SOCK_STREAM, IPPROTO_TCP);

    if (origin_socket < 0)
        return -1;

    int opt = 1;

    if (selector_fd_set_nio(origin_socket) < 0) {
        close(origin_socket);
        return -1;
    }

    if (setsockopt(origin_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(origin_socket);
        return -1;
    }

    if (connect(origin_socket, addr, addrlen) < 0) {
        if (errno != EINPROGRESS) {
            close(origin_socket);
            return -1; 
        }
    }

    return origin_socket;
}