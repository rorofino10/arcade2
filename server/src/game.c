#include "raylib.h"
#include "raymath.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"

#include "game.h"
#include "network.h"

typedef uint8_t EntityID;
typedef float _float32_t;
typedef float _float16_t;
typedef _float32_t COOLDOWN;
typedef _float32_t SPEED;
typedef _float32_t LIFETIME;

const _Float16 BLUE_ENEMY_BULLETS = 6.0f;
const _Float16 FULL_CIRCLE = 360.0f;
const SPEED DEFAULT_PLAYER_SPEED = 100.0f;
const SPEED DEFAULT_ENEMY_SPEED = 40.0f;
const SPEED DEFAULT_BULLET_SPEED = 500.0f;
const COOLDOWN DEFAULT_SHOOTING_COOLDOWN = 0.5f;
const LIFETIME DEFAULT_BULLET_LIFETIME = 2.0f;
const LIFETIME DEFAULT_POWERUP_SPEED_LIFETIME = 5.0f;
const LIFETIME DEFAULT_POWERUP_SHOOTING_LIFETIME = 2.0f;
const Vector2 DEFAULT_ENTITY_SIZE = (Vector2){50, 50};
const Vector2 DEFAULT_ENTITY_FACING = (Vector2){1, 0};
const Vector2 DEFAULT_BULLET_SIZE = (Vector2){11, 5};

const uint16_t worldSize = 1000;

const SPEED DEFAULT_REDENEMIES_SPEED_INCREASE = 10;
const EntityID DEFAULT_REDENEMIES_INCREASE = 3;
const EntityID DEFAULT_BLUEENEMIES_INCREASE = 1;
const COOLDOWN DEFAULT_WAVE_REFRESH_COOLDOWN = 2.0f;
const COOLDOWN DEFAULT_SPAWN_INTERVAL = 0.5f;

typedef struct WaveManager
{
    uint8_t wave;
    bool isWaveInProgress;
    COOLDOWN waveRefreshCooldownRemaining;
    COOLDOWN waveRefreshCooldown;

    // Spawn Timings
    COOLDOWN spawnCooldown;
    COOLDOWN spawnCooldownRemaining;

    // Red Enemies
    SPEED redEnemiesSpeedIncrease;
    SPEED redEnemiesSpeed;
    EntityID redEnemiesIncrease;
    EntityID redEnemiesToSpawn;
    EntityID redEnemiesLeftToSpawn;
    EntityID redEnemiesRemaining;

    // Blue Enemies
    EntityID blueEnemiesIncrease;
    EntityID blueEnemiesToSpawn;
    EntityID blueEnemiesLeftToSpawn;
    EntityID blueEnemiesRemaining;

} WaveManager;

WaveManager waveManager = {
    .wave = 1,
    .isWaveInProgress = false,
    .waveRefreshCooldown = DEFAULT_WAVE_REFRESH_COOLDOWN,
    .waveRefreshCooldownRemaining = DEFAULT_WAVE_REFRESH_COOLDOWN,

    // Spawn Timings
    .spawnCooldown = DEFAULT_SPAWN_INTERVAL,
    .spawnCooldownRemaining = 0.0f,

    // Red Enemies
    .redEnemiesIncrease = DEFAULT_REDENEMIES_INCREASE,
    .redEnemiesSpeedIncrease = DEFAULT_REDENEMIES_SPEED_INCREASE,
    .redEnemiesSpeed = DEFAULT_ENEMY_SPEED,
    .redEnemiesToSpawn = 0,
    .redEnemiesLeftToSpawn = 0,
    .redEnemiesRemaining = 0,

    // Blue Enemies
    .blueEnemiesIncrease = DEFAULT_BLUEENEMIES_INCREASE,
    .blueEnemiesToSpawn = 0,
    .blueEnemiesLeftToSpawn = 0,
    .blueEnemiesRemaining = 0,
};

const uint16_t PLAYER_SAFE_RADIUS = 100;
const uint16_t ENTITY_SPAWN_RADIUS = 300;
const _float32_t NEUTRAL_SHOOT_RADIUS = 80.0f;

