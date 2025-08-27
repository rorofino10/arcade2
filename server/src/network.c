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

ServerWaveSnapshot waveStateToSend;

char eventBuffer[MAX_PACKET_SIZE];
size_t eventBufferOffset = 0;

char unreliableEventBuffer[MAX_PACKET_SIZE];
size_t unreliableEventBufferOffset = 0;

Server *server;

void NetworkSetServer(Server *serverToSet) { server = serverToSet; }

void NetworkPrepareEventBuffer() { eventBufferOffset = 0; }
void NetworkPrepareUnreliableEventBuffer() { unreliableEventBufferOffset = 0; }

int NetworkPushBulletSpawnEvent(ServerBulletSpawnEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerBulletSpawnEvent);
    const size_t need = sizeof(ServerEventHeader) + size;
    printf("Pushing BulletSpawnEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_EVENT_BULLET_SPAWN;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

int NetworkPushEntityDiedEvent(ServerEntityDiedEvent event)
{

    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerEntityDiedEvent);
    const size_t need = sizeof(ServerEventHeader) + size;
    printf("Pushing Entity Died Event, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_EVENT_ENTITY_DIED;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

int NetworkPushPlayerCanShootEvent(ServerPlayerCanShootEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerPlayerCanShootEvent);
    const size_t need = sizeof(ServerEventHeader) + size;
    printf("Pushing PlayerCanShootEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_EVENT_PLAYER_CAN_SHOOT;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

int NetworkPushNewEntityEvent(ServerEntityState entity)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerEntityState);
    const size_t need = sizeof(ServerEventHeader) + size;
    printf("Pushing NewEntityEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_EVENT_NEW_ENTITY;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &entity, size);
    eventBufferOffset += size;
    return 0;
}

int NetworkPushPowerupEvent(ServerPowerupEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerPowerupEvent);
    const size_t need = sizeof(ServerEventHeader) + size;
    printf("Pushing PowerupEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_EVENT_POWERUP;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}
int NetworkPushEntityFacingDelta(ServerEntityFacingDelta delta)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerPacketHeader);
    const size_t size = sizeof(ServerEntityFacingDelta);
    const size_t need = sizeof(ServerEventHeader) + size;
    printf("Pushing EntityFacingDelta, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_DELTA_ENTITY_FACING;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &delta, size);
    eventBufferOffset += size;
    return 0;
}

void NetworkSendEventPacket()
{
    char buffer[MAX_PACKET_SIZE];
    ServerPacketHeader *header = (ServerPacketHeader *)buffer;

    header->type = PACKET_SERVER_EVENTS;
    header->size = eventBufferOffset;

    if (header->size <= 0)
    {
        // printf("[SERVER] No events to send\n");
        return;
    }
    int totalSize = sizeof(ServerPacketHeader) + header->size;

    char *payload = (char *)(buffer + sizeof(ServerPacketHeader));
    memcpy(payload, eventBuffer, header->size);

    int sent;
    // printf("[SERVER]: Broadcasting Event Packets, %d bytes\n", eventBufferOffset);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        SOCKET client = server->tcpClients[i];
        if (client == INVALID_SOCKET)
            continue;
        header->lastProcessedBullet = lastProcessedBullet[i];
        header->lastProcessedMovementInput = lastProcessedMovementInput[i];
        sent = send(client, buffer, totalSize, 0);
        if (sent == SOCKET_ERROR)
        {
            printf("Client[%d]: Buffer send failed: %d\n", i, WSAGetLastError());
        }
        else if (sent != totalSize)
        {
            printf("Warning: not all bytes sent (%d/%d)\n", sent, totalSize);
        }
    }
    NetworkPrepareEventBuffer();
}

void NetworkSendAssignedPlayerID(int clientIndex, uint8_t playerID)
{
    char buffer[MAX_PACKET_SIZE];
    ServerPacketHeader *header = (ServerPacketHeader *)buffer;
    ServerAssignPlayerMessage *message = (ServerAssignPlayerMessage *)(buffer + sizeof(ServerPacketHeader));
    header->type = PACKET_ASSIGN_PLAYER;
    header->size = sizeof(ServerAssignPlayerMessage);

    message->playerID = playerID;

    int totalSize = sizeof(ServerPacketHeader) + header->size;

    SOCKET client = server->tcpClients[clientIndex];
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

void NetworkSendEntitiesSnapshot()
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

    // printf("[NETWORK] Broadcasting EntitiesSnapshot: size:%d, count:%d\n", header->size, count);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        SOCKET client = server->tcpClients[i];
        if (client == INVALID_SOCKET)
            continue;
        // printf("Sending to Client[%d], %d entities\n", i, count);
        // printf("Sending to Client[%d], Entity[%d], .x=%d, .y=%d\n", i, entity, entities[entity].x, entities[entity].y);
        header->lastProcessedBullet = lastProcessedBullet[i];
        header->lastProcessedMovementInput = lastProcessedMovementInput[i];
        int sent = send(client, buffer, totalSize, 0);
        if (sent == SOCKET_ERROR)
        {
            printf("Failed to send entities to client[%d]: %d\n", i, WSAGetLastError());
        }
    }
}

void NetworkSendWaveSnapshot()
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
        SOCKET client = server->tcpClients[i];
        if (client != INVALID_SOCKET)
        {
            header->lastProcessedBullet = lastProcessedBullet[i];
            header->lastProcessedMovementInput = lastProcessedMovementInput[i];
            int sent = send(client, buffer, totalSize, 0);
            if (sent == SOCKET_ERROR)
            {
                printf("Failed to send Wave Snapshot to Client[%d]: %d\n", i, WSAGetLastError());
            }
        }
    }
}

