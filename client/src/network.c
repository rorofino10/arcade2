#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "client.h"
#include "network.h"
#include "packet.h"
#include "game.h"

char eventBuffer[MAX_PACKET_SIZE];
size_t eventBufferOffset = 0;

void NetworkPrepareBuffer() { eventBufferOffset = 0; }

void NetworkRecievePacket(Client *client)
{
    ServerPacketHeader header;
    int recvlen;
    recvlen = recv(client->socket, (char *)&header, sizeof(header), 0);
    if (recvlen <= 0)
        return; // disconnect or error

    // printf("Received %d bytes, header.size: %d, header.type: %d\n", recvlen, header.size, header.type);
    char buffer[MAX_PACKET_SIZE];
    recvlen = recv(client->socket, buffer, header.size, 0);
    if (recvlen <= 0)
        return;

    switch (header.type)
    {
    case PACKET_ENTITY_SNAPSHOT:
        ServerEntityState *entitiesPacket = (ServerEntityState *)buffer;
        int count = header.size / sizeof(ServerEntityState);
        // printf("Received %d entities from server\n", count);
        GameUpdateNetworkEntities(entitiesPacket, count);

        GameReconciliatePlayerPosition(header.lastProcessedMovementInput);
        GameResetClientsideBullets(header.lastProcessedBullet);
        break;
    case PACKET_WAVE_SNAPSHOT:
        ServerWaveSnapshot *waveSnapshot = (ServerWaveSnapshot *)buffer;
        // printf("Received Wave Snapshot, wave:%d\n", waveSnapshot->wave);
        GameUpdateNetworkWave(waveSnapshot);
        break;
    case PACKET_ASSIGN_PLAYER:
        ServerAssignPlayerMessage *assignPlayerMessage = (ServerAssignPlayerMessage *)buffer;
        // printf("Received Assign Player Message: %d\n\n\n\n\n\n\n\n\n", assignPlayerMessage->playerID);
        GameUpdatePlayerID(assignPlayerMessage->playerID);
        break;
    case PACKET_SERVER_EVENTS:
        size_t offset = 0;
        printf("[NETWORK]: SERVER_EVENT, Last Sequence: %d\n", header.lastProcessedBullet);
        while (offset < header.size)
        {
            // printf("OFFSET: %d\n", offset);
            GameResetClientsideBullets(header.lastProcessedBullet);
            ServerEventHeader *eheader = (ServerEventHeader *)(buffer + offset);
            offset += sizeof(ServerEventHeader);

            // printf("OFFSET DATA: %d\n", offset);

            char *edata = buffer + offset;
            offset += eheader->size;
            // printf("EVENT TYPE: %d, SIZE:%d\n", eheader->type, eheader->size);
            switch (eheader->type)
            {
            case SERVER_EVENT_ENTITY_DIED:
            {
                ServerEntityDiedEvent *event = (ServerEntityDiedEvent *)edata;
                printf("[SERVER]: EntityDiedEvent id:[%d] at (%d,%d)\n", event->id, event->deathPosX, event->deathPosY);
                GameHandleEntityDiedEvent(event);
            }
            break;
            case SERVER_EVENT_PLAYER_CAN_SHOOT:
            {
                ServerPlayerCanShootEvent *event = (ServerPlayerCanShootEvent *)edata;
                printf("[SERVER]: PlayerCanShootEvent id:[%d]\n", event->id);
                GameHandlePlayerCanShootEvent(event);
            }
            break;
            case SERVER_EVENT_NEW_ENTITY:
            {
                ServerEntityState *entity = (ServerEntityState *)edata;
                printf("[SERVER]: NewEntityEvent id:[%d], dx:%f, dy:%f\n", entity->id, entity->vx, entity->vy);
                GameHandleNewEntityEvent(entity);
            }
            break;
            case SERVER_DELTA_ENTITY_FACING:
            {
                ServerEntityFacingDelta *delta = (ServerEntityFacingDelta *)edata;
                printf("[SERVER]: EntityFacingDelta id:[%d], fx:%f, fy:%f\n", delta->id, delta->fx, delta->fy);
                GameHandleEntityFacingDelta(delta);
            }
            break;
            default:
                break;
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
    header.size = eventBufferOffset;

    if (header.size <= 0)
        return;

    int totalSize = sizeof(ClientPacketHeader) + header.size;

    char *buffer = (char *)malloc(totalSize);
    if (!buffer)
    {
        printf("Failed to allocate packet buffer\n");
        return;
    }

    // copy header and payload into buffer
    memcpy(buffer, &header, sizeof(ClientPacketHeader));
    memcpy(buffer + sizeof(ClientPacketHeader), eventBuffer, header.size);

    // send everything in one go
    int sent = send(client->socket, buffer, totalSize, 0);
    if (sent == SOCKET_ERROR)
    {
        printf("Packet send failed: %d\n", WSAGetLastError());
    }
    else if (sent != totalSize)
    {
        printf("Warning: not all bytes sent (%d/%d)\n", sent, totalSize);
    }
    else
    {
        printf("[NETWORK] Sent Packet: type=%d, payload=%d, total=%d\n",
               header.type, header.size, totalSize);
    }

    free(buffer);
}

int NetworkPushInputAuthorativeMoveEvent(ClientInputAuthorativeMoveEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ClientPacketHeader);
    const size_t size = sizeof(ClientInputAuthorativeMoveEvent);
    const size_t need = sizeof(ClientPacketHeader) + size;
    printf("Pushing ClientInputAuthorativeMoveEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ClientEventHeader eventHeader;
    eventHeader.type = CLIENT_EVENT_INPUT_AUTHORATIVE_MOVE;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

int NetworkPushInputMoveEvent(ClientInputEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ClientPacketHeader);
    const size_t size = sizeof(ClientInputEvent);
    const size_t need = sizeof(ClientPacketHeader) + size;
    printf("Pushing ClientInputMoveEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ClientEventHeader eventHeader;
    eventHeader.type = CLIENT_EVENT_INPUT_MOVE;
    eventHeader.size = size;
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

int NetworkPushInputShootEvent(ClientInputShootEvent event)
{
    const size_t capacity = MAX_PACKET_SIZE - sizeof(ClientPacketHeader);
    const size_t size = sizeof(ClientInputShootEvent);
    const size_t need = sizeof(ClientPacketHeader) + size;
    printf("Pushing ClientInputShootEvent, %d bytes\n", size);

    if (need > capacity || eventBufferOffset + need > capacity)
        return 1;

    ClientEventHeader eventHeader;
    eventHeader.type = CLIENT_EVENT_INPUT_SHOOT;
    eventHeader.size = size;
    printf("[NETWORK]: Pushing ClientEventHeader type: %d, size: %d\n", eventHeader.type, eventHeader.size);
    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    printf("[NETWORK]: Pushing ClientInputShootEvent dx: %f, dy: %f, sequence: %d\n", event.dx, event.dy, event.bulletSequence);
    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}
