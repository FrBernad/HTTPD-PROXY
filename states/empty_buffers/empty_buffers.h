#ifndef EMPTY_BUFFERS_H
#define EMPTY_BUFFERS_H

#include "utils/selector/selector.h"

void
empty_buffers_on_arrival(unsigned state, struct selector_key *key);

unsigned
empty_buffers_on_write_ready(struct selector_key *key);

#endif
