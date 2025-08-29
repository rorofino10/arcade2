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

const double timeBetweenSnapshotTicks = 2.0f;
double elapsedTimeBetweenSnapshotTicks = 0.0f;

const double DeltaTPS = 10.0f;
const double timeBetweenDeltaTicks = 1.0f / DeltaTPS;
double elapsedTimeBetweenDeltaTicks = 0.0f;

const double EventTPS = 60.0f;
const double timeBetweenEventTicks = 1.0 / EventTPS;
double elapsedTimeBetweenEventTicks = 0.0f;

const double timeBetweenHeartbeatDisconnect = 5.0f;

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
        NetworkSendUnreliableEntitiesSnapshot();
        NetworkSendUnreliableWaveSnapshot();
    }
}

void ServerTryDeltaTick(Server *server)
{

    while (elapsedTimeBetweenDeltaTicks >= timeBetweenDeltaTicks)
    {
        elapsedTimeBetweenDeltaTicks -= timeBetweenDeltaTicks;
        NetworkSendUnreliableEntitiesDeltas();
        NetworkSendUnreliableWaveSnapshot();
    }
}

void ServerTryEventTick(Server *server)
{

    while (elapsedTimeBetweenEventTicks >= timeBetweenEventTicks)
    {
        elapsedTimeBetweenEventTicks -= timeBetweenEventTicks;

        NetworkSendEventPacket();

        GamePushEntityDeltas();
        NetworkSendUnreliableEventPacket();
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

    server->fds[server->nfds].fd = server->listenSocket;
    server->fds[server->nfds].events = POLLRDNORM;
    server->nfds++;

    // Setting Up UDP Socket
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET udpSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (udpSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    iResult = bind(udpSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(udpSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
    u_long mode = 1;
    ioctlsocket(udpSocket, FIONBIO, &mode);

    server->fds[server->nfds].fd = udpSocket;
    server->fds[server->nfds].events = POLLRDNORM;
    server->nfds++;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        server->tcpClients[i] = INVALID_SOCKET;
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        server->udpClients[i].active = FALSE;
    }
    printf("[SERVER] Initialized\n");
    NetworkSetServer(server);
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
        if (server->tcpClients[i] == INVALID_SOCKET)
        {
            u_long mode = 1;
            ioctlsocket(newClientSocket, FIONBIO, &mode);
            server->tcpClients[i] = newClientSocket;
            server->fds[server->nfds].fd = newClientSocket;
            server->fds[server->nfds].events = POLLRDNORM;
            server->nfds++;
            printf("New client connected: %d\n", i);
            uint8_t playerID = GameAssignPlayerToClient(i);
            NetworkSendAssignedPlayerID(i, playerID);
            NetworkSendEntitiesSnapshot();
            NetworkSendWaveSnapshot(); // Send Reliable Snapshot to have something to start with.
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
        if (server->tcpClients[i] != INVALID_SOCKET)
        {
            send(server->tcpClients[i], buf, len, 0);
        }
    }
}

int FindOrAddUDPClient(Server *server, struct sockaddr_in *from)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        UDPClient *client = &server->udpClients[i];
        if (client->active && client->addr.sin_addr.S_un.S_addr == from->sin_addr.S_un.S_addr && client->addr.sin_port == from->sin_port)
            return i;
    }
    printf("Did not find. Adding.\n");
    // Did not find, Add.
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        UDPClient *client = &server->udpClients[i];
        if (client->active)
            continue;
        client->addr = *from;
        client->lastAckedSeq = 0;
        client->nextSeq = 1;
        client->active = TRUE;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from->sin_addr, ip, sizeof(ip));
        printf("New client from %s:%d\n", ip, ntohs(from->sin_port));
        return i;
    }
    printf("No space to add.\n");
    // No space to add.
    return -1;
}

