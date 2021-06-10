#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connections/connections.h"
#include "metrics/metrics.h"
#include "utils/args/args.h"
#include "utils/doh/doh_utils.h"
#include "logger/logger.h"
#include "utils/selector/selector.h"

typedef struct connectionsManager_t {
    struct in6_addr ipv6addr;
    struct in_addr ipv4addr;
    in_port_t port;
    fd_handler proxyHandler;
    int proxyFd;
} connectionsManager_t;

enum proxy_defaults {
    MAX_CONNECTIONS = 1000,
    MAX_PENDING = 5,
    DEFAULT_CHECK_TIME = 8,
};

static int
init_proxy_listener(fd_selector selector);

static int
ipv6_only_proxy_listener(struct sockaddr_in6 sockaddr);

static int
ipv4_only_proxy_listener(struct sockaddr_in sockaddr);

static int
ipv6_default_proxy_listener(struct sockaddr_in6 sockaddr);

static int
default_proxy_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len);

static fd_selector
init_selector();

static void
signal_handler(int signum);

static void
init_signal_handler(int signum);

static connectionsManager_t connectionsManager;
static struct http_args args;
static bool finished = false;

int main(int argc, char *argv[]) {
    parse_args(argc, argv, &args);

    connectionsManager.port = htons(args.http_port);

    init_signal_handler(SIGINT);
    init_signal_handler(SIGTERM);

    // Init variables for try/catch
    int returnVal = 0;
    selector_status status = SELECTOR_SUCCESS;

    init_selector_handlers();
    fd_selector selector = init_selector();
    if (selector == NULL) {
        goto finally;
    }

    if (init_logger(selector) < 0) {
        goto finally;
    }
    
    init_doh(args.doh);
    init_metrics();

    if (init_proxy_listener(selector) < 0) {
        fprintf(stderr, "Passive socket creation\n");
        returnVal = 2;
        goto finally;
    }

    while (!finished) {
        status = selector_select(selector);

        if (status != SELECTOR_SUCCESS) {
            returnVal = 1;
            goto finally;
        }
    }

finally:

    if (selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();

    if (connectionsManager.proxyFd >= 0) {
        close(connectionsManager.proxyFd);
    }

    return returnVal;
}

static void
init_signal_handler(int signum) {
    struct sigaction new_action;

    //Set the handler in the new_action struct
    new_action.sa_handler = signal_handler;

    sigemptyset(&new_action.sa_mask);

    //Remove any flag from sa_flag.
    new_action.sa_flags = 0;

    sigaction(signum, &new_action, NULL);
}

static void
signal_handler(int signum) {
    finished = true;
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
        fprintf(stderr, "selector_init\n");
        return NULL;
    }

    fd_selector selector = selector_new(MAX_CONNECTIONS);

    if (selector == NULL) {
        fprintf(stderr, "selector_new\n");
        return NULL;
    }

    return selector;
}

static int
init_proxy_listener(fd_selector selector) {
    // Zero out structure
    memset(&connectionsManager.proxyHandler, 0, sizeof(connectionsManager.proxyHandler));

    int *proxyFd = &connectionsManager.proxyFd;
    *proxyFd = -1;

    if (args.http_addr == NULL) {
        struct sockaddr_in6 sockaddr = {
            .sin6_addr = in6addr_any,
            .sin6_family = AF_INET6,
            .sin6_port = connectionsManager.port,
        };
        if ((*proxyFd = ipv6_default_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    } else if (inet_pton(AF_INET, args.http_addr, &connectionsManager.ipv4addr)) {
        struct sockaddr_in sockaddr = {
            .sin_addr = connectionsManager.ipv4addr,
            .sin_family = AF_INET,
            .sin_port = connectionsManager.port,
        };
        if ((*proxyFd = ipv4_only_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    } else if (inet_pton(AF_INET6, args.http_addr, &connectionsManager.ipv6addr)) {
        struct sockaddr_in6 sockaddr = {
            .sin6_addr = connectionsManager.ipv6addr,
            .sin6_family = AF_INET6,
            .sin6_port = connectionsManager.port,
        };
        if ((*proxyFd = ipv6_only_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    } else {
        return -1;
    }
    connectionsManager.proxyHandler.handle_read = accept_new_connection;

    if (selector_register(selector, *proxyFd, &connectionsManager.proxyHandler, OP_READ, NULL) != SELECTOR_SUCCESS) {
        return -1;
    }

    return *proxyFd;
}

static int
ipv6_only_proxy_listener(struct sockaddr_in6 sockaddr) {
    int proxyFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        fprintf(stderr, "Passive socket creation\n");
        return -1;
    }

    if (setsockopt(proxyFd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Setsockopt opt: IPV6_ONLY\n");
        return -1;
    }

    return default_proxy_settings(proxyFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

static int
ipv4_only_proxy_listener(struct sockaddr_in sockaddr) {
    int proxyFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        fprintf(stderr, "Passive socket creation\n");
        return -1;
    }

    return default_proxy_settings(proxyFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

static int
ipv6_default_proxy_listener(struct sockaddr_in6 sockaddr) {
    int proxyFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        fprintf(stderr, "Passive socket creation\n");
        return -1;
    }

    if (setsockopt(proxyFd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){0}, sizeof(int)) < 0) {
        fprintf(stderr, "Setsockopt opt: IPV6_ONLY\n");
        return -1;
    }

    return default_proxy_settings(proxyFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

static int
default_proxy_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len) {
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Setsockopt opt: SO_REUSEADDR\n");
        return -1;
    }

    if (selector_fd_set_nio(socketfd) < 0) {
        fprintf(stderr, "selector_fd_set_nio\n");
        return -1;
    }

    if (bind(socketfd, sockaddr, len) < 0) {
        fprintf(stderr, "bind\n");
        return -1;
    }

    if (listen(socketfd, MAX_PENDING) < 0) {
        fprintf(stderr, "listen\n");
        return -1;
    }

    return socketfd;
}

struct http_args
get_httpd_args() {
    return args;
}