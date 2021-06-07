#ifndef _DOH_UTILS_H_
#define _DH_UTILS_H_

#include <stdint.h>

#include "utils/selector/selector.h"

/*
*  recieves a domain and queryType and stores the doh query in dst returning the size of the request.
*/
int 
build_doh_request(uint8_t *dst, uint8_t *domain, uint8_t queryType);

unsigned
handle_origin_doh_connection(struct selector_key *key);

unsigned
try_next_dns_connection(struct selector_key *key);

#endif