typedef enum GameState
{
    PLAYING,
    LOST,
    PAUSED,
} GameState;

typedef enum
{
    ENTITY_PLAYER = 0,
    ENTITY_RED_ENEMY,
    ENTITY_BLUE_ENEMY,
    ENTITY_BULLET,
    ENTITY_POWERUP_SPEED,
    ENTITY_POWERUP_SHOOTING,
    ENTITY_EXPLOSION,
    ENTITY_NEUTRAL,
    // Count
    ENTITY_TYPE_COUNT
} EntityType;

typedef struct
{
    EntityID id;
    Vector2 position;
    Vector2 direction;
    Vector2 facing;
    Vector2 size;
    EntityType type;
    EntityID parent;
    SPEED speed;
    COOLDOWN shootingCooldown;
    COOLDOWN shootingCooldownRemaining;
    LIFETIME lifetime;
    LIFETIME powerupSpeedLifetime;
    LIFETIME powerupShootingLifetime;
    bool isAlive;
    bool isPowerupSpeedActive;
    bool isPowerupShootingActive;
    bool dirty;
} Entity;

const EntityID entitiesCount = UINT8_MAX - 1;
GameState gameState = PLAYING;
EntityID playerID = 0;
Entity *entities = NULL;

#define FOR_EACH_ALIVE_ENTITY(i)                 \
    for (EntityID i = 0; i < entitiesCount; i++) \
        if (!entities[i].isAlive)                \
            continue;                            \
        else

Vector2 RandomSpawnPosition(Vector2 playerPos)
{
    Vector2 pos;
    float distance;
    float angle = ((float)GetRandomValue(0, 360)) * DEG2RAD;

    distance = (float)GetRandomValue((int)PLAYER_SAFE_RADIUS, ENTITY_SPAWN_RADIUS);

    pos.x = playerPos.x + cosf(angle) * distance;
    pos.y = playerPos.y + sinf(angle) * distance;
    return pos;
}

EntityID SpawnEntity(Vector2 position, Vector2 size, SPEED speed, EntityType type)
{
    for (EntityID id = 0; id < entitiesCount; id++)
    {
        if (entities[id].isAlive)
            continue;

        entities[id] = (Entity){
            .id = id,
            .position = position,
            .direction = Vector2Zero(),
            .facing = DEFAULT_ENTITY_FACING,
            .size = size,
            .speed = speed,
            .type = type,
            .parent = id,
            .isAlive = true,
            .lifetime = 1.0f,
            .dirty = true,
        };
        return id;
    }
    return -1;
}

EntityID SpawnPlayer()
{
    playerID = SpawnEntity(Vector2Zero(), DEFAULT_ENTITY_SIZE, DEFAULT_PLAYER_SPEED, ENTITY_PLAYER);
    entities[playerID].shootingCooldownRemaining = 0.0f;
    entities[playerID].shootingCooldown = DEFAULT_SHOOTING_COOLDOWN;
    entities[playerID].powerupShootingLifetime = 0.0f;
    entities[playerID].powerupSpeedLifetime = 0.0f;
    entities[playerID].isPowerupSpeedActive = false;
    entities[playerID].isPowerupShootingActive = false;
    return playerID;
}
EntityID SpawnRedEnemy()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID enemyId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, waveManager.redEnemiesSpeed, ENTITY_RED_ENEMY);
    waveManager.spawnCooldownRemaining = waveManager.spawnCooldown;
    waveManager.redEnemiesLeftToSpawn--;
    waveManager.redEnemiesRemaining++;
    return enemyId;
}
EntityID SpawnNeutral()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID neutralId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_NEUTRAL);
    entities[neutralId].shootingCooldownRemaining = DEFAULT_SHOOTING_COOLDOWN;
    entities[neutralId].shootingCooldown = DEFAULT_SHOOTING_COOLDOWN * 3;
    return neutralId;
}
EntityID SpawnBlueEnemy()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID enemyId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_BLUE_ENEMY);
    waveManager.spawnCooldownRemaining = waveManager.spawnCooldown;
    waveManager.blueEnemiesLeftToSpawn--;
    waveManager.blueEnemiesRemaining++;
    return enemyId;
}
EntityID SpawnPowerupSpeed()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID powerupId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_POWERUP_SPEED);

    return powerupId;
}
EntityID SpawnPowerupShooting()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID powerupId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_POWERUP_SHOOTING);
    return powerupId;
}
EntityID SpawnExplosion()
{
    EntityID explosionId = SpawnEntity(Vector2Zero(), DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_EXPLOSION);
    return explosionId;
}
EntityID SpawnBullet()
{
    EntityID bulletId = SpawnEntity(Vector2Zero(), DEFAULT_BULLET_SIZE, DEFAULT_BULLET_SPEED, ENTITY_BULLET);
    entities[bulletId].lifetime = DEFAULT_BULLET_LIFETIME;
    return bulletId;
}

