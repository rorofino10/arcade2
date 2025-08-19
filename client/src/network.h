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

void NetworkPrepareBuffer();
void NetworkSendPacket(struct Client *client);
int NetworkPushInputEvent(PACKET_INPUT_EVENT_TYPE type, void *data, uint16_t size);
#endif
