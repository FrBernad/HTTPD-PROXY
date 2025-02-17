#include "management/management.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "connections_manager/connections_manager.h"
#include "httpd.h"
#include "metrics/metrics.h"
#include "parsers/percy_request_parser/percy_request_parser.h"
#include "logger/logger.h"
#include "logger/logger_utils.h"

#define LOG_BUFFER 128

    enum {
        MAX_MSG_LEN = 128,
        SUCCESS_STATUS = 0x00,
        UNAUTH_STATUS = 0x01,
        UNSUCCESFUL_STATUS = 0x02,
        PERCY_RESPONSE_SIZE = 11,
        PERCY_VERSION = 0x01,
        PERCY_RESV = 0x00,
        RETRIEVAL = 0,
        RETRIEVAL_METHODS_COUNT = 8,
        MODIFICATION = 1,
        MODIFICATION_METHODS_COUNT = 3,
    };

typedef struct values_range {
    uint16_t min;
    uint16_t max;
} values_range;

typedef struct modification_method {
    values_range range_of_value;

    void (*modification_method)(uint16_t);

} modification_method;

static uint64_t (*retrieval_methods[RETRIEVAL_METHODS_COUNT])(void);
static struct modification_method modification_methods[MODIFICATION_METHODS_COUNT];
static char log_buffer[LOG_BUFFER];

typedef struct {
    fd_selector selector;

    int socketFd4;
    int socketFd6;

    struct in_addr ipv4addr;
    struct in6_addr ipv6addr;
    in_port_t port;

    fd_handler managementHandler;

    struct percy_request_parser requestParser;
    struct request_percy request;
    uint8_t buffer[MAX_MSG_LEN];

    uint8_t *passphrase;
} management_t;

static int
init_listener_socket();

static int
ipv4_listener_socket(struct sockaddr_in sockaddr);

static int
ipv6_listener_socket(struct sockaddr_in6 sockaddr);

static int
default_management_socket_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len);

static void
init_management_handlers();

static void
init_management_functions();

static void
management_read(struct selector_key *key);

static void
send_reply(int fd, struct sockaddr *addr, socklen_t clnt_addr_len, uint8_t ver, uint8_t status, uint8_t resv,
           uint64_t value);

static void
build_reply(uint8_t *reply, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value);

static void
serialize(uint64_t value, uint8_t serialized_value[8]);

static bool
validate_passphrase();

static bool
parse_request();

static void
management_close(struct selector_key *key);

static int validate_input_value(uint8_t method, uint16_t value);

static void
set_sniffer_mode(uint16_t op);

static uint64_t
get_select_timeout();

static void
set_select_timeout(uint16_t value);

static management_t management;

int init_management(fd_selector selector) {
    if (init_listener_socket() < 0) {
        return -1;
    }

    init_management_handlers();
    init_management_functions();

    if (management.socketFd4 >= 0) {
        if (selector_register(selector, management.socketFd4, &management.managementHandler, OP_READ, NULL) !=
            SELECTOR_SUCCESS &&
            management.socketFd6 < 0) {
            return -1;
        }
    }

    if (management.socketFd6 >= 0) {
        if (selector_register(selector, management.socketFd6, &management.managementHandler, OP_READ, NULL) !=
            SELECTOR_SUCCESS &&
            management.socketFd4 < 0) {
            return -1;
        }
    }

    management.selector = selector;
    management.requestParser.request = &management.request;
    management.passphrase = (uint8_t *) "123456";

    return 1;
}

