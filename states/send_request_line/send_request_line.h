#ifndef _SEND_REQUEST_LINE_H_
#define _SEND_REQUEST_LINE_H_

#include "utils/selector/selector.h"

void 
send_request_line_on_arrival(unsigned state, struct selector_key *key);

unsigned
send_request_line_on_write_ready(struct selector_key *key);

#endif
