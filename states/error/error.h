#ifndef ERROR_H
#define ERROR_H

#include "utils/selector/selector.h"

void
error_on_arrival(unsigned state, struct selector_key *key);

unsigned
error_on_write_ready(struct selector_key *key);

#endif