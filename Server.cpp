// Server.cpp : Contains all functions of the server.
#include "Server.h"
#include <iostream>

using namespace std;

// Initializes the server. (NOTE: Does not wait for player connections!)
int Server::init(unsigned short port)
{
	initState();

	// TODO:
	//1) Set up a socket for listening.
	svSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (svSocket == INVALID_SOCKET)
	{
		cout << "socket error" << endl;
		return SETUP_ERROR;
	}

	unsigned long sockVal = 1;

	ioctlsocket(svSocket, FIONBIO, &sockVal);
	if (ioctlsocket(svSocket, FIONBIO, &sockVal) == SOCKET_ERROR)
	{
		int getError = WSAGetLastError();
	}
	
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	servAddr.sin_port = htons(port);

	int result = bind(svSocket, (SOCKADDR*)&servAddr, sizeof(servAddr));
	if (result == SOCKET_ERROR)
	{
		cout << "bind error" << endl;
		return BIND_ERROR;
	}

	//2) Mark the server as active.
	active = true;

	play0 = 0;
	play1 = 0;

	return SUCCESS;
}

// Updates the server; called each game "tick".
int Server::update()
{
	// TODO: 
	//1) Get player input and process it.
	//2) If any player's timer exceeds 50, "disconnect" the player.
	//3) Update the state and send the current snapshot to each player.

	NetworkMessage msg(_INPUT);
	sockaddr_in servAddr;

	int result = recvfromNetMessage(svSocket, msg, &servAddr); //check for message
	if (result > 0)
	{
		parseMessage(servAddr, msg);
		for (int i = 0; i < 2; i++)
		{
			if (playerTimer[i] >= 50) //timeout check
			{
				disconnectClient(i);
			}
		}
		updateState();
		sendState();
		return SUCCESS;
	}

	if (result == SOCKET_ERROR)
	{
		int getError = WSAGetLastError();
		if (getError == WSAEWOULDBLOCK)
		{
			for (int i = 0; i < 2; i++)
			{
				if (playerTimer[i] > 50)
				{
					disconnectClient(i);
				}
			}
			updateState();
			sendState();
			return SUCCESS;
		}
		else
		{
			return SHUTDOWN;
		}
	}

	return SUCCESS;
}

// Stops the server.
void Server::stop()
{
	// TODO:
	//1) Sends a "close" message to each client.
	sendClose();

	//2) Shuts down the server gracefully (update method should exit with SHUTDOWN code.)
	active = false;
	shutdown(svSocket, SD_BOTH);
	closesocket(svSocket);

	state.gamePhase = DISCONNECTED;
}

// Parses a message and responds if necessary. (private, suggested)
int Server::parseMessage(sockaddr_in& source, NetworkMessage& message)
{
	// TODO: Parse a message from client "source."
	byte msgRead = message.readByte();
	if (msgRead == CL_CONNECT)
	{
		if (noOfPlayers < 2)
		{
			sendOkay(source);
			connectClient(noOfPlayers, source);
			return SUCCESS;
		}
		else
		{
			sendFull(source);
			return SUCCESS;
		}
	}

	//player 0
	if (source.sin_addr.S_un.S_addr == playerAddress[0].sin_addr.S_un.S_addr && source.sin_port == playerAddress[0].sin_port)
	{
		if (msgRead == CL_KEYS)
		{
			state.player0.keyUp = message.readByte();
			state.player0.keyDown = message.readByte();
		}
		if (msgRead == CL_ALIVE)
		{
			playerTimer[0] = 0;
		}
		if (msgRead == SV_CL_CLOSE)
		{
			disconnectClient(0);
		}
	}
	//player 1
	else if (source.sin_addr.S_un.S_addr == playerAddress[1].sin_addr.S_un.S_addr && source.sin_port == playerAddress[1].sin_port)
	{
		if (msgRead == CL_KEYS)
		{
			state.player1.keyUp = message.readByte();
			state.player1.keyDown = message.readByte();
		}
		if (msgRead == CL_ALIVE)
		{
			playerTimer[1] = 0;
		}
		if (msgRead == SV_CL_CLOSE)
		{
			disconnectClient(1);
		}
	}

	return SUCCESS;
}

