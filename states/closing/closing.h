#ifndef _CLOSING_H_68358fef672f84939dbd309e4e52caa422b5a140c024afa549c3ebb7
#define _CLOSING_H_68358fef672f84939dbd309e4e52caa422b5a140c024afa549c3ebb7

#include "utils/selector/selector.h"

void 
closing_on_arrival(unsigned state, struct selector_key *key);

unsigned
closing_on_read_ready(struct selector_key *key);

unsigned
closing_on_write_ready(struct selector_key *key);

#endif
