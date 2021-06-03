#ifndef _DOH_UTILS_H_
#define _DOH_UTILS_H_

#include <stdint.h>

/*
*  recieves a domain and queryType and stores the doh query in dst returning the size of the request.
*/
int build_doh_request(uint8_t *dst, const uint8_t *domain, uint8_t queryType);

#endif
