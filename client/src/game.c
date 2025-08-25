#include "raylib.h"
#include "raymath.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "game.h"
#include "network.h"
#include "packet.h"

typedef uint8_t EntityID;
typedef float _float32_t;
typedef float _float16_t;
typedef _float32_t COOLDOWN;
typedef _float32_t SPEED;
typedef _float32_t LIFETIME;

const uint16_t screenWidth = 800;
const uint16_t screenHeight = 600;
const char *gameTitle = "Game Title";

const _Float16 BLUE_ENEMY_BULLETS = 6.0f;
const _Float16 FULL_CIRCLE = 360.0f;
const SPEED DEFAULT_BULLET_SPEED = 500.0f;
const LIFETIME DEFAULT_BULLET_LIFETIME = 2.0f;
const Vector2 DEFAULT_ENTITY_SIZE = (Vector2){50, 50};
const Vector2 DEFAULT_ENTITY_FACING = (Vector2){1, 0};
const Vector2 DEFAULT_BULLET_SIZE = (Vector2){11, 5};

const uint16_t worldSize = 1000;

const SPEED DEFAULT_REDENEMIES_SPEED_INCREASE = 10;
const EntityID DEFAULT_REDENEMIES_INCREASE = 3;
const EntityID DEFAULT_BLUEENEMIES_INCREASE = 1;
const COOLDOWN DEFAULT_WAVE_REFRESH_COOLDOWN = 2.0f;
const COOLDOWN DEFAULT_SPAWN_INTERVAL = 0.5f;

const double SnapshotTPS = 30.0f;
const double timeBetweenSnapshotTicks = 1.0 / SnapshotTPS;
double elapsedTimeBetweenSnapshotTicks = 0.0f;

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
    .isWaveInProgress = true,
    .waveRefreshCooldown = DEFAULT_WAVE_REFRESH_COOLDOWN,
    .waveRefreshCooldownRemaining = DEFAULT_WAVE_REFRESH_COOLDOWN,

    // Spawn Timings
    .spawnCooldown = DEFAULT_SPAWN_INTERVAL,
    .spawnCooldownRemaining = 0.0f,

    // Red Enemies
    .redEnemiesIncrease = DEFAULT_REDENEMIES_INCREASE,
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

const LIFETIME DEFAULT_SHAKE_DURATION = 0.5f;
const _Float16 DEFAULT_SHAKE_INTENSITY = 5.0f;
LIFETIME shakeLifetime = 0.0f;

const _float32_t DEFAULT_BASE_ZOOM = 1.0f;
const LIFETIME DEFAULT_ZOOM_DURATION = 2.0f;

const _float32_t DEFAULT_ZOOM_OUT_INTENSITY = 0.8f;
const _float32_t DEFAULT_ZOOM_IN_INTENSITY = 1 / DEFAULT_ZOOM_OUT_INTENSITY;
_float32_t zoomBase = DEFAULT_BASE_ZOOM;
_float32_t zoomIntensity = DEFAULT_BASE_ZOOM;
LIFETIME zoomLifetime = 0.0f;

typedef enum GameState
{
    PLAYING,
    LOST,
    PAUSED,
} GameState;

GameState gameState = PLAYING;
bool debug = false;

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

typedef enum
{
    SOUND_EFFECT_BULLET,
    SOUND_EFFECT_POWERUP,
    SOUND_EFFECT_EXPLOSION,

    // Count
    SOUND_EFFECT_COUNT
} SoundEffect;

typedef struct
{
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
    bool canShoot;
    bool isAlive;
    bool isPowerupSpeedActive;
    bool isPowerupShootingActive;
    uint32_t bulletSequence;
} Entity;

const EntityID entitiesCount = UINT8_MAX - 1;
EntityID playerID = 0;
Vector2 prevPlayerPosition = (Vector2){0};
Entity *entities = NULL;
Entity *clientsideEntities = NULL;
uint32_t lastBulletSequence = 0;
uint32_t lastInputSequence = 0;
Vector2 prevFacing = DEFAULT_ENTITY_FACING;
Vector2 prevDir = (Vector2){0, 0};

