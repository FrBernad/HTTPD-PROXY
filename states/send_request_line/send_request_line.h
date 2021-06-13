#ifndef _SEND_REQUEST_LINE_H_ac606b306139910d6c08e0689bd59bfb1d1fd79b4e93f107f1037cb5
#define _SEND_REQUEST_LINE_H_ac606b306139910d6c08e0689bd59bfb1d1fd79b4e93f107f1037cb5

#include "utils/selector/selector.h"

void 
send_request_line_on_arrival(unsigned state, struct selector_key *key);

unsigned
send_request_line_on_write_ready(struct selector_key *key);

#endif
