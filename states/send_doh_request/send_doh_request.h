#ifndef SEND_DOH_REQUEST_H
#define SEND_DOH_REQUEST_H

#include "utils/selector/selector.h"

void 
send_doh_request_on_arrival(const unsigned state, struct selector_key *key);

unsigned
send_doh_request_on_write_ready(struct selector_key *key);


#endif