ClientInputEvent pendingInputs[128];
int pendingInputCount = 0;

/*
[ASSETS] - DEFINITIONS - LOAD - UNLOAD
[ASSETS] - DEFINITIONS - LOAD - UNLOAD
[ASSETS] - DEFINITIONS - LOAD - UNLOAD
[ASSETS] - DEFINITIONS - LOAD - UNLOAD
[ASSETS] - DEFINITIONS - LOAD - UNLOAD
[ASSETS] - DEFINITIONS - LOAD - UNLOAD
*/

Texture2D textures[ENTITY_TYPE_COUNT];
Music backgroundMusic;
Sound soundEffects[SOUND_EFFECT_COUNT];

void LoadAllTextures()
{
    textures[ENTITY_PLAYER] = LoadTexture("assets/player.png");
    textures[ENTITY_RED_ENEMY] = LoadTexture("assets/red_enemy.png");
    textures[ENTITY_BLUE_ENEMY] = LoadTexture("assets/blue_enemy.png");
    textures[ENTITY_BULLET] = LoadTexture("assets/bullet.png");
    textures[ENTITY_POWERUP_SPEED] = LoadTexture("assets/powerup_speed.png");
    textures[ENTITY_POWERUP_SHOOTING] = LoadTexture("assets/powerup_shooting.png");
    textures[ENTITY_EXPLOSION] = LoadTexture("assets/explosion.png");
    textures[ENTITY_NEUTRAL] = LoadTexture("assets/neutral.png");
}

void UnloadAllTextures()
{
    for (size_t i = 0; i < ENTITY_TYPE_COUNT; i++)
    {
        UnloadTexture(textures[i]);
    }
}
void LoadAllSoundEffects()
{
    backgroundMusic = LoadMusicStream("assets/background_music.mp3");

    soundEffects[SOUND_EFFECT_BULLET] = LoadSound("assets/laser.mp3");
    soundEffects[SOUND_EFFECT_EXPLOSION] = LoadSound("assets/explosion.wav");
    soundEffects[SOUND_EFFECT_POWERUP] = LoadSound("assets/power_up.mp3");
}

void UnloadAllSoundEffects()
{
    UnloadMusicStream(backgroundMusic);
    for (size_t i = 0; i < SOUND_EFFECT_COUNT; i++)
    {
        UnloadSound(soundEffects[i]);
    }
}

/*
ASSETS - DEFINITIONS - LOAD - UNLOAD
ASSETS - DEFINITIONS - LOAD - UNLOAD
ASSETS - DEFINITIONS - LOAD - UNLOAD
ASSETS - DEFINITIONS - LOAD - UNLOAD
ASSETS - DEFINITIONS - LOAD - UNLOAD
ASSETS - DEFINITIONS - LOAD - UNLOAD
*/

Camera2D camera = {.offset = (Vector2){screenWidth / 2, screenHeight / 2}, .rotation = 0.0f, .zoom = DEFAULT_BASE_ZOOM};

void ShakeCamera(LIFETIME duration)
{
    shakeLifetime = duration;
}

#define FOR_EACH_ALIVE_ENTITY(i)                 \
    for (EntityID i = 0; i < entitiesCount; i++) \
        if (!entities[i].isAlive)                \
            continue;                            \
        else

#define FOR_EACH_ALIVE_CLIENTSIDEENTITY(i)       \
    for (EntityID i = 0; i < entitiesCount; i++) \
        if (!clientsideEntities[i].isAlive)      \
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

EntityID SpawnClientsideEntity(Vector2 position, Vector2 size, SPEED speed, EntityType type)
{
    for (EntityID id = 0; id < entitiesCount; id++)
    {
        if (clientsideEntities[id].isAlive)
            continue;

        clientsideEntities[id] = (Entity){
            .position = position,
            .direction = Vector2Zero(),
            .facing = DEFAULT_ENTITY_FACING,
            .size = size,
            .speed = speed,
            .type = type,
            .parent = id,
            .isAlive = true,
            .lifetime = 1.0f,
        };
        return id;
    }
    return -1;
}
void SpawnServersideEntityAt(EntityID id, Vector2 position, Vector2 size, SPEED speed, EntityType type)
{

    entities[id] = (Entity){
        .position = position,
        .direction = Vector2Zero(),
        .facing = DEFAULT_ENTITY_FACING,
        .size = size,
        .speed = speed,
        .type = type,
        .parent = id,
        .isAlive = true,
        .lifetime = 1.0f,
    };
}
EntityID SpawnServersideEntity(Vector2 position, Vector2 size, SPEED speed, EntityType type)
{
    for (EntityID id = 0; id < entitiesCount; id++)
    {
        if (entities[id].isAlive)
            continue;

        SpawnServersideEntityAt(id, position, size, speed, type);
        return id;
    }
    return -1;
}

