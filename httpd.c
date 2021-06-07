#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils/args.h"
#include "utils/connections.h"
#include "utils/selector.h"

typedef struct connectionsManager_t {
    struct in6_addr ipv6addr;
    struct in_addr ipv4addr;
    in_port_t port;
    fd_handler proxyHandler;
    int proxyFd;
} connectionsManager_t;

enum proxy_defaults {
    MAX_CONNECTIONS = 1024,
    MAX_PENDING = 5,
    DEFAULT_CHECK_TIME = 8,
};

static struct http_args args;

static int
init_proxy_listener(fd_selector selector);

static int
ipv6_proxy_listener(struct sockaddr_in6 sockaddr);

static int
ipv4_proxy_listener(struct sockaddr_in sockaddr);

static int
ipv6_default_proxy_listener(struct sockaddr_in6 sockaddr);

static fd_selector
init_selector();

#define ERROR_MANAGER(msg, retVal, errno)                                                                                      \
    do {                                                                                                                       \
        fprintf(stderr, "Error: %s (%s).\nReturned: %d (%s).\nLine: %d.\n", msg, __FILE__, retVal, strerror(errno), __LINE__); \
        exit(EXIT_FAILURE);                                                                                                    \
    } while (0)

static connectionsManager_t connectionsManager;

int main(int argc, char *argv[]) {
    parse_args(argc, argv, &args);

    connectionsManager.port = htons(args.http_port);

    if (close(STDIN_FILENO) < 0)
        ERROR_MANAGER("Closing STDIN fd", -1, errno);

    fd_selector selector = init_selector();
    if (selector == NULL) {
        goto finally;
    }

    init_selector_handlers();

    if(init_proxy_listener(selector)<0){
        perror("Passive socket creation");
        goto finally;
    }

    selector_status status = SELECTOR_SUCCESS;

    while (true) {
        status = selector_select(selector);

        if (status != SELECTOR_SUCCESS) {
            goto finally;
        }
    }

    int returnVal = 0;

finally:

    returnVal = 1;

    if (selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();

    if (connectionsManager.proxyFd >= 0) {
        close(connectionsManager.proxyFd);
    }

    return returnVal;
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
        perror("selector_init");
        return NULL;
    }

    fd_selector selector = selector_new(MAX_CONNECTIONS);

    if (selector == NULL) {
        perror("selector_new");
        return NULL;
    }

    return selector;
}

static int
init_proxy_listener(fd_selector selector) {
    // Zero out structure
    memset(&connectionsManager.proxyHandler, 0, sizeof(connectionsManager.proxyHandler));

    int proxyFd;

    if (args.http_addr == NULL) {
        struct sockaddr_in6 sockaddr = {
            .sin6_addr = in6addr_any,
            .sin6_family = AF_INET6,
            .sin6_port = connectionsManager.port,
        };
        if((proxyFd = ipv6_default_proxy_listener(sockaddr))<0){
            return -1;
        }
    }
    else if (inet_pton(AF_INET, args.http_addr, &connectionsManager.ipv4addr)) {
        struct sockaddr_in sockaddr = {
            .sin_addr = connectionsManager.ipv4addr,
            .sin_family = AF_INET,
            .sin_port = connectionsManager.port,
        };
        if ((proxyFd = ipv4_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    }
    else if (inet_pton(AF_INET6, args.http_addr, &connectionsManager.ipv6addr)) {
        struct sockaddr_in6 sockaddr = {
            .sin6_addr = connectionsManager.ipv6addr,
            .sin6_family = AF_INET6,
            .sin6_port = connectionsManager.port,
        };
        if ((proxyFd = ipv6_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    }
    else {
        return -1;
    }

    connectionsManager.proxyHandler.handle_read = accept_new_connection;

    if (selector_register(selector, proxyFd, &connectionsManager.proxyHandler, OP_READ, NULL) != SELECTOR_SUCCESS) {
        return -1;
    }

    return proxyFd;
}

static int
ipv6_proxy_listener(struct sockaddr_in6 sockaddr) {
    int proxyFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        perror("Passive socket creation");
        return -1;
    }

    int opt = 1;
    if (setsockopt(proxyFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt opt: SO_REUSEADDR");
        return -1;
    }

    if (selector_fd_set_nio(proxyFd) < 0) {
        perror("selector_fd_set_nio");
        return -1;
    }

    if (bind(proxyFd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(proxyFd, MAX_PENDING) < 0) {
        perror("listen");
        return -1;
    }

    return proxyFd;
}

static int
ipv4_proxy_listener(struct sockaddr_in sockaddr) {
    int proxyFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        perror("Passive socket creation");
        return -1;
    }

    int opt = 1;
    if (setsockopt(proxyFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt opt: SO_REUSEADDR");
        return -1;
    }

    if (selector_fd_set_nio(proxyFd) < 0) {
        perror("selector_fd_set_nio");
        return -1;
    }

    if (bind(proxyFd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(proxyFd, MAX_PENDING) < 0) {
        perror("listen");
        return -1;
    }

    return proxyFd;
}

static int
ipv6_default_proxy_listener(struct sockaddr_in6 sockaddr) {
    int proxyFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        perror("Passive socket creation");
        return -1;
    }

    int opt = 1;
    if (setsockopt(proxyFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt opt: SO_REUSEADDR");
        return -1;
    }
    opt = 0;
    if (setsockopt(proxyFd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt opt: IPV6_ONLY");
        return -1;
    }

    if (selector_fd_set_nio(proxyFd) < 0) {
        perror("selector_fd_set_nio");
        return -1;
    }

    if (bind(proxyFd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(proxyFd, MAX_PENDING) < 0) {
        perror("listen");
        return -1;
    }

    return proxyFd;
}
