#ifndef MANAGEMENT_H
#define MANAGEMENT_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <time.h>

typedef struct user {
    char* username;
    char* password;
} user_t;

typedef struct {
    char* client;
    char* origin;
    char* resource;
    struct tm date;
} register_connection_t;

bool addNewUserAndPassword(user_t user);
bool addRegister(register_connection_t connection);
bool initManagement();
void destroyManagement();
void connectFailed();
void connectionClosed();
void newConnectionRegistered();
void decreaseConcurrentConnections();
void addBytesTransfered(uint64_t bytes);


#endif