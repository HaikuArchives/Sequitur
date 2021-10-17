// =============================================================================
//     BTSSocket.cpp
// =============================================================================

#include <ArpNetwork/BTSSocket.h>
#include <ArpKernel/ArpDebug.h>
#include <cstdio>
#include <support/Debug.h>
#include <errno.h>
#include <cstring>

ArpMOD();

// =============================================================================
//     BTSSocket
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
//     BTSSocket
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
//     BTSSocket
// =============================================================================
BTSSocket::BTSSocket(const int socketID) : fFamily(-1), fType(-1), fProtocol(-1)
{
	fID = socketID;
	Init();
	return;
}

// =============================================================================
//     Init
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
//     SetOption
// =============================================================================
int32
BTSSocket::SetOption(const int level, const int option, char* data, 
					const unsigned int size) const
{
	int32 result = ::setsockopt(fID, level, option, data, size);	
	ArpD(PRINT(("Result of socket option setting is %ld\n", result)));
	return result < 0 ? errno : result;
}
// =============================================================================
//     Open
// =============================================================================
int32
BTSSocket::Open()
{
	if (fID > -1)
	{
		::close(fID);
	}
	fID = ::socket(fFamily,  fType, fProtocol);

	return fID < 0 ? errno : fID;
}

// =============================================================================
//     SendLock
// =============================================================================
int32
BTSSocket::SendLock() const 
{
	return ::acquire_sem(fSendSem);
}

// =============================================================================
//     SendUnlock
// =============================================================================
int32
BTSSocket::SendUnlock() const 
{
	return ::release_sem(fSendSem);
}

// =============================================================================
//     RecvLock
// =============================================================================
int32
BTSSocket::RecvLock() const 
{
	return ::acquire_sem(fRecvSem);
}

// =============================================================================
//     RecvUnlock
// =============================================================================
int32
BTSSocket::RecvUnlock() const 
{
	return ::release_sem(fRecvSem);
}
// =============================================================================
//     ConnectToAddress
// =============================================================================
int32
BTSSocket::ConnectToAddress(const ArpHostName& host)
{
	int32 				result;
	fHost = host;
	const sockaddr_in* 	sockAddr = &fHost.SockAddr();
	ArpD(cdb << ADH << "Connect to: family=" << (sockAddr->sin_family)
				<< ", port=" << (sockAddr->sin_port)
				<< ", addr=" << (sockAddr->sin_addr.s_addr) << std::endl);
	result = ::connect(fID, (struct sockaddr*)sockAddr, 
						sizeof(*sockAddr));
	return result < 0 ? errno : result;
}

// =============================================================================
//     BindTo
// =============================================================================
int32	
BTSSocket::BindTo(const ArpHostName& host)
{
	int32	result;
	fHost = host;
	const 	sockaddr_in* sockAddr  = &fHost.SockAddr();
	
	result = ::bind(fID, (struct sockaddr*)sockAddr, 
						sizeof(*sockAddr));
	return result < 0 ? errno : result;
}

// =============================================================================
//     Listen
// =============================================================================
int32	
BTSSocket::Listen(const int maxConnections)
{
	fMaxConnections = maxConnections;
	int32 result = ::listen(fID, maxConnections);
	return result < 0 ? errno : result;
}

// =============================================================================
//     Close
// =============================================================================
int32
BTSSocket::Close()
{
	if (fID > -1)
	{
 		::close(fID);
 		fID = -1;
 	}
 	return 0;
}

// =============================================================================
//     Send
// =============================================================================
int32
BTSSocket::Send(const char* buf, const int32 bufSize) const
{
	// Sends the data for a BMessage over a socket, preceded by the message's
	// size.
	
	int32 	result = B_NO_ERROR;
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
	ArpD(PRINT( ("SOCKET SEND - EXIT, sent %d bytes on %d result is %s\n", sentBytes, 
				fID, strerror(result))));
	return result;
}

// =============================================================================
//     Recv
// =============================================================================
int32
BTSSocket::Recv(const char* buf, const int32 bufSize, int32* recvlen) const
{
	// Receives a network data buffer of a certain size. Does not return until
	// the buffer is full or if the socket returns 0 bytes (meaning it was 
	// closed) or returns an error besides EINTR. (EINTR can be generated when a
	// send() occurs on the same socket.
	
	int32 result = B_NO_ERROR;	// error value of socket calls
	int  receivedBytes = 0;		
	int  numBytes = 0;
	
	ArpD(PRINT(("SOCKET %d RECEIVE: ENTER bufSize=%ld, placelen=%p\n",
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
	ArpD(PRINT(("SOCKET %d RECEIVE - Received %d bytes result is %s\n", fID, numBytes,
				strerror(result))));

	if (result == EINTR && receivedBytes == bufSize) result = B_NO_ERROR;
	ArpD(PRINT(("SOCKET %d RECEIVE: EXIT\n", fID)));
	return result;
}

// =============================================================================
//     UpdateSendCount
// =============================================================================
void BTSSocket::UpdateSendCount(const int32 numBytes)
{
	static int32 sendCount = 0;
	sendCount += numBytes;
	ArpD(PRINT(("Total bytes sent: %ld\n", sendCount)));
	return;
}

// =============================================================================
//     UpdateReceiveCount
// =============================================================================
void BTSSocket::UpdateReceiveCount(const int32 numBytes)
{
	static int32 receiveCount = 0;
	receiveCount += numBytes;
	ArpD(PRINT(("Total bytes received: %ld\n", receiveCount)));
	return;
}
