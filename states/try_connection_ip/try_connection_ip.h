#ifndef _TRY_CONNECTIONS_H_a64a286846d76840d32e3a22a60933575657e87228601fe39c46207f
#define _TRY_CONNECTIONS_H_a64a286846d76840d32e3a22a60933575657e87228601fe39c46207f

#include "utils/selector/selector.h"

void 
try_connection_ip_on_arrival(unsigned state, struct selector_key *key);

unsigned
try_connection_ip_on_write_ready(struct selector_key *key);

#endif
