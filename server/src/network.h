
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"
#include "game.h"

struct Server;

void NetworkSetServer(struct Server *);

void NetworkSetEntities(ServerEntityState *entities, int amount);
void NetworkSetWaveState(ServerWaveSnapshot waveState);
void NetworkSendEntitiesSnapshot();
void NetworkSendWaveSnapshot();
void NetworkSendAssignedPlayerID(int clientIndex, uint8_t playerID);

void NetworkSendUnreliableEntitiesSnapshot();
void NetworkSendUnreliableEntitiesDeltas();
void NetworkSendUnreliableWaveSnapshot();
void NetworkSendUnreliableEventPacket();
int NetworkPushUnreliableEntityFacingDelta(ServerEntityFacingDelta delta);

int NetworkPushEntityDiedEvent(ServerEntityDiedEvent event);
int NetworkPushPlayerCanShootEvent(ServerPlayerCanShootEvent event);
int NetworkPushNewEntityEvent(ServerEntityState entity);
int NetworkPushBulletSpawnEvent(ServerBulletSpawnEvent event);
int NetworkPushEntityFacingDelta(ServerEntityFacingDelta delta);
int NetworkPushPowerupEvent(ServerPowerupEvent event);

void NetworkPrepareEventBuffer();
void NetworkSendEventPacket();
#endif
