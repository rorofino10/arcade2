#ifndef GAME_H
#define GAME_H

#define MAX_PLAYERS 2

#include "stdint.h"

struct ClientInputEvent;

extern uint32_t lastProcessedBullet[MAX_PLAYERS];
extern uint32_t lastProcessedMovementInput[MAX_PLAYERS];
void GameInit();
void GameUpdate(double delta);
void GameUnload();
uint8_t GameAssignPlayerToClient(int clientIndex);
void UpdatePlayerPosition(int clientIndex, int16_t nx, int16_t ny);
void GameApplyPlayerMovementInput(int clientIndex, struct ClientInputEvent *input);

void GameUpdateNetworkEntities(int type);
void GameUpdateNetworkWave();

void GamePushEntityDeltas();
void ShootBulletInput(int clientIndex, float dx, float dy, uint32_t sequence);
#endif