EntityID SpawnClientsideExplosion()
{
    EntityID explosionId = SpawnClientsideEntity(Vector2Zero(), DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_EXPLOSION);
    PlaySound(soundEffects[SOUND_EFFECT_EXPLOSION]);
    ShakeCamera(DEFAULT_SHAKE_DURATION);
    return explosionId;
}
EntityID SpawnClientsideBullet()
{
    EntityID bulletId = SpawnClientsideEntity(Vector2Zero(), DEFAULT_BULLET_SIZE, DEFAULT_BULLET_SPEED, ENTITY_BULLET);
    clientsideEntities[bulletId].lifetime = DEFAULT_BULLET_LIFETIME;
    clientsideEntities[bulletId].bulletSequence = ++lastBulletSequence;
    return bulletId;
}
EntityID SpawnServersideBullet()
{
    EntityID bulletId = SpawnServersideEntity(Vector2Zero(), DEFAULT_BULLET_SIZE, DEFAULT_BULLET_SPEED, ENTITY_BULLET);
    entities[bulletId].lifetime = DEFAULT_BULLET_LIFETIME;
    return bulletId;
}

EntityID ShootBullet(EntityID from, Vector2 direction)
{
    PlaySound(soundEffects[SOUND_EFFECT_BULLET]);
    EntityID bulletId = SpawnClientsideBullet();
    Entity *entity = &clientsideEntities[bulletId];
    entity->position = entities[from].position;
    entity->parent = from;
    entity->direction = direction;
    entity->facing = direction;
    return bulletId;
}
void EntityShootBullet(EntityID entity)
{
    if (!entities[entity].canShoot)
        return;

    entities[entity].shootingCooldownRemaining = entities[entity].shootingCooldown;
    entities[entity].canShoot = false;

    EntityID bulletId = ShootBullet(entity, entities[entity].facing);

    ClientInputShootEvent sh = {.dx = entities[entity].facing.x, .dy = entities[entity].facing.y, .bulletSequence = clientsideEntities[bulletId].bulletSequence};
    NetworkPushInputShootEvent(sh);
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

void ApplyPowerupSpeed(EntityID entityId, EntityID powerupId)
{
    PlaySound(soundEffects[SOUND_EFFECT_POWERUP]);
}

void ApplyPowerupShooting(EntityID entityId, EntityID powerupId)
{
    PlaySound(soundEffects[SOUND_EFFECT_POWERUP]);
}

void handlePlayerDeath(EntityID entityID)
{
    if (playerID != entityID)
        return;
    gameState = LOST;
}

/*
    FUNCTIONS FOR HANDLING SERVER EVENTS
    FUNCTIONS FOR HANDLING SERVER EVENTS
    FUNCTIONS FOR HANDLING SERVER EVENTS
    FUNCTIONS FOR HANDLING SERVER EVENTS
*/

void GameHandleEntityDiedEvent(ServerEntityDiedEvent *event)
{
    switch (entities[event->id].type)
    {
    case ENTITY_PLAYER:
        handlePlayerDeath(event->id);
        break;
    default:
        break;
    }
    EntityID explosionId = SpawnClientsideExplosion();
    clientsideEntities[explosionId].position = (Vector2){.x = event->deathPosX, .y = event->deathPosY};
    entities[event->id].isAlive = false;
}

void GameHandlePlayerCanShootEvent(ServerPlayerCanShootEvent *event)
{
    entities[event->id].canShoot = true;
}

void GameResetClientsideBullets(uint32_t sequence)
{
    FOR_EACH_ALIVE_CLIENTSIDEENTITY(i)
    {
        Entity *entity = &clientsideEntities[i];
        if (entity->type != ENTITY_BULLET)
            continue;
        if (entity->bulletSequence <= sequence)
            entity->isAlive = false;
    }
}

void GameHandleNewEntityEvent(ServerEntityState *event)
{
    Vector2 size;
    switch (event->type)
    {
    case ENTITY_BULLET:
        size = DEFAULT_BULLET_SIZE;
        break;
    default:
        size = DEFAULT_ENTITY_SIZE;
        break;
    }
    SpawnServersideEntityAt(event->id, (Vector2){event->x, event->y}, size, event->speed, event->type);
    entities[event->id].direction.x = event->vx;
    entities[event->id].direction.y = event->vy;
}

/*
    FUNCTIONS FOR HANDLING SERVER DELTAS
    FUNCTIONS FOR HANDLING SERVER DELTAS
    FUNCTIONS FOR HANDLING SERVER DELTAS
    FUNCTIONS FOR HANDLING SERVER DELTAS
*/

void GameHandleEntityFacingDelta(ServerEntityFacingDelta *delta)
{
    if (delta->id == playerID)
        return;
    Entity *entity = &entities[delta->id];
    entity->facing = (Vector2){delta->fx, delta->fy};
    entity->position = (Vector2){delta->x, delta->y};
}

void KillAllEntities()
{

    for (EntityID entityId = 0; entityId < entitiesCount; entityId++)
    {
        entities[entityId].isAlive = false;
    }
}

void ZoomIn()
{
    // zoomBase *= DEFAULT_ZOOM_IN_INTENSITY;
    zoomBase += 0.20f;
}

void ZoomOut()
{
    // zoomBase *= DEFAULT_ZOOM_OUT_INTENSITY;
    zoomBase -= 0.20f;
}

void ApplyPlayerInput(ClientInputEvent event)
{
    Vector2 newPos = Vector2Add(entities[playerID].position, Vector2Scale((Vector2){event.dx, event.dy}, entities[playerID].speed * event.dt));
    if (Vector2DistanceSqr(newPos, Vector2Zero()) <= (worldSize * worldSize))
        entities[playerID].position = newPos;
}

void GameReconciliatePlayerPosition(uint32_t serverLastProcessedInput)
{
    int processed = 0;
    while (processed < pendingInputCount &&
           pendingInputs[processed].sequence <= serverLastProcessedInput)
        processed++;

    // Shift to the start of pendingInput, from the processed ones, with a size of the remainingInputs.
    // Invariant that the array's valid inputs are at the start.
    memmove(pendingInputs, pendingInputs + processed, (pendingInputCount - processed) * sizeof(ClientInputEvent));
    pendingInputCount -= processed;

    for (int i = 0; i < pendingInputCount; i++)
    {
        ApplyPlayerInput(pendingInputs[i]);
    }
}

bool VectorsAreMoreThanDegreesApart(Vector2 a, Vector2 b, float deg)
{
    float rad = deg * DEG2RAD;

    // dot = cos(theta)
    float dot = a.x * b.x + a.y * b.y;

    float angle = acosf(dot);

    return angle > rad;
}

void Input(struct Client *client)
{
    if (IsKeyPressed(KEY_P))
        gameState = PAUSED;

    if (IsKeyPressed(KEY_O))
        debug = !debug;

    if (IsKeyPressed(KEY_J))
        ZoomOut();
    if (IsKeyPressed(KEY_K))
        ZoomIn();

    Vector2 direction = {0};
    if (IsKeyDown(KEY_W))
        direction.y -= 1;
    if (IsKeyDown(KEY_A))
        direction.x -= 1;
    if (IsKeyDown(KEY_S))
        direction.y += 1;
    if (IsKeyDown(KEY_D))
        direction.x += 1;

    Vector2 mousePositionWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    entities[playerID].facing = Vector2Normalize(Vector2Subtract(mousePositionWorld, entities[playerID].position));

    if (direction.x != 0.0f || direction.y != 0.0f || VectorsAreMoreThanDegreesApart(entities[playerID].facing, prevFacing, 5))
    {
        ClientInputEvent input = {.sequence = ++lastInputSequence, .dx = direction.x, .dy = direction.y, .fx = entities[playerID].facing.x, .fy = entities[playerID].facing.y, .dt = GetFrameTime()};

        NetworkPushInputMoveEvent(input);
        ApplyPlayerInput(input);

        pendingInputs[pendingInputCount++] = input;
        prevFacing = entities[playerID].facing;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE))
        EntityShootBullet(playerID);
}

