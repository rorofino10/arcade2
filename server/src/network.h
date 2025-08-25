
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"
#include "game.h"

struct Server;

void NetworkSetEntities(ServerEntityState *entities, int amount);
void NetworkSetWaveState(ServerWaveSnapshot waveState);
void NetworkSendEntitiesSnapshot(struct Server *server);
void NetworkSendWaveSnapshot(struct Server *server);
void NetworkSendAssignedPlayerID(struct Server *server, int clientIndex, uint8_t playerID);

void NetworkPrepareEventBuffer();
int NetworkPushEntityDiedEvent(struct ServerEntityDiedEvent event);
int NetworkPushPlayerCanShootEvent(struct ServerPlayerCanShootEvent event);
int NetworkPushNewEntityEvent(ServerEntityState entity);
int NetworkPushBulletSpawnEvent(ServerBulletSpawnEvent event);
void NetworkSendEventPacket(struct Server *server);
#endif
