#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>

void
init_metrics();

void
increase_bytes_received(uint64_t bytes);

void
increase_bytes_sent(uint64_t bytes);

void
increase_failed_connections();

void
register_new_connection();

void
unregister_connection();


uint64_t
get_historical_connections();

uint64_t
get_concurrent_connections();

uint64_t
get_failed_connections();

uint64_t
get_total_bytes_transferred();

uint64_t
get_total_bytes_sent();

uint64_t
get_total_bytes_received();


#endif


