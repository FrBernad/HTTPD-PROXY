#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "management_client/percy_response_parser/percy_response_parser.h"

#define MAX_BUFF 1024
#define MAX_PORT 65535
#define PASS_PHRASE_LEN 6
#define IS_DIGIT(x) (x >= '0' && x <= '9')
#define ARGS_QUANTITY 3

struct request_percy {
    uint8_t ver;
    uint8_t passphrase[PASS_PHRASE_LEN];
    uint8_t type;
    uint8_t method;
    uint8_t resv;
    uint16_t value;
};

static void
build_request(uint8_t *buffer, int *size_of_buffer, int option, uint16_t value);

static void
clear_screen();

static void
show_options();

static void
error();

static int
process_input(uint8_t *buff, int bytes_to_read, int *option, int *value);

static int
get_value(int *option, int *value);

static int
process_value(char *buff, int bytes_to_read, int *value);

static void
parse_answer(uint8_t *buff, int len);

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        perror("Wrong number of arguments. Use ./http <ip> <port>\n\n");
        exit(1);
    }

    struct sockaddr_storage serv_addr;
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    } ip_addr;

    socklen_t servaddr_len;
    int socket_fd;
    int port = atoi(argv[2]);
    if (port <= 0 || port >= MAX_PORT) {
        perror("Invalid port. \n\n");
        exit(1);
    }

    if (inet_pton(AF_INET, argv[1], &ip_addr.ipv4) > 0) {
        servaddr_len = sizeof(serv_addr);
        if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Client socket creation failed");
            exit(1);
        }
        memset(&serv_addr, 0, servaddr_len);
        serv_addr.ss_family = AF_INET;
        ((struct sockaddr_in *)&serv_addr)->sin_port = htons(port);
        ((struct sockaddr_in *)&serv_addr)->sin_addr = ip_addr.ipv4;
    } else if (inet_pton(AF_INET6, argv[1], &ip_addr.ipv6) > 0) {
        servaddr_len = sizeof(serv_addr);
        if ((socket_fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
            perror("Client socket creation failed");
            exit(1);
        }
        memset(&serv_addr, 0, servaddr_len);
        serv_addr.ss_family = AF_INET6;
        ((struct sockaddr_in6 *)&serv_addr)->sin6_port = htons(port);
        ((struct sockaddr_in6 *)&serv_addr)->sin6_addr = ip_addr.ipv6;
    } else {
        perror("Invalid ip address");
        exit(1);
    }

    int bytes_to_read;
    uint8_t buff[MAX_BUFF];

    bool reading = true;
    int option = 0, value = 0;

    while (reading) {
        show_options();
        bytes_to_read = read(STDIN_FILENO, (char *)buff, MAX_BUFF);
        if (bytes_to_read > 0) {
            if (process_input(buff, bytes_to_read, &option, &value) < 0)
                error();
            else {
                int len = 0;
                build_request(buff, &len, option, value);
                sendto(socketFd, buff, len, MSG_CONFIRM, (const struct sockaddr *)&serv_addr, servaddr_len);
                int n = recvfrom(socketFd, buff, MAX_BUFF, MSG_WAITALL, (struct sockaddr *)&serv_addr, &servaddr_len);
                parse_answer(buff, n);
            }
        } else {
            reading = false;
        }
    }
    close(socket_fd);

    return 0;
}

static int
process_input(uint8_t *buff, int bytes_to_read, int *option, int *value) {
    int i = 0;
    int aux = 0;
    while (i < bytes_to_read) {
        if (buff[i] == '\n')
            break;
        if (IS_DIGIT(buff[i]))
            aux = i == 0 ? buff[i] - '0' : aux * 10 + buff[i] - '0';
        else {
            return -1;
        }
        i++;
    }

    *option = aux;

    if (*option >= 1 && *option <= 8) {
        *value = 0;
        return 0;
    } else if (*option >= 9 && *option <= 11) {
        return get_value(option, value);
    } else {
        return -1;
    }
}

static int
get_value(int *option, int *value) {
    printf("\n\n1 to set on the sniffer and 0 to set off:   \n\n");

    printf("Value: ");
    fflush(stdout);

    char buff[MAX_BUFF];
    int bytes_to_read = read(STDIN_FILENO, buff, MAX_BUFF);
    return process_value(buff, bytes_to_read, value);
}

