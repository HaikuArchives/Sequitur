// =============================================================================
//    ¥ BTSSocket.cpp
// =============================================================================

#include <ArpNetwork/BTSSocket.h>
#include <ArpKernel/ArpDebug.h>
#include <stdio.h>
#include <support/Debug.h>
#include <errno.h>
#include <string.h>

ArpMOD();

// =============================================================================
//    ¥ BTSSocket
// =============================================================================
BTSSocket::BTSSocket() : fFamily(AF_INET), fType(SOCK_STREAM), 
						fProtocol(IPPROTO_TCP)
{
	int		ones = 0xFFFFFFFF;
	int		result;
	
	fID = ::socket(fFamily,  fType, fProtocol);
	result = SetOption(SOL_SOCKET, SO_NONBLOCK, (char*)&ones, sizeof(int));
	
	Init();
	return;
}

// =============================================================================
//    ¥ BTSSocket
// =============================================================================

BTSSocket::BTSSocket(const int type, const int protocol, const int family) :
			fFamily(family)
{
	int		ones = 0xFFFFFFFF;
	int		result;
	
	if (protocol == 0) fProtocol = IPPROTO_TCP;
	else
		fProtocol = protocol;

	if (type == 0) fType  = SOCK_STREAM; 
	else
		fType = type;
		
	fID = ::socket(fFamily,  fType, fProtocol);
	result = SetOption(SOL_SOCKET, SO_NONBLOCK, (char*)&ones, sizeof(int));
	Init();
	return;	
}

// =============================================================================
//    ¥ BTSSocket
// =============================================================================
BTSSocket::BTSSocket(const int socketID) : fFamily(-1), fType(-1), fProtocol(-1)
{
	fID = socketID;
	Init();
	return;
}

// =============================================================================
//    ¥ Init
// =============================================================================
void
BTSSocket::Init()
{
	char 	sendSemName[30];
	char	recvSemName[30];
	::sprintf(sendSemName, "Send Sem %d", fID);
	::sprintf(recvSemName, "Recv Sem %d", fID);
	fSendSem = create_sem(1, sendSemName);
	fRecvSem = create_sem(1, recvSemName);
	return;
}

// =============================================================================
//    ¥ SetOption
// =============================================================================
long
BTSSocket::SetOption(const int level, const int option, char* data, 
					const unsigned int size) const
{
	long result = ::setsockopt(fID, level, option, data, size);	
	ArpD(PRINT(("Result of socket option setting is %d\n", result)));
	return result < 0 ? errno : result;
}
// =============================================================================
//    ¥ Open
// =============================================================================
long
BTSSocket::Open()
{
	if (fID > -1)
	{
		::closesocket(fID);
	}
	fID = ::socket(fFamily,  fType, fProtocol);

	return fID < 0 ? errno : fID;
}

// =============================================================================
//    ¥ SendLock
// =============================================================================
long
BTSSocket::SendLock() const 
{
	return ::acquire_sem(fSendSem);
}

// =============================================================================
//    ¥ SendUnlock
// =============================================================================
long
BTSSocket::SendUnlock() const 
{
	return ::release_sem(fSendSem);
}

// =============================================================================
//    ¥ RecvLock
// =============================================================================
long
BTSSocket::RecvLock() const 
{
	return ::acquire_sem(fRecvSem);
}

// =============================================================================
//    ¥ RecvUnlock
// =============================================================================
long
BTSSocket::RecvUnlock() const 
{
	return ::release_sem(fRecvSem);
}
// =============================================================================
//    ¥ ConnectToAddress
// =============================================================================
long
BTSSocket::ConnectToAddress(const ArpHostName& host)
{
	long 				result;
	fHost = host;
	const sockaddr_in* 	sockAddr = &fHost.SockAddr();
	ArpD(cdb << ADH << "Connect to: family=" << (sockAddr->sin_family)
				<< ", port=" << (sockAddr->sin_port)
				<< ", addr=" << (sockAddr->sin_addr.s_addr) << endl);
	result = ::connect(fID, (struct sockaddr*)sockAddr, 
						sizeof(*sockAddr));
	return result < 0 ? errno : result;
}

// =============================================================================
//    ¥ BindTo
// =============================================================================
long	
BTSSocket::BindTo(const ArpHostName& host)
{
	long	result;
	fHost = host;
	const 	sockaddr_in* sockAddr  = &fHost.SockAddr();
	
	result = ::bind(fID, (struct sockaddr*)sockAddr, 
						sizeof(*sockAddr));
	return result < 0 ? errno : result;
}

