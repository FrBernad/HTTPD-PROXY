#include "management.h"

#include <string.h>

#include "utils/list/listADT.h"
#include "../metrics/metrics.h"

typedef struct {
    global_proxy_metrics metrics;
    list_adt userList;
    list_adt registers;
} management_t;

static management_t management;

bool initManagement() {
    memset(&(management.metrics), 0, sizeof(global_proxy_metrics));
    management.userList = new_list(sizeof(user_t));
    if (management.userList == NULL) {
        return false;
    }
    management.registers = new_list(sizeof(register_connection_t));
    if (management.registers == NULL) {
        free_list(management.userList);
        return false;
    }
    return true;
}

void destroyManagement() {
    free_list(management.userList);
    free_list(management.registers);
}

bool addNewUserAndPassword(user_t user) {
    return add_to_list(management.userList, &user);
}

bool addRegister(register_connection_t connection) {
    return add_to_list(management.registers, &connection);
}

void connectFailed(){
    connection_failed_metrics(&(management.metrics));
}

void connectionClosed(){
    connection_closed_metrics(&(management.metrics));
}

void newConnectionRegistered(){
    new_connection_added_metrics(&(management.metrics));
}

void decreaseConcurrentConnections() {
    connection_closed_metrics(&(management.metrics));
}

void addBytesTransfered(uint64_t bytes) {
    add_n_bytes_sent_metrics(&(management.metrics), bytes);
}
