#include "raylib.h"
#include "raymath.h"

#include "stdio.h"
#include "stdlib.h"

#include "stdint.h"

typedef uint8_t EntityID;
typedef float _float32_t;
typedef _float32_t COOLDOWN;
typedef _float32_t SPEED;
typedef _float32_t LIFETIME;

const uint16_t screenWidth = 800;
const uint16_t screenHeight = 600;
const char *gameTitle = "Game Title";

const uint8_t DEFAULT_PLAYER_SPEED = 100;
const uint8_t DEFAULT_BULLET_SPEED = 200;
const COOLDOWN DEFAULT_SHOOTING_COOLDOWN = 0.5f;
const LIFETIME DEFAULT_BULLET_LIFETIME = 2.0f;
const Vector2 DEFAULT_ENTITY_SIZE = (Vector2){50, 50};
const Vector2 DEFAULT_ENTITY_FACING = (Vector2){1, 0};
const Vector2 DEFAULT_BULLET_SIZE = (Vector2){11, 5};

const uint16_t PLAYER_SAFE_RADIUS = 100;

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
    bool isAlive;
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
    return playerID;
}
EntityID SpawnRedEnemy()
{
    Vector2 position = RandomSpawnPosition(entities[playerID].position);
    EntityID enemyId = SpawnEntity(position, DEFAULT_ENTITY_SIZE, DEFAULT_PLAYER_SPEED, ENTITY_RED_ENEMY);
    return enemyId;
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

void KillEntity(EntityID entityId)
{
    EntityID explosionId = SpawnExplosion();
    entities[explosionId].position = entities[entityId].position;
    entities[entityId].isAlive = false;
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

#define SetCollision(entityA, entityB) if ((entities[idA].type == entityA && entities[idB].type == entityB))

void handleCollisionBetween(EntityID idA, EntityID idB)
{
    if (entities[idA].parent == idB)
        return;
    Rectangle recA = (Rectangle){.x = entities[idA].position.x, .y = entities[idA].position.y, .width = entities[idA].size.x, .height = entities[idA].size.y};
    Rectangle recB = (Rectangle){.x = entities[idB].position.x, .y = entities[idB].position.y, .width = entities[idB].size.x, .height = entities[idB].size.y};
    if (!CheckCollisionRecs(recA, recB))
        return;
    SetCollision(ENTITY_BULLET, ENTITY_RED_ENEMY)
    {
        KillEntity(idB);
    }
    SetCollision(ENTITY_PLAYER, ENTITY_RED_ENEMY)
    {
        KillEntity(idA);
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
        case ENTITY_BULLET:
            entities[i].lifetime -= GetFrameTime();
            break;
        case ENTITY_EXPLOSION:
            entities[i].lifetime -= GetFrameTime();
            break;
        case ENTITY_PLAYER:
            if (entities[i].shootingCooldownRemaining > 0.0f)
                entities[i].shootingCooldownRemaining -= GetFrameTime();
            break;
        default:
            break;
        }
    }
    HandleCollisions();
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

int main(int argc, char **argv)
{
    InitWindow(screenWidth, screenHeight, gameTitle);
    InitAudioDevice();
    SetTargetFPS(60);

    entities = calloc(entitiesCount, sizeof(Entity));
    SpawnPlayer();
    SpawnRedEnemy();
    SpawnRedEnemy();
    SpawnRedEnemy();

    LoadAllTextures();
    LoadAllSoundEffects();
    PlayMusicStream(backgroundMusic);
    while (!WindowShouldClose())
    {
        Input();

        Update();

        Render();
    }

    StopMusicStream(backgroundMusic);
    UnloadAllTextures();
    UnloadAllSoundEffects();

    CloseAudioDevice();
    CloseWindow();
    free(entities);
    return 0;
}
