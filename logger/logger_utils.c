#include "logger_utils.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "connections/connections_def.h"
#include "logger.h"

#define MAX_BUFFER 2048
#define IP_MAX 50

void log_new_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    time_t rawtime;
    struct tm *info;

    time(&rawtime);

    info = localtime(&rawtime);

    int bytesWritten = strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ\tA\t", info);

    char clientAddr[IP_MAX] = {0};

    if (connection->client_addr.ss_family == AF_INET) {
        inet_ntop(AF_INET,&((struct sockaddr_in *)&connection->client_addr)->sin_addr, clientAddr, IP_MAX);
        bytesWritten += sprintf(buffer + bytesWritten, "%s\t", clientAddr);
        bytesWritten += sprintf(buffer + bytesWritten, "%d\t",
                                (int)ntohs((((struct sockaddr_in *)&connection->client_addr)->sin_port))
                                        );

    } else {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&connection->client_addr)->sin6_addr, clientAddr, IP_MAX);
        bytesWritten += sprintf(buffer + bytesWritten, "%s\t", clientAddr);
        bytesWritten += sprintf(buffer + bytesWritten, "%d\t",
                                (int)ntohs((((struct sockaddr_in6 *)&connection->client_addr)->sin6_port))
                                        );
    }

    bytesWritten += sprintf(buffer + bytesWritten, "%s\t", connection->connectionRequest.method);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t", connection->connectionRequest.target);
    bytesWritten += sprintf(buffer + bytesWritten, "%d\t\n", 300);  //status_code connection->connectionRequest.originForm

    // fecha tipo de registro direccion IpOrigen puertoOrigen Metodo Target(scheme:host:port:origin) status.
    logger_log(buffer, bytesWritten);
}

void log_user_and_password(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    char buffer[MAX_BUFFER];
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);
    int bytesWritten = strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ\tP\t", info);

    char *protocol = "HTTP";
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t", protocol);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t", connection->connectionRequest.target);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t%s\t\n", connection->sniffer.sniffer_parser.user,
                            connection->sniffer.sniffer_parser.password);

    logger_log(buffer, bytesWritten);
}

void log_connection_closed(struct selector_key *key) {
    /*REGISTRO DE PASSWORDS
       fecha  tipo de registro (P) protocolo (HTTP o POP3)
       destino (nombre  o  dirección  IP) 
       puerto destino
       usuario
       password*/
}

void log_connection_failed(struct selector_key *key) {
    // char buffer[MAX_BUFFER];

    // time_t rawtime;
    // struct tm *info;
    // time(&rawtime);
    // info = localtime(&rawtime);
    // int len = strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", info);

    // if(len == 0){
    //     return;
    // }

    // sprintf(buffer + len, "P\t???");

    /*     logger_log(buffer,); */
}
void log_error(char *msg) {
    // char buffer[MAX_BUFFER];

    // time_t rawtime;
    // struct tm *info;
    // time(&rawtime);
    // info = localtime(&rawtime);
    // int len = strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", info);

    // int n = snprintf(buffer + len, MAX_BUFFER - len, "| ERROR: %s", msg);

    // if(len == 0){
    //     return;
    // }

    // if (n < 0 || n >= MAX_BUFFER - len)
    //     return;

    // logger_log(buffer, len + n);
}
void log_fatal_error(char *msg) {
    // char buffer[MAX_BUFFER];

    // time_t rawtime;
    // struct tm *info;
    // time(&rawtime);
    // info = localtime(&rawtime);
    // int len = strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", info);

    // if(len == 0){
    //     return;
    // }

    // int n = snprintf(buffer + len, MAX_BUFFER - len, "| FATAL: %s", msg);

    // if (n < 0 || n >= MAX_BUFFER - len)
    //     return;

    // logger_log(buffer, len + n);
}
