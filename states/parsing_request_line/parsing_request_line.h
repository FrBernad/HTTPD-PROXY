#ifndef _PARSING_REQUEST_LINE_H_c8460b856a4ada55bcd5113e75973b164a945486978b012993183a48
#define _PARSING_REQUEST_LINE_H_c8460b856a4ada55bcd5113e75973b164a945486978b012993183a48

#include "utils/selector/selector.h"

void 
parsing_host_on_arrival(unsigned state, struct selector_key *key);

unsigned
parsing_host_on_read_ready(struct selector_key *key);

#endif