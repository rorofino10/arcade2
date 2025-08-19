#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "2112"
#define MAX_CLIENTS 2

typedef struct Server
{
    SOCKET listenSocket;
    SOCKET clients[MAX_CLIENTS];
    WSAPOLLFD fds[1 + MAX_CLIENTS];
    int nfds;
} Server;

#endif
