#include "connected.h"

#include <stdio.h>

#include "connections/connections_def.h"
#include "httpd.h"
#include "logger/logger_utils.h"
#include "parsers/status_line_parser/status_line_parser.h"
#include "utils/sniffer/sniffer_utils.h"

static void
set_connection_interests(struct selector_key *key);

void init_http_response(struct selector_key *key);

/*
    ENTRO AL ESTADO CON EL BUFFER DEL CLIENTE CON INFO Y EL DEL ORIGIN VACIO
*/

void connected_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    if (!connection->connectionRequest.connect) {
        init_http_response(key);
    }
    set_connection_interests(key);
}

unsigned
connected_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    if (!connection->connectionRequest.connect && key->fd == connection->origin_fd && !connection->http_response.done) {
        status_line_state state;

        size_t maxBytes;
        uint8_t *data = buffer_read_ptr(&connection->origin_buffer, &maxBytes);
        uint8_t *dataToParse = data;

        for (int i = 0; i < connection->bytesToAnalize; i++) {
            state = status_line_parser_feed(&connection->http_response.statusLineParser, dataToParse[i]);
            if (state == status_line_done) {
                connection->connectionRequest.status_code = connection->http_response.statusLine.status_code;
                connection->http_response.done = true;
                log_new_connection(key);
                break;
            } else if (state == status_line_error) {
                connection->http_response.done = true;
                break;
            }
        }
    }

    if (connection->client_status == CLOSING_STATUS || connection->origin_status == CLOSING_STATUS) {
        return CLOSING;
    }

    //    if (connection->sniffer.sniffEnabled && !connection->sniffer.isDone) {
    //        sniff_data(key);
    //    }

    set_connection_interests(key);
    return stm_state(&connection->stm);
}

unsigned
connected_on_write_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    set_connection_interests(key);
    return stm_state(&connection->stm);
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

void init_http_response(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    connection->http_response.done = false;
    connection->http_response.statusLineParser.status_line = &connection->http_response.statusLine;
    status_line_parser_init(&connection->http_response.statusLineParser);
}