#include "connected.h"

#include "connections_manager/connections_def.h"
#include "logger/logger_utils.h"
#include "utils/sniffer/sniffer_utils.h"

static void
set_connection_interests(struct selector_key *key);

void init_http_response(struct selector_key *key);

/*
    ENTRO AL ESTADO CON EL BUFFER DEL CLIENTE CON INFO Y EL DEL ORIGIN VACIO
*/

void connected_on_arrival(unsigned state, struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    if (!connection->connection_request.connect) {
        init_http_response(key);
    }
    set_connection_interests(key);
}

unsigned
connected_on_read_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    // Parse http response
    if (!connection->connection_request.connect && key->fd == connection->origin_fd && !connection->connection_parsers.status_line.done) {
        status_line_state state;

        size_t max_bytes;
        uint8_t *data = buffer_read_ptr(&connection->origin_buffer, &max_bytes);

        for (int i = 0; i < connection->bytes_to_analize; i++) {
            state = status_line_parser_feed(&connection->connection_parsers.status_line.status_line_parser, data[i]);
            if (state == status_line_done) {
                connection->connection_request.status_code = connection->connection_parsers.status_line.status_line.status_code;
                connection->connection_parsers.status_line.done = true;
                log_new_connection(key);
                break;
            } else if (state == status_line_error) {
                connection->connection_parsers.status_line.done = true;
                break;
            }
        }
    }

    if (connection->sniffer.sniff_enabled && !connection->sniffer.is_done) {
        sniff_data(key);
    }

    if (connection->client_status == CLOSING_STATUS || connection->origin_status == CLOSING_STATUS) {
        return CLOSING;
    }

    set_connection_interests(key);
    return stm_state(&connection->stm);
}

unsigned
connected_on_write_ready(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    set_connection_interests(key);
    return stm_state(&connection->stm);
}

static void
set_connection_interests(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    buffer *origin_buffer = &connection->origin_buffer;
    buffer *client_buffer = &connection->client_buffer;

    fd_interest client_interest = OP_NOOP;
    fd_interest origin_interest = OP_NOOP;

    /*Si hay algo escrito en el buffer del origin, significa que al clientSocket
        le interesa poder saber cuando puede enviarle bytes al cliente*/
    if (buffer_can_read(origin_buffer)) {
        client_interest |= OP_WRITE;
    }

    /*Si el cliente puede escribir en su buffer, significa que tiene lugar y entonces le interesa
    poder leer del socket ya que no se va a bloquear*/
    if (buffer_can_write(client_buffer)) {
        client_interest |= OP_READ;
    }

    /*Si hay algo escrito en el buffer del cliente, significa que al originSocket
        le interesa poder saber cuando puede enviarle bytes al origin*/
    if (buffer_can_read(client_buffer)) {
        origin_interest |= OP_WRITE;
    }

    /*Si el origin puede escribir en su buffer, significa que tiene lugar y entonces le interesa
    poder leer del socket ya que no se va a bloquear*/
    if (buffer_can_write(origin_buffer)) {
        origin_interest |= OP_READ;
    }

    selector_set_interest(key->s, connection->client_fd, client_interest);
    selector_set_interest(key->s, connection->origin_fd, origin_interest);
}

void init_http_response(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);
    connection->connection_parsers.status_line.done = false;
    connection->connection_parsers.status_line.status_line_parser.status_line = &connection->connection_parsers.status_line.status_line;
    status_line_parser_init(&connection->connection_parsers.status_line.status_line_parser);
}