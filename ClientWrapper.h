#ifndef _CLIENT_WRAPPER_H_
#define _CLIENT_WRAPPER_H_

#include "../platform.h"
#include "../definitions.h"

EXPORTED int init(char* address, unsigned short port, byte player);
EXPORTED int run();
EXPORTED void stop();

EXPORTED int sendInput(byte keyUp, byte keyDown, byte keyQuit);
EXPORTED void getState(GameState* target);

#endif
