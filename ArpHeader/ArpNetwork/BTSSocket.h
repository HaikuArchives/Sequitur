// =============================================================================
//    ¥ BTSSocket.h
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
		
		virtual long	SetOption(const int level, const int option,
									char* data, 
									const unsigned int size) const;
		virtual long	ConnectToAddress(const ArpHostName& host);
		virtual long	BindTo(const ArpHostName& address);
		virtual	long	Send(const char* buf, const long bufSize) const;
		virtual	long	Recv(const char* buf, const long bufSize,
								long* recvlen) const;
		virtual long 	Open();
		virtual long	Listen(const int maxConnections);
		virtual long	Close();
		virtual bool	IsOpen() {return fID >= 0 ? true : false;}
		virtual int		ID() const {return fID;}
		
		virtual long	SendLock() const;
		virtual long	SendUnlock() const;
		virtual long	RecvLock() const;
		virtual long	RecvUnlock() const;
		
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
		static void		UpdateSendCount(const long numBytes);
		static void		UpdateReceiveCount(const long numBytes);
};


#endif
