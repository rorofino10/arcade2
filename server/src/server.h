#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "2112"
#define MAX_CLIENTS 2

typedef struct
{
    struct sockaddr_in addr;
    int active;

    uint32_t nextSeq;
    uint32_t lastAckedSeq;

    double timeBetweenHeartbeat;
} UDPClient;

typedef struct Server
{
    SOCKET listenSocket;
    SOCKET tcpClients[MAX_CLIENTS];
    UDPClient udpClients[MAX_CLIENTS];
    WSAPOLLFD fds[2 + MAX_CLIENTS];
    int nfds;
} Server;

#endif