// =============================================================================
//    ¥ Listen
// =============================================================================
long	
BTSSocket::Listen(const int maxConnections)
{
	fMaxConnections = maxConnections;
	long result = ::listen(fID, maxConnections);
	return result < 0 ? errno : result;
}

// =============================================================================
//    ¥ Close
// =============================================================================
long
BTSSocket::Close()
{
	if (fID > -1)
	{
 		::closesocket(fID);
 		fID = -1;
 	}
 	return 0;
}

// =============================================================================
//    ¥ Send
// =============================================================================
long
BTSSocket::Send(const char* buf, const long bufSize) const
{
	// Sends the data for a BMessage over a socket, preceded by the message's
	// size.
	
	long 	result = B_NO_ERROR;
	int 	numBytes = -1;
	int		sentBytes = 0;
	ArpD(PRINT(( "SOCKET SEND - ENTER, %ld bytes on socket %d\n", bufSize, fID)));
	if (bufSize > 0 && buf != NULL)
	{
		while (sentBytes < bufSize && result == B_NO_ERROR || result == EINTR)
		{
			ArpD(PRINT(("SOCKET SEND - Sending data..\n")));
			numBytes = ::send(fID, buf, bufSize, 0);
			if (numBytes < 0) result = errno;
			if (numBytes > 0) sentBytes += numBytes;
			if (sentBytes < numBytes) result = errno;
			#if DEBUG
			else UpdateSendCount(sentBytes);
			#endif
		}
	}	
	ArpD(PRINT( ("SOCKET SEND - EXIT, sent %ld bytes on %d result is %s\n", sentBytes, 
				fID, strerror(result))));
	return result;
}

// =============================================================================
//    ¥ Recv
// =============================================================================
long
BTSSocket::Recv(const char* buf, const long bufSize, long* recvlen) const
{
	// Receives a network data buffer of a certain size. Does not return until
	// the buffer is full or if the socket returns 0 bytes (meaning it was 
	// closed) or returns an error besides EINTR. (EINTR can be generated when a
	// send() occurs on the same socket.
	
	long result = B_NO_ERROR;	// error value of socket calls
	int  receivedBytes = 0;		
	int  numBytes = 0;
	
	ArpD(PRINT(("SOCKET %d RECEIVE: ENTER bufSize=%d, placelen%x=\n",
				fID, bufSize, recvlen)));

	while ( receivedBytes < bufSize
				&& ( result == B_NO_ERROR
					|| (!recvlen && result == EINTR) ) )
	{
		ArpD(PRINT(("Receiving %ld bytes on %d\n", bufSize- receivedBytes, ID())));
		numBytes = ::recv(fID, (char*)(buf+receivedBytes), 
							bufSize - receivedBytes, 0);
		
		ArpD(PRINT(("RECEIVE RETURNED: RETURN = %d\n",numBytes)));
		
		if (numBytes == 0)
		{
			result = ECONNABORTED;
			break;
		}
		else if (numBytes < 0) 
		{
			ArpD(PRINT(("error when receiving data!\n")));
			result = errno;
			ArpD(PRINT(("return error = %s, errno = %s\n",
					strerror(numBytes), strerror(result))));
			if( result == EINTR ) break;
		}
		else 
		{
			receivedBytes += numBytes;
			#if DEBUG
			UpdateReceiveCount(numBytes);
			#endif
			if( recvlen ) {
				*recvlen = numBytes;
				if( result == EINTR ) result = B_NO_ERROR;
				break;
			}
		}
	}
	ArpD(PRINT(("SOCKET %d RECEIVE - Received %ld bytes result is %s\n", fID, numBytes,
				strerror(result))));

	if (result == EINTR && receivedBytes == bufSize) result = B_NO_ERROR;
	ArpD(PRINT(("SOCKET %d RECEIVE: EXIT\n", fID)));
	return result;
}

// =============================================================================
//    ¥ UpdateSendCount
// =============================================================================
void BTSSocket::UpdateSendCount(const long numBytes)
{
	static long sendCount = 0;
	sendCount += numBytes;
	ArpD(PRINT(("Total bytes sent: %ld\n", sendCount)));
	return;
}

// =============================================================================
//    ¥ UpdateReceiveCount
// =============================================================================
void BTSSocket::UpdateReceiveCount(const long numBytes)
{
	static long receiveCount = 0;
	receiveCount += numBytes;
	ArpD(PRINT(("Total bytes received: %ld\n", receiveCount)));
	return;
}
