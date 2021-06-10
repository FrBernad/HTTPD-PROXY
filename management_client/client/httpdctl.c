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

#define PORT 9090
#define MAX_BUFF 1024
#define PASS_PHRASE_LEN 6
#define IS_DIGIT(x) (x >= '0' && x <= '9')

struct request_percy {
    uint8_t ver;
    uint8_t passphrase[PASS_PHRASE_LEN];
    uint8_t type;
    uint8_t method;
    uint8_t resv;
    uint16_t value;
};

static void
buildRequest(uint8_t *buffer, int *sizeOfBuffer, int option, uint16_t value);

static void
clearScreen();

static void
showOptions();

static void
error();

static int
processInput(uint8_t *buff, int bytesToRead, int *option, int *value);

static int
getValue(int *option, int *value);

static int
processValue(char *buff, int bytesToRead, int *value);

static void
parseAnswer(uint8_t *buff, int len);

int main(int argc, char const *argv[]) {
    int socketFd;

    struct sockaddr_in servaddr;
    socklen_t servaddr_len = sizeof(servaddr);

    if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Client socket creation failed");
        exit(1);
    }

    memset(&servaddr, 0, servaddr_len);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int bytesToRead;
    uint8_t buff[MAX_BUFF];

    bool reading = true;
    int option = 0, value = 0;

    while (reading) {
        showOptions();
        bytesToRead = read(STDIN_FILENO, (char *)buff, MAX_BUFF);
        if (bytesToRead > 0) {
            if (processInput(buff, bytesToRead, &option, &value) < 0)
                error();
            else {
                int len = 0;
                buildRequest(buff, &len, option, value);
                sendto(socketFd, buff, len, MSG_CONFIRM, (const struct sockaddr *)&servaddr, servaddr_len);
                int n = recvfrom(socketFd, buff, MAX_BUFF, MSG_WAITALL, (struct sockaddr *)&servaddr, &servaddr_len);
                parseAnswer(buff, n);
            }
        } else {
            reading = false;
        }
    }
    close(socketFd);

    return 0;
}

static int
processInput(uint8_t *buff, int bytesToRead, int *option, int *value) {
    int i = 0;
    int aux = 0;
    while (i < bytesToRead) {
        if (buff[i] == '\n')
            break;
        if (IS_DIGIT(buff[i]))
            aux = i == 0 ? buff[i] - '0' : aux * 10 + buff[i] - '0';

        i++;
    }

    *option = aux;

    if (*option >= 0 && *option <= 8) {
        *value = 0;
        return 0;
    } else if (*option >= 9 && *option <= 10) {
        return getValue(option, value);
    } else {
        return -1;
    }
}

static int
getValue(int *option, int *value) {
    switch (*option) {
        case 9:
            printf("\n\nThe value of the I/O buffer size should be between 1 and 1024:  \n\n");
            break;
        case 10:
            printf("\n\n1 to set on the sniffer and 0 to set off:   \n\n");
            break;
    }
    printf("Value: ");
    fflush(stdout);

    char buff[MAX_BUFF];
    int bytesToRead = read(STDIN_FILENO, buff, MAX_BUFF);
    return processValue(buff, bytesToRead, value);
}

static int
processValue(char *buff, int bytesToRead, int *value) {
    int i = 0;
    int aux = 0;
    while (i < bytesToRead) {
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
buildRequest(uint8_t *buffer, int *sizeOfBuffer, int option, uint16_t value) {
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
            buffer[10] = value >> 8;  //VALUE8;
            buffer[11] = value;       //VALUE
            break;
        default:
            error();
    }
    *sizeOfBuffer = sizeof(request);
}

static void
clearScreen() {
    printf("\033{1;1H\033[2J\n");
}

static void
showOptions() {
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

    printf("-9  Set I/O buffer size. \n");
    printf("-10 Set selector timeout. \n\n");

    printf("Option: ");
    fflush(stdout);
}

static void
error() {
    printf("Invalid option\n");
}

static void
parseAnswer(uint8_t *buff, int len) {
    struct percy_response_parser response;
    struct percy_response percy_response;
    response.response = &percy_response;

    percy_response_parser_init(&response);
    int i = 0;
    while (i < len) {
        percy_response_parser_feed(&response, buff[i++]);
    }

    clearScreen();

    printf("\n\nRESPONSE: \n\n");

    printf("Version %d\n", response.response->ver);
    printf("Status  %d\n", response.response->status);
    printf("Resv    %d\n", response.response->resv);
    printf("Value   %ld\n", response.response->value);

    printf("\n\npress enter to continue\n");

    while (getchar() != '\n')
        ;

    clearScreen();
}
