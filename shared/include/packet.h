#ifndef PACKET_H
#define PACKET_H
#define MAX_PACKET_SIZE 1400

#include <stdint.h>

typedef enum
{
    PACKET_ASSIGN_PLAYER,
    PACKET_INPUT_EVENT,
    PACKET_SERVER_EVENTS,
    PACKET_ENTITY_SNAPSHOT,
    PACKET_ENTITY_DELTAS,
    PACKET_WAVE_SNAPSHOT,
    PACKET_HEARTBEAT,
} PACKET_TYPE;

typedef struct
{
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ClientPacketHeader;

typedef struct
{
    float dx;
    float dy;
    uint32_t bulletSequence;
} ClientInputShootEvent;

typedef struct
{
    int16_t nx;
    int16_t ny;
} __attribute__((packed)) ClientInputAuthorativeMoveEvent;

typedef struct ClientInputEvent
{
    uint32_t sequence;
    float dx;
    float dy;
    float fx, fy;
    float dt;
} __attribute__((packed)) ClientInputEvent;

typedef enum CLIENT_EVENT_TYPE
{
    CLIENT_EVENT_INPUT_SHOOT,
    CLIENT_EVENT_INPUT_AUTHORATIVE_MOVE,
    CLIENT_EVENT_INPUT_MOVE,
} CLIENT_EVENT_TYPE;

typedef struct
{
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ClientEventHeader;

// sizeof(ServerPacketHeader)=11
typedef struct
{
    uint8_t type;
    uint16_t size;
    uint32_t lastProcessedBullet;
    uint32_t lastProcessedMovementInput;
} __attribute__((packed)) ServerPacketHeader;

typedef struct
{
    uint32_t sequence;
    uint32_t lastProcessedBullet;
    uint32_t lastProcessedMovementInput;
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ServerUDPPacketHeader;

typedef struct
{
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ClientUDPPacketHeader;

// sizeof(ServerEntityState)=16
typedef struct ServerEntityState
{
    uint8_t id;
    int16_t x, y;
    float vx, vy;
    uint8_t type;
    uint16_t speed;
} __attribute__((packed)) ServerEntityState;

// sizeof(ServerEntityState)=17
typedef struct ServerBulletSpawnEvent
{
    uint8_t id;
    int16_t x, y;
    float dx, dy;
    uint32_t sequence;
    uint16_t speed;
} __attribute__((packed)) ServerBulletSpawnEvent;

// sizeof(ServerWaveSnapshot)=3
typedef struct ServerWaveSnapshot
{
    uint8_t wave;
    uint8_t blueEnemiesRemaining;
    uint8_t redEnemiesRemaining;
} __attribute__((packed)) ServerWaveSnapshot;

typedef struct ServerAssignPlayerMessage
{
    uint8_t playerID;
} __attribute__((packed)) ServerAssignPlayerMessage;

typedef enum SERVER_EVENT_TYPE
{
    SERVER_EVENT_ENTITY_DIED,
    SERVER_EVENT_PLAYER_CAN_SHOOT,
    SERVER_EVENT_NEW_ENTITY,
    SERVER_EVENT_BULLET_SPAWN,
    SERVER_EVENT_POWERUP,
    SERVER_DELTA_ENTITY_FACING,
} SERVER_EVENT_TYPE;

typedef struct ServerEventHeader
{
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ServerEventHeader;

typedef struct ServerEntityDiedEvent
{
    uint8_t id;
    int16_t deathPosX;
    int16_t deathPosY;
} __attribute__((packed)) ServerEntityDiedEvent;

typedef struct ServerPowerupEvent
{
    uint8_t type;
    uint8_t picker;
} __attribute__((packed)) ServerPowerupEvent;

typedef struct ServerPlayerCanShootEvent
{
    uint8_t id;
} __attribute__((packed)) ServerPlayerCanShootEvent;

typedef struct ServerEntityFacingDelta
{
    uint8_t id;
    float fx, fy;
    float x, y;
} __attribute__((packed)) ServerEntityFacingDelta;

#endif
