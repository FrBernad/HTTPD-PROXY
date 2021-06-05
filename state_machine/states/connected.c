#include "connected.h"

#include "../../utils/connections_def.h"

#include <stdio.h>

static void
set_connection_interests(struct selector_key *key);

/*
    ENTRO AL ESTADO CON EL BUFFER DEL CLIENTE CON INFO Y EL DEL ORIGIN VACIO
*/
void
connected_on_arrival(const unsigned state, struct selector_key *key) {
    set_connection_interests(key);
}

unsigned
connected_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
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

    if (selector_set_interest(key->s, connection->client_fd, clientInterest) != SELECTOR_SUCCESS)
        printf("error!!\n");//FIXME: MANEJAR ESTO
    if (selector_set_interest(key->s, connection->origin_fd, originInterest) != SELECTOR_SUCCESS)
        printf("error!!\n");//FIXME: MANEJAR ESTO

}