void UpdateCameraMovement()
{
    camera.target = entities[playerID].position;
    if (shakeLifetime > 0.0f)
    {
        shakeLifetime -= GetFrameTime();

        float offsetX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float offsetY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;

        float fade = shakeLifetime / DEFAULT_SHAKE_DURATION;

        camera.offset.x += offsetX * DEFAULT_SHAKE_INTENSITY * fade;
        camera.offset.y += offsetY * DEFAULT_SHAKE_INTENSITY * fade;
    }
    if (zoomLifetime > 0.0f)
    {
        float zoomChange = 0.0f;
        zoomLifetime -= GetFrameTime();
        float elapsed = DEFAULT_ZOOM_DURATION - zoomLifetime;

        float phaseOut = 0.25f;
        float phaseIn = 0.25f;

        if (elapsed < phaseOut)
        {
            float t = elapsed / phaseOut;
            zoomChange = zoomBase + (zoomIntensity - zoomBase) * t;
        }
        else if (elapsed > DEFAULT_ZOOM_DURATION - phaseIn)
        {
            float t = (DEFAULT_ZOOM_DURATION - elapsed) / phaseIn;
            zoomChange = zoomBase + (zoomIntensity - zoomBase) * t;
        }
        else
        {
            zoomChange = zoomIntensity;
        }
        camera.zoom = zoomChange;
    }
    else
        camera.zoom = zoomBase;
}

