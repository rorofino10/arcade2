#include "raylib.h"
#include "raymath.h"

#include "stdio.h"
#include "stdlib.h"

#include "stdint.h"

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

uint8_t wave = 1;

const SPEED redEnemiesSpeedIncrease = 10;
SPEED redEnemiesSpeed = DEFAULT_ENEMY_SPEED;
const EntityID redEnemiesIncrease = 3;
EntityID redEnemiesLeftToSpawn = 0;
EntityID redEnemiesToSpawn = 0;
EntityID redEnemiesRemaining = 0;

const EntityID blueEnemiesIncrease = 1;
EntityID blueEnemiesLeftToSpawn = 0;
EntityID blueEnemiesToSpawn = 0;
EntityID blueEnemiesRemaining = 0;

const COOLDOWN waveRefreshCooldown = 2.0f;
COOLDOWN waveRefreshCooldownRemaining = 2.0f;
bool isWaveInProgress = false;

const COOLDOWN spawnCooldown = 0.5f;
COOLDOWN spawnCooldownRemaining = 0.0f;

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
    bool isAlive;
    bool isPowerupSpeedActive;
    bool isPowerupShootingActive;
} Entity;

const EntityID entitiesCount = UINT8_MAX - 1;
EntityID playerID = 0;
Entity *entities = NULL;
Texture2D textures[ENTITY_TYPE_COUNT];
Music backgroundMusic;
Sound soundEffects[SOUND_EFFECT_COUNT];

Camera2D camera = {.offset = (Vector2){screenWidth / 2, screenHeight / 2}, .rotation = 0.0f, .zoom = DEFAULT_BASE_ZOOM};

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
    EntityID enemyId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, redEnemiesSpeed, ENTITY_RED_ENEMY);
    spawnCooldownRemaining = spawnCooldown;
    redEnemiesLeftToSpawn--;
    redEnemiesRemaining++;
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
    spawnCooldownRemaining = spawnCooldown;
    blueEnemiesLeftToSpawn--;
    blueEnemiesRemaining++;
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
    PlaySound(soundEffects[SOUND_EFFECT_EXPLOSION]);
    shakeLifetime = DEFAULT_SHAKE_DURATION;
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
    PlaySound(soundEffects[SOUND_EFFECT_BULLET]);
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
    isWaveInProgress = true;
    waveRefreshCooldownRemaining = waveRefreshCooldown;

    redEnemiesToSpawn += redEnemiesIncrease;
    redEnemiesLeftToSpawn = redEnemiesToSpawn;
    redEnemiesSpeed += redEnemiesSpeedIncrease;

    blueEnemiesToSpawn += blueEnemiesIncrease;
    blueEnemiesLeftToSpawn = blueEnemiesToSpawn;
}

void CheckWaveEnded()
{
    if (redEnemiesRemaining == 0 && blueEnemiesRemaining == 0)
    {
        wave++;
        isWaveInProgress = false;
    }
}

void ApplyPowerupSpeed(EntityID entityId, EntityID powerupId)
{
    PlaySound(soundEffects[SOUND_EFFECT_POWERUP]);
    entities[entityId].speed *= 3.0f;
    entities[entityId].powerupSpeedLifetime = DEFAULT_POWERUP_SPEED_LIFETIME;
    entities[entityId].isPowerupSpeedActive = true;
    entities[powerupId].isAlive = false;
}

void ApplyPowerupShooting(EntityID entityId, EntityID powerupId)
{
    PlaySound(soundEffects[SOUND_EFFECT_POWERUP]);
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
    blueEnemiesRemaining--;
}
void handleRedEnemyDeath(EntityID entityId)
{
    redEnemiesRemaining--;
}

void handlePlayerDeath(EntityID playerId)
{
    gameState = LOST;
}

void KillEntity(EntityID entityId)
{
    if (!entities[entityId].isAlive)
        return;
    switch (entities[entityId].type)
    {
    case ENTITY_BLUE_ENEMY:
        handleBlueEnemyDeath(entityId);
        break;

    case ENTITY_RED_ENEMY:
        handleRedEnemyDeath(entityId);
        break;
    case ENTITY_PLAYER:
        handlePlayerDeath(entityId);
    default:
        break;
    }
    EntityID explosionId = SpawnExplosion();
    entities[explosionId].position = entities[entityId].position;
    entities[entityId].isAlive = false;
    CheckWaveEnded();
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
    SetCollision(ENTITY_BULLET, ENTITY_PLAYER)
    {
        KillEntity(idB);
        return;
    }
    SetCollision(ENTITY_PLAYER, ENTITY_RED_ENEMY)
    {
        KillEntity(idA);
        return;
    }
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
    redEnemiesRemaining = 0;
    blueEnemiesRemaining = 0;
}

void RestartWave()
{
    wave = 1;
    blueEnemiesToSpawn = 0;
    redEnemiesToSpawn = 0;
}

void StartGame()
{
    SpawnPlayer();
    SpawnNeutral();
    GenerateWave();
}

void RestartGame()
{
    StopMusicStream(backgroundMusic);
    PlayMusicStream(backgroundMusic);
    KillAllEntities();
    RestartWave();
    StartGame();
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
void Input()
{
    if (IsKeyPressed(KEY_P))
        gameState = PAUSED;

    if (IsKeyPressed(KEY_R))
        RestartGame();
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

    entities[playerID].direction = Vector2Normalize(direction);
    Vector2 mousePositionWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    entities[playerID].facing = Vector2Normalize(Vector2Subtract(mousePositionWorld, entities[playerID].position));

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyDown(KEY_SPACE))
        EntityShootBullet(playerID);
}

