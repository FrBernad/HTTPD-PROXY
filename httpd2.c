// #include <arpa/inet.h>
// #include <buffer.h>
// #include <doh_parser.h>
// #include <doh_utils.h>
// #include <errno.h>
// #include <fcntl.h>
// #include <request.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/select.h>

// #define MAX_PENDING 5
// #define MAX_CLIENTS 500
// #define MAX_CONNECTIONS (MAX_CLIENTS * 2)
// #define MAX_BUFF 256
// #define PROXY_BUFF_LENGTH 8 * 1024

// typedef enum status {

//     INACTIVE = 0,

//     CLIENT STATUS
//     AWAITING_HOST,
//     AWAITING_ORIGIN_CONNECTION,
//     DOH_REQUEST_IPV4,
//     DOH_REQUEST_IPV6,
//     AWAITING_DOH_REQUEST, //ESTOY ESPERANDO PARA PODER MANDAR LA REQUEST
//     AWAITING_DOH_RESPONSE,


//     ORIGIN STATUS
//     CONNECTING,
//     CONNECTING_TO_DOH_SERVER,
//     CONNECTED_TO_DOH_SERVER,

//     CONNECTED,

// } status_t;

// enum initFlags {
//     F_BUFF = 0x01,
//     F_REQP = 0x02,
//     F_DOH,
//     = 0x04
// }

// struct doh {
//     enum {
//         ipv4,
//         ipv6,
//     } domain;
//     union {
//         struct sockaddr_in ipv4;
//         struct sockaddr_in6 ipv6;
//     } addr;
//     char *ip;
//     uint16_t port;
//     char *path;
//     char *query;
// };

// #define ERROR_MANAGER(msg, retVal, errno)                                                                                      \
//     do {                                                                                                                       \
//         fprintf(stderr, "Error: %s (%s).\nReturned: %d (%s).\nLine: %d.\n", msg, __FILE__, retVal, strerror(errno), __LINE__); \
//         exit(EXIT_FAILURE);                                                                                                    \
//     } while (0)

// typedef struct client_t {
//     int peerfd;
//     status_t status;
//     buffer buffer;
//     uint8_t *data;
//     struct request_parser requestParser;
//     struct request_line request;
//     struct doh_response response;
//     struct doh_response_parser dohParser;
// } client_t;

// typedef struct connectionsManager_t {
//     uint16_t establishedConnections;
//     uint16_t maxConnections;
//     uint16_t maxClients;
//     client_t clients[MAX_CONNECTIONS];
//     int passiveSocket;
//     struct doh doh;
// } connectionsManager_t;

// ----------- SOCKET MANAGMENT -----------

// Initializes connection manager structure
// static void initConnectionManager(connectionsManager_t *connectionsManager, int listeningPort);

// Initializes passive sockets for connections request and starts listening in the port given by the user
// static int initializePassiveSocket(int listeningPort);

// Establish new connection CLIENT - PROXY - SERVER
// static void establishNewConnection(connectionsManager_t *connectionsManager);

// Establish the connection between the client and the proxy
// static int handleNewClient(connectionsManager_t *connectionsManager);

// static int createOriginSocket(const struct sockaddr *addr, socklen_t addrlen, int domain);

// Establish the conection between the proxy and the server
// static int handleOriginConnection(int clientSocket, connectionsManager_t *connectionsManager);

// Initialize client_t structure
// static void initClient(client_t *client);

// Set peer and client status
// static void initPairStatus(int clientSocket, int serverSocket, connectionsManager_t *connectionsManager, status_t clientStatus, status_t originStatus);

// Initialize doh structure
// static void initDoh(struct doh *doh);

// End connection of the given fd
// static void endConnection(client_t *client, int fd, connectionsManager_t *connectionsManager);

// ----------- SELECT MANAGMENT -----------

// reset write and read sets
// static void resetSets(fd_set *readfds, fd_set *writefds, int *maxfd, connectionsManager_t *connectionsManager);

// process client/server requests
// static void processRequest(int socketfd, fd_set *readfds, fd_set *writefds, connectionsManager_t *connectionsManager);

// process read set
// static void handleReadSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager);

// process write set
// static void handleWriteSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager);

// int main(int argc, char const *argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "Usage : %s <port>", argv[0]);
//         exit(EXIT_FAILURE);
//     }

//     int listeningPort = atoi(argv[1]);

//     if (listeningPort == 0) {
//         fprintf(stderr, "Atoi error");
//         exit(EXIT_FAILURE);
//     }

