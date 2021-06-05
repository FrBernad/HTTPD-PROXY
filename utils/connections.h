#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

#include "selector.h"

int
accept_new_connection(struct selector_key *key);

void 
init_selector_handlers();

int 
register_origin_socket(struct selector_key *key);

#endif