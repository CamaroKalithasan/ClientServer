#include "ClientWrapper.h"
#include "Client.h"

Client* client = new Client();

int init(char* address, unsigned short port, byte player)
{
	if (startup())
		return STARTUP_ERROR;

	return client->init(address, port, player);
	stop();
}

int run()
{
	return client->run();
}

void stop()
{
	client->stop();
	shutdown();
}

int sendInput(byte keyUp, byte keyDown, byte keyQuit)
{
	return client->sendInput(keyUp, keyDown, keyQuit);
}

void getState(GameState* target)
{
	return client->getState(target);
}