void UpdateWave()
{
    if (!isWaveInProgress && waveRefreshCooldownRemaining > 0.0f)
        waveRefreshCooldownRemaining -= GetFrameTime();
    if (!isWaveInProgress && waveRefreshCooldownRemaining <= 0.0f)
        GenerateWave();
    if (isWaveInProgress)
    {
        if (redEnemiesLeftToSpawn != 0 || blueEnemiesLeftToSpawn != 0)
        {
            if (spawnCooldownRemaining > 0.0f)
                spawnCooldownRemaining -= GetFrameTime();
            if (spawnCooldownRemaining <= 0.0f && redEnemiesLeftToSpawn != 0)
            {
                SpawnRedEnemy();
            }
            if (spawnCooldownRemaining <= 0.0f && blueEnemiesLeftToSpawn != 0)
            {
                SpawnBlueEnemy();
            }
        }
    }
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

void Update()
{
    UpdateMusicStream(backgroundMusic);

    FOR_EACH_ALIVE_ENTITY(i)
    {
        Vector2 newPos = Vector2Add(entities[i].position, Vector2Scale(entities[i].direction, entities[i].speed * GetFrameTime()));
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
            entities[i].lifetime -= GetFrameTime();
            break;
        case ENTITY_EXPLOSION:
            entities[i].lifetime -= GetFrameTime();
            break;
        case ENTITY_PLAYER:
            if (entities[i].shootingCooldownRemaining > 0.0f)
                entities[i].shootingCooldownRemaining -= GetFrameTime();
            if (entities[i].powerupSpeedLifetime > 0.0f)
                entities[i].powerupSpeedLifetime -= GetFrameTime();
            if (entities[i].isPowerupSpeedActive && entities[i].powerupSpeedLifetime <= 0.0f)
            {
                entities[i].isPowerupSpeedActive = false;
                entities[i].speed = DEFAULT_PLAYER_SPEED;
            }
            if (entities[i].powerupShootingLifetime > 0.0f)
                entities[i].powerupShootingLifetime -= GetFrameTime();
            if (entities[i].isPowerupShootingActive && entities[i].powerupShootingLifetime <= 0.0f)
            {
                entities[i].powerupShootingLifetime = false;
                entities[i].shootingCooldown = DEFAULT_SHOOTING_COOLDOWN;
            }
            break;
        case ENTITY_NEUTRAL:
            if (entities[i].shootingCooldownRemaining > 0.0f)
                entities[i].shootingCooldownRemaining -= GetFrameTime();
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
    UpdateWave();
    UpdateCameraMovement();
}

const uint32_t cellSize = 50;

void RenderDebug()
{

    DrawCircleV(entities[playerID].position, PLAYER_SAFE_RADIUS, (Color){230, 41, 55, 100});
    DrawCircleV(entities[playerID].position, ENTITY_SPAWN_RADIUS, (Color){0, 121, 241, 100});
    for (int i = -worldSize; i <= worldSize; i += cellSize)
        for (int j = -worldSize; j <= worldSize; j += cellSize)
            DrawText(TextFormat("%2i,%2i", i, j), i, j, 5, GRAY);

    for (EntityID i = 0; i < entitiesCount; i++)
    {
        Entity entity = entities[i];
        if (!entity.isAlive)
            continue;
        switch (entity.type)
        {
        case ENTITY_NEUTRAL:
            DrawCircleV(entity.position, NEUTRAL_SHOOT_RADIUS, (Color){253, 249, 0, 100});
            break;

        default:
            break;
        }
        DrawLineV(entities[playerID].position, entity.position, RED);
        DrawLineV(entity.position, Vector2Add(entity.position, Vector2Scale(entity.direction, 50)), BLUE);
        DrawLineV(entity.position, Vector2Add(entity.position, Vector2Scale(entity.facing, 50)), GREEN);
    }
}

void Render()
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    DrawText(TextFormat("Red enemies: %2i", redEnemiesRemaining), 10, 10, 30, RED);
    DrawText(TextFormat("Blue enemies: %2i", blueEnemiesRemaining), 10, 50, 30, BLUE);

    BeginMode2D(camera);

    DrawText(TextFormat("Wave %2i", wave), 0, 0, 40, GRAY);

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

    if (debug)
        RenderDebug();
    EndMode2D();

    DrawText(TextFormat("%03i%%", (int)lroundf(zoomBase * 100.0f)), screenWidth - 50, screenHeight - 25, 20, BLACK);

    EndDrawing();
}

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

void RenderLostScreen()
{

    BeginDrawing();

    ClearBackground(RED);

    Vector2 titlePos = (Vector2){screenWidth / 2, 20};
    DrawText("YOU LOST", titlePos.x, titlePos.y, 50, WHITE);
    DrawText(TextFormat("Reached wave %02i", wave), titlePos.x, titlePos.y + 70, 30, GRAY);
    EndDrawing();
}

int main(int argc, char **argv)
{
    InitWindow(screenWidth, screenHeight, gameTitle);
    InitAudioDevice();
    SetTargetFPS(60);

    entities = calloc(entitiesCount, sizeof(Entity));

    StartGame();

    LoadAllTextures();
    LoadAllSoundEffects();
    PlayMusicStream(backgroundMusic);
    while (!WindowShouldClose())
    {
        switch (gameState)
        {
        case PLAYING:
            Input();

            Update();

            Render();
            break;
        case PAUSED:
            if (IsKeyPressed(KEY_P))
                gameState = PLAYING;

            RenderLostScreen();

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
    return 0;
}
