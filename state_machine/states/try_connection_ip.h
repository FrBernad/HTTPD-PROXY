#ifndef _TRY_CONNECTIONS_H_
#define _TRY_CONNECTIONS_H_

#include "../../utils/selector.h"

void 
try_connection_ip_on_arrival(const unsigned state, struct selector_key *key);

unsigned
try_connection_ip_on_write_ready(struct selector_key *key);

#endif