//     if (close(STDIN_FILENO) < 0)
//         ERROR_MANAGER("Closing STDIN fd", -1, errno);

//     connectionsManager_t connectionsManager;
//     initConnectionManager(&connectionsManager, listeningPort);

//     set of socket descriptors
//     fd_set readfds;
//     fd_set writefds;

//     int maxfd = connectionsManager.passiveSocket;
//     int activity;

//     while (true) {
//         resetSets(&readfds, &writefds, &maxfd, &connectionsManager);

//         activity = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
//         if (activity < 0)
//             ERROR_MANAGER("select", activity, errno);

//         Receive a new connection in passive socket
//         if (FD_ISSET(connectionsManager.passiveSocket, &readfds))
//             establishNewConnection(&connectionsManager);

//         else its some IO operation on some other socket
//         for (int i = 0; i < connectionsManager.maxConnections; i++)  //FIXME: Aca no deberia empezar en 0 me parece porque lo tiene el passiveSocket
//             processRequest(i, &readfds, &writefds, &connectionsManager);
//     }

//     return 0;
// }

// TODO: cerrar conexiones. Ver casos no felices. GetaddressInfo. Parseo. Ver indices array que empieza desde 3.
// Habria que ver de hacer que escuche en ipv6. Que pasa con UDP si nuestro socket maneja TCP
// Cambiar maxClients

// static void initConnectionManager(connectionsManager_t *connectionsManager, int listeningPort) {
//     memset(connectionsManager, 0, sizeof(connectionsManager_t));
//     connectionsManager->maxConnections = MAX_CONNECTIONS;
//     connectionsManager->maxClients = MAX_CLIENTS;
//     create listenting non-blocking socket
//     connectionsManager->passiveSocket = initializePassiveSocket(listeningPort);

//     struct doh *doh = &connectionsManager->doh;
//     initDoh(doh);
// }

// static void initDoh(struct doh *doh) {
//     doh->ip = "127.0.0.1";    //FIXME: with params
//     doh->port = htons(8053);  //FIXME: with params
//     doh->domain = ipv4;
//     doh->addr.ipv4.sin_port = doh->port;
//     doh->addr.ipv4.sin_family = AF_INET;

//     if (inet_pton(AF_INET, doh->ip, &doh->addr.ipv4.sin_addr) <= 0)
//         ERROR_MANAGER("inet_pton", -1, errno);
// }

// static int initializePassiveSocket(int listeningPort) {
//     create listenting non-blocking socket
//     int passiveSocket = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
//     if (passiveSocket < 0)
//         ERROR_MANAGER("Passive socket creation", passiveSocket, errno);

//     int opt = 1;

//     if (setsockopt(passiveSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
//         ERROR_MANAGER("Setsockopt opt: SO_REUSEADDR", -1, errno);

//     opt = MAX_BUFF;

//     if (setsockopt(passiveSocket, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) < 0)
//         ERROR_MANAGER("Setsockopt opt: SO_RCVBUF", -1, errno);

//     if (setsockopt(passiveSocket, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0)
//         ERROR_MANAGER("Setsockopt opt: SO_SNDBUF", -1, errno);

//     opt = 0;

//     if (setsockopt(passiveSocket, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0)
//         ERROR_MANAGER("Setsockopt opt: IPV6_ONLY", -1, errno);

//     struct sockaddr_in6 address;
//     socklen_t addrlen = sizeof(address);

//     memset(&address, 0, addrlen);  // Zero out structure

//     address.sin6_family = AF_INET6;
//     address.sin6_addr = in6addr_any;
//     address.sin6_port = htons(listeningPort);

//     if (bind(passiveSocket, (struct sockaddr *)&address, addrlen) < 0)
//         ERROR_MANAGER("bind", -1, errno);

//     if (listen(passiveSocket, MAX_PENDING) < 0)
//         ERROR_MANAGER("listen", -1, errno);

//     return passiveSocket;
// }

// static void resetSets(fd_set *readfds, fd_set *writefds, int *maxfd, connectionsManager_t *connectionsManager) {
//     FD_ZERO(readfds);
//     FD_ZERO(writefds);

//     /*Si puedo soportar mas conexiones */
//     if (connectionsManager->establishedConnections < connectionsManager->maxClients)
//         FD_SET(connectionsManager->passiveSocket, readfds);

//     TODO: Ver como optimizar esto
//     client_t *client;
//     client_t *peer;

//     for (int i = 0; i < connectionsManager->maxConnections; i++) {
//         client = &connectionsManager->clients[i];
//         peer = &connectionsManager->clients[client->peerfd];
//         status_t status = client->status;

