#ifndef CONNECTIONS_DEF_H
#define CONNECTIONS_DEF_H

#include <netdb.h>
#include <sys/types.h>
#include <time.h>

#include "parsers/doh_parser/doh_parser.h"
#include "parsers/headers_parser/headers_parser.h"
#include "parsers/request_line_parser/request_line_parser.h"
#include "parsers/sniffer_parser/sniffer_parser.h"
#include "parsers/status_line_parser/status_line_parser.h"
#include "state_machine/stm.h"
#include "utils/buffer/buffer.h"

#define ATTACHMENT(key) ((proxy_connection_t *)(key)->data)

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
    CLOSED_STATUS,
} connection_status_t;

typedef struct request_line_t {
    buffer *buffer;
    struct request_line request;
    struct request_parser request_parser;
} request_line_st;

typedef struct doh_connection {
    bool is_active;
    uint8_t current_try;
    enum {
        ipv4_try,
        ipv6_try
    } current_type;
    struct status_line status_line;
    struct status_line_parser status_line_parser;
    struct headers_parser headers_parser;
    struct doh_response doh_response;
    struct doh_response_parser doh_parser;
} doh_connection_t;

typedef struct status_line_t {
    struct status_line status_line;
    struct status_line_parser status_line_parser;
    bool done;
} status_line_t;

typedef struct connection_request {
    uint8_t request_line[
            MAX_METHOD_LENGTH + SCHEME_LENGTH + MAX_FQDN_LENGTH + MAX_PORT_LENGTH + MAX_ORIGIN_FORM + VERSION_LENGTH +
            16];
    uint8_t method[MAX_METHOD_LENGTH];
    uint8_t target[SCHEME_LENGTH + MAX_FQDN_LENGTH + MAX_PORT_LENGTH + MAX_ORIGIN_FORM + 16];
    uint16_t status_code;

    bool connect;

    enum host_type host_type;
    union {
        char domain[MAX_FQDN_LENGTH + 1];
        struct sockaddr_in ipv4;
        struct sockaddr_in6 ipv6;
    } host;
    in_port_t port;
} connection_request_t;

typedef struct sniffer_t {
    struct sniffer_parser sniffer_parser;
    bool sniff_enabled;
    bool is_done;
} sniffer_t;

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
    HTTP_VERSION_NOT_SUPPORTED = 505
} errors_t;

typedef struct proxy_connection {

    /*Tipo de error de la conexion*/
    errors_t error;

    /*Timestamp de ultima interaccion en la conexion*/
    time_t last_action_time;

    /*Informacion del cliente*/
    struct sockaddr_storage client_addr;
    int client_fd;
    connection_status_t client_status;  //usado para determinar si quiere cerrar la conexión pero puede que todavia quede algo en el buffer

    doh_connection_t *doh_connection;

    /*información del origin server*/
    int origin_fd;
    connection_status_t origin_status;  //usado para determinar si quiere cerrar la conexión pero puede que todavia quede algo en el buffer

    /*maquinas de estados*/
    struct state_machine stm;

    ssize_t bytes_to_analize;

    /*Parsers de la conexion*/
    union client_state {
        struct request_line_t request_line;
        headers_parser_t headers;
        status_line_t status_line;
    } connection_parsers;

    sniffer_t sniffer;

    connection_request_t connection_request;

    buffer origin_buffer, client_buffer;

} proxy_connection_t;

#endif