// Sends the "SV_OKAY" message to destination. (private, suggested)
int Server::sendOkay(sockaddr_in& destination)
{
	// TODO: Send "SV_OKAY" to the destination.
	NetworkMessage msg(_OUTPUT);
	if (destination.sin_addr.S_un.S_addr == playerAddress[0].sin_addr.S_un.S_addr && destination.sin_port == playerAddress[0].sin_port)
	{
		//player 0
		play0++;

		msg.writeShort(play0);
	}
	else
	{
		//player 1
		play1++;

		msg.writeShort(play1);
	}

	msg.writeByte(SV_OKAY);
	int result = sendtoNetMessage(svSocket, msg, &destination);
	if (result < 1)
	{
		return DISCONNECT;
	}
	return SUCCESS;
}

// Sends the "SV_FULL" message to destination. (private, suggested)
int Server::sendFull(sockaddr_in& destination)
{
	// TODO: Send "SV_FULL" to the destination.
	NetworkMessage msg(_OUTPUT);
	if (destination.sin_addr.S_un.S_addr == playerAddress[0].sin_addr.S_un.S_addr && destination.sin_port == playerAddress[0].sin_port)
	{
		play0++;

		msg.writeShort(play0);
	}
	if (destination.sin_addr.S_un.S_addr == playerAddress[1].sin_addr.S_un.S_addr && destination.sin_port == playerAddress[1].sin_port)
	{
		play1++;

		msg.writeShort(play1);
	}
	else
	{
		short notPlay = 0;
		msg.writeShort(notPlay);
	}

	msg.writeByte(SV_FULL);
	int result = sendtoNetMessage(svSocket, msg, &destination);
	if (result < 1)
	{
		return DISCONNECT;
	}

	return SUCCESS;

}

// Sends the current snapshot to all players. (private, suggested)
int Server::sendState()
{
	// TODO: Send the game state to each client.
	NetworkMessage play0State(_OUTPUT);
	play0++;
	play0State.writeShort(play0);
	play0State.writeByte(SV_SNAPSHOT);
	play0State.writeByte(state.gamePhase);
	play0State.writeShort(state.ballX);
	play0State.writeShort(state.ballY);
	play0State.writeShort(state.player0.y);
	play0State.writeShort(state.player0.score);
	play0State.writeShort(state.player1.y);
	play0State.writeShort(state.player1.score);
	int result = sendtoNetMessage(svSocket, play0State, &playerAddress[0]);

	NetworkMessage play1State(_OUTPUT);
	play1++;
	play1State.writeShort(play1);
	play1State.writeByte(SV_SNAPSHOT);
	play1State.writeByte(state.gamePhase);
	play1State.writeShort(state.ballX);
	play1State.writeShort(state.ballY);
	play1State.writeShort(state.player0.y);
	play1State.writeShort(state.player0.score);
	play1State.writeShort(state.player1.y);
	play1State.writeShort(state.player1.score);
	result = sendtoNetMessage(svSocket, play1State, &playerAddress[1]);

	return SUCCESS;
}

// Sends the "SV_CL_CLOSE" message to all clients. (private, suggested)
void Server::sendClose()
{
	// TODO: Send the "SV_CL_CLOSE" message to each client
	NetworkMessage msg(_OUTPUT);
	play0++;
	msg.writeByte(play0);
	msg.writeByte(SV_CL_CLOSE);
	int result = sendtoNetMessage(svSocket, msg, &playerAddress[0]);

	NetworkMessage msg1(_OUTPUT);
	play1++;
	msg.writeByte(play1);
	msg.writeByte(SV_CL_CLOSE);
	result = sendtoNetMessage(svSocket, msg1, &playerAddress[1]);
}

// Server message-sending helper method. (private, suggested)
int Server::sendMessage(sockaddr_in& destination, NetworkMessage& message)
{
	// TODO: Send the message in the buffer to the destination.

	return SUCCESS;
}

