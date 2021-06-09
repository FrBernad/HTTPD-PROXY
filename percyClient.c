/**
 * 
 * 
 *  +----------+----------+----------+----------+----------+----------+----------+
    |    VER   |      PASSPHRASE     |   TYPE   |  METHOD  |   RESV   |   VALUE  |
    +----------+---------------------+----------+----------+----------+----------+
    |     1    |          6          |    1     |     1    |    1     |    2     |
    +----------+----------+----------+----------+----------+----------+----------+
 * 
        +----------+----------+----------+----------+
//         |    VER   |  STATUS  |   RESV   |   VALUE  |
        +----------+----------+----------+----------+
        |     1    |     1    |    1     |     8    |
        +----------+----------+----------+----------+
*/

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_BUFF 1024
#define PASS_PHRASE_LEN 6

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

static void buildRequest(char *buffer, int option, int value);
static void clearScreen();
static void showOptions();
static void error();

int main(int argc, char const *argv[]) {
    int socketFd;

    struct sockaddr_in servaddr;

    // if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    //     perror("Client socket creation failed");
    //     exit(1);
    // }

    // memset(&servaddr, 0, sizeof(servaddr));

    // servaddr.sin_family = AF_INET;
    // servaddr.sin_port = htons(PORT);
    // servaddr.sin_addr.s_addr = INADDR_ANY;

    char buff[MAX_BUFF];
    int bytesToRead;
    char sendBuff[MAX_BUFF];

    bool reading = true;

    while (reading) {
        showOptions();
        bytesToRead = read(STDIN_FILENO, buff, MAX_BUFF);
        if (bytesToRead > 0) {
            
        } else {
            reading = false;
        }
    }
    close(socketFd);

    return 0;
}

static void buildRequest(char *buffer, int option, int value) {
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
            request.value = 0;
            break;
        case 10:
            request.type = 1;
            request.method = 1;
            request.value = 0;
            break;
        default:
            error();
    }

}

static void clearScreen() {
    printf("\033{1;1H\033[2J");
}

static void showOptions() {
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
}

static void error() {
    printf("Invalid option\n");
}