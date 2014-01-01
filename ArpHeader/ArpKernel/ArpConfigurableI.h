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
 * ArpConfigurableI.h
 *
 * An abstract interface to an object that is "configurable": it supports
 * setting and getting its parameter state through BMessages, and can
 * create a view to edit these parameters.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * None.
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
 * 11/2/1998:
 * 	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPCONFIGURABLEI_H
#define ARPKERNEL_ARPCONFIGURABLEI_H

#ifndef ARPCOLLECTIONS_ARPVECTORI_H
#include <ArpCollections/ArpVectorI.h>
#endif

enum {
	/**	This is a standard message that configurable objects
	 ** can implement to update their settings.
	 **
	 **	"settings" (B_MESSAGE_TYPE): A BMessage containing
	 ** the new settings values, exactly as passed to
	 ** GetConfiguration().
	 **/
	ARP_PUT_CONFIGURATION_MSG = 'aPCM',
	
	/** Request to receive change reports.
	 **
	 ** Send this command to an ArpConfigureWatch to receive reports
	 ** of changes to its object's settings.  Reports are in the form of
	 ** an ARP_PUT_CONFIGURATION_MSG, whose settings field contains
	 ** only the values that changed.  The first time you send this,
	 ** you will get a change report containing all the current settings.
	 **
	 ** Data fields are:
	 **
	 ** "watch" (B_MESSENGER_TYPE) is the place to send reports.
	 **/
	ARP_ADD_WATCHER_MSG = 'aAWT',
	
	/** Request to end change reports.
	 **
	 ** Send this command to an ArpConfigureWatch to stop receiving
	 ** change reports, previously requested with TERM_ADD_WATCHER_MSG.
	 **
	 ** Data fields are:
	 **
	 ** "watch" (B_MESSENGER_TYPE) is the BMessenger that was previously
	 ** supplied to receive reports.
	 **/
	ARP_REM_WATCHER_MSG = 'aRWT',
	
	/** Inform configuration view of its owner
	 **
	 ** This message is sent from ArpConfigurePanel to each of its
	 ** configuration views as they are attached to the window, to
	 ** tell them who their owner is.  They can use this message to
	 ** figure out how to send commands back to the panel.
	 **
	 ** Data fields are:
	 **
	 ** "panel" (B_MESSENGER_TYPE) is a BMessenger to the configuration
	 ** panel that this view is in.
	 **/
	ARP_SET_CONFIG_PANEL_MSG = 'aSCP',
	
	/** Tell the configuration panel to rebuild itself.
	 **
	 ** This message can be sent from a configuration view to its owner
	 ** panel (as given by ARP_SET_CONFIG_PANEL_MSG), to ask it to rebuild
	 ** its configuration panes.  This is typically done when the underlying
	 ** object has changed in such a way that the set of views used to
	 ** configure it has changed.
	 **
	 ** Data fields are:
	 **
	 ** None.
	 **/
	ARP_REBUILD_PANEL_MSG = 'aRBP'
};

class ArpConfigurableI {
public:
	virtual status_t GetConfiguration(BMessage* values) const = 0;
	virtual status_t PutConfiguration(const BMessage* values) = 0;
	virtual status_t Configure(ArpVectorI<BView*>& views) = 0;
};

// Your configurable object can implement this interface
// if it is not a BHandler, but instead will be directly called.
// Note that in this situation you must guarantee that -both- of
// these interfaces are thread-safe, and ensure that this object
// remains valid while any configuration panels for it exist.

class ArpDirectConfigurableI : public ArpConfigurableI {
public:
	virtual status_t AddWatcher(const BMessenger& who) = 0;
	virtual status_t RemWatcher(const BMessenger& who) = 0;
};

#endif
