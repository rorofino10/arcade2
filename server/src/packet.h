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
    PACKET_WAVE_SNAPSHOT,
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
} PacketShootEvent;

typedef struct
{
    int16_t nx;
    int16_t ny;
} __attribute__((packed)) PacketMoveEvent;

typedef enum PACKET_INPUT_EVENT_TYPE
{
    PACKET_INPUT_SHOOT,
    PACKET_INPUT_MOVE,
} PACKET_INPUT_EVENT_TYPE;

typedef struct
{
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ClientEventHeader;

typedef struct
{
    uint8_t type;
    uint16_t size;
} __attribute__((packed)) ServerPacketHeader;

// sizeof(ServerEntityState)=16
typedef struct ServerEntityState
{
    uint8_t id;
    int16_t x, y;
    float vx, vy;
    uint8_t type;
    uint16_t speed;
} __attribute__((packed)) ServerEntityState;

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

typedef enum PACKET_SERVER_EVENT_TYPE
{
    PACKET_ENTITY_DIED,
} PACKET_SERVER_EVENT_TYPE;

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

#endif
