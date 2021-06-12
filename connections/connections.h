#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

#include "utils/selector/selector.h"
#include <stdint.h>

void 
accept_new_connection(struct selector_key *key);

void 
init_connections_manager(double threshold);

void
connection_garbage_collect(struct selector_key *key);

int 
register_origin_socket(struct selector_key *key);

uint64_t
get_buffer_size();

#endif