// Marks a client as connected and adjusts the game state.
void Server::connectClient(int player, sockaddr_in& source)
{
	playerAddress[player] = source;
	playerTimer[player] = 0;

	noOfPlayers++;
	if (noOfPlayers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = RUNNING;
}

// Marks a client as disconnected and adjusts the game state.
void Server::disconnectClient(int player)
{
	playerAddress[player].sin_addr.s_addr = INADDR_NONE;
	playerAddress[player].sin_port = 0;

	noOfPlayers--;
	if (noOfPlayers == 1)
		state.gamePhase = WAITING;
	else
		state.gamePhase = DISCONNECTED;
}

// Updates the state of the game.
void Server::updateState()
{
	// Tick counter.
	static int timer = 0;

	// Update the tick counter.
	timer++;

	// Next, update the game state.
	if (state.gamePhase == RUNNING)
	{
		// Update the player tick counters (for ALIVE messages.)
		playerTimer[0]++;
		playerTimer[1]++;

		// Update the positions of the player paddles
		if (state.player0.keyUp)
			state.player0.y--;
		if (state.player0.keyDown)
			state.player0.y++;

		if (state.player1.keyUp)
			state.player1.y--;
		if (state.player1.keyDown)
			state.player1.y++;
		
		// Make sure the paddle new positions are within the bounds.
		if (state.player0.y < 0)
			state.player0.y = 0;
		else if (state.player0.y > FIELDY - PADDLEY)
			state.player0.y = FIELDY - PADDLEY;

		if (state.player1.y < 0)
			state.player1.y = 0;
		else if (state.player1.y > FIELDY - PADDLEY)
			state.player1.y = FIELDY - PADDLEY;

		//just in case it get stuck...
		if (ballVecX)
			storedBallVecX = ballVecX;
		else
			ballVecX = storedBallVecX;

		if (ballVecY)
			storedBallVecY = ballVecY;
		else
			ballVecY = storedBallVecY;

		state.ballX += ballVecX;
		state.ballY += ballVecY;

		// Check for paddle collisions & scoring
		if (state.ballX < PADDLEX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player0.y && state.ballY < state.player0.y + PADDLEY)
			{
				state.ballX = PADDLEX;
				ballVecX *= -1;
			}
			// Otherwise, the second player has scored.
			else
			{
				newBall();
				state.player1.score++;
				ballVecX *= -1;
			}
		}
		else if (state.ballX >= FIELDX - PADDLEX - BALLX)
		{
			// If the ball has struck the paddle...
			if (state.ballY + BALLY > state.player1.y && state.ballY < state.player1.y + PADDLEY)
			{
				state.ballX = FIELDX - PADDLEX - BALLX - 1;
				ballVecX *= -1;
			}
			// Otherwise, the first player has scored.
			else
			{
				newBall();
				state.player0.score++;
				ballVecX *= -1;
			}
		}

		// Check for Y position "bounce"
		if (state.ballY < 0)
		{
			state.ballY = 0;
			ballVecY *= -1;
		}
		else if (state.ballY >= FIELDY - BALLY)
		{
			state.ballY = FIELDY - BALLY - 1;
			ballVecY *= -1;
		}
	}

	// If the game is over...
	if ((state.player0.score > 10 || state.player1.score > 10) && state.gamePhase == RUNNING)
	{
		state.gamePhase = GAMEOVER;
		timer = 0;
	}
	if (state.gamePhase == GAMEOVER)
	{
		if (timer > 30)
		{
			initState();
			state.gamePhase = RUNNING;
		}
	}
}

// Initializes the state of the game.
void Server::initState()
{
	playerAddress[0].sin_addr.s_addr = INADDR_NONE;
	playerAddress[1].sin_addr.s_addr = INADDR_NONE;
	playerTimer[0] = playerTimer[1] = 0;
	noOfPlayers = 0;

	state.gamePhase = DISCONNECTED;

	state.player0.y = 0;
	state.player1.y = FIELDY - PADDLEY - 1;
	state.player0.score = state.player1.score = 0;
	state.player0.keyUp = state.player0.keyDown = false;
	state.player1.keyUp = state.player1.keyDown = false;

	newBall(); // Get new random ball position

	// Get a new random ball vector that is reasonable
	ballVecX = (rand() % 10) - 5;
	if ((ballVecX >= 0) && (ballVecX < 2))
		ballVecX = 2;
	if ((ballVecX < 0) && (ballVecX > -2))
		ballVecX = -2;

	ballVecY = (rand() % 10) - 5;
	if ((ballVecY >= 0) && (ballVecY < 2))
		ballVecY = 2;
	if ((ballVecY < 0) && (ballVecY > -2))
		ballVecY = -2;


}

// Places the ball randomly within the middle half of the field.
void Server::newBall()
{
	// (randomly in 1/2 playable area) + (1/4 playable area) + (left buffer) + (half ball)
	state.ballX = (rand() % ((FIELDX - 2 * PADDLEX - BALLX) / 2)) +
						((FIELDX - 2 * PADDLEX - BALLX) / 4) + PADDLEX + (BALLX / 2);

	// (randomly in 1/2 playable area) + (1/4 playable area) + (half ball)
	state.ballY = (rand() % ((FIELDY - BALLY) / 2)) + ((FIELDY - BALLY) / 4) + (BALLY / 2);
}
