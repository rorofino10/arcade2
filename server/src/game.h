#ifndef GAME_H
#define GAME_H

void GameInit();
void GameUpdate(double delta);
void GameUnload();
uint8_t GameAssignPlayerToClient(int clientIndex);
void UpdatePlayerPosition(int clientIndex, int16_t nx, int16_t ny);
void ShootBulletInput(int clientIndex, float dx, float dy);
#endif