void GameUpdatePlayerID(uint8_t newPlayerID)
{
    playerID = newPlayerID;
}

void GameUpdateNetworkEntities(ServerEntityState *networkEntity, int count)
{
    for (int i = 0; i < entitiesCount; i++)
    {
        entities[i].isAlive = false;
    }

    for (int i = 0; i < count; i++)
    {

        entities[networkEntity[i].id].position.x = networkEntity[i].x;
        entities[networkEntity[i].id].position.y = networkEntity[i].y;
        entities[networkEntity[i].id].direction.x = networkEntity[i].vx;
        entities[networkEntity[i].id].direction.y = networkEntity[i].vy;
        if (networkEntity[i].type != ENTITY_RED_ENEMY && networkEntity[i].type != ENTITY_PLAYER)
            entities[networkEntity[i].id].facing = DEFAULT_ENTITY_FACING;
        // General attributes
        entities[networkEntity[i].id].speed = networkEntity[i].speed;
        entities[networkEntity[i].id].type = networkEntity[i].type;

        // Default implementation
        entities[networkEntity[i].id].size = DEFAULT_ENTITY_SIZE;
        entities[networkEntity[i].id].parent = networkEntity->id;
        entities[networkEntity[i].id].lifetime = 1.0f;

        if (networkEntity[i].type == ENTITY_BULLET)
        {
            entities[networkEntity[i].id].size = DEFAULT_BULLET_SIZE;
            entities[networkEntity[i].id].facing = entities[networkEntity[i].id].direction;
        }

        // Snapshots are only alive entities
        entities[networkEntity[i].id].isAlive = true;
    }
}
void GameUpdateNetworkWave(ServerWaveSnapshot *waveSnapshot)
{
    waveManager.wave = waveSnapshot->wave;
    waveManager.blueEnemiesRemaining = waveSnapshot->blueEnemiesRemaining;
    waveManager.redEnemiesRemaining = waveSnapshot->redEnemiesRemaining;
}