void NetworkSendUnreliableWaveSnapshot()
{
    char buffer[MAX_PACKET_SIZE];
    ServerUDPPacketHeader *header = (ServerUDPPacketHeader *)buffer;
    ServerWaveSnapshot *waveState = (ServerWaveSnapshot *)(buffer + sizeof(ServerUDPPacketHeader));

    memcpy(waveState, &waveStateToSend, sizeof(ServerWaveSnapshot));

    int payloadSize = sizeof(ServerWaveSnapshot);
    int totalSize = sizeof(ServerUDPPacketHeader) + payloadSize;

    if (totalSize > MAX_PACKET_SIZE)
    {
        printf("Snapshot exceeds capacity\n");
        return;
    }
    header->type = PACKET_WAVE_SNAPSHOT;
    header->size = payloadSize;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        UDPClient *client = &server->udpClients[i];
        if (!client->active)
            continue;
        header->sequence = client->nextSeq++;
        header->lastProcessedBullet = lastProcessedBullet[i];
        header->lastProcessedMovementInput = lastProcessedMovementInput[i];
        int sent = sendto(server->fds[1].fd, buffer, totalSize, 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        if (sent == SOCKET_ERROR)
        {
            printf("Failed to send wave snapshot to client[%d]: %d\n", i, WSAGetLastError());
        }
        else
        {
            // printf("Sending to Client[%d] Addr:%s:%d, Wave Snapshot\n", i, inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
        }
    }
}
void NetworkSendUnreliableEntitiesSnapshot()
{
    char buffer[MAX_PACKET_SIZE];
    ServerUDPPacketHeader *header = (ServerUDPPacketHeader *)buffer;
    ServerEntityState *entities = (ServerEntityState *)(buffer + sizeof(ServerUDPPacketHeader));

    int count = entitiesAmountToSend;
    for (int i = 0; i < entitiesAmountToSend; i++)
    {
        memcpy(&entities[i], &entitiesToSend[i], sizeof(ServerEntityState));
    }

    int payloadSize = count * sizeof(ServerEntityState);
    int totalSize = sizeof(ServerUDPPacketHeader) + payloadSize;

    if (totalSize > MAX_PACKET_SIZE)
    {
        printf("Snapshot exceeds capacity\n");
        return;
    }
    header->type = PACKET_ENTITY_SNAPSHOT;
    header->size = payloadSize;

    // printf("[NETWORK] Broadcasting EntitiesSnapshot: size:%d, count:%d\n", header->size, count);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        UDPClient *client = &server->udpClients[i];
        if (!client->active)
            continue;
        header->sequence = client->nextSeq++;
        header->lastProcessedBullet = lastProcessedBullet[i];
        header->lastProcessedMovementInput = lastProcessedMovementInput[i];
        int sent = sendto(server->fds[1].fd, buffer, totalSize, 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        if (sent == SOCKET_ERROR)
        {
            printf("Failed to send entities to client[%d]: %d\n", i, WSAGetLastError());
        }
        else
        {
            // printf("Sending to Client[%d] Addr:%s:%d, %d entities\n", i, inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), count);
        }
    }
}

int NetworkPushUnreliableEntityFacingDelta(ServerEntityFacingDelta delta)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ServerUDPPacketHeader);
    const size_t size = sizeof(ServerEntityFacingDelta);
    const size_t need = sizeof(ServerUDPPacketHeader) + size;
    printf("Pushing UnreliableEntityFacingDelta, %d bytes\n", size);

    if (need > capacity || unreliableEventBufferOffset + need > capacity)
        return 1;

    ServerEventHeader eventHeader;
    eventHeader.type = SERVER_DELTA_ENTITY_FACING;
    eventHeader.size = size;
    memcpy(unreliableEventBuffer + unreliableEventBufferOffset, &eventHeader, sizeof(eventHeader));
    unreliableEventBufferOffset += sizeof(eventHeader);

    memcpy(unreliableEventBuffer + unreliableEventBufferOffset, &delta, size);
    unreliableEventBufferOffset += size;
    return 0;
}

void NetworkSendUnreliableEventPacket()
{
    char buffer[MAX_PACKET_SIZE];
    ServerUDPPacketHeader *header = (ServerUDPPacketHeader *)buffer;

    header->type = PACKET_SERVER_EVENTS;
    header->size = unreliableEventBufferOffset;

    if (header->size <= 0)
    {
        // printf("[SERVER] No events to send\n");
        return;
    }
    int totalSize = sizeof(ServerUDPPacketHeader) + header->size;

    char *payload = (char *)(buffer + sizeof(ServerUDPPacketHeader));
    memcpy(payload, unreliableEventBuffer, header->size);

    // printf("[SERVER]: Broadcasting Event Packets, %d bytes\n", eventBufferOffset);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        UDPClient *client = &server->udpClients[i];
        if (!client->active)
            continue;
        header->lastProcessedBullet = lastProcessedBullet[i];
        header->lastProcessedMovementInput = lastProcessedMovementInput[i];
        header->sequence = client->nextSeq++;
        int sent = sendto(server->fds[1].fd, buffer, totalSize, 0, (struct sockaddr *)&client->addr, sizeof(client->addr));
        if (sent == SOCKET_ERROR)
        {
            printf("Client[%d]: Buffer send failed: %d\n", i, WSAGetLastError());
        }
        else if (sent != totalSize)
        {
            printf("Warning: not all bytes sent (%d/%d)\n", sent, totalSize);
        }
        else
        {
            // printf("Sending to Client[%d] Addr:%s:%d, UnreliableEvent\n", i, inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port), count);
        }
    }
    NetworkPrepareUnreliableEventBuffer();
}
