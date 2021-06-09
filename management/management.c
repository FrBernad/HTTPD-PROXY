#include "management/management.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
};

typedef struct {
    int socketFd;
    struct in_addr ipv4addr;
    in_port_t port;
    uint8_t buffer[MAX_MSG_LEN];
    fd_handler managementHandler;
    struct percy_request_parser requestParser;
    struct request_percy request;
    uint8_t *passphrase = "123456";
} management_t;

static int
create_listener_socket(struct sockaddr_in sockaddr);

static void
init_management_handlers();

static void
management_read(struct selector_key *key);

static void
send_error_reply(struct sockaddr *addr, socklen_t clntAddrLen);

static void 
build_reply(char *reply, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value);

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

    management.port = htons(args.mng_port);

    if (inet_pton(AF_INET, args.mng_addr, &management.ipv4addr) < 0) {
        return -1;
    }

    struct sockaddr_in sockaddr = {
        .sin_addr = management.ipv4addr,
        .sin_family = AF_INET,
        .sin_port = management.port,
    };

    management.socketFd = create_listener_socket(sockaddr);

    if (management.socketFd < 0) {
        return -1;
    }

    init_management_handlers();

    if (selector_register(selector, management.socketFd, &management.managementHandler, OP_READ, NULL) != SELECTOR_SUCCESS) {
        return -1;
    }

    management.requestParser.request = &management.request;
    percy_request_parser_init(&management.requestParser);

    return 1;
}

static int
create_listener_socket(struct sockaddr_in sockaddr) {
    int socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socketfd < 0) {
        fprintf(stderr, "Management socket creation\n");
        return -1;
    }

    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        fprintf(stderr, "Setsockopt opt: SO_REUSEADDR\n");
        return -1;
    }

    if (selector_fd_set_nio(socketfd) < 0) {
        fprintf(stderr, "selector_fd_set_nio\n");
        return -1;
    }

    if (bind(socketfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
        fprintf(stderr, "bind\n");
        return -1;
    }

    return socketfd;
}

static void
init_management_handlers() {
    management.managementHandler.handle_read = management_read;
    management.managementHandler.handle_write = NULL;
    management.managementHandler.handle_close = management_close;
    management.managementHandler.handle_block = NULL;
}

static void
management_read(struct selector_key *key) {
    struct sockaddr_storage clntAddr;

    socklen_t clntAddrLen = sizeof(clntAddr);

    ssize_t bytesRcvd = recvfrom(management.socketFd, management.buffer, MAX_MSG_LEN, 0, (struct sockaddr *)&clntAddr, &clntAddrLen);
    if (bytesRcvd < 0) {
        printf("Smoething went wrong(management)\n");
    }

    if (parse_request(bytesRcvd)) {
        if (!validate_passphrase()) {
            send_reply((struct sockaddr *)&clntAddr, clntAddrLen, PERCY_VERSION, UNAUTH_STATUS, PERCY_RESV, 0);
        }else{
            send_reply((struct sockaddr *)&clntAddr, clntAddrLen, PERCY_VERSION, UNAUTH_STATUS, PERCY_RESV, 50);
        }
    } else {
        send_reply((struct sockaddr *)&clntAddr, clntAddrLen, PERCY_VERSION, UNSUCCESFUL_STATUS, PERCY_RESV, 0);
    }
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

static void build_reply(char *reply, uint8_t ver, uint8_t status, uint8_t resv, uint64_t value) {
    reply[0] = ver;
    reply[1] = status;
    reply[2] = resv;
    uint8_t serialized_value[8] ={0};

    serialize(value,serialized_value);

    memcpy(reply + 3, serialized_value);
}

static void
serialize(uint64_t value, uint8_t serialized_value[8]){
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