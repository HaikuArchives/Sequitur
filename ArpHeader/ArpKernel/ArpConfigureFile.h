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
 * ArpConfigureFile.h
 *
 * Store and retrieve a set of ArpConfigurableI objects as a
 * single preferences file on-disk.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• Should probably store settings through libprefs, rather than
 *	  directly in files.
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 1/10/1999:
 *	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPCONFIGUREFILE_H
#define ARPKERNEL_ARPCONFIGUREFILE_H

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef _MESSAGE_H
#include <be/app/Message.h>
#endif

#ifndef _HANDLER_H
#include <be/app/Handler.h>
#endif

#ifndef _MESSENGER_H
#include <be/app/Messenger.h>
#endif

#ifndef _ENTRY_H
#include <be/storage/Entry.h>
#endif

#ifndef _FIND_DIRECTORY
#include <be/storage/FindDirectory.h>
#endif

#ifndef _PATH_H
#include <be/storage/Path.h>
#endif

#ifndef __BSTRING__
#include <be/support/String.h>
#endif

// forward references

class ArpConfigureFile : BHandler
{
public:
	// Constructor and destructor.
	ArpConfigureFile();
	virtual ~ArpConfigureFile();

	// Copy and assignment.
	ArpConfigureFile(const ArpConfigureFile& o);
	ArpConfigureFile& operator=(const ArpConfigureFile& o);
	
	// Add a new ArpConfigurableI object to this configuration file.
	// If 'name' is supplied, the settings values in this configurable
	// will be added to the file's configuration as a child BMessage
	// with that name.   Otherwise, its settings will be merged into
	// the root BMessage.
	status_t AddConfig(ArpConfigurableI* object, const char* name = 0);
	
	// Return vectors of the configurable objects and their names.
	const ArpVectorI<ArpConfigurableI*>& Configs() const;
	const ArpVectorI<BString>& Names() const;
	
	// These functions get and set the destination of messages
	// about changes.  When set, the object keeps a node monitor
	// active on the configure file, which will result in messages
	// being sent to you when it changes; if not set (the default),
	// the only information it will know or remember about the file
	// is the name you supplied in SetFile() below.
	//
	// Some notes about how this works:
	// The BMessenger you pass in will be given BMessage objects about
	// changes to the file, much like the node monitor.  However, you
	// will not be the direct recipient of them -- instead, the
	// ArpConfigureFile will add itself to your own BLooper, and direct
	// the node monitor messages to itself.  It uses these messages
	// to keep its file lead name in sync with the actual file, and
	// then sends them on to you.  When a message is sent to you, it
	// is copied into the 'message' BMessage you supplied, allowing you
	// to add fields and set the 'what' field to be easier to identify.
	//
	// Note that one implication of this is that the BLooper you pass
	// in from the BMessenger must exist in the same team as the
	// ArpConfigureFile...  however, this shouldn't be a problem, as
	// the node monitor imposes that same restriction.
	//
	// Note also that calling StartWatching() implies that this
	// ArpConfigureFile object is owned by the destination looper --
	// if you are accessing from some other thread, you must lock
	// the looper before doing so.
	BMessenger Watcher() const;
	status_t StartWatching(const BMessenger& destination, BMessage* message);
	void StopWatching();
	
	// Get and set the configuration file.  If a Watcher() destination
	// has previously been set, any currently active node monitor will
	// be stopped and a new one started on this file.
	entry_ref EntryRef() const;
	status_t SetFile(const entry_ref& file);
	
	// Same as the above, but in terms of paths rather than entry_ref's.
	BPath Path() const;
	status_t SetFile(const BPath& file);
	
	// Set to the given path and filename, relative to a specific
	// system directory.  Any directories in 'path' that do not
	// exist will be created for you.
	status_t SetFile(const char* path, const char* name,
					 directory_which base = B_USER_SETTINGS_DIRECTORY);
	
	// Set the name of the settings file, keeping the current path.
	status_t SetFile(const char* name);
	
	// Return true if a file has been set for this object, else false.
	bool HasFile() const;
	
	// Get and set the MIME type of files written by this object.
	void SetMimeType(const char* mimetype);
	const char* MimeType() const;
	
	// Generate and apply a BMessage containing settings for the entire
	// set of ArpConfigurableI objects.
	status_t MakeSettings(BMessage* in) const;
	status_t ApplySettings(const BMessage* from);
	
	// Read and write the configuration file using an explicit
	// BMessage object.
	status_t WriteSettings(const BMessage* from) const;
	status_t ReadSettings(BMessage* to) const;
	
	// Read and write the configuration file.
	status_t WriteSettings() const;
	status_t ReadSettings();
	
	// Handle messages from the node monitor, and redistribute them
	// to this object's watcher.
	virtual	void MessageReceived(BMessage *message);
	
private:
	typedef BHandler inherited;
	
	void Initialize(void);
	void FreeMemory(void);
	status_t StartWatcher(void);
	status_t StopWatcher(void);
	
	// Configurable objects.
	ArpVectorI<ArpConfigurableI*>* mConfigs;
	ArpVectorI<BString>* mNames;
	
	// File management.  The file should probably be stored as an
	// entry_ref rather than a BPath.
	entry_ref mFile;
	BString mMimeType;
	
	// Node monitoring.
	BMessenger mWatcher;
	BMessage* mWatchMessage;
	node_ref mNode;
};

#endif
