// Client.cpp : handles all client network functions.
#include "Client.h"
#include "../NetworkMessage.h"
#include <iostream>

using namespace std;

// Initializes the client; connects to the server.
int Client::init(char* address, unsigned short port, byte _player)
{
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;
	state.gamePhase = WAITING;
	// TODO:
	//1) Set the player.
	player = _player;

	//2) Set up the connection.
	clSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clSocket == INVALID_SOCKET)
	{
		return SETUP_ERROR;
	}

	clientAddr.sin_family = AF_INET;
	clientAddr.sin_addr.S_un.S_addr = inet_addr(address);
	clientAddr.sin_port = htons(port);
	if (clientAddr.sin_addr.S_un.S_addr == INADDR_NONE)
	{
		return ADDRESS_ERROR;
	}
	
	//3) Connect to the server.
	connect(clSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
	NetworkMessage msg(_OUTPUT);
	msg.writeByte(CL_CONNECT);
	msg.writeByte(_player);

	int result = sendNetMessage(clSocket, msg);
	if (result < 1)
	{
		int getError = WSAGetLastError();
		return DISCONNECT;
	}

	//4) Get response from server.
	NetworkMessage servMsg(_INPUT);
	result = recvNetMessage(clSocket, servMsg);
	if (result < 1)
	{
		return DISCONNECT;
	}
	clientSeq = servMsg.readShort();
	
	//5) Make sure to mark the client as running.
	byte servRead = servMsg.readByte();
	if (servRead == SV_OKAY)
	{
		active = true;
		return SUCCESS;
	}

	if (servRead == SV_FULL)
	{
		active = false;
		return SHUTDOWN;
	}

	return SHUTDOWN;
}

// Receive and process messages from the server.
int Client::run()
{
	// TODO: Continuously process messages from the server aslong as the client running.
	// HINT: Set game phase to DISCONNECTED on SV_CL_CLOSE! (Try calling stop().)
	// HINT: You can keep track of the number of snapshots with a static variable...
	snaps = 0; //reset snapshots

	while (active) //loop while active
	{
		NetworkMessage servMsg(_INPUT);
		int result = recvNetMessage(clSocket, servMsg);

		if (result < 1)
		{
			return MESSAGE_ERROR;
		}

		short tempRead = servMsg.readShort();
		if (tempRead > clientSeq)
		{
			clientSeq = tempRead;
			byte servRead = servMsg.readByte();
			if (servRead == SV_CL_CLOSE)
			{
				stop();
				return SHUTDOWN;
			}
			if (servRead == SV_SNAPSHOT)
			{
				snaps++;

				//player data
				state.gamePhase = servMsg.readByte();
				state.ballX = servMsg.readShort();
				state.ballY = servMsg.readShort();
				state.player0.y = servMsg.readShort();
				state.player0.score = servMsg.readShort();
				state.player1.y = servMsg.readShort();
				state.player1.score = servMsg.readShort();

				if (snaps == 10)
				{
					sendAlive();
					snaps = 0;
				}
			}
		}
	}
}

// Clean up and shut down the client.
void Client::stop()
{
	// TODO:
	//1) Make sure to send a SV_CL_CLOSE message.
	sendClose();

	//2) Make sure to mark the client as shutting down and close socket.
	shutdown(clSocket, SD_BOTH);
	closesocket(clSocket);

	//3) Set the game phase to DISCONNECTED.
	state.gamePhase = DISCONNECTED;

}

// Send the player's input to the server.
int Client::sendInput(byte keyUp, byte keyDown, byte keyQuit)
{
	if (keyQuit)
	{
		stop();
		return SHUTDOWN;
	}

	cs.enter();
	if (player == 0)
	{
		state.player0.keyUp = keyUp;
		state.player0.keyDown = keyDown;
	}
	else
	{
		state.player1.keyUp = keyUp;
		state.player1.keyDown = keyDown;
	}
	cs.leave();

	//TODO:	Transmit the player's input status.
	NetworkMessage playerData(_OUTPUT);
	playerData.writeByte(CL_KEYS);
	playerData.writeByte(keyUp);
	playerData.writeByte(keyDown);

	sendNetMessage(clSocket, playerData);

	return SUCCESS;
}

// Copies the current state into the struct pointed to by target.
void Client::getState(GameState* target)
{
	// TODO: Copy state into target.
	memcpy(target, &state, sizeof(GameState));
}

// Sends a SV_CL_CLOSE message to the server (private, suggested)
void Client::sendClose()
{
	// TODO: Send a CL_CLOSE message to the server.
	NetworkMessage msg(_OUTPUT);
	msg.writeByte(SV_CL_CLOSE);
	sendNetMessage(clSocket, msg);
}

// Sends a CL_ALIVE message to the server (private, suggested)
int Client::sendAlive()
{
	// TODO: Send a CL_ALIVE message to the server.
	NetworkMessage msg(_OUTPUT);
	msg.writeByte(CL_ALIVE);
	sendNetMessage(clSocket, msg);

	return SUCCESS;
}