//         if (status != INACTIVE) {
//             if (i > *maxfd)
//                 *maxfd = i;

//             /*Estoy esperando a que me envien la request para poder parsearla y determinar el host a conectars*/
//             if (status == AWAITING_HOST && buffer_can_write(&client->buffer)){ //FIXME: status == AWAITING_ORIGIN_CONNECTION???
//                 FD_SET(i, readfds);
//                 break;
//             }
           
//             El socket se esta intentando conectar al origin
//             if (status == CONNECTING || status == CONNECTING_TO_DOH_SERVER) {
//                 FD_SET(i, writefds);
//                 break;
//             }

//             Escribir la query en el peer
//             if (status == DOH_REQUEST_IPV4) {
//                 client->status = AWAITING_DOH_REQUEST;
//                 int max;
//                 uint8_t *buffer = buffer_write_ptr(&peer->buffer, &max);
//                 int size = build_doh_request(buffer, client->request.request_target.host.domain, IPV4);
//                 if (size >= max)
//                     printf("error!");
//                 break;
//             }

//             if (status == DOH_REQUEST_IPV6) {
//                 client->status = AWAITING_DOH_REQUEST;
//                 int max;
//                 uint8_t *buffer = buffer_write_ptr(&peer->buffer, &max);
//                 int size = build_doh_request(buffer, client->request.request_target.host.domain, IPV6);
//                 if (size >= max)
//                     printf("error!");
//                 break;
//             }

//             FIXME
//             /*Me suscribo para escritura si la conexion con el server ya está establecida:
//                 -El origin se subscribe para escritura cuando el cliente tiene algo para mandarle.
//                 -El cliente se subscribe para escritura cuando el origin tiene algo para mandarle. */
//             if (status == CONNECTED)
//                 if (buffer_can_read(&peer->buffer) && peer->status == CONNECTED) {
//                     FD_SET(i, writefds);
//                     break;
//                 }

//             /*Soy el cliente. Estoy esperando una DOH response y el servidor tiene contenido en su buffer,
//             me suscribo para que me puedan enviar*/
//             if (status == AWAITING_DOH_REQUEST)
//                 if (buffer_can_read(&peer->buffer) && peer->status == CONNECTED_TO_DOH_SERVER) {
//                     FD_SET(i, writefds);
//                     break;
//                 }

//             Soy origin DOH
//             if (status == CONNECTED_TO_DOH_SERVER && (peer->status == AWAITING_DOH_REQUEST || peer->status == AWAITING_DOH_RESPONSE)) {
//                 FD_SET(i, writefds);
//                 break;
//             }
//         }
//     }
// }

// static void establishNewConnection(connectionsManager_t *connectionsManager) {
//     int clientSocket = handleNewClient(connectionsManager);
//     connectionsManager->clients[clientSocket].peerfd = -1;
//     connectionsManager->clients[clientSocket].status = AWAITING_HOST;
// }

// static int createOriginSocket(const struct sockaddr *addr, socklen_t addrlen, int domain) {
//     int serverSocket = socket(domain, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

//     if (serverSocket < 0)
//         ERROR_MANAGER("server socket creation", serverSocket, errno);

//     int opt = 1;

//     if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
//         ERROR_MANAGER("setsockopt", -1, errno);

//     if (connect(serverSocket, addr, addrlen) < 0) {
//         if (errno != EINPROGRESS)
//             ERROR_MANAGER("connect", -1, errno);  //FIXME: aca habria que ver que hacer
//     }

//     return serverSocket;
// }

// static int handleOriginConnection(int clientSocket, connectionsManager_t *connectionsManager) {
//     client_t *client = &connectionsManager->clients[clientSocket];
//     int serverSocket;

//     enum request_line_addr_type type = client->request.request_target.host_type;

//     struct sockaddr *addr;
//     socklen_t addrlen;
//     int af;
//     int flags = F_BUFF;
//     status_t clientStatus = AWAITING_ORIGIN_CONNECTION;
//     status_t serverStatus = CONNECTING;

