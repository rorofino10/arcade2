
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"
#include "game.h"

struct Server;

typedef struct
{
    uint8_t id;
    uint16_t x, y;
    float dx, dy;
} NetworkEntity;

void NetworkSetEntities(NetworkEntity *entities, int amount);
void NetworkSendEntities(struct Server *server);

#endif
