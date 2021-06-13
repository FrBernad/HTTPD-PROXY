#ifndef _AWAIT_DOH_RESPONSE_H_98d74d29570a086c5a5c5289543193595d1c1dacedc6792180ecadc1
#define _AWAIT_DOH_RESPONSE_H_98d74d29570a086c5a5c5289543193595d1c1dacedc6792180ecadc1

#include "utils/selector/selector.h"

void 
await_doh_response_on_arrival(unsigned state, struct selector_key *key);

unsigned
await_doh_response_on_read_ready(struct selector_key *key);

#endif