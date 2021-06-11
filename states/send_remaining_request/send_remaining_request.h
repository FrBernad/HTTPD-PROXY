#ifndef _SEND_REMAINING_REQUEST_H_
#define _SEND_REMAINING_REQUEST_H_

#include "utils/selector/selector.h"

void 
send_remaining_request_on_arrival(const unsigned state, struct selector_key *key);

unsigned
send_remaining_request_on_write_ready(struct selector_key *key);

#endif
