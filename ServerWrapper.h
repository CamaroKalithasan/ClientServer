#ifndef _SERVER_WRAPPER_H_
#define _SERVER_WRAPPER_H_

#include "../platform.h"
#include "../definitions.h"

EXPORTED int init(unsigned short port);
EXPORTED int update();
EXPORTED void stop();

#endif
