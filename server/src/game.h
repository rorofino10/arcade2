#ifndef GAME_H
#define GAME_H

#define MAX_PLAYERS 2

extern uint8_t lastReceivedSequence[MAX_PLAYERS];
void GameInit();
void GameUpdate(double delta);
void GameUnload();
uint8_t GameAssignPlayerToClient(int clientIndex);
void UpdatePlayerPosition(int clientIndex, int16_t nx, int16_t ny);
void ShootBulletInput(int clientIndex, float dx, float dy, uint32_t sequence);
#endif
