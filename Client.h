#include "../platform.h"
#include "../definitions.h"

class Client
{
private:
	int player;
	SOCKET clSocket;
	volatile bool active;//controls the activity of the clients run which is on its own thread.
	sockaddr_in clientAddr;
	short clientSeq;
	int snaps;

	GameState state;
	CriticalSection cs;//just incase you need to protect data, you might not need to.

public:
	inline Client() { state.gamePhase = WAITING; }

	int init(char* address, unsigned short port, byte player);
	int run();
	void stop();

	int sendInput(byte keyUp, byte keyDown, byte keyQuit);
	void getState(GameState* target);

private:
	int sendAlive();
	void sendClose();
};