// Socket creation
static int
init_listener_socket() {
    struct http_args args = get_httpd_args();

    management.port = htons(args.mng_port);

    management.socketFd6 = -1;
    management.socketFd4 = -1;

    if (args.mng_addr == NULL) {
        //Try ipv6 default listening socket
        struct sockaddr_in6 sockaddr6 = {
                .sin6_addr = in6addr_loopback,
                .sin6_family = AF_INET6,
                .sin6_port = management.port};

        management.socketFd6 = ipv6_listener_socket(sockaddr6);

        //Try ipv4 default listening socket
        struct sockaddr_in sockaddr4 = {
                .sin_addr.s_addr = htonl(INADDR_LOOPBACK),
                .sin_family = AF_INET,
                .sin_port = management.port,
        };

        if ((management.socketFd4 = ipv4_listener_socket(sockaddr4)) < 0 && management.socketFd6 < 0) {
            return -1;
        }

    } else if (inet_pton(AF_INET, args.mng_addr, &management.ipv4addr)) {
        struct sockaddr_in sockaddr = {
                .sin_addr = management.ipv4addr,
                .sin_family = AF_INET,
                .sin_port = management.port,
        };
        if ((management.socketFd4 = ipv4_listener_socket(sockaddr)) < 0) {
            return -1;
        }
    } else if (inet_pton(AF_INET6, args.mng_addr, &management.ipv6addr)) {
        struct sockaddr_in6 sockaddr = {
                .sin6_addr = management.ipv6addr,
                .sin6_family = AF_INET6,
                .sin6_port = management.port,
        };
        if ((management.socketFd6 = ipv6_listener_socket(sockaddr)) < 0) {
            return -1;
        }
    } else {
        return -1;
    }

    return 1;
}

static int
ipv4_listener_socket(struct sockaddr_in sockaddr) {
    int socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd < 0) {
        log_level_msg("Management socket creation", LEVEL_ERROR);
        return -1;
    }

    return default_management_socket_settings(socketfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
}

static int
ipv6_listener_socket(struct sockaddr_in6 sockaddr) {
    int socketfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd < 0) {
        log_level_msg("Management socket creation", LEVEL_ERROR);
        return -1;
    }

    if (setsockopt(socketfd, IPPROTO_IPV6, IPV6_V6ONLY, &(int) {1}, sizeof(int)) < 0) {
        log_level_msg("Management Setsockopt opt: IPV6_ONLY", LEVEL_ERROR);
        return -1;
    }

    return default_management_socket_settings(socketfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
}

static int
default_management_socket_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len) {
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        log_level_msg("Management Setsockopt opt: SO_REUSEADDR", LEVEL_ERROR);
        return -1;
    }

    if (selector_fd_set_nio(socketfd) < 0) {
        log_level_msg("Management selector_fd_set_nio", LEVEL_ERROR);
        return -1;
    }

    if (bind(socketfd, sockaddr, len) < 0) {
        log_level_msg("Management bind", LEVEL_ERROR);
        return -1;
    }

    return socketfd;
}

// Management initialization

static void
init_management_handlers() {
    management.managementHandler.handle_read = management_read;
    management.managementHandler.handle_write = NULL;
    management.managementHandler.handle_close = management_close;
    management.managementHandler.handle_block = NULL;
}

static void init_management_functions() {
    retrieval_methods[0] = get_historical_connections;
    retrieval_methods[1] = get_concurrent_connections;
    retrieval_methods[2] = get_total_bytes_sent;
    retrieval_methods[3] = get_total_bytes_received;
    retrieval_methods[4] = get_total_bytes_transferred;
    retrieval_methods[5] = get_buffer_size;
    retrieval_methods[6] = get_select_timeout;
    retrieval_methods[7] = get_failed_connections;


    //max and min values for modification methods are specified in the documentation of the protocol

    modification_methods[0].range_of_value.min = 0;
    modification_methods[0].range_of_value.max = 1;
    modification_methods[0].modification_method = set_sniffer_mode;

    modification_methods[1].range_of_value.min = 1024;
    modification_methods[1].range_of_value.max = 8192;
    modification_methods[1].modification_method = set_buffer_size;

    modification_methods[2].range_of_value.min = 4;
    modification_methods[2].range_of_value.max = 12;
    modification_methods[2].modification_method = set_select_timeout;
}

// Management functions

