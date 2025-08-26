#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>

#include "game.h"
#include "client.h"
#include "network.h"

#pragma comment(lib, "Ws2_32.lib")

int ClientInit(Client *client);
void ClientRun(Client *client);
void ClientClose(Client *client);

int ClientInit(Client *client)
{
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(DEFAULT_HOST, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    ptr = result;

    // Create a SOCKET for connecting to server
    client->socket = socket(ptr->ai_family, ptr->ai_socktype,
                            ptr->ai_protocol);
    if (client->socket == INVALID_SOCKET)
    {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    client->clientaddrinfo = result;
    NetworkSetClient(client);
}

void ClientRun(Client *client)
{
    GameRun();
}

void ClientClose(Client *client)
{
    shutdown(client->socket, SD_BOTH);
    printf("[Client]: Shutdown client socket\n");
    closesocket(client->socket);
    printf("[Client]: Close client socket\n");
    WSACleanup();
}

int main(int argc, char **argv)
{
    Client client;

    ClientInit(&client);

    ClientRun(&client);

    ClientClose(&client);

    return 0;
}
