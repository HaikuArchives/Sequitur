/*
 * Copyright (c)1999 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * ----------------------------------------------------------------------
 *
 * ArpRemoteAddon.h
 *
 * This file describes the interface that remote device add-ons
 * supply in their global name space.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• This is a preliminary design, and subject to significant change.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 10/30/1998:
 *	Created file from ArpEmulatorAddon.h
 *
 */

#ifndef ARPTERMINAL_ARPREMOTEADDON_H
#define ARPTERMINAL_ARPREMOTEADDON_H

#include <support/SupportDefs.h>

#ifndef ARPTERMINAL_ARPREMOTEINTERFACE_H
#include <ArpTerminal/ArpRemoteInterface.h>
#endif

typedef enum {
	ArpRemote_Current = 1
} ArpRemoteVersion;

struct ArpRemoteAddon {
	ArpRemoteVersion InterfaceVersion;	// ArpRemote_Current
	
	const char* Name;			// Official long name of add-on
	const char* Company;		// Company that created add-on
	const char* Copyright;		// Copyright informtion
	
	const char* BuildDate;		// __DATE__
	const char* BuildTime;		// __TIME__
	
	int32 Version;				// Addon-specific version code
	
	const char* Description;	// Lots of enlightening information
	
	// These let you iterate through all of the devices implemented
	// by this add-on.
	int32 (*CountDevices)(void);
	const ArpRemoteType* (*DeviceType)(int32 idx);
	
	ArpRemoteInterface* (*AllocRemote)(const char* url);
};

typedef const ArpRemoteAddon* (ArpRemoteAddonFunc)(void);

#if ARPREMOTE_ADDON
	#if __POWERPC__
	#pragma export on
	#endif

	extern "C" _EXPORT const ArpRemoteAddon* GetRemoteAddon(void);

	#if __POWERPC__
	#pragma export reset
	#endif
#endif

#endif
