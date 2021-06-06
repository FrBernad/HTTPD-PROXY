#ifndef _SEND_REQUEST_LINE_H_
#define _SEND_REQUEST_LINE_H_

#include "../../utils/selector.h"

void 
send_request_line_on_arrival(const unsigned state, struct selector_key *key);

unsigned
send_request_line_on_write_ready(struct selector_key *key);

#endif
