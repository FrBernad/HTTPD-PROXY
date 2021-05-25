#include <arpa/inet.h>
#include <buffer.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
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
#define ADDR_LENGTH 40

typedef enum status { INACTIVE = 0,
                      CONNECTING,
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
    uint8_t data[MAX_BUFF];
    char address[ADDR_LENGTH];
    uint16_t port;
} client_t;

typedef struct connectionsManager_t {
    uint16_t establishedConnections;
    uint16_t maxConnections;
    uint16_t maxClients;
    client_t clients[MAX_CONNECTIONS];
    int passiveSocket;
} connectionsManager_t;


// ----------- SOCKET MANAGMENT -----------

// Initializes passive sockets for connections request and starts listening in the port given by the user
static int initializePassiveSocket(int listeningPort);

// Establish new connection CLIENT - PROXY - SERVER
static void establishNewConnection(fd_set *writefds, connectionsManager_t *connectionsManager);

// Establish the connection between the client and the proxy
static int handleNewClient(connectionsManager_t *connectionsManager);

// Establish the conection between the proxy and the server
static int handleServerConnection(fd_set *writefds, connectionsManager_t *connectionsManager);

// Initialize client_t structure
static void initClient(client_t *client, struct sockaddr_in6 *address);

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
    memset(&connectionsManager, 0, sizeof(connectionsManager_t));
    connectionsManager.maxConnections = MAX_CONNECTIONS;
    connectionsManager.maxClients = MAX_CLIENTS;

    //create listenting non-blocking socket
    int passiveSocket = initializePassiveSocket(listeningPort);

    connectionsManager.passiveSocket=passiveSocket;

    //set of socket descriptors
    fd_set readfds;
    fd_set writefds;

    int maxfd = passiveSocket;
    int activity;

    while (true) {
        resetSets(&readfds, &writefds, &maxfd, &connectionsManager);

        activity = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
        if (activity < 0)
            ERROR_MANAGER("select", activity, errno);

        //Receive a new connection in passive socket
        if (FD_ISSET(passiveSocket, &readfds))
            establishNewConnection(&writefds, &connectionsManager);

        //else its some IO operation on some other socket
        for (size_t i = 0; i < connectionsManager.maxConnections; i++)
            processRequest(i, &readfds, &writefds, &connectionsManager);
    }

    return 0;
}

//TODO: cerrar conexiones. Ver casos no felices. GetaddressInfo. Parseo. Ver indices array que empieza desde 3.
//Habria que ver de hacer que escuche en ipv6. Que pasa con UDP si nuestro socket maneja TCP
//Cambiar maxClients

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

        if (client->status == CONNECTED) {
            if (buffer_can_write(&client->buffer))
                FD_SET(i, readfds);

            peer = &connectionsManager->clients[client->peerfd];
            //Me suscribo para escritura si tengo para leer bytes para enviar por el socket
            if (peer->status == CONNECTED && buffer_can_read(&peer->buffer))
                FD_SET(i, writefds);

            if (i > *maxfd)
                *maxfd = i;
        }
    }
}

static void establishNewConnection(fd_set *writefds, connectionsManager_t *connectionsManager) {
    int clientSocket = handleNewClient(connectionsManager);
    int serverSocket = handleServerConnection(writefds, connectionsManager);
    initPairStatus(clientSocket, serverSocket, connectionsManager);
}

static int handleServerConnection(fd_set *writefds, connectionsManager_t *connectionsManager) {
    //create listenting non-blocking socket
    int serverSocket = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

    if (serverSocket < 0)
        ERROR_MANAGER("server socket creation", serverSocket, errno);

    int opt = 1;

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("setsockopt", -1, errno);

    opt = 0;

    if (setsockopt(serverSocket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0)
        ERROR_MANAGER("Setsockopt opt: IPV6_ONLY", -1, errno);

    struct sockaddr_in6 address;
    socklen_t addrlen = sizeof(address);

    memset(&address, 0, addrlen);
    //TODO: Cambiar a getaddrinfo
    address.sin6_family = AF_INET6;
    if (inet_pton(address.sin6_family, "::1", &address.sin6_addr) <= 0)
        ERROR_MANAGER("inet_pton", -1, errno);
    address.sin6_port = htons(9091);

    client_t *newClient = &connectionsManager->clients[serverSocket];
    initClient(newClient, &address);
    connectionsManager->establishedConnections++;

    FD_SET(serverSocket, writefds);

    if (connect(serverSocket, (struct sockaddr *)&address, addrlen) < 0) {
        if (errno != EINPROGRESS)
            ERROR_MANAGER("connect", -1, errno);  //FIXME: aca habria que ver que hacer
    }

    return serverSocket;
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
    initClient(newClient, &address);

    printf("New connection:\n-Socket fd: %d\n-IP: %s\n-Port: %d\n\n", clientSocket, newClient->address, newClient->port);

    connectionsManager->establishedConnections++;

    return clientSocket;
}

static void initClient(client_t *client, struct sockaddr_in6 *address) {
    buffer_init(&client->buffer, MAX_BUFF, client->data);
    if (inet_ntop(address->sin6_family, &address->sin6_addr, client->address, ADDR_LENGTH) == NULL)
        ERROR_MANAGER("inet_ntop", 0, errno);

    client->port = ntohs(address->sin6_port);
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
            handleReadSet(socketfd,client,peer,connectionsManager);
        
        if (FD_ISSET(socketfd, writefds)) 
            handleWriteSet(socketfd, client, peer, connectionsManager);
    }
}

static void handleReadSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager) {
    size_t maxBytes;
    uint8_t *data = buffer_write_ptr(&client->buffer, &maxBytes);
    int totalBytes;

    totalBytes = read(socketfd, data, maxBytes);
    if (totalBytes < 0)
        ERROR_MANAGER("read", totalBytes, errno);
    if (totalBytes == 0) {
        printf("Connection closed.\n\n");
        endConnection(peer, client->peerfd, connectionsManager);
        endConnection(client, socketfd, connectionsManager);
        return;
    }

    buffer_write_adv(&client->buffer, totalBytes);
}

static void handleWriteSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager) {
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
        size_t maxBytes;
        uint8_t *data = buffer_read_ptr(&client->buffer, &maxBytes);
        buffer_read_adv(&peer->buffer, maxBytes);
        send(socketfd, data, maxBytes, 0);
    }
}

static void endConnection(client_t *client, int fd, connectionsManager_t *connectionsManager) {
    if (close(fd) < 0)
        ERROR_MANAGER("close", -1, errno);

    connectionsManager->establishedConnections--;
    memset(client, 0, sizeof(client_t));  // Zero out structure
}
