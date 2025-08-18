#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

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

void NetworkSendMoveMessage(struct Client *client, int8_t x, int8_t y);

#endif
