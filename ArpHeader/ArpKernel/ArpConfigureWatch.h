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
 * ArpConfigureWatch.h
 *
 * This is a helper class for implementing objects that can have
 * configuration views actively changing/watching them.
 *
 * Note that you should NOT inherit from this class, but just include
 * an instance as a member of your own class.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 05/04/1999:
 *	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPCONFIGUREWATCH_H
#define ARPKERNEL_ARPCONFIGUREWATCH_H

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#include <ArpKernel/ArpConfigurableI.h>
#endif

#ifndef _MESSAGE_H
#include <be/app/Message.h>
#endif

#ifndef _MESSENGER_H
#include <be/app/Messenger.h>
#endif

#ifndef _INVOKER_H
#include <be/app/Invoker.h>
#endif

class ArpConfigureWatch : public ArpDirectConfigurableI
{
public:
	// Form when you are a BHandler.
	ArpConfigureWatch(BHandler* object, ArpConfigurableI* configure);
	// Form when you are using the ArpDirectConfigurableI interface.
	// Note that with this style the ArpConfigurableI interface
	// you implement must be thread safe!
	ArpConfigureWatch();
	
	virtual ~ArpConfigureWatch();

	// ------ High-Level Updating And Reporting Controls ------
	
	// Handle standard configuration messages.  Returns B_OK if 'message'
	// is something it understands, or an error if it isn't.  Currently
	// implements standard functionality for these messages:
	//
	// ARP_PUT_CONFIGURATION_MSG:
	//   Extract configuration and call your objects PutConfiguration().
	// ARP_ADD_WATCHER_MSG:
	//   Add a watcher BMessenger to the watch list, and send it the
	//   current settings values.
	// ARP_REM_WATCHER_MSG:
	//   Remove a watcher BMessenger from the watch list.
	status_t MessageReceived(const BMessage* message);
	
	// Send information about a change in settings to any actively watching
	// objects.  The parameter 'changes' contains the settings that
	// have changed.  An ARP_PUT_CONFIGURATION_MSG with these settings
	// is sent to all watchers if 'to' is 0, or only to that specific
	// handler.
	void ReportChange(const BMessage* changes, BMessenger* to = 0);
	
	// Send an update message with all of the current settings state to
	// any actively watching objects.  If 'to' is non-NULL, the message
	// is only sent to that specific handler.
	void ReportState(BMessenger* to = 0);
	
	// ------ Low-Level Updating And Reporting Controls ------
	
	// Return true if there are any active watchers
	bool HaveWatchers();
	
	virtual status_t AddWatcher(const BMessenger& w);
	virtual status_t RemWatcher(const BMessenger& w);
	
	// Stub the ArpConfigurableI interface so this is not an
	// abstract class.
	virtual status_t GetConfiguration(BMessage* ) const { return B_OK; }
	virtual status_t PutConfiguration(const BMessage* ) { return B_OK; }
	virtual status_t Configure(ArpVectorI<BView*>& ) { return B_OK; }

private:
	
	BLocker mAccess;
	BHandler* mObject;
	ArpConfigurableI* mConfigure;
	ArpVectorI<BMessenger>* mWatchers;
};

#endif
