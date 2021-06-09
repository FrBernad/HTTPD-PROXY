#include "metrics.h"

#include <string.h>

static struct proxy_metrics {
    uint64_t historical_connections;
    uint64_t concurrent_connections;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    uint64_t failed_connections;
} proxy_metrics;

void init_metrics() {
    memset(&proxy_metrics, 0, sizeof(proxy_metrics));
}

void increase_bytes_received(uint64_t bytes) {
    if (proxy_metrics.bytes_received < UINT64_MAX){
        proxy_metrics.bytes_received += bytes;
    }
}

void increase_bytes_sent(uint64_t bytes) {
    if (proxy_metrics.bytes_sent < UINT64_MAX){
        proxy_metrics.bytes_sent += bytes;
    }
}

void increase_failed_connections() {
    if (proxy_metrics.failed_connections < UINT64_MAX){
        proxy_metrics.failed_connections++;
    }
}

void register_new_connection() {
    if (proxy_metrics.historical_connections < UINT64_MAX){
        proxy_metrics.historical_connections++;
    }
    if (proxy_metrics.concurrent_connections < UINT64_MAX){
        proxy_metrics.concurrent_connections++;
    }
}

void unregister_connection() {
    if (proxy_metrics.concurrent_connections > 0){
        proxy_metrics.concurrent_connections--;
    }
}

uint64_t get_historical_connections(){
    return proxy_metrics.historical_connections;
}

uint64_t get_concurrent_connections(){ 
    return proxy_metrics.concurrent_connections;
}

uint64_t get_failed_connections(){
    return proxy_metrics.failed_connections;
}

uint64_t get_total_bytes_transfered(){
    return proxy_metrics.bytes_received + proxy_metrics.bytes_sent;
}

uint64_t get_total_bytes_sent() {
    return proxy_metrics.bytes_sent;
}

uint64_t get_total_bytes_received() {
    return proxy_metrics.bytes_received;
}
