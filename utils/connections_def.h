#ifndef CONNECTIONS_DEF_H
#define CONNECTIONS_DEF_H

#include <netdb.h>
#include <sys/types.h>

#include "../parsers/doh_parser.h"
#include "../parsers/request_parser.h"
#include "../state_machine/stm.h"
#include "buffer.h"

#define ATTACHMENT(key) ((proxyConnection *)(key)->data)
#define REQUEST_LINE_MAX 1200

enum connection_state {
    PARSING_REQUEST_LINE = 0,
    TRY_CONNECTION_IP,
    SEND_DOH_REQUEST,
    DOH_RESPONSE,
    DOH_RESOLVE_REQUEST_IPV4,
    DOH_RESOLVE_REQUEST_IPV6,

    SEND_REQUEST_LINE,
    CONNECTED,

    CLOSING,
    EMPTY_BUFFERS,

    DONE,

    ERROR
};

typedef enum doh_state {
    DOH_CONNECTING,
    DOH_CONNECTED,
} doh_state_t;

typedef enum connection_status {
    ACTIVE_STATUS,
    CLOSING_STATUS,
} connection_status_t;

typedef struct request_line_st {
    buffer *buffer;
    struct request_line request;
    struct request_parser request_parser;
} request_line_st;

typedef struct doh_st {
    doh_state_t state;
    buffer *buffer;
    struct doh_response response;
    struct doh_response_parser dohParser;
} doh_st;

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
        struct doh_st doh;
    } client;

    connection_request_t connectionRequest;

    buffer origin_buffer, client_buffer;

} proxyConnection;

#endif
