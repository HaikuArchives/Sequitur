/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpAddonManager.h
 *
 * This class keeps track of a set of add-ons in which a program
 * is interested.  It is derived from ArpMultiDir, providing
 * all of that class's functionality of searching through
 * multiple directories for files.  On top of that, the
 * ArpAddonManager keeps track of the add-ons it finds and
 * provides an interface for accessing them.
 *
 * The interface here is presented as something upon which more
 * specific add-on interfaces may be built; it defines nothing
 * about the internal structure of the add-ons it manipulates.
 *
 * Note on locking:
 * The ArpAddonManager is also a subclass of BLocker. All of
 * its functions make sure the object is locked before they do
 * anything, so you may not have to worry about locking it
 * yourself.  However, note that if you are doing anything
 * with a BasicAddon object in the manager, you must be sure
 * to lock the manager before you retrieve the addon object,
 * and keep it locked until you are done with it.  Otherwise,
 * you may let other threads trample over the addon object you
 * are using -- or even deallocate it from under you!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• Should check attributes on each file it finds, to make
 *	  sure they are actually add-ons.  Currently, we don't
 *	  find out about this <ahem> slight problem until actually
 *	  calling Open() on a BasicAddon object.
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
 * 7/22/97:
 *	Created this file.
 *
 */

#pragma once

#ifndef ARPKERNEL_ARPADDONMANAGER_H
#define ARPKERNEL_ARPADDONMANAGER_H

#ifndef ARPKERNEL_ARPMULTIDIR_H
#include <ArpKernel/ArpMultiDir.h>
#endif

#ifndef _IMAGE_H
#include <kernel/image.h>
#endif

#ifndef _PATH_H
#include <storage/Path.h>
#endif

#ifndef _LOCK_H
#include <support/Locker.h>
#endif

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _MENU_H
#include <interface/Menu.h>
#endif

// Removed multiple inheritance because the x86 compiler produced code that
// crashes when trying to lock it.
class ArpAddonManager : public ArpMultiDir /*, public BLocker*/ {

public:

	// This is the information about a single add-on that is
	// available.  It provides interfaces to find out basic
	// information about the add-on, and to load it into
	// memory.  The Open()/Close() interace is reference-based:
	// the add-on image will only be created once, and that same
	// image is returned in later calls to Open(); the add-on is
	// unloaded after Close() has been called for every Open().
	
	class BasicAddon {
	public:
		BasicAddon(BEntry* entry);
		BasicAddon(const char* path);
		virtual ~BasicAddon();
		
		virtual status_t InitCheck(void) const;
		virtual const BPath& Path(void) const;
		virtual const char* Name(void) const;
		virtual const char* LongName(void) const;
		
		image_id Open(void);
		void Close(void);
	
	private:
		BPath where;
		image_id image;
		int ref_count;
	};
	
	ArpAddonManager() : ArpMultiDir(), lock("ArpAddonManager") { }
	
	// Examine directories entered into ArpMultiDir for all
	// available add-ons.
	void Start(void);
	
	// Return a BasicAddon object with the given name.  This
	// is determined by comparing with each BasicAddon's Name().
	BasicAddon* FindAddon(const char* name) const;
	
	// Attach menu items to the given menu that represent the
	// available add-ons.
	int32 CreateAddonMenu(BMenu* inmenu, BMessage* tmpl = NULL) const;
	
	// A BList-like interface to the add-on objects.
	
	int32 CountAddons(void) const;
	BasicAddon* AddonAt(int32 index) const;
	
	void AddAddon(BasicAddon* addon);
	bool RemoveAddon(BasicAddon* addon);
	BasicAddon* RemoveAddon(int32 index);
	bool RemoveAddons(int32 index, int32 count);
	
	BLocker& Lock() const { return ((ArpAddonManager*)this)->lock; }
	operator BLocker&() const { return ((ArpAddonManager*)this)->lock; }
	
protected:

	// Override this if you have defined your own subclass of
	// BasicAddon that contains additional information specific
	// to your add-on implementation.
	virtual BasicAddon* AllocAddon(BEntry* entry);
	
	BLocker lock;
	
private:

	BList addons;
	
};

#endif
