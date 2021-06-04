#ifndef CONNECTIONS_DEF_H
#define CONNECTIONS_DEF_H

#include "../parsers/doh_parser.h"
#include "../parsers/request.h"
#include "../state_machine/stm.h"
#include "buffer.h"

#include <netdb.h>
#include <sys/types.h>

#define ATTACHMENT(key) ((proxyConnection *)(key)->data)


enum connection_state {
    PARSING_HOST = 0,
    TRY_CONNECTION_IP,
    DOH_REQUEST,
    DOH_RESPONSE,
    TRY_CONNECTION_DOH_SERVER,
    DOH_RESOLVE_REQUEST_IPV4,
    DOH_RESOLVE_REQUEST_IPV6,

    CONNECTED,
    DONE,
    ERROR
};

typedef struct request_line_st {
    buffer *buffer;
    struct request_line request;
    struct request_parser request_parser;
} request_line_st;

typedef struct doh_response_st {
    buffer *buffer;
    struct doh_response response;
    struct doh_response_parser dohParser;
} doh_response_st;

typedef struct proxyConnection {
    /*Informacion del cliente*/
    struct sockaddr_storage client_addr;
    socklen_t client_addr_en;
    int client_fd;

    /* resolucion de la direccion del origin server*/
    struct addrinfo origin_resolution;
    /*intento actual de la direccion del origin server*/
    struct addrinfo origin_resolution_current;

    /*información del origin server*/
    struct sockaddr_storage origin_addr;
    socklen_t origin_addr_en;
    int origin_domain;
    int origin_fd;

    /*maquinas de estados*/
    struct state_machine stm;

    // estados para el cliente
    union client_state {
        struct request_line_st request_line;
        struct doh_response_st doh_response;
    } client;

    // estados para el origin
    // union{
    // estados
    // }

    buffer origin_buffer, client_buffer;

} proxyConnection;

#endif

