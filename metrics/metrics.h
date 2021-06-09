#ifndef METRICS_H
#define METRICS_H

void initMetrics();
void increase_bytes_transfered(unsigned long bytes);
void connection_failed_metrics();
void new_connection_connections();
void close_connection();
unsigned long get_historical_connections();
unsigned long get_concurrent_connections();
unsigned long get_failed_connections();
unsigned long get_total_bytes_transfered();


#endif


