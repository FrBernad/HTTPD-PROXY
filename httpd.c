#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "connections_manager/connections_manager.h"
#include "logger/logger.h"
#include "logger/logger_utils.h"
#include "management/management.h"
#include "metrics/metrics.h"
#include "utils/args/args.h"
#include "utils/doh/doh_utils.h"

typedef struct httpd_t {
    struct in6_addr ipv6_addr;
    struct in_addr ipv4_addr;
    in_port_t port;
    fd_handler proxy_handler;
    int proxy_fd_6;
    int proxy_fd_4;
} httpd_t;

enum proxy_defaults {
    MAX_CONNECTIONS = 1024,
    MAX_PENDING = 5,
    DEFAULT_SELECTOR_TIMEOUT = 8,
    MAX_INACTIVE_TIME = 12,
};

static int
init_proxy_listener(fd_selector selector);

static int
ipv6_only_proxy_listener(struct sockaddr_in6 sockaddr);

static int
ipv4_only_proxy_listener(struct sockaddr_in sockaddr);

static int
default_proxy_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len);

static fd_selector
init_selector();

static void
signal_handler(int signum);

static void
init_signal_handler(int signum);

static httpd_t httpd;
static struct http_args args;
static bool finished = false;

int main(int argc, char *argv[]) {
    parse_args(argc, argv, &args);

    httpd.port = htons(args.http_port);

    close(STDIN_FILENO);

    init_signal_handler(SIGINT);
    init_signal_handler(SIGTERM);

    // Init variables for try/catch
    int return_val = 0;
    selector_status status;

    init_connections_manager(MAX_INACTIVE_TIME);

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
        log_level_msg("Passive socket creation",LEVEL_ERROR);
        return_val = 2;
        goto finally;
    }

    if (init_management(selector) < 0) {
        log_level_msg("Management socket creation",LEVEL_ERROR);
        return_val = 2;
        goto finally;
    }

    while (!finished) {
        status = selector_select(selector);

        if (status != SELECTOR_SUCCESS) {
            return_val = 1;
            goto finally;
        }
    }

    finally:

    destroy_logger();

    if (selector != NULL) {
        selector_destroy(selector);
    }

    selector_close();

    if (httpd.proxy_fd_6 >= 0) {
        close(httpd.proxy_fd_6);
    }

    if (httpd.proxy_fd_4 >= 0) {
        close(httpd.proxy_fd_6);
    }

    return return_val;
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
                    .tv_sec = DEFAULT_SELECTOR_TIMEOUT,
                    .tv_nsec = 0,
            }};

    if (selector_init(&initConfig) != SELECTOR_SUCCESS) {
        return NULL;
    }

    fd_selector selector = selector_new(MAX_CONNECTIONS);

    if (selector == NULL) {
        return NULL;
    }

    if (selector_set_garbage_collector(selector, connection_garbage_collect, DEFAULT_SELECTOR_TIMEOUT) < 0) {
        return NULL;
    }

    return selector;
}

static int
init_proxy_listener(fd_selector selector) {
    // Zero out structure
    memset(&httpd.proxy_handler, 0, sizeof(httpd.proxy_handler));

    int *proxy_fd_6 = &httpd.proxy_fd_6;
    *proxy_fd_6 = -1;
    int *proxy_fd_4 = &httpd.proxy_fd_4;
    *proxy_fd_4 = -1;

    if (args.http_addr == NULL) {
        //Try ipv6 default listening socket
        struct sockaddr_in6 sockaddr6 = {
                .sin6_addr = in6addr_any,
                .sin6_family = AF_INET6,
                .sin6_port = httpd.port
        };

        *proxy_fd_6 = ipv6_only_proxy_listener(sockaddr6);

        //Try ipv4 default listening socket
        struct sockaddr_in sockaddr4 = {
                .sin_addr.s_addr = INADDR_ANY,
                .sin_family = AF_INET,
                .sin_port = httpd.port,
        };

        if ((*proxy_fd_4 = ipv4_only_proxy_listener(sockaddr4)) < 0 && *proxy_fd_6 < 0) {
            return -1;
        }

    } else if (inet_pton(AF_INET, args.http_addr, &httpd.ipv4_addr)) {
        struct sockaddr_in sockaddr = {
                .sin_addr = httpd.ipv4_addr,
                .sin_family = AF_INET,
                .sin_port = httpd.port,
        };
        if ((*proxy_fd_4 = ipv4_only_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    } else if (inet_pton(AF_INET6, args.http_addr, &httpd.ipv6_addr)) {
        struct sockaddr_in6 sockaddr = {
                .sin6_addr = httpd.ipv6_addr,
                .sin6_family = AF_INET6,
                .sin6_port = httpd.port,
        };
        if ((*proxy_fd_6 = ipv6_only_proxy_listener(sockaddr)) < 0) {
            return -1;
        }
    } else {
        return -1;
    }

    httpd.proxy_handler.handle_read = accept_new_connection;

    if (*proxy_fd_4 >= 0) {
        if (selector_register(selector, *proxy_fd_4, &httpd.proxy_handler, OP_READ, NULL) !=
            SELECTOR_SUCCESS && *proxy_fd_6 < 0) {
            return -1;
        }
    }

    if (*proxy_fd_6 >= 0) {
        if (selector_register(selector, *proxy_fd_6, &httpd.proxy_handler, OP_READ, NULL) !=
            SELECTOR_SUCCESS && *proxy_fd_4 < 0) {
            return -1;
        }
    }

    return 1;
}

static int
ipv6_only_proxy_listener(struct sockaddr_in6 sockaddr) {
    int proxyFd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        log_level_msg("Passive socket creation",LEVEL_ERROR);
        return -1;
    }

    if (setsockopt(proxyFd, IPPROTO_IPV6, IPV6_V6ONLY, &(int) {1}, sizeof(int)) < 0) {
        log_level_msg("Setsockopt opt: IPV6_ONLY",LEVEL_ERROR);
        return -1;
    }

    return default_proxy_settings(proxyFd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
}

static int
ipv4_only_proxy_listener(struct sockaddr_in sockaddr) {
    int proxyFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxyFd < 0) {
        log_level_msg("Passive socket creation",LEVEL_ERROR);
        return -1;
    }

    return default_proxy_settings(proxyFd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
}

static int
default_proxy_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len) {
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        log_level_msg("Setsockopt opt: SO_REUSEADDR",LEVEL_ERROR);
        return -1;
    }

    if (selector_fd_set_nio(socketfd) < 0) {
        log_level_msg("Socket selector_fd_set_nio",LEVEL_ERROR);
        return -1;
    }

    if (bind(socketfd, sockaddr, len) < 0) {
        log_level_msg("Socket bind",LEVEL_ERROR);
        return -1;
    }

    if (listen(socketfd, MAX_PENDING) < 0) {
        log_level_msg("Socket listen",LEVEL_ERROR);
        return -1;
    }

    return socketfd;
}

struct http_args
get_httpd_args() {
    return args;
}

void
set_disector_value(uint16_t value) {
    if (value) {
        args.disectors_enabled = true;
    } else {
        args.disectors_enabled = false;
    }
}
