//
// Created by olivier on 23/10/2021.
//

#ifndef ONELIFE_SOCKET_H
#define ONELIFE_SOCKET_H

#include "../../../minorGems/util/SimpleVector.h"

namespace client::component
{
	class Socket
	{
		public:
			Socket(
				char* forceDisconnect,
				char* serverSocketConnected,
				double* connectedTime,
				SimpleVector<unsigned char>* serverSocketBuffer,
				int* numServerBytesRead,
				int* bytesInCount);

			~Socket();

			char readServerSocketFull( int inServerSocket );

		private:
			char* forceDisconnect;
			char* serverSocketConnected;
			double* connectedTime;
			SimpleVector<unsigned char>* serverSocketBuffer;
			int* numServerBytesRead;
			int* bytesInCount;
	};
}

int readFromSocket( int inHandle, unsigned char *inDataBuffer, int inBytesToRead );

#endif //ONELIFE_SOCKET_H