static int
process_value(char *buff, int bytes_to_read, int *value) {
    int i = 0;
    int aux = 0;
    while (i < bytes_to_read) {
        if (buff[i] == '\n')
            break;
        if (IS_DIGIT(buff[i])) {
            aux = i == 0 ? buff[i] - '0' : aux * 10 + buff[i] - '0';
        } else {
            return -1;
        }
        i++;
    }

    *value = aux;
    return 0;
}

static void
build_request(uint8_t *buffer, int *size_of_buffer, int option, uint16_t value) {
    struct request_percy request;
    //VERSION
    buffer[0] = 1;

    //PASSPHRASE
    buffer[1] = '1';
    buffer[2] = '2';
    buffer[3] = '3';
    buffer[4] = '4';
    buffer[5] = '5';
    buffer[6] = '6';

    //RESV
    buffer[9] = 0;

    switch (option) {
        case 1:
            buffer[7] = 0;   //TYPE
            buffer[8] = 0;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 2:
            buffer[7] = 0;   //TYPE
            buffer[8] = 1;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 3:
            buffer[7] = 0;   //TYPE
            buffer[8] = 2;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 4:
            buffer[7] = 0;   //TYPE
            buffer[8] = 3;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 5:
            buffer[7] = 0;   //TYPE
            buffer[8] = 4;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 6:
            buffer[7] = 0;   //TYPE
            buffer[8] = 5;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 7:
            buffer[7] = 0;   //TYPE
            buffer[8] = 6;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;
        case 8:
            buffer[7] = 0;   //TYPE
            buffer[8] = 7;   //METHOD
            buffer[10] = 0;  //VALUE
            buffer[11] = 0;  //VALUE
            break;

        case 9:
            buffer[7] = 1;            //TYPE
            buffer[8] = 0;            //METHOD
            buffer[10] = value >> 8;  //VALUE
            buffer[11] = value;       //VALUE
            break;
        case 10:
            buffer[7] = 1;            //TYPE
            buffer[8] = 1;            //METHOD
            buffer[10] = value >> 8;  //VALUE
            buffer[11] = value;       //VALUE
            break;
        case 11:
            buffer[7] = 1;            //TYPE
            buffer[8] = 2;            //METHOD
            buffer[10] = value >> 8;  //VALUE
            buffer[11] = value;       //VALUE
            break;
        default:
            error();
    }
    *size_of_buffer = sizeof(request);
}

static void
clear_screen() {
    printf("\033{1;1H\033[2J\n");
}

static void
show_options() {
    printf("Select an option\n\n");

    printf("Request methods:\n\n");

    printf("-1  Request the number of historical connections.\n");
    printf("-2  Request the number of concurrent connections. \n");
    printf("-3  Request the number of bytes sent.\n");
    printf("-4  Request the number of bytes received.\n");
    printf("-5  Request I/O buffer sizes  \n");
    printf("-6  Request selector timeout.\n");
    printf("-7  Request the maximum amount of concurrent connections.\n");
    printf("-8  Request the number of failed connections.\n\n");

    printf("Modification methods:\n\n");

    printf("-9  Enable or disable sniffer mode.  \n");
    printf("-10 Set I/O buffer size.  \n");
    printf("-11 Set selector timeout.  \n\n");

    printf("Option: ");
    fflush(stdout);
}

static void
error() {
    printf("Invalid option\n");
}

static void
parse_answer(uint8_t *buff, int len) {
    struct percy_response_parser percy_response_parser;
    struct percy_response percy_response;
    percy_response_parser.response = &percy_response;

    percy_response_parser_init(&percy_response_parser);
    int i = 0;
    while (i < len) {
        percy_response_parser_feed(&percy_response_parser, buff[i++]);
    }

    clear_screen();

    printf("\n\nRESPONSE: \n\n");

    printf("Version %d\n", percy_response_parser.response->ver);
    printf("Status  %d\n", percy_response_parser.response->status);
    printf("Resv    %d\n", percy_response_parser.response->resv);
    printf("Value   %ld\n", percy_response_parser.response->value);

    printf("\n\npress enter to continue\n");

    while (getchar() != '\n')
        ;

    clear_screen();
}
