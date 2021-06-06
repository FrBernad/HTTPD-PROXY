#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/buffer.h"
#include "utils/selector.h"
#include "state_machine/stm.h"
#include "utils/connections.h"

typedef struct connectionsManager_t {
    in_port_t port;
    fd_handler proxyHandler;
    int proxyFd;
} connectionsManager_t;

enum proxy_defaults {
    MAX_CONNECTIONS = 1024,
    MAX_PENDING = 5,
    DEFAULT_CHECK_TIME = 8,
};

static void
init_proxy_listener(fd_selector selector);

static int
create_listening_socket(struct sockaddr *sockaddr, socklen_t addrlen);

static fd_selector
init_selector();

#define ERROR_MANAGER(msg, retVal, errno)                                                                                      \
    do {                                                                                                                       \
        fprintf(stderr, "Error: %s (%s).\nReturned: %d (%s).\nLine: %d.\n", msg, __FILE__, retVal, strerror(errno), __LINE__); \
        exit(EXIT_FAILURE);                                                                                                    \
    } while (0)

static connectionsManager_t connectionsManager;

int main(int argc, char const *argv[]) {
    
    if (argc != 2) {
        fprintf(stderr, "Usage : %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int listeningPort = atoi(argv[1]);

    if (listeningPort == 0) {
        fprintf(stderr, "Atoi error");
        exit(EXIT_FAILURE);
    }

    connectionsManager.port = htons(listeningPort);

    if (close(STDIN_FILENO) < 0)
        ERROR_MANAGER("Closing STDIN fd", -1, errno);

    fd_selector selector = init_selector();

    init_selector_handlers();

    init_proxy_listener(selector);
    selector_status status;

    while (true) {
        status = selector_select(selector);

        if (status != SELECTOR_SUCCESS) {
            ERROR_MANAGER("select", status, errno);  //FIXME:
        }
     }

    return 0;
}

static fd_selector
init_selector() {

    const struct selector_init initConfig = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = DEFAULT_CHECK_TIME,
            .tv_nsec = 0,
        }};

    if (selector_init(&initConfig) != SELECTOR_SUCCESS) {
        ERROR_MANAGER("selector init", -1, errno);
    }

    fd_selector selector = selector_new(MAX_CONNECTIONS);

    if (selector == NULL) {
        ERROR_MANAGER("selector_new", -1, errno);
    }

    return selector;
}

static void
init_proxy_listener(fd_selector selector) {

    // Zero out structure
    memset(&connectionsManager.proxyHandler, 0, sizeof(connectionsManager.proxyHandler));

    connectionsManager.proxyHandler.handle_read = accept_new_connection;

    struct sockaddr_in6 sockaddr = {
        .sin6_addr = in6addr_any,
        .sin6_family = AF_INET6,
        .sin6_port = connectionsManager.port,
    };

    int proxyFd = create_listening_socket((struct sockaddr *)&sockaddr, sizeof(sockaddr));

    selector_register(selector, proxyFd, &connectionsManager.proxyHandler, OP_READ, NULL);

}

static int
create_listening_socket(struct sockaddr *sockaddr, socklen_t addrlen) {
    int proxyFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0)
        ERROR_MANAGER("Passive socket creation", proxyFd, errno);

    int opt = 1;
    if (setsockopt(proxyFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: SO_REUSEADDR", -1, errno);
    opt = 0;
    if (setsockopt(proxyFd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: IPV6_ONLY", -1, errno);

    if (selector_fd_set_nio(proxyFd) < 0) {
        ERROR_MANAGER("selector_fd_set_nio", -1, errno);
    }

    if (bind(proxyFd, sockaddr, addrlen) < 0)
        ERROR_MANAGER("bind", -1, errno);

    if (listen(proxyFd, MAX_PENDING) < 0)
        ERROR_MANAGER("listen", -1, errno);

    return proxyFd;
}


