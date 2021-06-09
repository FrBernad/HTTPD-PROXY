#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 9090
#define MAX_BUFF 1024
#define PASS_PHRASE_LEN 6
#define IS_DIGIT(x) (x >= '0' && x <= '9')

struct percy_response {
    uint8_t ver;
    uint8_t status;
    uint8_t resv;
    uint64_t value;
};

struct request_percy {
    uint8_t ver;
    uint8_t type;
    uint8_t passphrase[PASS_PHRASE_LEN];
    uint8_t method;
    uint8_t resv;
    uint16_t value;
};

static void buildRequest(char *buffer, int *sizeOfBuffer, int option, int value);
static void clearScreen();
static void showOptions();
static void error();
static int processInput(char *buff, int bytesToRead, int *option, int *value);
int getValue(int *option, int *value);
static int processValue(char *buff, int bytesToRead, int *value);

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
    char buff[MAX_BUFF];

    bool reading = true;
    int option = 0, value = 0;

    while (reading) {
        showOptions();
        bytesToRead = read(STDIN_FILENO, buff, MAX_BUFF);
        if (bytesToRead > 0) {
            if (processInput(buff, bytesToRead, &option, &value) < 0)
                error();
            else {
                int len = 0;
                buildRequest(buff, &len, option, value);
                sendto(socketFd, buff, len, MSG_CONFIRM, (const struct sockaddr *)&servaddr, servaddr_len);
                int n = recvfrom(socketFd, buff, MAX_BUFF, MSG_WAITALL, (struct sockaddr *)&servaddr, &servaddr_len);
                printf("%d\n", n);
            }
        } else {
            reading = false;
        }
    }
    close(socketFd);

    return 0;
}

static int processInput(char *buff, int bytesToRead, int *option, int *value) {
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

int getValue(int *option, int *value) {
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

static int processValue(char *buff, int bytesToRead, int *value) {
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

static void buildRequest(char *buffer, int *sizeOfBuffer, int option, int value) {
    struct request_percy request;
    request.ver = 1;
    strcpy(request.passphrase, "CONTRA");
    request.resv = 0;
    switch (option) {
        case 1:
            request.type = 0;
            request.method = 0;
            request.value = 0;
            break;
        case 2:
            request.type = 0;
            request.method = 1;
            request.value = 0;
            break;
        case 3:
            request.type = 0;
            request.method = 2;
            request.value = 0;
            break;
        case 4:
            request.type = 0;
            request.method = 3;
            request.value = 0;
            break;
        case 5:
            request.type = 0;
            request.method = 4;
            request.value = 0;
            break;
        case 6:
            request.type = 0;
            request.method = 5;
            request.value = 0;
            break;
        case 7:
            request.type = 0;
            request.method = 6;
            request.value = 0;
            break;
        case 8:
            request.type = 0;
            request.method = 7;
            request.value = 0;
            break;

        case 9:
            request.type = 1;
            request.method = 0;
            request.value = value;
            break;
        case 10:
            request.type = 1;
            request.method = 1;
            request.value = value;
            break;
        default:
            error();
    }
    *sizeOfBuffer = sizeof(request);
    memcpy(buffer, &request, *sizeOfBuffer);
}

static void clearScreen() {
    printf("\033{1;1H\033[2J");
}

static void showOptions() {
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

static void error() {
    printf("Invalid option\n");
}