void ShootBullet(EntityID from, Vector2 direction)
{
    EntityID bulletId = SpawnBullet();
    entities[bulletId].position = entities[from].position;
    entities[bulletId].parent = from;
    entities[bulletId].direction = direction;
    entities[bulletId].facing = direction;
}
void EntityShootBullet(EntityID entity)
{
    if (entities[entity].shootingCooldownRemaining <= 0.0f)
    {
        entities[entity].shootingCooldownRemaining = entities[entity].shootingCooldown;
        ShootBullet(entity, entities[entity].facing);
    }
}

void ShootBulletInput(int clientIndex, float dx, float dy)
{
    if (entities[playerID].shootingCooldownRemaining <= 0.0f)
    {
        entities[playerID].shootingCooldownRemaining = entities[playerID].shootingCooldown;
        ShootBullet(playerID, (Vector2){dx, dy});
    }
}

Rectangle MakeRectangleFromCenter(Vector2 center, Vector2 size)
{
    Rectangle rect;
    rect.x = center.x - size.x / 2.0f;
    rect.y = center.y - size.y / 2.0f;
    rect.width = size.x;
    rect.height = size.y;
    return rect;
}

void GenerateWave()
{
    SpawnPowerupShooting();
    SpawnPowerupSpeed();
    waveManager.isWaveInProgress = true;
    waveManager.waveRefreshCooldownRemaining = waveManager.waveRefreshCooldown;

    waveManager.redEnemiesToSpawn += waveManager.redEnemiesIncrease;
    waveManager.redEnemiesLeftToSpawn = waveManager.redEnemiesToSpawn;
    waveManager.redEnemiesSpeed += waveManager.redEnemiesSpeedIncrease;

    waveManager.blueEnemiesToSpawn += waveManager.blueEnemiesIncrease;
    waveManager.blueEnemiesLeftToSpawn = waveManager.blueEnemiesToSpawn;
}

void CheckWaveEnded()
{
    if (waveManager.redEnemiesRemaining == 0 && waveManager.blueEnemiesRemaining == 0)
    {
        waveManager.wave++;
        waveManager.isWaveInProgress = false;
    }
}

void ApplyPowerupSpeed(EntityID entityId, EntityID powerupId)
{
    entities[entityId].speed *= 3.0f;
    entities[entityId].powerupSpeedLifetime = DEFAULT_POWERUP_SPEED_LIFETIME;
    entities[entityId].isPowerupSpeedActive = true;
    entities[powerupId].isAlive = false;
}

void ApplyPowerupShooting(EntityID entityId, EntityID powerupId)
{
    entities[entityId].shootingCooldown /= 3.0f;
    entities[entityId].powerupShootingLifetime = DEFAULT_POWERUP_SHOOTING_LIFETIME;
    entities[entityId].isPowerupShootingActive = true;
    entities[powerupId].isAlive = false;
}

void handleBlueEnemyDeath(EntityID blueEnemyId)
{
    for (_Float16 angleDeg = 0; angleDeg < FULL_CIRCLE; angleDeg += FULL_CIRCLE / BLUE_ENEMY_BULLETS)
    {
        _Float16 angleRad = angleDeg * DEG2RAD;

        Vector2 dir = {cosf(angleRad), sinf(angleRad)};

        ShootBullet(blueEnemyId, dir);
    }
    waveManager.blueEnemiesRemaining--;
}
void handleRedEnemyDeath(EntityID entityId)
{
    waveManager.redEnemiesRemaining--;
}

