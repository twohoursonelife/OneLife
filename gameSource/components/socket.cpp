//
// Created by olivier on 23/10/2021.
//

#include "socket.h"
#include <cstdio>
#include "minorGems/game/game.h"

client::component::Socket::Socket(
	char* forceDisconnect,
	char* serverSocketConnected,
	double* connectedTime,
	SimpleVector<unsigned char>* serverSocketBuffer,
	int* numServerBytesRead,
	int* bytesInCount)
{
	this->forceDisconnect = forceDisconnect;
	this->serverSocketConnected = serverSocketConnected;
	this->connectedTime = connectedTime;
	this->serverSocketBuffer = serverSocketBuffer;
	this->numServerBytesRead = numServerBytesRead;
	this->bytesInCount = bytesInCount;
}

client::component::Socket::~Socket() {}

/**
 *
 * @param inServerSocket
 * @return
 * @note reads all waiting data from socket and stores it in buffer returns false on socket error
 */
char client::component::Socket::readServerSocketFull( int inServerSocket )
{

	if( *(this->forceDisconnect) )
	{
		*(this->forceDisconnect) = false;
		return false;
	}


	unsigned char buffer[512];

	int numRead = readFromSocket( inServerSocket, buffer, 512 );


	while( numRead > 0 ) {
		if( ! *(this->serverSocketConnected) ) {
			*(this->serverSocketConnected) = true;
			*(this->connectedTime) = game_getCurrentTime();
		}

		(*(this->serverSocketBuffer)).appendArray( buffer, numRead );
		*(this->numServerBytesRead) += numRead;
		*(this->bytesInCount) += numRead;

		numRead = readFromSocket( inServerSocket, buffer, 512 );
	}

	if( numRead == -1 ) {
		printf( "Failed to read from server socket at time %f\n",
				game_getCurrentTime() );
		return false;
	}

	return true;
}