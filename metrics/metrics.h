#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>

void init_metrics();
void increase_bytes_transfered(uint64_t bytes);
void increase_failed_connections();
void register_new_connection();
void close_connection();
unsigned long get_historical_connections();
unsigned long get_concurrent_connections();
unsigned long get_failed_connections();
unsigned long get_total_bytes_transfered();


#endif


