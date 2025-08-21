#ifndef GAME_H
#define GAME_H

#include "stdint.h"

struct Client;
struct ServerEntityState;
struct ServerWaveSnapshot;
struct ServerEntityDiedEvent;

void GameRun(struct Client *client);
void GameUpdatePlayerID(uint8_t newPlayerID);
void GameUpdateNetworkEntities(struct ServerEntityState *networkEntity, int count);
void GameUpdateNetworkWave(struct ServerWaveSnapshot *waveSnapshot);
void GameHandleEntityDiedEvent(struct ServerEntityDiedEvent *event);
#endif
