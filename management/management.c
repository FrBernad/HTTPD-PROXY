#include "management/management.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "httpd.h"
#include "metrics/metrics.h"
#include "parsers/percy_request_parser/percy_request_parser.h"

enum {
    MAX_MSG_LEN = 128,
    SUCCESS_STATUS = 0x00,
    UNAUTH_STATUS = 0x01,
    UNSUCCESFUL_STATUS = 0x02,
    PERCY_RESPONSE_SIZE = 11,
    PERCY_VERSION = 0x01,
    PERCY_RESV = 0x00,
    RETRIEVAL = 0,
    RETRIEVAL_METHODS_COUNT = 9,
    MODIFICATION = 1,
    MODIFICATION_METHODS_COUNT = 2,
};

static uint64_t (*retrievalMethods[RETRIEVAL_METHODS_COUNT])(void);

typedef struct {
    int socketFd;
    //FIXME:union
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
create_ipv4_listener_socket(struct sockaddr_in sockaddr);

static int
create_ipv6_listener_socket(struct sockaddr_in6 sockaddr);

static int
default_management_socket_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len);

static void
init_management_handlers();

static void
init_management_functions();

static void
management_read(struct selector_key *key);

static void
send_reply(struct sockaddr *addr, socklen_t clntAddrLen, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value);

static void
build_reply(uint8_t *reply, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value);

static void
serialize(uint64_t value, uint8_t serialized_value[8]);

static bool
validate_passphrase();

static bool
parse_request(ssize_t bytesRcvd);

static void
management_close(struct selector_key *key);

static management_t management;

int init_management(fd_selector selector) {
    struct http_args args = get_httpd_args();

    bool isIpv4 = true;

    management.port = htons(args.mng_port);
    if (inet_pton(AF_INET, args.mng_addr, &management.ipv4addr) < 0) {
        isIpv4 = false;
    }
    if (inet_pton(AF_INET6, args.mng_addr, &management.ipv6addr) < 0) {
        return -1;
    }

    if (isIpv4) {
        struct sockaddr_in sockaddr = {
            .sin_addr = management.ipv4addr,
            .sin_family = AF_INET,
            .sin_port = management.port,
        };
        management.socketFd = create_ipv4_listener_socket(sockaddr);
    } else {
        struct sockaddr_in6 sockaddr = {
            .sin6_addr = management.ipv6addr,
            .sin6_family = AF_INET6,
            .sin6_port = management.port,
        };
        management.socketFd = create_ipv6_listener_socket(sockaddr);
    }

    if (management.socketFd < 0) {
        return -1;
    }

    init_management_handlers();
    init_management_functions();

    if (selector_register(selector, management.socketFd, &management.managementHandler, OP_READ, NULL) != SELECTOR_SUCCESS) {
        return -1;
    }

    management.requestParser.request = &management.request;
    management.passphrase = (uint8_t *)"123456";

    return 1;
}




// Socket creation
static int
create_ipv4_listener_socket(struct sockaddr_in sockaddr) {
    int socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd < 0) {
        fprintf(stderr, "Management socket creation\n");
        return -1;
    }

    return default_management_socket_settings(socketfd,(struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

static int
create_ipv6_listener_socket(struct sockaddr_in6 sockaddr) {
    int socketfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd < 0) {
        fprintf(stderr, "Management socket creation\n");
        return -1;
    }

    return default_management_socket_settings(socketfd,(struct sockaddr *)&sockaddr, sizeof(sockaddr));
}

static int
default_management_socket_settings(int socketfd, struct sockaddr *sockaddr, socklen_t len) {
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
    retrievalMethods[0] = get_historical_connections;
    retrievalMethods[1] = get_concurrent_connections;
    retrievalMethods[2] = get_total_bytes_sent;
    retrievalMethods[3] = get_total_bytes_received;
    retrievalMethods[4] = get_total_bytes_transfered;
    retrievalMethods[5] = NULL;
    retrievalMethods[6] = NULL;
    retrievalMethods[7] = get_concurrent_connections;
    retrievalMethods[8] = get_failed_connections;
}



// Management functions

static void
management_read(struct selector_key *key) {
    // reset parser
    percy_request_parser_init(&management.requestParser);

    struct sockaddr_storage clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);

    ssize_t bytesRcvd = recvfrom(management.socketFd, management.buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&clntAddr, &clntAddrLen);
    if (bytesRcvd < 0) {
        printf("Smoething went wrong(management)\n");
    }

    if (parse_request(bytesRcvd)) {
        if (!validate_passphrase()) {
            send_reply((struct sockaddr *)&clntAddr, clntAddrLen, PERCY_VERSION, UNAUTH_STATUS, PERCY_RESV, 0);
        } else {
            switch (management.request.type) {
                case RETRIEVAL:
                    printf("received %d\n\n", management.request.method);
                    if (management.request.method <= RETRIEVAL_METHODS_COUNT - 1) {
                        send_reply((struct sockaddr *)&clntAddr, clntAddrLen, PERCY_VERSION, SUCCESS_STATUS, PERCY_RESV, retrievalMethods[management.request.method]());
                        return;
                    }
                    break;
                case MODIFICATION:
                    break;
                default:
                    break;
            }
        }
    }

    send_reply((struct sockaddr *)&clntAddr, clntAddrLen, PERCY_VERSION, UNSUCCESFUL_STATUS, PERCY_RESV, 0);
}

static void
send_reply(struct sockaddr *addr, socklen_t clntAddrLen, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value) {
    uint8_t reply[PERCY_RESPONSE_SIZE] = {0};
    build_reply(reply, ver, status, resv, value);
    ssize_t bytesSent = sendto(management.socketFd, reply, PERCY_RESPONSE_SIZE, 0, addr, clntAddrLen);
    if (bytesSent < 0) {
        printf("Smoething went wrong(management)\n");
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
parse_request(ssize_t bytesRcvd) {
    bool retVal = false;

    for (int i = 0; i < PERCY_REQUEST_SIZE; i++) {
        enum percy_request_state state = percy_request_parser_feed(&management.requestParser, management.buffer[i]);
        if (state == percy_request_error) {
            retVal = false;
            break;
        } else if (state == percy_request_done) {
            retVal = true;
            break;
        }
    }

    return retVal;
}

static void
management_close(struct selector_key *key) {
    if (management.socketFd >= 0) {
        close(management.socketFd);
    }
}