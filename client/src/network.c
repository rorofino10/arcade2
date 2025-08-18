#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "client.h"
#include "network.h"
#include "packet.h"

void NetworkSendPacket(Client *client, const ClientPacket *packet)
{
    int sent = send(client->socket, (const char *)packet, sizeof(ClientPacket), 0);
    if (sent == SOCKET_ERROR)
    {
        printf("send failed: %d\n", WSAGetLastError());
    }
    else if (sent != sizeof(ClientPacket))
    {
        printf("Warning: not all bytes sent (%d/%d)\n", sent, sizeof(ClientPacket));
    }
}

void NetworkSendMoveMessage(Client *client, int8_t x, int8_t y)
{
    ClientPacket packet;
    packet.header.type = 1;
    packet.data.x = x;
    packet.data.y = y;
    NetworkSendPacket(client, &packet);
}
