#include "metrics.h"

#include <string.h>

static struct proxy_metrics {
    uint64_t historical_connections;
    uint16_t concurrent_connections;
    uint64_t total_bytes_transfer;
    uint64_t failed_connections;
} proxy_metrics;

void init_metrics() {
    memset(&proxy_metrics, 0, sizeof(proxy_metrics));
}

void increase_bytes_transfered(uint64_t bytes) {
    if (proxy_metrics.total_bytes_transfer < UINT64_MAX)
        proxy_metrics.total_bytes_transfer += bytes;
}

void increase_failed_connections() {
    if (proxy_metrics.failed_connections < UINT64_MAX)
        proxy_metrics.failed_connections++;
}

void register_new_connection() {
    if (proxy_metrics.historical_connections < UINT64_MAX)
        proxy_metrics.historical_connections++;
    if (proxy_metrics.concurrent_connections < UINT16_MAX)
        proxy_metrics.concurrent_connections++;
}

void close_connection() {
    if (proxy_metrics.concurrent_connections > 0)
        proxy_metrics.concurrent_connections--;
}

unsigned long get_historical_connections(){
    return proxy_metrics.historical_connections;
}

unsigned long get_concurrent_connections(){ 
    return proxy_metrics.concurrent_connections;
}

unsigned long get_failed_connections(){
    return proxy_metrics.failed_connections;
}

unsigned long get_total_bytes_transfered(){
    return proxy_metrics.total_bytes_transfer;
}