//     /*Hay que conectarse con el servidor doh si es un dominio para resolver el nombre*/
//     if (type == domain) {
//         struct doh *doh = &connectionsManager->doh;
//         if (doh->domain == ipv4) {
//             af = AF_INET;
//             addr = (struct sockaddr *)&connectionsManager->doh.addr.ipv4;
//             addrlen = sizeof(connectionsManager->doh.addr.ipv4);
//         } else {
//             af = AF_INET6;
//             addr = (struct sockaddr *)&connectionsManager->doh.addr.ipv6;
//             addrlen = sizeof(connectionsManager->doh.addr.ipv6);
//         }
//         serverStatus = CONNECTING_TO_DOH_SERVER;
//         flags = flags | F_DOH ;
//     } else if (type == ipv4) {
//         af = AF_INET;
//         addr = (struct sockaddr *)&client->request.request_target.host.ipv4;
//         addrlen = sizeof(client->request.request_target.host.ipv4);
//     } else {
//         af = AF_INET6;
//         addr = (struct sockaddr *)&client->request.request_target.host.ipv6;
//         addrlen = sizeof(client->request.request_target.host.ipv6);
//     }

//     createOriginSocket(addr, addrlen, af);

//     client_t *newClient = &connectionsManager->clients[serverSocket];

//     initClient(newClient, flags);

//     connectionsManager->establishedConnections++;

//     initPairStatus(clientSocket, serverSocket, connectionsManager, clientStatus, originStatus);

//     return 1;
// }

// static int handleNewClient(connectionsManager_t *connectionsManager) {
//     struct sockaddr_in6 address;
//     socklen_t addrlen = sizeof(address);

//     int clientSocket;

//     if ((clientSocket = accept(connectionsManager->passiveSocket, (struct sockaddr *)&address, &addrlen)) < 0)
//         ERROR_MANAGER("accept", clientSocket, errno);

//     int flags = fcntl(clientSocket, F_GETFL, 0);
//     if (flags < 0)
//         ERROR_MANAGER("fnctl", flags, errno);
//     if (fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK) < 0)
//         ERROR_MANAGER("fnctl", -1, errno);

//     client_t *newClient = &connectionsManager->clients[clientSocket];
//     initClient(newClient, F_BUFF | F_REQP);

//     connectionsManager->establishedConnections++;

//     return clientSocket;
// }

// static void initClient(client_t *client, int flags) {
//     if (flags & F_BUFF) {
//         client->data = malloc(PROXY_BUFF_LENGTH);
//         buffer_init(&client->buffer, PROXY_BUFF_LENGTH, client->data);
//     }
//     if (flags & F_REQP) {
//         client->requestParser.request = &client->request;
//         request_parser_init(&client->requestParser);
//     }
//     if (flags & F_DOH) {
//         client->dohParser.response = &client->response;
//         doh_response_parser_init(&client->dohParser);
//     }
// }

// static void initPairStatus(int clientSocket, int serverSocket, connectionsManager_t *connectionsManager, status_t clientStatus, status_t originStatus) {
//     connectionsManager->clients[serverSocket].status = clientStatus;
//     connectionsManager->clients[serverSocket].peerfd = clientSocket;

//     connectionsManager->clients[clientSocket].peerfd = serverSocket;
//     connectionsManager->clients[clientSocket].status = originStatus;
// }

// static void processRequest(int socketfd, fd_set *readfds, fd_set *writefds, connectionsManager_t *connectionsManager) {
//     client_t *client = &connectionsManager->clients[socketfd];

//     if (client->status != INACTIVE) {
//         client_t *peer = &connectionsManager->clients[client->peerfd];

//         if (FD_ISSET(socketfd, readfds))
//             handleReadSet(socketfd, client, peer, connectionsManager);

//         if (FD_ISSET(socketfd, writefds))
//             handleWriteSet(socketfd, client, peer, connectionsManager);
//     }
// }

// static void handleReadSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager) {
//     size_t maxBytes;
//     uint8_t *data;
  
//     data = buffer_write_ptr(&client->buffer, &maxBytes);

//     int totalBytes = read(socketfd, data, maxBytes);
//     if (totalBytes < 0)
//         ERROR_MANAGER("read", totalBytes, errno);
//     if (totalBytes == 0) {
//         printf("Connection closed.\n\n");

//         if (client->peerfd > 0)
//             endConnection(peer, client->peerfd, connectionsManager);

//         endConnection(client, socketfd, connectionsManager);
//         return;
//     }

//     buffer_write_adv(&client->buffer, totalBytes);

