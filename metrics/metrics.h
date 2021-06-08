#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct global_proxy_metrics
{
    u_int64_t historical_connections;
    u_int16_t current_connections;
    u_int64_t total_bytes_transfer;
    u_int64_t failed_connections;

}global_proxy_metrics;

void connection_failed_metrics(global_proxy_metrics  *metrics);
void new_connection_added_metrics(global_proxy_metrics  *metrics);
void connection_closed_metrics(global_proxy_metrics  *metrics);
void add_n_bytes_sent_metrics(global_proxy_metrics  *metrics, uint64_t n_bytes);

#endif


