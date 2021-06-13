#ifndef _ERROR_H_0e6d975d33cb7462212c76baf66d87397226e7e10ce167a66554489b
#define _ERROR_H_0e6d975d33cb7462212c76baf66d87397226e7e10ce167a66554489b

#include "utils/selector/selector.h"

void
error_on_arrival(unsigned state, struct selector_key *key);

unsigned
error_on_write_ready(struct selector_key *key);

#endif