void handlePlayerDeath(EntityID playerId)
{
    gameState = LOST;
    printf("Player died\n");
}

void KillEntity(EntityID entityId)
{
    if (!entities[entityId].isAlive)
        return;
    switch (entities[entityId].type)
    {
    case ENTITY_BLUE_ENEMY:
        handleBlueEnemyDeath(entityId);
        CheckWaveEnded();
        break;

    case ENTITY_RED_ENEMY:
        handleRedEnemyDeath(entityId);
        CheckWaveEnded();
        break;
    case ENTITY_PLAYER:
        handlePlayerDeath(entityId);
        break;
    default:
        break;
    }
    EntityID explosionId = SpawnExplosion();
    entities[explosionId].position = entities[entityId].position;
    entities[entityId].isAlive = false;
}

#define SetCollision(entityA, entityB) if ((entities[idA].type == entityA && entities[idB].type == entityB))
void handleCollisionBetween(EntityID idA, EntityID idB)
{
    if (entities[idA].parent == idB)
        return;

    Rectangle recA = MakeRectangleFromCenter(entities[idA].position, entities[idA].size);
    Rectangle recB = MakeRectangleFromCenter(entities[idB].position, entities[idB].size);
    if (!CheckCollisionRecs(recA, recB))
        return;
    SetCollision(ENTITY_BULLET, ENTITY_RED_ENEMY)
    {
        KillEntity(idB);
        return;
    }
    SetCollision(ENTITY_BULLET, ENTITY_BLUE_ENEMY)
    {
        KillEntity(idB);
        return;
    }
    SetCollision(ENTITY_BULLET, ENTITY_NEUTRAL)
    {
        KillEntity(idB);
        return;
    }
    // SetCollision(ENTITY_BULLET, ENTITY_PLAYER)
    // {
    //     KillEntity(idB);
    //     return;
    // }
    // SetCollision(ENTITY_PLAYER, ENTITY_RED_ENEMY)
    // {
    //     KillEntity(idA);
    //     return;
    // }
    SetCollision(ENTITY_PLAYER, ENTITY_POWERUP_SPEED)
    {
        ApplyPowerupSpeed(idA, idB);
        return;
    }
    SetCollision(ENTITY_PLAYER, ENTITY_POWERUP_SHOOTING)
    {
        ApplyPowerupShooting(idA, idB);
        return;
    }
}

void HandleCollisions()
{
    FOR_EACH_ALIVE_ENTITY(idA)
    FOR_EACH_ALIVE_ENTITY(idB)
    handleCollisionBetween(idA, idB);
}

void KillAllEntities()
{

    for (EntityID entityId = 0; entityId < entitiesCount; entityId++)
    {
        entities[entityId].isAlive = false;
    }
    waveManager.redEnemiesRemaining = 0;
    waveManager.blueEnemiesRemaining = 0;
}

void RestartWave()
{
    waveManager.wave = 1;
    waveManager.blueEnemiesToSpawn = 0;
    waveManager.redEnemiesToSpawn = 0;
}

void StartGame()
{
    SpawnPlayer();
    // SpawnNeutral();
    GenerateWave();
}

void RestartGame()
{
    KillAllEntities();
    RestartWave();
    StartGame();
}

void UpdateNetworkEntities()
{
    int entitiesAmountToSend = 0;
    NetworkEntity entitiesToSend[entitiesCount];
    FOR_EACH_ALIVE_ENTITY(id)
    {
        entitiesToSend[entitiesAmountToSend].id = entities[id].id;
        entitiesToSend[entitiesAmountToSend].x = entities[id].position.x;
        entitiesToSend[entitiesAmountToSend].y = entities[id].position.y;
        entitiesToSend[entitiesAmountToSend].dx = entities[id].direction.x;
        entitiesToSend[entitiesAmountToSend].dy = entities[id].direction.y;
        entitiesAmountToSend++;
    }
    NetworkSetEntities(entitiesToSend, entitiesAmountToSend);
}

