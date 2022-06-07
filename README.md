# ClientServer
Networking Project for creating a multiplexed Client and Server chat application

Creating two console applications for the client and the server respectively.

The client application starts by prompting the user for a username, before establishing a TCP connection with the server.

The client can send commands (using the special character $), and/or chat messages.

The client then requests registration with the server prior to sending messages and other commands such as “$get list” and “$get log”. Closes down the established communications at networking termination.

On the server program, a multiplexing logic is implemented and designed around the management of multiple client I/O. Also, reinforced sending and receiving as you respond to the messages sent by the clients.
