#ifndef _SEND_DOH_REQUEST_H_21404efdc7b518e90527f77123daa1923ccbdca13c3c2954eca3fcd9
#define _SEND_DOH_REQUEST_H_21404efdc7b518e90527f77123daa1923ccbdca13c3c2954eca3fcd9

#include "utils/selector/selector.h"

void 
send_doh_request_on_arrival(unsigned state, struct selector_key *key);

unsigned
send_doh_request_on_write_ready(struct selector_key *key);


#endif