#include "management.h"

#include <string.h>

#include "utils/list/listADT.h"

typedef struct {
    unsigned long historicalConnections;
    unsigned long concurrentConnections;
    unsigned long bytesTransfered;
    list_adt userList;
    list_adt registers;
} management_t;

static management_t management;

bool initManagement() {
    management.historicalConnections = 0;
    management.concurrentConnections = 0;
    management.bytesTransfered = 0;
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

void increaseHistoricalConnections() {
    management.historicalConnections += 1;
}

void increaseConcurrentConnections() {
    management.concurrentConnections += 1;
}

void decreaseConcurrentConnections() {
    management.concurrentConnections -= 1;
}

void addBytesTransfered(unsigned long bytes) {
    management.bytesTransfered += bytes;
}
