#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "client.h"
#include "network.h"
#include "packet.h"

char buffer[MAX_PACKET_SIZE];
size_t offset = 0;

void NetworkPrepareBuffer() { offset = 0; }

void NetworkRecievePacket(Client *client)
{
    ServerPacketHeader header;
    int recvlen;
    recvlen = recv(client->socket, (char *)&header, sizeof(header), 0);
    if (recvlen <= 0)
        return; // disconnect or error

    char buffer[MAX_PACKET_SIZE];
    recvlen = recv(client->socket, buffer, header.size, 0);
    if (recvlen <= 0)
        return;

    switch (header.type)
    {
    case PACKET_ENTITY_SNAPSHOT:
    {
        ServerEntityState *entitiesPacket = (ServerEntityState *)buffer;
        int count = header.size / sizeof(ServerEntityState);
        printf("Received %d entities from server\n", count);

        for (int i = 0; i < count; i++)
        {
            ServerEntityState *e = &entitiesPacket[i];
            printf("Entity %d: id=%d, x=%d, y=%d\n",
                   i, e->id, e->x, e->y);
        }
    }
    break;

    default:
        printf("Unknown packet type %d\n", header.type);
        break;
    }
}

void NetworkSendPacket(Client *client)
{
    ClientPacketHeader header;
    header.type = PACKET_INPUT_EVENT;
    header.size = offset;

    int sent;
    sent = send(client->socket, (char *)&header, sizeof(header), 0);
    if (sent == SOCKET_ERROR)
    {
        printf("Header send failed: %d\n", WSAGetLastError());
        return;
    }
    else if (sent != sizeof(ClientPacketHeader))
    {
        printf("Warning: not all bytes sent (%d/%d)\n", sent, sizeof(ClientPacketHeader));
        return;
    }
    // printf("Sent PacketHeader, type: %d, size: %d\n", header.type, sizeof(ClientPacketHeader));

    sent = send(client->socket, buffer, header.size, 0);
    if (sent == SOCKET_ERROR)
    {
        printf("Buffer send failed: %d\n", WSAGetLastError());
    }
    else if (sent != header.size)
    {
        printf("Warning: not all bytes sent (%d/%d)\n", sent, header.size);
    }
    // printf("Sent Buffer, size: %d\n", header.size);
}

int NetworkPushInputEvent(PACKET_INPUT_EVENT_TYPE type, void *data, uint16_t size)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ClientPacketHeader);
    const size_t need = sizeof(ClientEventHeader) + (size_t)size;

    if (need > capacity || offset + need > capacity)
        return 1;

    ClientEventHeader header;
    header.type = type;
    header.size = size;

    memcpy(buffer + offset, &header, sizeof(header));
    offset += sizeof(header);

    memcpy(buffer + offset, data, size);
    offset += size;
    return 0;
}
