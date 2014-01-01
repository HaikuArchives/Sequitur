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
 * ArpEmulatorAddon.h
 *
 * This file describes the interface that emulator add-ons
 * supply in their global name space.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• I am not currently trying to maintain binary compatibility with
 *	  new versions of this interface.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 7/19/97:
 *	Created file from ArpEmulatorInterface.h
 *
 */

#ifndef ARPTERMINAL_ARPEMULATORADDON_H
#define ARPTERMINAL_ARPEMULATORADDON_H

#include <be/support/SupportDefs.h>
#if !defined(_EXPORT)
#define _EXPORT
#endif

#ifndef ARPTERMINAL_ARPEMULATORINTERFACE_H
#include <ArpTerminal/ArpEmulatorInterface.h>
#endif

typedef enum {
	ArpEmulator_Current = 1
} ArpEmulatorVersion;

struct ArpEmulatorAddon {
	ArpEmulatorVersion InterfaceVersion;	// ArpEmulator_Current
	
	const char* Name;			// Official long name of add-on
	const char* Company;		// Company that created add-on
	const char* Copyright;		// Copyright informtion
	
	const char* BuildDate;		// __DATE__
	const char* BuildTime;		// __TIME__
	
	int32 Version;				// Addon-specific version code
	
	const char* Description;	// Lots of enlightening information
	
	int32 (*CountEmulations)(void);
	const ArpEmulationType* (*EmulationType)(int32 idx);
	ArpEmulatorInterface* (*AllocEmulator)(const char* name,
											ArpTerminalInterface& terminal);
};

typedef const ArpEmulatorAddon* (ArpEmulatorAddonFunc)(void);

#if ARPTERMINAL_ADDON
	#if __POWERPC__
	#pragma export on
	#endif

	extern "C" _EXPORT const ArpEmulatorAddon* GetEmulatorAddon(void);

	#if __POWERPC__
	#pragma export reset
	#endif
#endif

#endif
