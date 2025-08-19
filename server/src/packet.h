#ifndef PACKET_H
#define PACKET_H
#define MAX_PACKET_SIZE 1400

#include <stdint.h>

typedef enum
{
    PACKET_INPUT_EVENT,
} PACKET_TYPE;

typedef struct
{
    uint8_t type;
    uint16_t size;
} ClientPacketHeader;

typedef struct
{
    float dx;
    float dy;
} PacketShootEvent;

typedef struct
{
    int16_t nx;
    int16_t ny;
} PacketMoveEvent;

typedef enum PACKET_INPUT_EVENT_TYPE
{
    PACKET_INPUT_SHOOT,
    PACKET_INPUT_MOVE,
} PACKET_INPUT_EVENT_TYPE;
typedef struct
{
    uint8_t type;
    uint16_t size;
} ClientEventHeader;

#endif
