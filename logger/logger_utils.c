#include "logger_utils.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <time.h>

#include "connections_manager/connections_def.h"

#define MAX_BUFFER 2048
#define IP_MAX 50

char *level_texts[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

static unsigned
get_time(char *buffer);

static unsigned
write_client_addr_n_port(proxy_connection_t *connection, char *buffer, unsigned bytes_written);

void log_new_connection(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytes_written = get_time(buffer);

    bytes_written += sprintf(buffer + bytes_written, "A\t");
    bytes_written = write_client_addr_n_port(connection, buffer, bytes_written);
    bytes_written += sprintf(buffer + bytes_written, "%s\t%s\t%d\t\n", connection->connection_request.method,
                             connection->connection_request.target, connection->connection_request.status_code);

    // fecha tipo de registro direccion IpOrigen puertoOrigen Metodo Target(scheme:host:port:origin) status.
    logger_log(buffer, bytes_written, LEVEL_INFO);
}

void log_user_and_password(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytes_written = get_time(buffer);

    bytes_written += sprintf(buffer + bytes_written, "P\t%s\t",
                             connection->sniffer.sniffer_parser.sniffed_protocol == HTTP ? "HTTP" : "POP3");
    bytes_written += sprintf(buffer + bytes_written, "%s\t", connection->connection_request.target);
    bytes_written += sprintf(buffer + bytes_written, "%d\t", connection->connection_request.port);
    bytes_written += sprintf(buffer + bytes_written, "%s\t%s\t\n", connection->sniffer.sniffer_parser.user,
                             connection->sniffer.sniffer_parser.password);

    logger_log(buffer, bytes_written, LEVEL_INFO);
}

void log_connection_closed(struct selector_key *key) {
    proxy_connection_t *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytes_written = get_time(buffer);

    bytes_written = write_client_addr_n_port(connection, buffer, bytes_written);
    bytes_written += sprintf(buffer + bytes_written, "%s\t%s\t\n",
                             connection->connection_request.target,
                             "CONNECTION CLOSED");  //status_code connection->connection_request.originForm
    logger_log(buffer, bytes_written, LEVEL_INFO);
}

void log_connection_failed(struct selector_key *key, char *reason_phrase) {
    proxy_connection_t *connection = ATTACHMENT(key);

    char buffer[MAX_BUFFER];
    unsigned bytes_written = get_time(buffer);

    bytes_written += sprintf(buffer + bytes_written, "A\t");
    bytes_written = write_client_addr_n_port(connection, buffer, bytes_written);
    bytes_written += sprintf(buffer + bytes_written, "%s\t%s\tStatus: %d %s\t\n", connection->connection_request.method,
                             connection->connection_request.target, connection->error, reason_phrase);

    logger_log(buffer, bytes_written, LEVEL_INFO);
}

void log_level_msg(char *msg, log_level level) {
    char buffer[MAX_BUFFER];
    unsigned len = get_time(buffer);

    if (len == 0) {
        return;
    }
    int new_max_len = MAX_BUFFER - len;

    int n = snprintf(buffer + len, new_max_len, "%s\t%s\n", level_texts[level], msg);

    if (n < 0 || n >= (int)(new_max_len)) {
        return;
    }

    logger_log(buffer, len + n, level);
}

static unsigned get_time(char *buffer) {
    time_t raw_time;
    struct tm *info;
    time(&raw_time);
    info = localtime(&raw_time);

    return strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ\t", info);
}

static unsigned write_client_addr_n_port(proxy_connection_t *connection, char *buffer, unsigned bytes_written) {
    char client_addr[IP_MAX] = {0};

    if (connection->client_addr.ss_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)&connection->client_addr)->sin_addr, client_addr, IP_MAX);
        bytes_written += sprintf(buffer + bytes_written, "%s\t%d\t", client_addr,
                                 (int)ntohs((((struct sockaddr_in *)&connection->client_addr)->sin_port)));
    } else {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&connection->client_addr)->sin6_addr, client_addr, IP_MAX);
        bytes_written += sprintf(buffer + bytes_written, "%s\t%d\t", client_addr,
                                 (int)ntohs((((struct sockaddr_in6 *)&connection->client_addr)->sin6_port)));
    }
    return bytes_written;
}
