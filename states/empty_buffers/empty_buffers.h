#ifndef _EMPTY_BUFFERS_H_baa55a654f9603962f1b91f1b63771b3a1d28c0b9d5434e4cd362e5d
#define _EMPTY_BUFFERS_H_baa55a654f9603962f1b91f1b63771b3a1d28c0b9d5434e4cd362e5d

#include "utils/selector/selector.h"

void
empty_buffers_on_arrival(unsigned state, struct selector_key *key);

unsigned
empty_buffers_on_write_ready(struct selector_key *key);

#endif
