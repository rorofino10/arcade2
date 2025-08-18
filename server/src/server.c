#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <time.h>

#include <stdlib.h>
#include <stdio.h>

#include "game.h"
#include "packet.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "2112"
#define MAX_CLIENTS 2

const double TPS = 60.0f;
const double timeBetweenTicks = 1.0 / TPS;
double elapsedTimeBetweenTicks = 0.0f;
const double NetworkTPS = 20.0f;
const double timeBetweenNetworkTicks = 1.0 / NetworkTPS;
double elapsedTimeBetweenNetworkTicks = 0.0f;

typedef struct
{
    SOCKET listenSocket;
    SOCKET clients[MAX_CLIENTS];
    WSAPOLLFD fds[1 + MAX_CLIENTS];
    int nfds;
} Server;

void ServerTryTick()
{
    while (elapsedTimeBetweenTicks >= timeBetweenTicks)
    {
        GameUpdate(timeBetweenTicks);
        elapsedTimeBetweenTicks -= timeBetweenTicks;
    }
}

void ServerTryNetworkTick()
{
    while (elapsedTimeBetweenNetworkTicks >= timeBetweenNetworkTicks)
    {
        elapsedTimeBetweenNetworkTicks -= timeBetweenNetworkTicks;
    }
}

int ServerInit(Server *server)
{
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    server->listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (server->listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(server->listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(server->listenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    iResult = listen(server->listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(server->listenSocket);
        WSACleanup();
        return 1;
    }

    server->fds[0].fd = server->listenSocket;
    server->fds[0].events = POLLRDNORM;
    server->nfds = 1;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        server->clients[i] = INVALID_SOCKET;
    }
    printf("init success\n");
    GameInit();
    return 0;
}

void ServerTryAcceptConnection(Server *server)
{
    SOCKET newClientSocket = accept(server->listenSocket, NULL, NULL);
    if (newClientSocket == INVALID_SOCKET)
    {
        printf("accept failed: %d\n", WSAGetLastError());
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (server->clients[i] == INVALID_SOCKET)
        {
            server->clients[i] = newClientSocket;
            server->fds[server->nfds].fd = newClientSocket;
            server->fds[server->nfds].events = POLLRDNORM;
            server->nfds++;
            printf("New client connected: %d\n", i);
            return;
        }
    }

    printf("Max amount of clients reached, did not accept.\n");
    closesocket(newClientSocket);
}

void ServerBroadcast(Server *server, char *buf, int len)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (server->clients[i] != INVALID_SOCKET)
        {
            send(server->clients[i], buf, len, 0);
        }
    }
}

void ServerHandleClient(Server *server, int fdsIndex)
{
    int packetSize = sizeof(ClientPacket);
    ClientPacket packet;
    SOCKET s = server->fds[fdsIndex].fd;

    int recvlen = recv(s, &packet, packetSize, 0);
    if (recvlen > 0)
    {
        // buf[recvlen] = '\0';
        // printf("Client[%d] says: %s\n", fdsIndex, buf);
        printf("Client[%d] sent packet: type: %d, x: %d, y: %d\n", fdsIndex, packet.header.type, packet.data.x, packet.data.y);
        // ServerBroadcast(server, buf, recvlen);
    }
    else
    {
        printf("Client[%d] disconnected\n", fdsIndex);
        closesocket(s);
        server->clients[fdsIndex - 1] = INVALID_SOCKET;        // fdsIndex is clientIndex+1, just set as invalid in clients
        server->fds[fdsIndex] = server->fds[server->nfds - 1]; // Swap last with the disconnected one
        server->nfds--;
    }
}

double getTimeSeconds()
{
    static double invFreq = 0.0;
    if (invFreq == 0.0)
    {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        invFreq = 1.0 / (double)freq.QuadPart;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * invFreq;
}

void ServerRun(Server *server)
{
    double prevTime = getTimeSeconds();
    while (TRUE)
    {
        double currentTime = getTimeSeconds();
        double deltaTime = currentTime - prevTime;
        prevTime = currentTime;

        elapsedTimeBetweenTicks += deltaTime;
        elapsedTimeBetweenNetworkTicks += deltaTime;

        int iResult;

        iResult = WSAPoll(server->fds, server->nfds, 0);
        if (iResult == SOCKET_ERROR)
        {
            printf("WSAPoll failed: %d\n", WSAGetLastError());
            break;
        }

        if (server->fds[0].revents & POLLRDNORM)
        {
            ServerTryAcceptConnection(server);
        }

        for (int i = 1; i < server->nfds; i++)
        {
            if (server->fds[i].revents & POLLRDNORM)
            {
                ServerHandleClient(server, i);
            }
        }

        ServerTryNetworkTick();
        ServerTryTick();
    }
}

void ServerStop(Server *server)
{
    closesocket(server->listenSocket);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (server->clients[i] != SOCKET_ERROR)
            closesocket(server->clients[i]);
    }
    WSACleanup();
}
int main(int argc, char **argv)
{
    Server server;
    int initRes = ServerInit(&server);
    if (initRes != 0)
        return 1;
    ServerRun(&server);
    ServerStop(&server);
    return 0;
}
