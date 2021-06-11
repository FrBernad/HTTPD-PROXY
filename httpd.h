#ifndef _HTTPD_H_
#define _HTTPD_H_

#include "utils/args/args.h"

struct http_args
get_httpd_args();

uint64_t get_selector_timeout();

void set_disectors_enabled();

void set_disectors_disabled();

#endif
