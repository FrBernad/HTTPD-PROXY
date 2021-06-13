#ifndef _CONNECTIONS_MANAGER_H_
#define _CONNECTIONS_MANAGER_H_

#include "utils/selector/selector.h"
#include <stdint.h>

void
init_connections_manager(double threshold);

void
accept_new_connection(struct selector_key *key);

void
connection_garbage_collect(struct selector_key *key);

int 
register_origin_socket(struct selector_key *key);

uint64_t
get_buffer_size();

void
set_buffer_size(uint16_t new_buff_size);


#endif