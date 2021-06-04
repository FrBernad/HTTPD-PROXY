#include "stm_initializer.h"
#include "stm.h"
#include "../utils/connections_def.h"
#include "states/parsing_host.h"
#include "states/try_connection_ip.h"

static const struct state_definition connection_states[] = {
    {
        .state = PARSING_HOST,
        .on_arrival = parsing_host_on_arrival,
        .on_read_ready = parsing_host_on_read_ready,
    },
    {
        .state = TRY_CONNECTION_IP,
        .on_arrival = try_connection_ip_on_arrival,
        .on_write_ready = try_connection_ip_on_write_ready,
    },
    {
        .state = DOH_REQUEST,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_RESPONSE,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = TRY_CONNECTION_DOH_SERVER,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_RESOLVE_REQUEST_IPV4,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DOH_RESOLVE_REQUEST_IPV6,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = CONNECTED,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = DONE,
        //  .on_arrival =,
        //  .on_read_ready =
    },
    {
        .state = ERROR,
        //  .on_arrival =,
        //  .on_write_ready =
    }
};

void 
init_connection_stm(struct state_machine *stm) {
    stm_init(stm, PARSING_HOST, ERROR, connection_states);
}
