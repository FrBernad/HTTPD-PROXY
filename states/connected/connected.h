#ifndef _CONNECTED_H_4081f12b10e32102b271e48f0977bb6a6b88464f883b334a30dbbbdc
#define _CONNECTED_H_4081f12b10e32102b271e48f0977bb6a6b88464f883b334a30dbbbdc

#include "utils/selector/selector.h"

void 
connected_on_arrival(unsigned state, struct selector_key *key);

unsigned
connected_on_read_ready(struct selector_key *key);

unsigned
connected_on_write_ready(struct selector_key *key);

#endif
