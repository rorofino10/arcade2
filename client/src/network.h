#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include "packet.h"

struct Client;

void NetworkSetClient(struct Client *clientToSet);

int NetworkTryConnect();

void NetworkPrepareBuffer();
void NetworkSendPacket();
void NetworkRecievePacket();

int NetworkPushInputShootEvent(ClientInputShootEvent event);
int NetworkPushInputAuthorativeMoveEvent(ClientInputAuthorativeMoveEvent event);
int NetworkPushInputMoveEvent(ClientInputEvent event);
#endif
