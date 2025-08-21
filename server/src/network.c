#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "packet.h"
#include "server.h"
#include "game.h"
#include "network.h"

ServerEntityState entitiesToSend[UINT8_MAX - 1];
int entitiesAmountToSend = 0;

char eventBuffer[MAX_PACKET_SIZE];
size_t eventBufferOffset = 0;

void NetworkPrepareEventBuffer() { eventBufferOffset = 0; }

int NetworkPushEntityDiedEvent(ServerEntityDiedEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerEntityDiedEvent);
    const size_t need = sizeof(ServerEventHeader) + size;

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader *eventHeader = (ServerEventHeader *)eventBuffer + eventBufferOffset;
    eventHeader->type = PACKET_ENTITY_DIED;
    eventHeader->size = size;

    eventBufferOffset += sizeof(ServerEventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

void NetworkSendEventPacket(Server *server)
{
    ServerPacketHeader header;
    header.type = PACKET_SERVER_EVENTS;
    header.size = eventBufferOffset;

    if (header.size <= 0)
    {
        // printf("[SERVER] No events to send\n");
        return;
    }

    int sent;
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        SOCKET client = server->clients[i];
        if (client == INVALID_SOCKET)
            continue;

        int sent = send(client, (char *)&header, sizeof(header), 0);
        if (sent == SOCKET_ERROR)
        {
            printf("Failed to send header to client[%d]: %d\n", i, WSAGetLastError());
            continue;
        }
        sent = send(client, eventBuffer, header.size, 0);
        if (sent == SOCKET_ERROR)
        {
            printf("Client[%d]: Buffer send failed: %d\n", i, WSAGetLastError());
        }
        else if (sent != header.size)
        {
            printf("Warning: not all bytes sent (%d/%d)\n", sent, header.size);
        }
    }
}

ServerWaveSnapshot waveStateToSend;

void NetworkSendAssignedPlayerID(Server *server, int clientIndex, uint8_t playerID)
{
    char buffer[MAX_PACKET_SIZE];
    ServerPacketHeader *header = (ServerPacketHeader *)buffer;
    ServerAssignPlayerMessage *message = (ServerAssignPlayerMessage *)(buffer + sizeof(ServerPacketHeader));
    header->type = PACKET_ASSIGN_PLAYER;
    header->size = sizeof(ServerAssignPlayerMessage);

    message->playerID = playerID;

    int totalSize = sizeof(ServerPacketHeader) + header->size;

    SOCKET client = server->clients[clientIndex];
    if (client == INVALID_SOCKET)
        return;
    printf("Sending to Client[%d] their assigned PlayerID: %d, type: %d, size: %d\n", clientIndex, message->playerID, header->type, header->size);

    int sent = send(client, buffer, totalSize, 0);
    if (sent == SOCKET_ERROR)
    {
        printf("Failed to send PACKET_ASSIGN_PLAYER to client[%d]: %d\n", clientIndex, WSAGetLastError());
    }
}

void NetworkSetEntities(ServerEntityState *entities, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        entitiesToSend[i] = entities[i];
    }
    entitiesAmountToSend = amount;
}

void NetworkSetWaveState(ServerWaveSnapshot waveState)
{
    waveStateToSend = waveState;
}

void NetworkSendEntitiesSnapshot(struct Server *server)
{
    char buffer[MAX_PACKET_SIZE];
    ServerPacketHeader *header = (ServerPacketHeader *)buffer;

    ServerEntityState *entities = (ServerEntityState *)(buffer + sizeof(ServerPacketHeader));

    int count = entitiesAmountToSend;
    for (int i = 0; i < entitiesAmountToSend; i++)
    {
        memcpy(&entities[i], &entitiesToSend[i], sizeof(ServerEntityState));
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
        if (client == INVALID_SOCKET)
            continue;
        printf("Sending to Client[%d], %d entities\n", i, count);
        // printf("Sending to Client[%d], Entity[%d], .x=%d, .y=%d\n", i, entity, entities[entity].x, entities[entity].y);

        int sent = send(client, buffer, totalSize, 0);
        if (sent == SOCKET_ERROR)
        {
            printf("Failed to send entities to client[%d]: %d\n", i, WSAGetLastError());
        }
    }
}

void NetworkSendWaveSnapshot(Server *server)
{
    char buffer[MAX_PACKET_SIZE];
    ServerPacketHeader *header = (ServerPacketHeader *)buffer;

    ServerWaveSnapshot *waveState = (ServerWaveSnapshot *)(buffer + sizeof(ServerPacketHeader));

    memcpy(waveState, &waveStateToSend, sizeof(ServerWaveSnapshot));

    int payloadSize = sizeof(ServerWaveSnapshot);

    if (payloadSize + sizeof(ServerPacketHeader) > MAX_PACKET_SIZE)
    {
        printf("Wave Snapshot exceeds capacity\n");
        return;
    }
    header->type = PACKET_WAVE_SNAPSHOT;
    header->size = payloadSize;

    int totalSize = sizeof(ServerPacketHeader) + payloadSize;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        SOCKET client = server->clients[i];
        if (client != INVALID_SOCKET)
        {

            int sent = send(client, buffer, totalSize, 0);
            if (sent == SOCKET_ERROR)
            {
                printf("Failed to send Wave Snapshot to Client[%d]: %d\n", i, WSAGetLastError());
            }
        }
    }
}
