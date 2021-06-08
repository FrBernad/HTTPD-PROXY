#include "metrics.h"

void add_n_bytes_sent_metrics(global_proxy_metrics *metrics, uint64_t n_bytes){

    if(metrics->total_bytes_transfer < UINT64_MAX)
        metrics->total_bytes_transfer+=n_bytes;
}

void connection_failed_metrics(global_proxy_metrics *metrics){
    if(metrics->total_bytes_transfer < UINT64_MAX)
        metrics->failed_connections++;
}

void new_connection_added_metrics(global_proxy_metrics *metrics){

    if(metrics->total_bytes_transfer < UINT64_MAX)
        metrics->historical_connections++;
     if(metrics->total_bytes_transfer < UINT16_MAX)
        metrics->current_connections++;

}

void connection_closed_metrics(global_proxy_metrics *metrics){

    if(metrics->current_connections != 0)
        metrics->current_connections--;
}


 

