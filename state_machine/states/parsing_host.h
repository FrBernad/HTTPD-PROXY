#ifndef PARSING_HOST_H
#define PARSING_HOST_H

#include "../../utils/selector.h"

void 
parsing_host_on_arrival(const unsigned state, struct selector_key *key);

unsigned
parsing_host_on_read_ready(struct selector_key *key);

#endif