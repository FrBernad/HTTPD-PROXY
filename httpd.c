#include <arpa/inet.h>
#include <buffer.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <parser_utils.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PENDING 5
#define MAX_CLIENTS 500
#define MAX_CONNECTIONS (MAX_CLIENTS * 2)
#define MAX_BUFF 256
#define PROXY_BUFF_LENGTH 8 * 1024
#define MAX_HOST_LENGTH 25
#define ADDR_LENGTH 40

typedef enum status { INACTIVE = 0,
                      AWAITING_HOST,
                      PARSING_HOST,
                      CONNECTING,
                      RESOLVING,
                      CONNECTED
} status_t;

#define ERROR_MANAGER(msg, retVal, errno)                                                                                      \
    do {                                                                                                                       \
        fprintf(stderr, "Error: %s (%s).\nReturned: %d (%s).\nLine: %d.\n", msg, __FILE__, retVal, strerror(errno), __LINE__); \
        exit(EXIT_FAILURE);                                                                                                    \
    } while (0)

typedef struct client_t {
    int peerfd;
    status_t status;
    buffer buffer;
    uint8_t *data;
    char address[ADDR_LENGTH];
    uint16_t port;

    char host[MAX_HOST_LENGTH];
    uint8_t hostLength;
    unsigned state;
} client_t;

typedef struct connectionsManager_t {
    uint16_t establishedConnections;
    uint16_t maxConnections;
    uint16_t maxClients;
    client_t clients[MAX_CONNECTIONS];
    int passiveSocket;
    struct parser *hostParser;
} connectionsManager_t;

// ----------- SOCKET MANAGMENT -----------

// Initializes connection manager structure
static void initConnectionManager(connectionsManager_t *connectionsManager, const struct parser_definition *d, int listeningPort);

static void closeConnectionManager(connectionsManager_t *connectionsManager, const struct parser_definition *d);

// Initializes passive sockets for connections request and starts listening in the port given by the user
static int initializePassiveSocket(int listeningPort);
// Establish new connection CLIENT - PROXY - SERVER
static void establishNewConnection(connectionsManager_t *connectionsManager);

// Establish the connection between the client and the proxy
static int handleNewClient(connectionsManager_t *connectionsManager);

// Establish the conection between the proxy and the server
static int handleServerConnection(int clientSocket, connectionsManager_t *connectionsManager);

// Initialize client_t structure
static void initClient(client_t *client, void *address, uint16_t port, int af);
// Set peer and client status
static void initPairStatus(int clientSocket, int serverSocket, connectionsManager_t *connectionsManager);

// End connection of the given fd
static void endConnection(client_t *client, int fd, connectionsManager_t *connectionsManager);

// ----------- SELECT MANAGMENT -----------

// reset write and read sets
static void resetSets(fd_set *readfds, fd_set *writefds, int *maxfd, connectionsManager_t *connectionsManager);

// process client/server requests
static void processRequest(int socketfd, fd_set *readfds, fd_set *writefds, connectionsManager_t *connectionsManager);

// process read set
static void handleReadSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager);

// process write set
static void handleWriteSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager);

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage : %s <port>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int listeningPort = atoi(argv[1]);

    if (listeningPort == 0) {
        fprintf(stderr, "Atoi error");
        exit(EXIT_FAILURE);
    }

    if (close(STDIN_FILENO) < 0)
        ERROR_MANAGER("Closing STDIN fd", -1, errno);

    connectionsManager_t connectionsManager;
    const struct parser_definition d = parser_utils_strcmpi("host:");
    initConnectionManager(&connectionsManager, &d, listeningPort);

    //set of socket descriptors
    fd_set readfds;
    fd_set writefds;

    int maxfd = connectionsManager.passiveSocket;
    int activity;

    while (true) {
        resetSets(&readfds, &writefds, &maxfd, &connectionsManager);

        activity = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
        if (activity < 0)
            ERROR_MANAGER("select", activity, errno);

        //Receive a new connection in passive socket
        if (FD_ISSET(connectionsManager.passiveSocket, &readfds))
            establishNewConnection(&connectionsManager);

        //else its some IO operation on some other socket
        for (size_t i = 0; i < connectionsManager.maxConnections; i++)
            processRequest(i, &readfds, &writefds, &connectionsManager);
    }

    closeConnectionManager(&connectionsManager, &d);

    return 0;
}

