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
 * ArpRemoteManager.h
 *
 * This class keeps track of all the remote devices available
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
 *	  shouldn't do this; we should probably stash a way the info
 *	  we need on the add-on in the files attributes so that we
 *	  can avoid opening it when first scanning add-ons, and need
 *	  to define a way to find out when ArpRemoteInterface
 *	  objects are no longer being used...
 *
 *	• Should abstract out commonality between this and ArpEmulatorManager.
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
 * 10/30/1998:
 *	Created this file.
 *
 */

#ifndef ARPTERMINAL_ARPREMOTEMANAGER_H
#define ARPTERMINAL_ARPREMOTEMANAGER_H

#ifndef ARPKERNEL_ARPADDONMANAGER_H
#include <ArpKernel/ArpAddonManager.h>
#endif

#ifndef ARPTERMINAL_ARPREMOTEADDON_H
#include <ArpTerminal/ArpRemoteAddon.h>
#endif

class ArpRemoteManager : public ArpAddonManager {
private:
	typedef ArpAddonManager inherited;

public:

	// Define the addition information we store about each add-on.
	class RemoteAddon : public BasicAddon {
	private:
		typedef BasicAddon inherited;
		
	public:
		RemoteAddon(BEntry* entry);
		RemoteAddon(const ArpRemoteAddon* remote);
		~RemoteAddon();
	
		virtual status_t InitCheck(void) const;
		
		const ArpRemoteAddon* GetRemote(void) const;
		
	private:
		const ArpRemoteAddon* interface;
	};

	ArpRemoteManager() : ArpAddonManager() { }
	
	/* This function can be used to place into the add-on list
	   remotes that are a static part of the executable.
	   Calling it results in an RemoteAddon being created
	   which has not been initialized; the ArpAddonManager
	   subclass ignores such objects.
	*/
	void AddStaticRemote(const ArpRemoteAddon* remote);
	
	/* Override the menu creator to extract many names from
	   the array of remote types available in each add-on.
	*/
	int32 CreateAddonMenu(BMenu* inmenu, BMessage* tmpl = NULL) const;
	
	/* Return the remote add-on that implements a particular
	   device/url name.
	*/
	const ArpRemoteAddon* RemoteAddonForURL(const char* name) const;
	
	/* Find the add-on that implements a remote on the given
	   name and ask it to create a new object for us.
	*/
	ArpRemoteInterface* AllocRemote(const char* name) const;
	
protected:
	virtual BasicAddon* AllocAddon(BEntry* entry);

};

#endif
