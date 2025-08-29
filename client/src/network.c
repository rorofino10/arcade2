#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "client.h"
#include "network.h"
#include "packet.h"
#include "game.h"

char *eventBuffer = NULL;
size_t eventBufferOffset = 0;

Client *client = NULL;

void NetworkSetClient(Client *clientToSet) { client = clientToSet; }

void NetworkPrepareBuffer()
{
    free(eventBuffer);
    eventBuffer = NULL;
    eventBufferOffset = 0;
}

void NetworkRecieveUDPPacket()
{
    char buffer[MAX_PACKET_SIZE];
    struct sockaddr_in from;
    int fromlen = sizeof(from);
    int recvlen = recvfrom(client->udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&from, &fromlen);
    if (recvlen == SOCKET_ERROR)
    {
        if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
        }
        else
        {
            printf("[UDP] Error receiving, error :%d.\n", WSAGetLastError());
        }
        return;
    }
    ServerUDPPacketHeader *header = (ServerUDPPacketHeader *)buffer;

    char *payload = buffer + sizeof(ServerUDPPacketHeader);
    size_t payload_len = header->size;
    size_t expected_len = payload_len + sizeof(ServerUDPPacketHeader);

    // printf("[UDP] Received %d bytes, size:%d, type:%d, sequence:%d, lastRecvSequence:%d\n", recvlen, payload_len, header->type, header->sequence, client->lastReceivedSequence);

    if (recvlen != expected_len)
    {
        printf("[NETWORK] Invalid recvlen\n");
        return;
    }

    if (header->sequence < client->lastReceivedSequence)
    {
        printf("[NETWORK] Old packet, dropping it...\n");
        return;
    }
    client->lastReceivedSequence = header->sequence;

    // Handle Packet
    switch (header->type)
    {
    case PACKET_ENTITY_SNAPSHOT:
    {
        ServerEntityState *entitiesPacket = (ServerEntityState *)payload;
        int count = payload_len / sizeof(ServerEntityState);
        printf("[UDP] [UDP] EntitySnapshot: Received %d entities from server\n", count);

        GameUpdateNetworkEntities(PACKET_ENTITY_SNAPSHOT, entitiesPacket, count);

        GameReconciliatePlayerPosition(header->lastProcessedMovementInput);
        GameResetClientsideBullets(header->lastProcessedBullet);
    }
    break;
    case PACKET_ENTITY_DELTAS:
    {
        ServerEntityState *entitiesPacket = (ServerEntityState *)payload;
        int count = payload_len / sizeof(ServerEntityState);
        printf("[UDP] EntityDeltas: Received %d entities from server\n", count);
        GameUpdateNetworkEntities(PACKET_ENTITY_DELTAS, entitiesPacket, count);

        GameReconciliatePlayerPosition(header->lastProcessedMovementInput);
        GameResetClientsideBullets(header->lastProcessedBullet);
    }
    break;
    case PACKET_WAVE_SNAPSHOT:
        ServerWaveSnapshot *waveSnapshot = (ServerWaveSnapshot *)payload;
        GameUpdateNetworkWave(waveSnapshot);
        break;
    case PACKET_SERVER_EVENTS:
        size_t offset = 0;
        GameResetClientsideBullets(header->lastProcessedBullet);
        printf("[UDP] Received %d bytes, size:%d, type:%d, expected:%d\n", recvlen, payload_len, header->type, expected_len);
        printf("[UDP] Received UnreliableEventPacket from Server\n");

        while (offset < header->size)
        {
            ServerEventHeader *eheader = (ServerEventHeader *)(payload + offset);
            offset += sizeof(ServerEventHeader);

            char *edata = payload + offset;
            offset += eheader->size;
            printf("[UDP] Parsing UnreliableEventPacket type:%d, size:%d\n", eheader->type, eheader->size);

            switch (eheader->type)
            {
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
        break;
    }
}

void NetworkRecievePacket()
{
    ServerPacketHeader header;
    int recvlen;
    recvlen = recv(client->socket, (char *)&header, sizeof(header), 0);
    if (recvlen <= 0)
        return; // disconnect or error

    printf("[TCP] Received %d bytes, header.size: %d, header.type: %d\n", recvlen, header.size, header.type);
    char *buffer = malloc(header.size);
    recvlen = recv(client->socket, buffer, header.size, 0);
    if (recvlen <= 0)
        return;

    switch (header.type)
    {
    case PACKET_ENTITY_SNAPSHOT:
    {
        ServerEntityState *entitiesPacket = (ServerEntityState *)buffer;
        int count = header.size / sizeof(ServerEntityState);
        printf("[TCP] Received %d entities from server\n", count);
        GameUpdateNetworkEntities(PACKET_ENTITY_SNAPSHOT, entitiesPacket, count);

        GameReconciliatePlayerPosition(header.lastProcessedMovementInput);
        GameResetClientsideBullets(header.lastProcessedBullet);
    }
    break;
    case PACKET_ENTITY_DELTAS:
    {
        ServerEntityState *entitiesPacket = (ServerEntityState *)buffer;
        int count = header.size / sizeof(ServerEntityState);
        // printf("Received %d entities from server\n", count);p
        GameUpdateNetworkEntities(PACKET_ENTITY_DELTAS, entitiesPacket, count);

        GameReconciliatePlayerPosition(header.lastProcessedMovementInput);
        GameResetClientsideBullets(header.lastProcessedBullet);
    }
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
        // GameResetClientsideBullets(header.lastProcessedBullet);

        while (offset < header.size)
        {
            // printf("OFFSET: %d\n", offset);
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
            case SERVER_EVENT_POWERUP:
            {
                ServerPowerupEvent *event = (ServerPowerupEvent *)edata;
                printf("[SERVER]: PowerupEvent\n");
                GameHandlePowerupEvent(event);
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
    free(buffer);
}

void NetworkSendPacket()
{

    if (eventBufferOffset <= 0)
        return;
    int totalSize = sizeof(ClientPacketHeader) + eventBufferOffset;

    char *buffer = malloc(totalSize);
    ClientPacketHeader *header = (ClientPacketHeader *)buffer;
    char *payload = (char *)(buffer + sizeof(ClientPacketHeader));

    header->type = PACKET_INPUT_EVENT;
    header->size = eventBufferOffset;

    memcpy(payload, eventBuffer, header->size);

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
               header->type, header->size, totalSize);
    }
    free(buffer);
    NetworkPrepareBuffer();
}

// Not used
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

    eventBuffer = realloc(eventBuffer, eventBufferOffset + need);

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

    eventBuffer = realloc(eventBuffer, eventBufferOffset + need);

    ClientEventHeader eventHeader;
    eventHeader.type = CLIENT_EVENT_INPUT_SHOOT;
    eventHeader.size = size;

    memcpy(eventBuffer + eventBufferOffset, &eventHeader, sizeof(eventHeader));
    eventBufferOffset += sizeof(eventHeader);

    memcpy(eventBuffer + eventBufferOffset, &event, size);
    eventBufferOffset += size;
    return 0;
}

void NetworkSendUnreliableHeartbeat()
{
    char buffer[MAX_PACKET_SIZE];
    ClientPacketHeader *header = (ClientPacketHeader *)buffer;
    header->type = PACKET_HEARTBEAT;
    header->size = 0;

    int totalSize = sizeof(ClientPacketHeader) + header->size;
    int sent = sendto(client->udpSocket, (char *)&buffer, totalSize, 0, (struct sockaddr *)&client->serveraddr, sizeof(client->serveraddr));

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
               header->type, header->size, totalSize);
    }
}

int NetworkTryConnect()
{
    int iResult = connect(client->socket, client->clientaddrinfo->ai_addr, (int)client->clientaddrinfo->ai_addrlen);
    if (iResult == SOCKET_ERROR || client->socket == INVALID_SOCKET)
    {
        Sleep(100); // Sleep isn't the best but oh well.
        printf("Unable to connect to server!\n");
        return FALSE;
    }
    freeaddrinfo(client->clientaddrinfo);

    u_long mode = 1;
    ioctlsocket(client->socket, FIONBIO, &mode);
    u_long udpmode = 1;
    ioctlsocket(client->udpSocket, FIONBIO, &udpmode);

    NetworkSendUnreliableHeartbeat();

    printf("Connected to %s:%s\n", DEFAULT_HOST, DEFAULT_PORT);
    return TRUE;
}
