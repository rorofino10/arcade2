#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "packet.h"
#include "server.h"
#include "game.h"
#include "network.h"

NetworkEntity entitiesToSend[UINT8_MAX - 1];
int entitiesAmountToSend = 0;

void NetworkSetEntities(NetworkEntity *entities, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        entitiesToSend[i] = entities[i];
    }
    entitiesAmountToSend = amount;
}

void NetworkSendEntities(struct Server *server)
{
    char buffer[MAX_PACKET_SIZE];
    ServerPacketHeader *header = (ServerPacketHeader *)buffer;

    ServerEntityState *entities = (ServerEntityState *)(buffer + sizeof(ServerPacketHeader));

    int count = 0;
    for (int i = 0; i < entitiesAmountToSend; i++)
    {
        entities[count].id = entitiesToSend[i].id;
        entities[count].x = entitiesToSend[i].x;
        entities[count].y = entitiesToSend[i].y;
        entities[count].vx = entitiesToSend[i].dx;
        entities[count].vy = entitiesToSend[i].dy;
        count++;
    }

    int payloadSize = count * sizeof(ServerEntityState);

    if (payloadSize + sizeof(ServerPacketHeader) > MAX_PACKET_SIZE)
    {
        printf("Snapshot exceeds capacity\n");
        return;
    }
    header->type = PACKET_ENTITY_SNAPSHOT;
    header->size = payloadSize;

    int totalSize = sizeof(ServerPacketHeader) + payloadSize;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        SOCKET client = server->clients[i];
        if (client != INVALID_SOCKET)
        {
            printf("Sending to Client[%d], %d entities\n", i, count);
            for (int entity = 0; entity < count; entity++)
            {
                printf("Sending to Client[%d], Entity[%d], .x=%d, .y=%d\n", i, entity, entities[entity].x, entities[entity].y);
            }

            int sent = send(client, buffer, totalSize, 0);
            if (sent == SOCKET_ERROR)
            {
                printf("Failed to send entities to client[%d]: %d\n", i, WSAGetLastError());
                // optionally close socket here
            }
        }
    }
}
