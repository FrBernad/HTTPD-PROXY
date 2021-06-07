#ifndef _CONNECTIONS_H_
#define _CONNECTIONS_H_

#include "utils/selector/selector.h"

void 
accept_new_ipv4_connection(struct selector_key *key);

void 
accept_new_ipv6_connection(struct selector_key *key);

void 
init_selector_handlers();

int 
register_origin_socket(struct selector_key *key);

#endif