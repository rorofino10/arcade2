#ifndef GAME_H
#define GAME_H

#include "stdint.h"

struct ServerEntityState;
struct ServerWaveSnapshot;
struct ServerEntityDiedEvent;
struct ServerPlayerCanShootEvent;
struct ServerEntityState;
struct ServerEntityFacingDelta;
struct ServerPowerupEvent;

void GameRun();

void GameUpdatePlayerID(uint8_t newPlayerID);
void GameUpdateNetworkEntities(struct ServerEntityState *networkEntity, int count);
void GameUpdateNetworkWave(struct ServerWaveSnapshot *waveSnapshot);

void GameHandleEntityDiedEvent(struct ServerEntityDiedEvent *event);
void GameHandlePlayerCanShootEvent(struct ServerPlayerCanShootEvent *event);
void GameHandleNewEntityEvent(struct ServerEntityState *event);
void GameHandlePowerupEvent(struct ServerPowerupEvent *event);

void GameHandleEntityFacingDelta(struct ServerEntityFacingDelta *delta);

void GameResetClientsideBullets(uint32_t sequence);
void GameReconciliatePlayerPosition(uint32_t serverLastProcessedInput);
#endif
