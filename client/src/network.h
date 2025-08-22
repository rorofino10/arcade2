#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"

struct Client;

void NetworkPrepareBuffer();
void NetworkSendPacket(struct Client *client);
void NetworkRecievePacket(struct Client *client);
int NetworkPushInputEvent(PACKET_INPUT_EVENT_TYPE type, void *data, uint16_t size);
#endif