void UpdateNetwork(struct Client *client)
{
    while (elapsedTimeBetweenSnapshotTicks >= timeBetweenSnapshotTicks)
    {
        elapsedTimeBetweenSnapshotTicks -= timeBetweenSnapshotTicks;
        // if (entities[playerID].direction.x != 0.0f || entities[playerID].direction.y != 0.0f)
        // {
        //     ClientInputAuthorativeMoveEvent mh = {.nx = (int16_t)entities[playerID].position.x, .ny = (int16_t)entities[playerID].position.y};
        //     NetworkPushInputAuthorativeMoveEvent(mh);
        // }
        NetworkSendPacket(client);
        NetworkPrepareBuffer();
    }
    NetworkRecievePacket(client);
}

void UpdateEntities()
{
    FOR_EACH_ALIVE_ENTITY(i)
    {
        Vector2 newPos = Vector2Add(entities[i].position, Vector2Scale(entities[i].direction, entities[i].speed * GetFrameTime()));
        if (Vector2DistanceSqr(newPos, Vector2Zero()) <= (worldSize * worldSize))
            entities[i].position = newPos;
        if (entities[i].lifetime < 0.0f)
            entities[i].isAlive = false;
        switch (entities[i].type)
        {
        case ENTITY_RED_ENEMY:
            Vector2 distance = Vector2Normalize(Vector2Subtract(entities[playerID].position, entities[i].position));
            entities[i].facing = distance;
            break;
        case ENTITY_NEUTRAL:
            FOR_EACH_ALIVE_ENTITY(target)
            {
                if (entities[target].type != ENTITY_RED_ENEMY)
                    continue;
                if (Vector2Distance(entities[i].position, entities[target].position) < NEUTRAL_SHOOT_RADIUS)
                {
                    Vector2 neutralFacing = Vector2Normalize(Vector2Subtract(entities[target].position, entities[i].position));

                    entities[i].facing = neutralFacing;
                }
            }
            break;
        default:
            break;
        }
    }
}
void UpdateClientsideEntities()
{
    FOR_EACH_ALIVE_CLIENTSIDEENTITY(i)
    {
        Entity *clientsideEntity = &clientsideEntities[i];
        clientsideEntity->position = Vector2Add(clientsideEntity->position, Vector2Scale(clientsideEntity->direction, clientsideEntity->speed * GetFrameTime()));
        if (clientsideEntity->lifetime < 0.0f)
            clientsideEntity->isAlive = false;
        switch (clientsideEntity->type)
        {
        case ENTITY_BULLET:
            clientsideEntity->lifetime -= GetFrameTime();
            break;
        case ENTITY_EXPLOSION:
            clientsideEntity->lifetime -= GetFrameTime();
            break;
        default:
            break;
        }
    }
}

void Update(struct Client *client)
{
    UpdateMusicStream(backgroundMusic);

    UpdateEntities();
    UpdateClientsideEntities();
    UpdateCameraMovement();
    UpdateNetwork(client);
}

const uint32_t cellSize = 50;

void RenderDebug()
{

    for (int i = -worldSize; i <= worldSize; i += cellSize)
        for (int j = -worldSize; j <= worldSize; j += cellSize)
            DrawText(TextFormat("%2i,%2i", i, j), i, j, 5, GRAY);

    FOR_EACH_ALIVE_ENTITY(i)
    {
        switch (entities[i].type)
        {
        case ENTITY_NEUTRAL:
            DrawCircleV(entities[i].position, NEUTRAL_SHOOT_RADIUS, (Color){253, 249, 0, 100});
            break;
        case ENTITY_PLAYER:
            DrawCircleV(entities[i].position, PLAYER_SAFE_RADIUS, (Color){230, 41, 55, 100});
            DrawCircleV(entities[i].position, ENTITY_SPAWN_RADIUS, (Color){0, 121, 241, 100});
            break;
        default:
            break;
        }
        DrawLineV(entities[playerID].position, entities[i].position, RED);
        DrawLineV(entities[i].position, Vector2Add(entities[i].position, Vector2Scale(entities[i].direction, 50)), BLUE);
        DrawLineV(entities[i].position, Vector2Add(entities[i].position, Vector2Scale(entities[i].facing, 50)), GREEN);
    }
    DrawLineV(entities[playerID].position, Vector2Add(entities[playerID].position, Vector2Scale(prevFacing, 50)), YELLOW);
}