void ServerHandleUDPClient(Server *server)
{
    // printf("Handling UDPClient\n");
    SOCKET udpSocket = server->fds[1].fd;
    char buffer[1400];
    struct sockaddr_in from;
    int fromLen = sizeof(from);
    int bytes = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromLen);

    if (bytes > 0)
    {

        ClientUDPPacketHeader *header = (ClientUDPPacketHeader *)buffer;

        int clientIdx = FindOrAddUDPClient(server, &from);
        if (clientIdx == -1)
            return;
        server->udpClients[clientIdx].timeBetweenHeartbeat = 0.0f; // Reset timer, received a packet
    }
}

void ServerDisconnectClient(Server *server, int fdsIndex)
{
    // Disconnect both TCP and UDP client's sockets.
    int clientIndex = fdsIndex - 2; // First two are Listener and UDP sockets.
    SOCKET s = server->fds[fdsIndex].fd;

    printf("Client[%d] disconnected\n", clientIndex);
    closesocket(s);
    server->tcpClients[clientIndex] = INVALID_SOCKET;
    server->udpClients[clientIndex].active = FALSE;
    server->fds[fdsIndex] = server->fds[server->nfds - 1];
    server->nfds--;
}

void ServerHandleClient(Server *server, int fdsIndex)
{
    ClientPacketHeader header;
    SOCKET s = server->fds[fdsIndex].fd;
    int recvlen;
    int clientIndex = fdsIndex - 2;
    recvlen = recv(s, (char *)&header, sizeof(header), 0);

    if (recvlen > 0)
    {
        printf("Client[%d] sent packet: type: %d, size: %d\n", clientIndex, header.type, header.size);
    }
    else if (recvlen == 0)
    {
        ServerDisconnectClient(server, fdsIndex);
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
            ServerDisconnectClient(server, fdsIndex);

            return;
        }
    }
    char *buffer = malloc(header.size);
    recvlen = recv(s, buffer, header.size, 0);
    size_t offset = 0;
    int events = 0;
    while (offset < header.size) // parse Events
    {
        ClientEventHeader *eheader = (ClientEventHeader *)(buffer + offset);
        offset += sizeof(ClientEventHeader);

        char *edata = buffer + offset;
        offset += eheader->size;

        printf("[NETWORK] Event[%d] Header.size: %d, Header.type: %d\n", events, eheader->size, eheader->type);
        events++;
        switch (eheader->type)
        {
        case CLIENT_EVENT_INPUT_AUTHORATIVE_MOVE:
            ClientInputAuthorativeMoveEvent *move = (ClientInputAuthorativeMoveEvent *)edata;
            UpdatePlayerPosition(clientIndex, move->nx, move->ny);
            break;
        case CLIENT_EVENT_INPUT_MOVE:
            ClientInputEvent *event = (ClientInputEvent *)edata;
            GameApplyPlayerMovementInput(clientIndex, event);
            break;
        case CLIENT_EVENT_INPUT_SHOOT:
            ClientInputShootEvent *shoot = (ClientInputShootEvent *)edata;
            ShootBulletInput(clientIndex, shoot->dx, shoot->dy, shoot->bulletSequence);
            break;
        default:
            break;
        }
    }
    free(buffer);
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
        elapsedTimeBetweenDeltaTicks += deltaTime;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            UDPClient *client = &server->udpClients[i];
            if (!client->active)
                continue;
            client->timeBetweenHeartbeat += deltaTime;
            if (client->timeBetweenHeartbeat >= timeBetweenHeartbeatDisconnect)
                ServerDisconnectClient(server, i + 2);
        }

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
        if (server->fds[1].revents & POLLRDNORM)
        {
            ServerHandleUDPClient(server);
        }

        for (int i = 2; i < server->nfds; i++)
        {

            if (server->fds[i].revents & (POLLRDNORM | POLLERR | POLLHUP))
            {
                ServerHandleClient(server, i);
                continue;
            }
        }

        ServerTrySnapshotTick(server);
        ServerTryEventTick(server);
        ServerTryDeltaTick(server);
        ServerTryTick();
    }
}

void ServerStop(Server *server)
{
    closesocket(server->listenSocket);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (server->tcpClients[i] != SOCKET_ERROR)
            closesocket(server->tcpClients[i]);
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
