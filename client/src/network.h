#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"
struct Client;

typedef enum
{
    MOVE
} ACTION;

typedef union
{
    struct
    {
        int8_t x;
        int8_t y;
    } direction;
} MessageData;

typedef struct
{
    uint8_t id;
    uint16_t x, y;
    float dx, dy;
} NetworkEntity;

void NetworkPrepareBuffer();
void NetworkSendPacket(struct Client *client);
void NetworkRecievePacket(struct Client *client);
int NetworkPushInputEvent(PACKET_INPUT_EVENT_TYPE type, void *data, uint16_t size);
#endif
