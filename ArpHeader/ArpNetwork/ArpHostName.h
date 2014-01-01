/*
 * Copyright (c)1997-98 by Dianne Hackborn.
 * All rights reserved.
 *
 * Classes to support networking, so we can avoid having to
 * deal with low-level socket calls.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpHostName.h
 *
 * A class that encapsulates information about a remote host.
 * It is primarily used to look up hosts by name.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 0.1: Created this file.
 *
 */

#pragma once

#ifndef ARPNETWORK_ARPHOSTNAME_H
#define ARPNETWORK_ARPHOSTNAME_H

#ifndef _NETDB_H
#include <net/netdb.h>
#endif

#ifndef _SOCKET_H
#include <net/socket.h>
#endif

#ifndef __errno_
#include <errno.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

class ArpHostName {

public:

	ArpHostName(const char* host = NULL, int port = 0);
	ArpHostName(ulong addr, int port);
	ArpHostName(const ArpHostName& o);
	~ArpHostName();
	
	// the returned value is in host order, -not- network order.
	uint32 IPAddress(void) const { return ipaddr; }
	const char* IPName(void) const { return (const char*)name; }
	const struct hostent& IPHost(void) const { return host; }
	const struct sockaddr_in& SockAddr() const { return sockaddr; }
	uint16 IPPort() const { return ntohs(sockaddr.sin_port); }
	
	status_t Error(void) const { return error; }
	const char* ErrorString(void) const;
	
	ArpHostName& operator=(const ArpHostName& o);
	
protected:

	void GrabHostEnt(const struct hostent& he);
	
	ArpString name;
	ulong ipaddr;				// host order
	struct hostent host;
	struct sockaddr_in sockaddr;
	status_t error;
};

#endif