//     /*Tengo que parsear la request del cliente para determinar a que host me conecto*/
//     if (client->status == AWAITING_HOST) {
//         for (int i = 0; i < totalBytes; i++) {
//             request_state state = request_parser_feed(&client->requestParser, data[i]);
//             if (state == request_error) {
//                 printf("BAD REQUEST\n");
//                 endConnection(client, socketfd, connectionsManager);
//                 break;
//             } else if (state == request_done) {
//                 if (!handleOriginConnection(socketfd, connectionsManager))
//                     endConnection(client, socketfd, connectionsManager);
//                 break;
//             }
//         }
//     }
// }
// static void handleWriteSet(int socketfd, client_t *client, client_t *peer, connectionsManager_t *connectionsManager) {
//     /*Si todavia se esta conectando hay que chequear la variable SO_ERROR para ver si se obtuvo un error en la función connect*/
//     if (client->status == CONNECTING) {
//         if (!check_origin_connection(socketfd)) {
//             endConnection(peer, client->peerfd, connectionsManager);
//             endConnection(client, socketfd, connectionsManager);
//         }
//         The connection is established
//         client->status = CONNECTED;
//         peer->status = CONNECTED;
//         return;
//     }

//     if (client->status == CONNECTING_TO_DOH_SERVER) {
//         if (!check_origin_connection(socketfd)) {
//             endConnection(peer, client->peerfd, connectionsManager);  //FIXME: Estoy cerrando una conexion que no se establecio
//             endConnection(client, socketfd, connectionsManager);
//         }
//         The connection is established
//         client->status = CONNECTED_TO_DOH_SERVER;
//         peer->status = DOH_REQUEST_IPV4;
//         return;
//     }

//     if (client->status == CONNECTED_TO_DOH_SERVER) {
//         if (peer->status == AWAITING_DOH_REQUEST) {
//             size_t maxBytes;
//             uint8_t *data = buffer_read_ptr(&client->buffer, &maxBytes);
//             int totalBytes = send(socketfd, data, maxBytes, 0);
//             buffer_read_adv(&peer->buffer, totalBytes);
//             pude mandar toda la request
//             if (!buffer_can_read(&client->buffer))
//                 peer->status == AWAITING_DOH_RESPONSE;

//         } else if (peer->status == AWAITING_DOH_RESPONSE) {
//             size_t maxBytes;
//             uint8_t *data;
//             data = buffer_write_ptr(&client->buffer, &maxBytes);
//             int totalBytes = read(socketfd, data, maxBytes);
//             if (totalBytes < 0)
//                 ERROR_MANAGER("read", totalBytes, errno);
//             if (totalBytes == 0)
//                 printf("Connection closed.\n\n");
        
//             buffer_write_adv(&client->buffer, totalBytes);

//             for (int i = 0; i < totalBytes; i++) {
//                 doh_response_state state = doh_response_parser_feed(&peer->dohParser, data[i]);
//                 if (state == response_error) {
//                     printf("BAD RESPONSE FROM DOH\n");
//                     endConnection(client, socketfd, connectionsManager);
//                     peer->status = peer->status == DOH_REQUEST_IPV6 ?  //poner que el client pregunte por ipv6?
//                     break;
//                 } else if (state == response_done) {
//                     cierro el socket que estaba conectado al doh server
//                     endConnection(client, socketfd, connectionsManager);
                    
                
//                     client->addr = answer[0];
//                     if (!handleOriginConnection(client->peerfd, connectionsManager))
//                         endConnection(client, socketfd, connectionsManager);
//                     break;
//                 }
//             }
//         }
//         return;
//     }

//     size_t maxBytes;
//     uint8_t *data = buffer_read_ptr(&peer->buffer, &maxBytes);
//     int totalBytes = send(socketfd, data, maxBytes, 0);
//     buffer_read_adv(&peer->buffer, totalBytes);
// }

// static int check_origin_connection(int socketfd) {
//     socklen_t optSize = sizeof(int);
//     int opt;
//     if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &opt, &optSize) < 0)
//         ERROR_MANAGER("getsockopt", -1, errno);
//     printf("opt val: %d, fd: %ld\n\n", opt, i);
//     MATAR EL SOCKET Y AVISARLE AL CLIENT SI ES QUE NO HAY MAS IPS. SI NO PROBAR CON LA LISTA DE IPS
//     if (opt != 0) {
//         printf("Connection could not be established, closing sockets.\n\n");
//         return false;
//     }

//     return true;
// }

// static void endConnection(client_t *client, int fd, connectionsManager_t *connectionsManager) {
//     if (close(fd) < 0)
//         ERROR_MANAGER("close", -1, errno);

//     connectionsManager->establishedConnections--;
//     doh_response_parser_destroy(&client->dohParser);
//     free(client->data);
//     memset(client, 0, sizeof(client_t));  // Zero out structure
// }