//TODO: cerrar conexiones. Ver casos no felices. GetaddressInfo. Parseo. Ver indices array que empieza desde 3.
//Habria que ver de hacer que escuche en ipv6. Que pasa con UDP si nuestro socket maneja TCP
//Cambiar maxClients

static void initConnectionManager(connectionsManager_t *connectionsManager, const struct parser_definition *d, int listeningPort) {
    memset(connectionsManager, 0, sizeof(connectionsManager_t));
    connectionsManager->maxConnections = MAX_CONNECTIONS;
    connectionsManager->maxClients = MAX_CLIENTS;
    connectionsManager->hostParser = parser_init(parser_no_classes(), d);
    //create listenting non-blocking socket
    connectionsManager->passiveSocket = initializePassiveSocket(listeningPort);
}

static void closeConnectionManager(connectionsManager_t *connectionsManager, const struct parser_definition *d) {
    parser_utils_strcmpi_destroy(d);
    parser_destroy(connectionsManager->hostParser);
}

static int initializePassiveSocket(int listeningPort) {
    //create listenting non-blocking socket
    int passiveSocket = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (passiveSocket < 0)
        ERROR_MANAGER("Passive socket creation", passiveSocket, errno);

    int opt = 1;

    if (setsockopt(passiveSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: SO_REUSEADDR", -1, errno);

    opt = MAX_BUFF;

    if (setsockopt(passiveSocket, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: SO_RCVBUF", -1, errno);

    if (setsockopt(passiveSocket, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: SO_SNDBUF", -1, errno);

    opt = 0;

    if (setsockopt(passiveSocket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: IPV6_ONLY", -1, errno);

    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    memset(&address, 0, addrlen);  // Zero out structure

    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(listeningPort);

    if (bind(passiveSocket, (struct sockaddr *)&address, addrlen) < 0)
        ERROR_MANAGER("bind", -1, errno);

    if (listen(passiveSocket, MAX_PENDING) < 0)
        ERROR_MANAGER("listen", -1, errno);

    return passiveSocket;
}

static void resetSets(fd_set *readfds, fd_set *writefds, int *maxfd, connectionsManager_t *connectionsManager) {
    FD_ZERO(readfds);
    FD_ZERO(writefds);

    if (connectionsManager->establishedConnections < connectionsManager->maxClients)
        FD_SET(connectionsManager->passiveSocket, readfds);

    //TODO: Ver como optimizar esto
    client_t *client;
    client_t *peer;

    for (int i = 0; i < connectionsManager->maxConnections; i++) {
        client = &connectionsManager->clients[i];
        status_t status = client->status;

        if (status != INACTIVE) {
            if (i > *maxfd)
                *maxfd = i;

            if (status == CONNECTING) {
                FD_SET(i, writefds);
                break;
            }

            //Suscribo para lectura si tiene lugar en el buffer
            if (buffer_can_write(&client->buffer))
                FD_SET(i, readfds);

            if (status == CONNECTED) {
                peer = &connectionsManager->clients[client->peerfd];
                /*Me suscribo para escritura si la conexion con el server ya está establecida y 
                 tengo para leer bytes para enviar por el socket*/
                if (peer->status == CONNECTED && buffer_can_read(&peer->buffer))
                    FD_SET(i, writefds);
            }
        }
    }
}

static void establishNewConnection(connectionsManager_t *connectionsManager) {
    int clientSocket = handleNewClient(connectionsManager);
    connectionsManager->clients[clientSocket].peerfd = -1;
    connectionsManager->clients[clientSocket].status = AWAITING_HOST;
}

static int handleServerConnection(int clientSocket, connectionsManager_t *connectionsManager) {
    client_t *client = &connectionsManager->clients[clientSocket];

    int ready = false;
    int af;
    socklen_t addrlen;
    void *address;
    uint16_t port = 80;

    //try ipv4
    af = AF_INET;
    struct sockaddr_in ipv4addr;
    addrlen = sizeof(ipv4addr);

    memset(&ipv4addr, 0, addrlen);
    int res;
    if ((res = inet_pton(AF_INET, client->host, &ipv4addr.sin_addr)) <= 0) {
        if (res < 0)
            ERROR_MANAGER("inet_pton", -1, errno);
    } else {
        ready = true;
        address = &ipv4addr;
        ipv4addr.sin_family = AF_INET;
        ipv4addr.sin_port = htons(80);
    }

    //try ipv6
    if (!ready) {
        af = AF_INET6;
        struct sockaddr_in6 ipv6addr;
        addrlen = sizeof(ipv6addr);

        memset(&ipv6addr, 0, addrlen);
        int res;
        if ((res = inet_pton(AF_INET6, client->host, &ipv6addr.sin6_addr)) <= 0) {
            if (res < 0)
                ERROR_MANAGER("inet_pton", -1, errno);
        } else {
            ready = true;
            address = &ipv6addr;
            ipv6addr.sin6_family = AF_INET6;
            ipv6addr.sin6_port = htons(80);
        }
    }

    //create listenting non-blocking socket
    int serverSocket = socket(af, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    if (serverSocket < 0)
        ERROR_MANAGER("server socket creation", serverSocket, errno);

    int opt = 1;

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("setsockopt", -1, errno);

    client_t *newClient = &connectionsManager->clients[serverSocket];

    if (connect(serverSocket, (struct sockaddr *)address, addrlen) < 0) {
        if (errno != EINPROGRESS)
            ERROR_MANAGER("connect", -1, errno);  //FIXME: aca habria que ver que hacer
    }

    initClient(newClient, address, port, af);

    connectionsManager->establishedConnections++;

    initPairStatus(clientSocket, serverSocket, connectionsManager);

    return 1;
}

static int handleNewClient(connectionsManager_t *connectionsManager) {
    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    int clientSocket;

    if ((clientSocket = accept(connectionsManager->passiveSocket, (struct sockaddr *)&address, &addrlen)) < 0)
        ERROR_MANAGER("accept", clientSocket, errno);

    int flags = fcntl(clientSocket, F_GETFL, 0);
    if (flags < 0)
        ERROR_MANAGER("fnctl", flags, errno);
    if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) < 0)
        ERROR_MANAGER("fnctl", -1, errno);

    client_t *newClient = &connectionsManager->clients[clientSocket];
    // initClient(newClient, &address);
    newClient->data = malloc(PROXY_BUFF_LENGTH);
    buffer_init(&newClient->buffer, MAX_BUFF, newClient->data);
    if (inet_ntop(address.sin6_family, &address.sin6_addr, newClient->address, ADDR_LENGTH) == NULL)
        ERROR_MANAGER("inet_ntop", 0, errno);

    newClient->port = ntohs(address.sin6_port);

    printf("New connection:\n-Socket fd: %d\n-IP: %s\n-Port: %d\n\n", clientSocket, newClient->address, newClient->port);

    connectionsManager->establishedConnections++;

    return clientSocket;
}

static void initClient(client_t *client, void *address, uint16_t port, int af) {
    client->data = malloc(PROXY_BUFF_LENGTH);  //FIXME: chequear malloc
    buffer_init(&client->buffer, MAX_BUFF, client->data);

    if (inet_ntop(af, address, client->address, ADDR_LENGTH) == NULL)
        ERROR_MANAGER("inet_ntop", 0, errno);

    client->port = ntohs(port);
}

static void initPairStatus(int clientSocket, int serverSocket, connectionsManager_t *connectionsManager) {
    connectionsManager->clients[serverSocket].status = CONNECTING;
    connectionsManager->clients[serverSocket].peerfd = clientSocket;

    connectionsManager->clients[clientSocket].peerfd = serverSocket;
    connectionsManager->clients[clientSocket].status = CONNECTED;
}

static void processRequest(int socketfd, fd_set *readfds, fd_set *writefds, connectionsManager_t *connectionsManager) {
    client_t *client = &connectionsManager->clients[socketfd];

    if (client->status != INACTIVE) {
        client_t *peer = &connectionsManager->clients[client->peerfd];

        if (FD_ISSET(socketfd, readfds))
            handleReadSet(socketfd, client, peer, connectionsManager);

        if (FD_ISSET(socketfd, writefds))
            handleWriteSet(socketfd, client, peer, connectionsManager);
    }
}

static void handleReadSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager) {
    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(&client->buffer, &maxBytes);

    int totalBytes = read(socketfd, data, maxBytes);
    if (totalBytes < 0)
        ERROR_MANAGER("read", totalBytes, errno);
    if (totalBytes == 0) {
        printf("Connection closed.\n\n");

        if (client->peerfd > 0)
            endConnection(peer, client->peerfd, connectionsManager);

        endConnection(client, socketfd, connectionsManager);
        return;
    }

    buffer_write_adv(&client->buffer, totalBytes);

    if (client->status != CONNECTED) {
        int i = 0;
        if (client->status == AWAITING_HOST) {
            changeState(connectionsManager->hostParser, client->state);
            const struct parser_event *event;
            for (i = 0; i < totalBytes; i++) {
                event = parser_feed(connectionsManager->hostParser, data[i]);
                if (event->type == STRING_CMP_EQ) {
                    client->status = PARSING_HOST;
                    i++;
                    break;
                } else if (event->type == STRING_CMP_NEQ)
                    parser_reset(connectionsManager->hostParser);
            }
            client->state = getState(connectionsManager->hostParser);
        }
        if (client->status == PARSING_HOST) {
            while (i < totalBytes && client->hostLength < MAX_HOST_LENGTH) {
                if (client->hostLength == MAX_HOST_LENGTH - 1 && data[i] != '\r') {
                    endConnection(client, socketfd, connectionsManager);
                    break;
                }

                if (data[i] == '\r' && data[i + 1] == '\n') {
                    client->host[client->hostLength] = 0;
                    if (!handleServerConnection(socketfd, connectionsManager))
                        endConnection(client, socketfd, connectionsManager);

                    break;
                }

                if (data[i] != '\t' && data[i] != ' ') {
                    client->host[client->hostLength] = data[i];
                    client->hostLength++;
                }
                i++;
            }
        }
    }
}

static void handleWriteSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager) {
    /*Si todavia se esta conectando hay que chequear la variable SO_ERROR para ver si se obtuvo un error en la función connect*/
    if (client->status == CONNECTING) {
        socklen_t optSize = sizeof(int);
        int opt;

        if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &opt, &optSize) < 0)
            ERROR_MANAGER("getsockopt", -1, errno);
        // printf("opt val: %d, fd: %ld\n\n", opt, i);
        //MATAR EL SOCKET Y AVISARLE AL CLIENT SI ES QUE NO HAY MAS IPS. SI NO PROBAR CON LA LISTA DE IPS
        if (opt != 0) {
            printf("Connection could not be established, closing sockets.\n\n");
            endConnection(peer, client->peerfd, connectionsManager);
            endConnection(client, socketfd, connectionsManager);
            return;
        }
        //The connection is established
        client->status = CONNECTED;
    }

    if (buffer_can_read(&peer->buffer)) {
        // char c = buffer_read(&peer->buffer);
        // send(socketfd, &c, 1, 0);

        size_t maxBytes;
        uint8_t *data = buffer_read_ptr(&peer->buffer, &maxBytes);
        int totalBytes = send(socketfd, data, maxBytes, 0);
        buffer_read_adv(&peer->buffer, totalBytes);
    }
}

static void endConnection(client_t *client, int fd, connectionsManager_t *connectionsManager) {
    if (close(fd) < 0)
        ERROR_MANAGER("close", -1, errno);

    connectionsManager->establishedConnections--;
    free(client->data);
    memset(client, 0, sizeof(client_t));  // Zero out structure
}

// static int getPort(char * host) {
//     while(*host) {
//         if(*host == ":") {
//             *host++;
//             return host;
//         }
//         *host++;
//     }
//     return 80;
// }