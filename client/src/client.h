#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>

#define DEFAULT_PORT "2112"
#define DEFAULT_HOST "127.0.0.1"

typedef struct Client
{
    SOCKET socket;
    struct addrinfo *clientaddrinfo;
} Client;

#endif