void Render()
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText(TextFormat("Red enemies: %2i", waveManager.redEnemiesRemaining), 10, 10, 30, RED);
    DrawText(TextFormat("Blue enemies: %2i", waveManager.blueEnemiesRemaining), 10, 50, 30, BLUE);
    DrawText(TextFormat("Player ID: %1i", playerID), 10, 90, 30, BLACK);

    BeginMode2D(camera);

    DrawText(TextFormat("Wave %2i", waveManager.wave), 0, 0, 40, GRAY);

    for (int x = -worldSize; x <= worldSize; x += cellSize)
        DrawLine(x, -worldSize, x, worldSize, GRAY);

    for (int y = -worldSize; y <= worldSize; y += cellSize)
        DrawLine(-worldSize, y, worldSize, y, GRAY);

    for (EntityID i = 0; i < entitiesCount; i++)
    {
        Entity entity = entities[i];
        if (!entity.isAlive)
            continue;
        Texture2D texture = textures[entity.type];
        Rectangle src = {.x = 0, .y = 0, .width = texture.width, .height = texture.height};
        Rectangle dest = {.x = entity.position.x, .y = entity.position.y, .width = entity.size.x, .height = entity.size.y};
        Vector2 origin = (Vector2){entity.size.x / 2.0f, entity.size.y / 2.0f};
        float rotation = atan2f(entity.facing.y, entity.facing.x) * RAD2DEG;
        DrawTexturePro(texture, src, dest, origin, rotation, WHITE);
    }
    for (EntityID i = 0; i < entitiesCount; i++)
    {
        Entity entity = clientsideEntities[i];
        if (!entity.isAlive)
            continue;
        Texture2D texture = textures[entity.type];
        Rectangle src = {.x = 0, .y = 0, .width = texture.width, .height = texture.height};
        Rectangle dest = {.x = entity.position.x, .y = entity.position.y, .width = entity.size.x, .height = entity.size.y};
        Vector2 origin = (Vector2){entity.size.x / 2.0f, entity.size.y / 2.0f};
        float rotation = atan2f(entity.facing.y, entity.facing.x) * RAD2DEG;
        DrawTexturePro(texture, src, dest, origin, rotation, WHITE);
    }

    if (debug)
        RenderDebug();
    EndMode2D();

    DrawFPS(0, 0);
    DrawText(TextFormat("%03i%%", (int)lroundf(zoomBase * 100.0f)), screenWidth - 50, screenHeight - 25, 20, BLACK);

    EndDrawing();
}

void RenderLostScreen()
{

    BeginDrawing();

    ClearBackground(RED);

    Vector2 titlePos = (Vector2){screenWidth / 2, 20};
    DrawText("YOU LOST", titlePos.x, titlePos.y, 50, WHITE);
    DrawText(TextFormat("Reached wave %02i", waveManager.wave), titlePos.x, titlePos.y + 70, 30, GRAY);
    EndDrawing();
}

void GameRun(struct Client *client)
{
    InitWindow(screenWidth, screenHeight, gameTitle);
    InitAudioDevice();
    SetTargetFPS(60);

    entities = calloc(entitiesCount, sizeof(Entity));
    clientsideEntities = calloc(entitiesCount, sizeof(Entity));

    LoadAllTextures();
    LoadAllSoundEffects();
    PlayMusicStream(backgroundMusic);

    double prevTime = GetTime();

    while (!WindowShouldClose())
    {
        double currentTime = GetTime();
        double deltaTime = currentTime - prevTime;
        prevTime = currentTime;

        elapsedTimeBetweenSnapshotTicks += deltaTime;

        switch (gameState)
        {
        case PLAYING:
            Input(client);

            Update(client);

            Render();
            break;
        case PAUSED:
            if (IsKeyPressed(KEY_P))
                gameState = PLAYING;

            Render();
            break;
        case LOST:
            RenderLostScreen();
            break;
        default:
            break;
        }
    }

    StopMusicStream(backgroundMusic);
    UnloadAllTextures();
    UnloadAllSoundEffects();

    CloseAudioDevice();
    CloseWindow();
    free(entities);
    free(clientsideEntities);
}
