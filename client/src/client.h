#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>

typedef struct Client
{
    SOCKET socket;
} Client;

#endif
