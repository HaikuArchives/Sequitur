#ifndef APRNETWORK_ARPHOSTNAME_H
#include "ArpNetwork/ArpHostName.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _NETDB_H
#include <posix/netdb.h>
#endif

#ifndef _LOCKER_H
#include <support/Locker.h>
#endif

#ifndef _AUTOLOCK_H
#include <support/Autolock.h>
#endif

#include <cstring>

ArpMOD();

// ---------------- support functions ----------------

static char** strarraydup(const char** sarray)
{
	ArpD(cdb << ADH << "ArpHostName: strarraydup " << sarray << std::endl);
	
	if( !sarray ) return NULL;
	
	int size=0;
	const char** pos;
	for(pos = sarray; *pos != NULL; pos++) size++;
	
	char** newarray = new char*[size+1];
	
	int i;
	for( i=0; i<size; i++ ) newarray[i] = strdup(sarray[i]);
	newarray[i] = NULL;
	
	return newarray;
}

static void freestrarray(char** sarray)
{
	ArpD(cdb << ADH << "ArpHostName: freestrarray " << sarray << std::endl);
	
	if( !sarray ) return;
	for(char** pos = sarray; *pos; pos++) {
		ArpD(cdb << ADH << "ArpHostName: free " << *pos << std::endl);
		free((void*)*pos);
	}
	delete[] sarray;
}

// ---------------- ArpHostName implementation ----------------

// This is used to arbitrate access to the host lookup function.
static BLocker dblock;
static void lock(void) {
	dblock.Lock();
}
static void unlock(void) {
	dblock.Unlock();
}

ArpHostName::ArpHostName(const char* host_in, int port_in)
	: name(""), ipaddr(0), error(B_BAD_VALUE)
{
	ArpD(cdb << ADH << "ArpHostName: constructing " << this << std::endl);
	
	memset(&host,0,sizeof(host));
	lock();
	
	try {
		if( host_in ) {
			ArpD(cdb << ADH << "ArpHostName: looking up "
								<< host_in << std::endl);
			struct hostent* he = ::gethostbyname(host_in);
			if( !he ) {
				error = h_errno;
				ArpD(cdb << ADH << "ArpHostName: error #" << h_errno << std::endl);
				unlock();
				return;
			}
			error = ENOERR;
			name = he->h_name;
			ipaddr = ntohl(*((ulong*)he->h_addr));
			GrabHostEnt(*he);
		} else {
			name = "";
			ipaddr = 0;
		}
			
		ArpD(cdb << ADH << "ArpHostName: storing socket info" << std::endl);
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons((int16)port_in);
		sockaddr.sin_addr.s_addr = htonl(IPAddress());
		::memset(sockaddr.sin_zero,0,sizeof(sockaddr.sin_zero));
	} catch(...) {
		error = B_ERROR;
	}
	
	unlock();
}

ArpHostName::ArpHostName(ulong addr, int port)
	: name(""), ipaddr(0), error(B_BAD_VALUE)
{
	ArpD(cdb << ADH << "ArpHostName: constructing " << this << std::endl);
	
	memset(&host,0,sizeof(host));
	lock();
	
	try {
		struct hostent* he = gethostbyaddr((char*)&addr,4,AF_INET);
		if( !he ) {
			error = h_errno;
			ArpD(cdb << ADH << "ArpHostName: error #" << h_errno << std::endl);
			unlock();
			return;
		}
		error = ENOERR;
	
		name = he->h_name;
		ipaddr = ntohl(*((ulong*)he->h_addr));
		GrabHostEnt(*he);
		
		sockaddr.sin_family = AF_INET;
		sockaddr.sin_port = htons(port);
		sockaddr.sin_addr.s_addr = htonl(IPAddress());
		::memset(sockaddr.sin_zero,0,sizeof(sockaddr.sin_zero));
		unlock();
	} catch(...) {
		error = B_ERROR;
	}
	
	unlock();
}

ArpHostName::ArpHostName(const ArpHostName& o)
{
	ArpD(cdb << ADH << "ArpHostName: constructing " << this << std::endl);
	memset(&host,0,sizeof(host));
	
	*this = o;
}

ArpHostName::~ArpHostName()
{
	ArpD(cdb << ADH << "ArpHostName: destroying " << this << std::endl);
	if( host.h_name ) free(host.h_name);
	if( host.h_aliases ) freestrarray(host.h_aliases);
	if( host.h_addr_list ) freestrarray(host.h_addr_list);
	ArpD(cdb << ADH << "ArpHostName: destroyed " << this << std::endl);
}

ArpHostName& ArpHostName::operator=(const ArpHostName& o)
{
	name = o.name;
	ipaddr = o.ipaddr;
	error = o.error;
	GrabHostEnt(o.host);
	sockaddr = o.sockaddr;
	return *this;
}

const char* ArpHostName::ErrorString() const
{
	switch( error ) {
		case B_NO_ERROR:		return "No error";
		case HOST_NOT_FOUND:	return "Unknown host name";
		case TRY_AGAIN:			return "Host name server busy";
		case NO_RECOVERY:		return "Unrecoverable system error";
		case NO_DATA:			return "No address data is available for this host name";
	}
	
	return "Unknown error";
}

void ArpHostName::GrabHostEnt(const struct hostent& he)
{
	ArpD(cdb << ADH << "ArpHostName: cleaning old entry" << std::endl);
	if( host.h_name ) free(host.h_name);
	if( host.h_aliases ) freestrarray(host.h_aliases);
	if( host.h_addr_list ) freestrarray(host.h_addr_list);
	ArpD(cdb << ADH << "ArpHostName: grabbing host entry" << std::endl);
	memset(&host,0,sizeof(host));
	if( he.h_name ) host.h_name = strdup(he.h_name);
	if( he.h_aliases ) host.h_aliases = strarraydup((const char**)he.h_aliases);
	host.h_addrtype = he.h_addrtype;
	host.h_length = he.h_length;
	if( he.h_addr_list ) host.h_aliases = strarraydup((const char**)he.h_addr_list);
}
