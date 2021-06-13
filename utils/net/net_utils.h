#ifndef _NET_UTILS_H_28227759991b074bcd6b1bb13b8e8b1f50cfc69e01fe2026d2e33182
#define _NET_UTILS_H_28227759991b074bcd6b1bb13b8e8b1f50cfc69e01fe2026d2e33182

#include <arpa/inet.h>

int
establish_origin_connection(struct sockaddr *addr, socklen_t addrlen, int protocol);

#endif