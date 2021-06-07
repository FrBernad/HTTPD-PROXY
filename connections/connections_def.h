#ifndef CONNECTIONS_DEF_H
#define CONNECTIONS_DEF_H

#include <netdb.h>
#include <sys/types.h>

#include "parsers/doh_parser/doh_parser.h"
#include "parsers/headers_parser/headers_parser.h"
#include "parsers/request_line_parser/request_line_parser.h"
#include "parsers/status_line_parser/status_line_parser.h"
#include "state_machine/stm.h"
#include "utils/buffer/buffer.h"

#define ATTACHMENT(key) ((proxyConnection *)(key)->data)
#define REQUEST_LINE_MAX 1200

enum connection_state {
    PARSING_REQUEST_LINE = 0,
    TRY_CONNECTION_IP,
    SEND_DOH_REQUEST,
    AWAIT_DOH_RESPONSE,

    SEND_REQUEST_LINE,
    CONNECTED,

    CLOSING,
    EMPTY_BUFFERS,

    DONE,

    ERROR
};

typedef enum connection_status {
    INACTIVE_STATUS,
    ACTIVE_STATUS,
    CLOSING_STATUS,
} connection_status_t;

typedef struct request_line_st {
    buffer *buffer;
    struct request_line request;
    struct request_parser request_parser;
} request_line_st;

typedef struct doh_connection {
    bool isActive;
    uint8_t currentTry;
    enum {
        ipv4_try,
        ipv6_try
    } currentType;
    struct status_line statusLine;
    struct status_line_parser statusLineParser;
    struct headers_parser headersParser;
    struct doh_response dohResponse;
    struct doh_response_parser dohParser;
} doh_connection_t;

typedef struct connection_request {
    uint8_t requestLine[REQUEST_LINE_MAX];
    enum host_type host_type;
    union {
        char domain[MAX_FQDN_LENGTH + 1];
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    } host;
    in_port_t port;
} connection_request_t;

typedef enum {
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    PROXY_AUTHENTICATION_REQUIRED = 407,
    URI_TOO_LONG = 414,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    HTTP_VERSION_NOT_SUPPORTED=505
} errors_t;

    typedef struct proxyConnection {
    /*Informacion del cliente*/
    struct sockaddr_storage client_addr;
    socklen_t client_addr_en;
    int client_fd;
    connection_status_t client_status;  //usado para determinar si quiere cerrar la conexión pero puede que todavia quede algo en el buffer

    /* resolucion de la direccion del origin server*/
    struct addrinfo origin_resolution;
    /*intento actual de la direccion del origin server*/
    struct addrinfo origin_resolution_current;

    /*información del origin server*/
    struct sockaddr_storage origin_addr;
    socklen_t origin_addr_en;
    int origin_domain;
    int origin_fd;
    connection_status_t origin_status;  //usado para determinar si quiere cerrar la conexión pero puede que todavia quede algo en el buffer

    /*maquinas de estados*/
    struct state_machine stm;

    // estados para el cliente
    union client_state {
        struct request_line_st request_line;
    } client;

    doh_connection_t *dohConnection;

    connection_request_t connectionRequest;
    
    errors_t error;

    buffer origin_buffer, client_buffer;

} proxyConnection;

#endif
