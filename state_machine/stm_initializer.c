#include "stm_initializer.h"

#include "states/await_doh_response/await_doh_response.h"
#include "states/closing/closing.h"
#include "states/connected/connected.h"
#include "states/done/done.h"
#include "states/empty_buffers/empty_buffers.h"
#include "states/parsing_request_line/parsing_request_line.h"
#include "states/send_doh_request/send_doh_request.h"
#include "states/send_request_line/send_request_line.h"
#include "states/try_connection_ip/try_connection_ip.h"
#include "stm.h"
#include "connections/connections_def.h"

//   unsigned (*on_read_ready) (struct selector_key *key);
//     /** ejecutado cuando hay datos disponibles para ser escritos */
//     unsigned (*on_write_ready)(struct selector_key *key);
//     /** ejecutado cuando hay una resolución de nombres lista */
//     unsigned (*on_block_ready)(struct selector_key *key);

static const struct state_definition connection_states[] = {
    {.state = PARSING_REQUEST_LINE,
     .on_arrival = parsing_host_on_arrival,
     .on_departure = NULL,
     .on_read_ready = parsing_host_on_read_ready,
     .on_write_ready = NULL,
     .on_block_ready = NULL},
    {.state = TRY_CONNECTION_IP,
     .on_arrival = try_connection_ip_on_arrival,
     .on_departure = NULL,
     .on_read_ready = NULL,
     .on_write_ready = try_connection_ip_on_write_ready,
     .on_block_ready = NULL},
    {.state = SEND_DOH_REQUEST,
     .on_arrival = send_doh_request_on_arrival,
     .on_departure = NULL,
     .on_read_ready = NULL,
     .on_write_ready = send_doh_request_on_write_ready,
     .on_block_ready = NULL},
    {.state = AWAIT_DOH_RESPONSE,
     .on_arrival = await_doh_response_on_arrival,
     .on_departure = NULL,
     .on_read_ready = await_doh_response_on_read_ready,
     .on_write_ready = NULL,
     .on_block_ready = NULL},
    {.state = SEND_REQUEST_LINE,
     .on_arrival = send_request_line_on_arrival,
     .on_departure = NULL,
     .on_read_ready = NULL,
     .on_write_ready = send_request_line_on_write_ready},
    {.state = CONNECTED,
     .on_arrival = connected_on_arrival,
     .on_departure = NULL,
     .on_read_ready = connected_on_read_ready,
     .on_write_ready = connected_on_write_ready,
     .on_block_ready = NULL},
    {.state = CLOSING,
     .on_arrival = closing_on_arrival,
     .on_departure = NULL,
     .on_read_ready = closing_on_read_ready,
     .on_write_ready = closing_on_write_ready,
     .on_block_ready = NULL},
    {.state = EMPTY_BUFFERS,
     .on_arrival = empty_buffers_on_arrival,
     .on_departure = NULL,
     .on_read_ready = NULL,
     .on_write_ready = empty_buffers_on_write_ready,
     .on_block_ready = NULL},
    {.state = DONE,
     .on_arrival = done_on_arrival,
     .on_departure = NULL,
     .on_read_ready = NULL,
     .on_write_ready = NULL,
     .on_block_ready = NULL},
    {.state = ERROR,
     .on_arrival = NULL,
     .on_departure = NULL,
     .on_read_ready = NULL,
     .on_write_ready = NULL,
     .on_block_ready = NULL}};

void init_connection_stm(struct state_machine *stm) {
    stm_init(stm, PARSING_REQUEST_LINE, ERROR, connection_states);
}
