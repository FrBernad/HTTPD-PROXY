#include "closing.h"
#include <stdio.h>

#include "../../utils/connections_def.h"

static void
set_closing_connection_interests(struct selector_key *key);

void closing_on_arrival(const unsigned state, struct selector_key *key) {
    set_closing_connection_interests(key);
}

unsigned
closing_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;
    buffer *clientBuffer = &connection->client_buffer;
    
    set_closing_connection_interests(key);

    if(connection->origin_status == CLOSING_STATUS && connection->client_status == CLOSING_STATUS){
        if (!buffer_can_read(clientBuffer) && !buffer_can_read(originBuffer))
            return DONE;
        return EMPTY_BUFFERS;
    }

    return connection->stm.current->state;
}

// unsigned
// closing_on_write_ready(struct selector_key *key) {
//     proxyConnection *connection = ATTACHMENT(key);
// }

static void
set_closing_connection_interests(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    buffer *originBuffer = &connection->origin_buffer;
    buffer *clientBuffer = &connection->client_buffer;

    fd_interest clientInterest = OP_NOOP;
    fd_interest originInterest = OP_NOOP;

    if (connection->origin_status == CLOSING_STATUS) {
        /*Si el origin esta cerrando la conexión significa que ya no va a leer nada mas de su socket 
        porque no le va a llegar mas nada
        Sin embargo hay que tener en cuenta que si el cliente quiere seguir mandando cosas, tengo que enviarlo a origin*/
        if (buffer_can_read(clientBuffer)) {
            originInterest |= OP_WRITE;
        }

        if (buffer_can_write(clientBuffer)) {
            clientInterest |= OP_READ;
        }
    }

    if (connection->client_status == CLOSING_STATUS) {
        /*Si el cliente esta cerrando la conexión significa que ya no va a leer nada mas de su socket 
        porque no le va a llegar mas nada
        Sin embargo hay que tener en cuenta que si el origin quiere seguir mandando cosas, tengo que enviarlo a cliente*/
        if (buffer_can_read(originBuffer)) {
            clientInterest |= OP_WRITE;
        }

        if (buffer_can_write(originBuffer)) {
            originInterest |= OP_READ;
        }
    }

    if (selector_set_interest(key->s, connection->client_fd, clientInterest) != SELECTOR_SUCCESS)
        printf("error!!\n");  //FIXME: MANEJAR ESTO
    if (selector_set_interest(key->s, connection->origin_fd, originInterest) != SELECTOR_SUCCESS)
        printf("error!!\n");  //FIXME: MANEJAR ESTO
}
