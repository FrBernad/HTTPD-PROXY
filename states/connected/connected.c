#include "connected.h"

#include <stdio.h>

#include "connections/connections_def.h"
#include "headers_parser.h"

static void
set_connection_interests(struct selector_key *key);

/*
    ENTRO AL ESTADO CON EL BUFFER DEL CLIENTE CON INFO Y EL DEL ORIGIN VACIO
*/
void connected_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    set_connection_interests(key);
    headers_parser_init(&connection->client_sniffer.request_header_parser);
    connection->client_sniffer.bytesToSniff = 0;
}

unsigned
connected_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    int bytesToSniff = connection->client_sniffer.bytesToSniff;
    if (bytesToSniff > 0) {
        sniffClient(connection, bytesToSniff);
    }

    if (connection->client_status == CLOSING_STATUS || connection->origin_status == CLOSING_STATUS) {
        return CLOSING;
    }

    set_connection_interests(key);
    return connection->stm.current->state;
}

unsigned
connected_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    set_connection_interests(key);
    return connection->stm.current->state;
}

static void
set_connection_interests(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;
    buffer *clientBuffer = &connection->client_buffer;

    fd_interest clientInterest = OP_NOOP;
    fd_interest originInterest = OP_NOOP;

    /*Si hay algo escrito en el buffer del origin, significa que al clientSocket
        le interesa poder saber cuando puede enviarle bytes al cliente*/
    if (buffer_can_read(originBuffer)) {
        clientInterest |= OP_WRITE;
    }

    /*Si el cliente puede escribir en su buffer, significa que tiene lugar y entonces le interesa
    poder leer del socket ya que no se va a bloquear*/
    if (buffer_can_write(clientBuffer)) {
        clientInterest |= OP_READ;
    }

    /*Si hay algo escrito en el buffer del cliente, significa que al originSocket
        le interesa poder saber cuando puede enviarle bytes al origin*/
    if (buffer_can_read(clientBuffer)) {
        originInterest |= OP_WRITE;
    }

    /*Si el origin puede escribir en su buffer, significa que tiene lugar y entonces le interesa
    poder leer del socket ya que no se va a bloquear*/
    if (buffer_can_write(originBuffer)) {
        originInterest |= OP_READ;
    }

    selector_set_interest(key->s, connection->client_fd, clientInterest);
    selector_set_interest(key->s, connection->origin_fd, originInterest);
}

void snifferClient(proxyConnection *connection, int bytesToSniff) {
    int i = 0;
    int maxBytes;

    char *data = buffer_read_ptr(&connection->client_buffer, &maxBytes);
    while (i < bytesToSniff) {
        headers_parser_feed(&connection->client_sniffer.request_header_parser, data[i]);
    }
}