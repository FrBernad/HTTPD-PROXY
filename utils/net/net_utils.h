#ifndef _NET_UTILS_H_
#define _NET_UTILS_H_

#include <arpa/inet.h>

int
establish_origin_connection(struct sockaddr *addr, socklen_t addrlen, int protocol);

#endif