#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

typedef struct
{
    uint8_t type;
} ClientPacketHeader;

typedef struct
{
    int8_t x;
    int8_t y;
} ClientPacketData;

typedef struct
{
    ClientPacketHeader header;
    ClientPacketData data;
} ClientPacket;

#endif
