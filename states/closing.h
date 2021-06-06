#ifndef CLOSING_H
#define CLOSING_H

#include "utils/selector.h"

void 
closing_on_arrival(const unsigned state, struct selector_key *key);

unsigned
closing_on_read_ready(struct selector_key *key);

unsigned
closing_on_write_ready(struct selector_key *key);

#endif