void UpdatePlayerPosition(int clientIndex, int16_t nx, int16_t ny)
{
    entities[playerID].position = (Vector2){.x = (float)nx, .y = (float)ny};
}

void UpdateWave(double delta)
{
    if (!waveManager.isWaveInProgress && waveManager.waveRefreshCooldownRemaining > 0.0f)
        waveManager.waveRefreshCooldownRemaining -= delta;
    if (!waveManager.isWaveInProgress && waveManager.waveRefreshCooldownRemaining <= 0.0f)
        GenerateWave();
    if (waveManager.isWaveInProgress)
    {
        if (waveManager.redEnemiesLeftToSpawn != 0 || waveManager.blueEnemiesLeftToSpawn != 0)
        {
            if (waveManager.spawnCooldownRemaining > 0.0f)
                waveManager.spawnCooldownRemaining -= delta;
            if (waveManager.spawnCooldownRemaining <= 0.0f && waveManager.redEnemiesLeftToSpawn != 0)
            {
                SpawnRedEnemy();
            }
            if (waveManager.spawnCooldownRemaining <= 0.0f && waveManager.blueEnemiesLeftToSpawn != 0)
            {
                SpawnBlueEnemy();
            }
        }
    }
}
void GameUpdate(double delta)
{
    FOR_EACH_ALIVE_ENTITY(i)
    {
        Vector2 newPos = Vector2Add(entities[i].position, Vector2Scale(entities[i].direction, entities[i].speed * delta));
        if (Vector2Distance(newPos, Vector2Zero()) <= worldSize)
            entities[i].position = newPos;
        if (entities[i].lifetime < 0.0f)
            entities[i].isAlive = false;
        switch (entities[i].type)
        {
        case ENTITY_RED_ENEMY:
            Vector2 distance = Vector2Normalize(Vector2Subtract(entities[playerID].position, entities[i].position));
            entities[i].direction = distance;
            entities[i].facing = distance;
            break;
        case ENTITY_BULLET:
            entities[i].lifetime -= delta;
            break;
        case ENTITY_EXPLOSION:
            entities[i].lifetime -= delta;
            break;
        case ENTITY_PLAYER:
            if (entities[i].shootingCooldownRemaining > 0.0f)
                entities[i].shootingCooldownRemaining -= delta;
            if (entities[i].powerupSpeedLifetime > 0.0f)
                entities[i].powerupSpeedLifetime -= delta;
            if (entities[i].isPowerupSpeedActive && entities[i].powerupSpeedLifetime <= 0.0f)
            {
                entities[i].isPowerupSpeedActive = false;
                entities[i].speed = DEFAULT_PLAYER_SPEED;
            }
            if (entities[i].powerupShootingLifetime > 0.0f)
                entities[i].powerupShootingLifetime -= delta;
            if (entities[i].isPowerupShootingActive && entities[i].powerupShootingLifetime <= 0.0f)
            {
                entities[i].powerupShootingLifetime = false;
                entities[i].shootingCooldown = DEFAULT_SHOOTING_COOLDOWN;
            }
            break;
        case ENTITY_NEUTRAL:
            if (entities[i].shootingCooldownRemaining > 0.0f)
                entities[i].shootingCooldownRemaining -= delta;
            FOR_EACH_ALIVE_ENTITY(target)
            {
                if (entities[target].type != ENTITY_RED_ENEMY)
                    continue;
                if (Vector2Distance(entities[i].position, entities[target].position) < NEUTRAL_SHOOT_RADIUS)
                {
                    Vector2 neutralFacing = Vector2Normalize(Vector2Subtract(entities[target].position, entities[i].position));

                    entities[i].facing = neutralFacing;
                    EntityShootBullet(i);
                }
            }
            break;
        default:
            break;
        }
    }
    HandleCollisions();
    UpdateWave(delta);
    UpdateNetworkEntities();
}

void GameInit()
{
    entities = calloc(entitiesCount, sizeof(Entity));
    StartGame();
}

void GameUnload()
{
    free(entities);
}
