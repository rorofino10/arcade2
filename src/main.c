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
const uint8_t DEFAULT_PLAYER_SPEED = 100;
const uint8_t DEFAULT_ENEMY_SPEED = 40;
const uint8_t DEFAULT_BULLET_SPEED = 200;
const COOLDOWN DEFAULT_SHOOTING_COOLDOWN = 0.5f;
const LIFETIME DEFAULT_BULLET_LIFETIME = 2.0f;
const LIFETIME DEFAULT_POWERUP_SPEED_LIFETIME = 2.0f;
const LIFETIME DEFAULT_POWERUP_SHOOTING_LIFETIME = 2.0f;
const Vector2 DEFAULT_ENTITY_SIZE = (Vector2){50, 50};
const Vector2 DEFAULT_ENTITY_FACING = (Vector2){1, 0};
const Vector2 DEFAULT_BULLET_SIZE = (Vector2){11, 5};

uint8_t wave = 1;
const EntityID redEnemiesIncrease = 2;
EntityID redEnemiesLeftToSpawn = 0;
EntityID redEnemiesToSpawn = 0;
EntityID redEnemiesRemaining = 0;

const COOLDOWN waveRefreshCooldown = 2.0f;
COOLDOWN waveRefreshCooldownRemaining = 2.0f;
bool isWaveInProgress = false;

const COOLDOWN spawnCooldown = 0.5f;
COOLDOWN spawnCooldownRemaining = 0.0f;

const uint16_t PLAYER_SAFE_RADIUS = 100;

typedef enum GameState
{
    PLAYING,
    LOST,
} GameState;

GameState gameState = PLAYING;

typedef enum
{
    ENTITY_PLAYER,
    ENTITY_RED_ENEMY,
    ENTITY_BLUE_ENEMY,
    ENTITY_BULLET,
    ENTITY_POWERUP_SPEED,
    ENTITY_POWERUP_SHOOTING,
    ENTITY_EXPLOSION,
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

const uint8_t entitiesCount = UINT8_MAX;
EntityID playerID = 0;
Entity *entities = NULL;
Texture2D textures[ENTITY_TYPE_COUNT];
Music backgroundMusic;
Sound soundEffects[SOUND_EFFECT_COUNT];

Camera2D camera = {.offset = (Vector2){screenWidth / 2, screenHeight / 2}, .rotation = 0.0f, .zoom = 1.0f};

Vector2 RandomSpawnPosition(Vector2 playerPos)
{
    Vector2 pos;
    do
    {
        pos.x = GetRandomValue(-screenWidth / 2, screenWidth / 2);
        pos.y = GetRandomValue(-screenHeight / 2, screenHeight / 2);
    } while (Vector2Distance(playerPos, pos) < PLAYER_SAFE_RADIUS);
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
    EntityID enemyId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, DEFAULT_ENEMY_SPEED, ENTITY_RED_ENEMY);
    return enemyId;
}
EntityID SpawnBlueEnemy()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID enemyId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, 0.0f, ENTITY_BLUE_ENEMY);
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
void PlayerShootBullet()
{
    if (entities[playerID].shootingCooldownRemaining <= 0.0f)
    {
        entities[playerID].shootingCooldownRemaining = entities[playerID].shootingCooldown;
        ShootBullet(playerID, entities[playerID].facing);
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
    isWaveInProgress = true;
    waveRefreshCooldownRemaining = waveRefreshCooldown;
    redEnemiesToSpawn += redEnemiesIncrease;
    redEnemiesLeftToSpawn = redEnemiesToSpawn;
}

void CheckWaveEnded()
{
    if (redEnemiesRemaining == 0)
    {
        wave++;
        isWaveInProgress = false;
    }
}

void ApplyPowerupSpeed(EntityID entityId, EntityID powerupId)
{
    PlaySound(soundEffects[SOUND_EFFECT_POWERUP]);
    entities[entityId].speed *= 2.0f;
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
    for (EntityID idA = 0; idA < entitiesCount; idA++)
    {
        if (!entities[idA].isAlive)
            continue;
        for (EntityID idB = 0; idB < entitiesCount; idB++)
        {
            if (!entities[idB].isAlive)
                continue;

            handleCollisionBetween(idA, idB);
        }
    }
}

void Input()
{
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

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        PlayerShootBullet();
}

void UpdateWave()
{
    if (!isWaveInProgress && waveRefreshCooldownRemaining > 0.0f)
        waveRefreshCooldownRemaining -= GetFrameTime();
    if (!isWaveInProgress && waveRefreshCooldownRemaining <= 0.0f)
        GenerateWave();
    if (isWaveInProgress)
    {
        if (redEnemiesLeftToSpawn != 0)
        {
            if (spawnCooldownRemaining > 0.0f)
                spawnCooldownRemaining -= GetFrameTime();
            if (spawnCooldownRemaining <= 0.0f)
            {
                SpawnRedEnemy();
                spawnCooldownRemaining = spawnCooldown;
                redEnemiesLeftToSpawn--;
                redEnemiesRemaining++;
            }
        }
    }
}

void Update()
{
    UpdateMusicStream(backgroundMusic);

    camera.target = entities[playerID].position;
    for (EntityID i = 0; i < entitiesCount; i++)
    {
        if (!entities[i].isAlive)
            continue;
        entities[i].position = Vector2Add(entities[i].position, Vector2Scale(entities[i].direction, entities[i].speed * GetFrameTime()));
        if (entities[i].lifetime < 0.0f)
            entities[i].isAlive = false;
        switch (entities[i].type)
        {
        case ENTITY_RED_ENEMY:
            entities[i].direction = Vector2Normalize(Vector2Subtract(entities[playerID].position, entities[i].position));
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
        default:
            break;
        }
    }
    HandleCollisions();
    UpdateWave();
}

void Render()
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode2D(camera);
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

    DrawText(TextFormat("Wave %2i", wave), 0, 0, 40, GRAY);
    EndMode2D();

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
    SpawnPlayer();
    GenerateWave();

    SpawnPowerupShooting();
    SpawnPowerupSpeed();
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
