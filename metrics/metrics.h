
#include <stdint.h>
#include <errno.h>
#include <stdio.h>

typedef struct global_proxy_metrics
{
    u_int64_t historical_connections;
    u_int16_t current_connections;
    u_int64_t total_bytes_transfer;
    u_int64_t failed_connections;

}global_proxy_metrics;


void init_metric();
void new_connection_added();
void connection_failed();
void add_n_bytes_sent(uint64_t n_bytes);
void connection_closed();




