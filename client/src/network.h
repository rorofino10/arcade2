#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"

struct Client;

void NetworkPrepareBuffer();
void NetworkSendPacket(struct Client *client);
void NetworkRecievePacket(struct Client *client);
int NetworkPushInputShootEvent(ClientInputShootEvent event);
int NetworkPushInputAuthorativeMoveEvent(ClientInputAuthorativeMoveEvent event);
int NetworkPushInputMoveEvent(ClientInputEvent event);
#endif