static void
management_read(struct selector_key *key) {
    // reset parser
    percy_request_parser_init(&management.requestParser);

    struct sockaddr_storage clnt_addr;
    socklen_t clnt_addr_len = sizeof(clnt_addr);

    ssize_t bytes_rcvd = recvfrom(key->fd, management.buffer, MAX_MSG_LEN, 0, (struct sockaddr *) &clnt_addr,
                                  &clnt_addr_len);
    if (bytes_rcvd < 0) {
        log_level_msg("Something went wrong(management)", LEVEL_DEBUG);
    }

    if (parse_request()) {
        if (!validate_passphrase()) {
            send_reply(key->fd, (struct sockaddr *) &clnt_addr, clnt_addr_len, PERCY_VERSION, UNAUTH_STATUS, PERCY_RESV,
                       0);
        } else {
            uint8_t method = management.request.method;
            uint16_t value = management.request.value;
           
            switch (management.request.type) {

                case RETRIEVAL:

                    sprintf(log_buffer, "Retrieval method: %d", method);
                    log_level_msg(log_buffer, LEVEL_DEBUG);

                    if (method <= RETRIEVAL_METHODS_COUNT - 1) {
                        send_reply(key->fd, (struct sockaddr *) &clnt_addr, clnt_addr_len, PERCY_VERSION,
                                   SUCCESS_STATUS,
                                   PERCY_RESV, retrieval_methods[method]());
                        return;
                    }
                    break;

                case MODIFICATION:

                    sprintf(log_buffer, "Modification method: %d with entry: %d", method, value);
                    log_level_msg(log_buffer, LEVEL_DEBUG);

                    if (method <= MODIFICATION_METHODS_COUNT - 1) {
                        if (validate_input_value(method, value)) {
                            modification_methods[method].modification_method(value);
                            send_reply(key->fd, (struct sockaddr *) &clnt_addr, clnt_addr_len, PERCY_VERSION,
                                       SUCCESS_STATUS, PERCY_RESV, 0
                            );
                            return;
                        } else {
                            send_reply(key->fd, (struct sockaddr *) &clnt_addr, clnt_addr_len, PERCY_VERSION,
                                       UNSUCCESFUL_STATUS, PERCY_RESV, 0
                            );
                            return;
                        }
                    }

                default:
                    break;
            }
        }
    }

    send_reply(key->fd, (struct sockaddr *) &clnt_addr, clnt_addr_len, PERCY_VERSION, UNSUCCESFUL_STATUS, PERCY_RESV,
               0);
}

static void
send_reply(int fd, struct sockaddr *addr, socklen_t clnt_addr_len, uint8_t ver, uint8_t status, uint8_t resv,
           uint64_t value) {
    uint8_t reply[PERCY_RESPONSE_SIZE] = {0};
    build_reply(reply, ver, status, resv, value);
    ssize_t bytes_sent = sendto(fd, reply, PERCY_RESPONSE_SIZE, 0, addr, clnt_addr_len);
    if (bytes_sent < 0) {
        sprintf(log_buffer, "Something went wrong(management)");
        log_level_msg(log_buffer, LEVEL_ERROR);
    }
}

static void
build_reply(uint8_t *reply, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value) {
    reply[0] = ver;
    reply[1] = status;
    reply[2] = resv;
    uint8_t serialized_value[8] = {0};

    serialize(value, serialized_value);

    memcpy(reply + 3, serialized_value, sizeof(value));
}

static void
serialize(uint64_t value, uint8_t serialized_value[8]) {
    serialized_value[0] = value >> 56;
    serialized_value[1] = value >> 48;
    serialized_value[2] = value >> 40;
    serialized_value[3] = value >> 32;
    serialized_value[4] = value >> 24;
    serialized_value[5] = value >> 16;
    serialized_value[6] = value >> 8;
    serialized_value[7] = value;
}

static bool
validate_passphrase() {
    for (int i = 0; i < PASS_PHRASE_LEN; i++) {
        if (management.passphrase[i] != management.request.passphrase[i]) {
            return false;
        }
    }
    return true;
}

static bool
parse_request() {
    bool ret_val = false;

    for (int i = 0; i < PERCY_REQUEST_SIZE; i++) {
        enum percy_request_state state = percy_request_parser_feed(&management.requestParser, management.buffer[i]);
        if (state == percy_request_error) {
            ret_val = false;
            break;
        } else if (state == percy_request_done) {
            ret_val = true;
            break;
        }
    }

    return ret_val;
}

static void
management_close(struct selector_key *key) {
    close(key->fd);
}

static int validate_input_value(uint8_t method, uint16_t value) {
    if (value <= modification_methods[method].range_of_value.max &&
        value >= modification_methods[method].range_of_value.min) {
        return true;
    }

    return false;
}

static void
set_sniffer_mode(uint16_t op) {
    set_disector_value(op);
}

static uint64_t
get_select_timeout() {
    return get_selector_timeout(management.selector);
}

static void
set_select_timeout(uint16_t value) {
    set_selector_timeout(management.selector, value);
}