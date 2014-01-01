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
 * ArpEmulatorManager.h
 *
 * This class keeps track of all the emulations available
 * to the terminal -- both those that are a static part of
 * the program, and through external add-ons.  There is usually
 * only one of these objects for an entire application.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• This class keeps the image of every add-on it finds loaded
 *	  in memory for the lifetime of the object.  It really really
 *	  shouldn't do this; we should probably stash away the info
 *	  we need on the add-on in the file's attributes so that we
 *	  can avoid opening it when first scanning add-ons, and need
 *	  to define a way to find out when ArpEmulatorInterface
 *	  objects are no longer being used...
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
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 7/22/97:
 *	Created this file.
 *
 */

#ifndef ARPTERMINAL_ARPEMULATORMANAGER_H
#define ARPTERMINAL_ARPEMULATORMANAGER_H

#ifndef ARPKERNEL_ARPADDONMANAGER_H
#include <ArpKernel/ArpAddonManager.h>
#endif

#ifndef ARPTERMINAL_ARPEMULATORADDON_H
#include <ArpTerminal/ArpEmulatorAddon.h>
#endif

#ifndef ARPTERMINAL_ARPTERMINALINTERFACE_H
#include <ArpTerminal/ArpTerminalInterface.h>
#endif

class ArpEmulatorManager : public ArpAddonManager {
private:
	typedef ArpAddonManager inherited;

public:

	// Define the addition information we store about each add-on.
	class EmulatorAddon : public BasicAddon {
	private:
		typedef BasicAddon inherited;
		
	public:
		EmulatorAddon(BEntry* entry);
		EmulatorAddon(const ArpEmulatorAddon* emulator);
		~EmulatorAddon();
	
		virtual status_t InitCheck(void) const;
		
		const ArpEmulatorAddon* GetEmulator(void) const;
		
	private:
		const ArpEmulatorAddon* interface;
	};

	ArpEmulatorManager() : ArpAddonManager() { }
	
	/* This function can be used to place into the add-on list
	   emulators that are a static part of the executable.
	   Calling it results in an EmulatorAddon being created
	   which has not been initialized; the ArpAddonManager
	   subclass ignores such objects.
	*/
	void AddStaticEmulator(const ArpEmulatorAddon* emulator);
	
	/* Override the menu creator to extract many names from
	   the array of emulation types available in each add-on.
	*/
	int32 CreateAddonMenu(BMenu* inmenu, BMessage* tmpl = NULL) const;
	
	/* Return the emulator add-on that implements a particular
	   terminal type name.
	*/
	const ArpEmulatorAddon* EmulatorAddonForType(const char* name) const;
	
	/* Find the add-on that implements an emulation on the given
	   name and ask it to create a new object for us.
	*/
	ArpEmulatorInterface* AllocEmulator(const char* name,
									ArpTerminalInterface* terminal) const;
	
protected:
	virtual BasicAddon* AllocAddon(BEntry* entry);

};

#endif
