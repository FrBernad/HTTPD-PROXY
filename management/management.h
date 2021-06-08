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
void increaseHistoricalConnections();
void increaseConcurrentConnections();
void decreaseConcurrentConnections();
void addBytesTransfered(unsigned long bytes);
void destroyManagement();
bool initManagement();


#endif