// =============================================================================
//    � BTSSocket.h
// =============================================================================
/*	Implementation of a socket object. Socket is not server/client specific,
	only does tcp/ip right now. */

/* This code was written by William Adams.
 */
 
/* April 28, 1997: Modification by Dianne Hackborn
                   (hackbod@lucent.com) to allow Recv() to do partial
                   reads and work with ArpHostName objects. */

#pragma once

#ifndef _B_BTSSOCKET_
#define _B_BTSSOCKET_

#include <kernel/OS.h>

#ifndef ARPNETWORK_ARPHOSTNAME_H
#include <ArpNetwork/ArpHostName.h>
#endif

// =============================================================================
class BTSSocket
{
	public:
						BTSSocket();
						BTSSocket(const int type, const int protocol , 
									const int family = AF_INET );
						BTSSocket(const int socketID);
		virtual			~BTSSocket() { Close(); }
		
		virtual int32	SetOption(const int level, const int option,
									char* data, 
									const unsigned int size) const;
		virtual int32	ConnectToAddress(const ArpHostName& host);
		virtual int32	BindTo(const ArpHostName& address);
		virtual	int32	Send(const char* buf, const int32 bufSize) const;
		virtual	int32	Recv(const char* buf, const int32 bufSize,
								int32* recvlen) const;
		virtual int32 	Open();
		virtual int32	Listen(const int maxConnections);
		virtual int32	Close();
		virtual bool	IsOpen() {return fID >= 0 ? true : false;}
		virtual int		ID() const {return fID;}
		
		virtual int32	SendLock() const;
		virtual int32	SendUnlock() const;
		virtual int32	RecvLock() const;
		virtual int32	RecvUnlock() const;
		
	private:
		
		ArpHostName		fHost;
		int				fID;
		const int		fFamily;
		int				fType;
		int				fProtocol;
		int 			fMaxConnections;
		
		sem_id			fSendSem;
		sem_id			fRecvSem;
		
		virtual void	Init();
		static void		UpdateSendCount(const int32 numBytes);
		static void		UpdateReceiveCount(const int32 numBytes);
};


#endif
