#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <time.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "game.h"
#include "packet.h"
#include "server.h"
#include "network.h"

#pragma comment(lib, "Ws2_32.lib")

const double TPS = 60.0f;
const double timeBetweenTicks = 1.0 / TPS;
double elapsedTimeBetweenTicks = 0.0f;

const double SnapshotTPS = 10.0f;
const double timeBetweenSnapshotTicks = 1.0 / SnapshotTPS;
double elapsedTimeBetweenSnapshotTicks = 0.0f;

const double EventTPS = 35.0f;
const double timeBetweenEventTicks = 1.0 / EventTPS;
double elapsedTimeBetweenEventTicks = 0.0f;

void ServerTryTick()
{

    while (elapsedTimeBetweenTicks >= timeBetweenTicks)
    {
        GameUpdate(timeBetweenTicks);
        elapsedTimeBetweenTicks -= timeBetweenTicks;
    }
}

void ServerTrySnapshotTick(Server *server)
{

    while (elapsedTimeBetweenSnapshotTicks >= timeBetweenSnapshotTicks)
    {
        elapsedTimeBetweenSnapshotTicks -= timeBetweenSnapshotTicks;
        NetworkSendEntitiesSnapshot(server);
        NetworkSendWaveSnapshot(server);
    }
}

void ServerTryEventTick(Server *server)
{

    while (elapsedTimeBetweenEventTicks >= timeBetweenEventTicks)
    {
        elapsedTimeBetweenEventTicks -= timeBetweenEventTicks;
        NetworkSendEventPacket(server);
        NetworkPrepareEventBuffer();
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
            u_long mode = 1;
            ioctlsocket(newClientSocket, FIONBIO, &mode);
            server->clients[i] = newClientSocket;
            server->fds[server->nfds].fd = newClientSocket;
            server->fds[server->nfds].events = POLLRDNORM;
            server->nfds++;
            printf("New client connected: %d\n", i);
            uint8_t playerID = GameAssignPlayerToClient(i);
            NetworkSendAssignedPlayerID(server, i, playerID);
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
    ClientPacketHeader header;
    SOCKET s = server->fds[fdsIndex].fd;
    int recvlen;
    recvlen = recv(s, (char *)&header, sizeof(header), 0);

    if (recvlen > 0)
    {
        printf("Client[%d] sent packet: type: %d, size: %d\n", fdsIndex - 1, header.type, header.size);
    }
    else if (recvlen == 0)
    {
        printf("Client[%d] disconnected\n", fdsIndex - 1);
        closesocket(s);
        server->clients[fdsIndex - 1] = INVALID_SOCKET;
        server->fds[fdsIndex] = server->fds[server->nfds - 1];
        server->nfds--;
        return;
    }
    else
    {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            return;
        }
        else
        {
            printf("Client[%d] disconnected, error %d\n", fdsIndex - 1, err);
            closesocket(s);
            server->clients[fdsIndex - 1] = INVALID_SOCKET;
            server->fds[fdsIndex] = server->fds[server->nfds - 1];
            server->nfds--;
            return;
        }
    }
    char buffer[MAX_PACKET_SIZE];
    if (header.size > sizeof(buffer)) // sent is bigger than MAX_PACKET_SIZE
        return;
    recvlen = recv(s, buffer, header.size, 0);
    size_t offset = 0;
    int events = 0;
    while (offset < header.size)
    {
        ClientEventHeader *eheader = (ClientEventHeader *)(buffer + offset);
        offset += sizeof(ClientEventHeader);

        char *edata = buffer + offset;
        offset += eheader->size;

        printf("[NETWORK] Event[%d] Header.size: %d, Header.type: %d\n", events, eheader->size, eheader->type);
        events++;
        switch (eheader->type)
        {
        case CLIENT_EVENT_INPUT_MOVE:
            ClientInputMoveEvent *move = (ClientInputMoveEvent *)edata;
            UpdatePlayerPosition(fdsIndex - 1, move->nx, move->ny);
            // printf("Move nx=%d ny=%d\n", move->nx, move->ny);
            break;
        case CLIENT_EVENT_INPUT_SHOOT:
            ClientInputShootEvent *shoot = (ClientInputShootEvent *)edata;
            ShootBulletInput(fdsIndex - 1, shoot->dx, shoot->dy);
            printf("Client[%d] ShootEvent\n", fdsIndex - 1);
            break;
        default:
            break;
        }
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
        elapsedTimeBetweenSnapshotTicks += deltaTime;
        elapsedTimeBetweenEventTicks += deltaTime;

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

            if (server->fds[i].revents & (POLLRDNORM | POLLERR | POLLHUP))
            {
                ServerHandleClient(server, i);
                continue;
            }
        }

        ServerTrySnapshotTick(server);
        ServerTryEventTick(server);
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
