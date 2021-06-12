#include "logger_utils.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>

#include "connections/connections_def.h"
#include "logger.h"

#define MAX_BUFFER 2048
#define IP_MAX 50

char * level_texts[] = {"TRACE","DEBUG","INFO","WARN", "ERROR", "FATAL"};

static unsigned get_time(char * buffer);
static unsigned write_client_addr_n_port(proxyConnection *connection , char * buffer, unsigned bytesWritten);

void log_new_connection(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytesWritten = get_time(buffer);

    bytesWritten += sprintf(buffer + bytesWritten, "A\t");
    bytesWritten = write_client_addr_n_port(connection,buffer,bytesWritten);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t%s\t%d\t\n", connection->connectionRequest.method,
                            connection->connectionRequest.target,connection->connectionRequest.status_code);

    // fecha tipo de registro direccion IpOrigen puertoOrigen Metodo Target(scheme:host:port:origin) status.
    logger_log(buffer, bytesWritten);
}

void log_user_and_password(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytesWritten = get_time(buffer);

    bytesWritten += sprintf(buffer + bytesWritten, "P\t%s\t", connection->sniffer.sniffer_parser.sniffedProtocol == HTTP?"HTTP":"POP3");
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t", connection->connectionRequest.target);
    bytesWritten += sprintf(buffer + bytesWritten, "%d\t", connection->connectionRequest.port);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t%s\t\n", connection->sniffer.sniffer_parser.user,
                            connection->sniffer.sniffer_parser.password);

    logger_log(buffer, bytesWritten);
}

void log_connection_closed(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytesWritten = get_time(buffer);

    bytesWritten = write_client_addr_n_port(connection,buffer,bytesWritten);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t%s\t\n",
                            connection->connectionRequest.target,"CONNECTION CLOSED"); //status_code connection->connectionRequest.originForm
     logger_log(buffer, bytesWritten);

}

void log_connection_failed(struct selector_key *key,char * reasonPhrase){
    proxyConnection *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytesWritten = get_time(buffer);

    bytesWritten += sprintf(buffer + bytesWritten, "A\t");
    bytesWritten = write_client_addr_n_port(connection,buffer,bytesWritten);
    bytesWritten += sprintf(buffer + bytesWritten, "%s\t%s\tStatus: %d %s\t\n", connection->connectionRequest.method,
                            connection->connectionRequest.target,connection->error,reasonPhrase);

    logger_log(buffer, bytesWritten);


}
void log_level_msg(char *msg,log_level level) {

    char buffer[MAX_BUFFER];
    unsigned len = get_time(buffer);

    if(len == 0) {
        return;
    }
    int newMaxLen = MAX_BUFFER-len;

    int n = snprintf(buffer + len,newMaxLen,"| %s : %s\n", level_texts[level], msg);

    if(n < 0 || n >= (int)(newMaxLen)){
        return;
    }

    logger_log(buffer, len + n);
}

static unsigned get_time(char * buffer){

    time_t rawTime;
    struct tm *info;
    time(&rawTime);
    info = localtime(&rawTime);

    return strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ\t", info);

}

static unsigned write_client_addr_n_port(proxyConnection *connection , char * buffer, unsigned bytesWritten){

    char clientAddr[IP_MAX] = {0};

     if (connection->client_addr.ss_family == AF_INET) {
        inet_ntop(AF_INET,&((struct sockaddr_in *)&connection->client_addr)->sin_addr, clientAddr, IP_MAX);
        bytesWritten += sprintf(buffer + bytesWritten, "%s\t%d\t", clientAddr,
                                (int)ntohs((((struct sockaddr_in *)&connection->client_addr)->sin_port)));
     } else {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&connection->client_addr)->sin6_addr, clientAddr, IP_MAX);
        bytesWritten += sprintf(buffer + bytesWritten, "%s\t%d\t", clientAddr,
                                (int)ntohs((((struct sockaddr_in6 *)&connection->client_addr)->sin6_port)));
     }
    return bytesWritten;

}
