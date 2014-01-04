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
 * ArpNotificationRegistry.h
 *
 * This file contains a class that is responsible for storing a list of
 * BHandlers with an interest in some piece of data, and sending notices
 * to them.  In many cases, the class which BHandlers will be requesting
 * notification from will subclass this registry.
 * 
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• In both NotifyDependents() functions, there is now a flag to tell
 * the notify method to deliver asynchronously -- return immediately --
 * or not.  This flag is a PLACE HOLDER.  There is no current capability
 * to actually deliver the messages asynchronously, but it's there for
 * clients who depend upon this behaviour being one way or the other.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 12.14.1998		dianne
 * Now uses BMessengers as notification destination, rather than
 * raw BHandlers.
 * 11.23.1998		hackborn
 * Added fake ability to notify dependents asynchronously.  See known bugs.
 */
 
#ifndef ARPKERNEL_ARPNOTIFICATIONREGISTRY_H
#define ARPKERNEL_ARPNOTIFICATIONREGISTRY_H


#include <List.h>
#include <Message.h>
#include <Handler.h>
#ifndef _SUPPORT_DEFS_H
#include <support/SupportDefs.h>
#endif


// Values of 0 - 99 are reserved for standard defines.
#define DEP_MISC		(0)


class ArpNotificationRegistry
{
public:
	ArpNotificationRegistry();
	virtual ~ArpNotificationRegistry();

	/* AddDependent is called when a particular handler decides it wants
	 * to hear about changes.  If no type information is supplied, the
	 * handler hears about every change that happens.  If the handler
	 * only cares about certain types of information (which are defined
	 * by whomever is subclassing or creating an instance of this class),
	 * they can supply the type code for the interesting information, and
	 * only hear about that.
	 */
	int32 AddDependent(BHandler *v);
	int32 AddDependent(int32 type, BHandler *v);
	/* When handlers are no longer interested in hearing about changes, they
	 * remove themselves via a call to RemoveDependent().  If no type is
	 * supplied, they are completely removed as a dependent from any type
	 * they might be registered for.  Otherwise, they are only removed from
	 * they specific type.
	 */
	int32 RemoveDependent(BHandler *v);
	int32 RemoveDependent(int32 type, BHandler *v);
	/* When a piece of data has changed, the subclass or owner of the
	 * registry informs all interested parties by notifying them with
	 * a message of the relevant info.
	 */
	void NotifyDependents(int32 type,
						  BMessage *m,
						  bool deliverAsynchronously = true);

	/* In order to segment various pieces of data that handlers can listen
	 * to, subclasses or owners of the registry should register every available
	 * type of information.
	 */
	bool RegisterType(int32 type);
			
private:
	BList	entries;		// stores ArpDependentEntries
};


// This class is internal to the ArpNotificationRegistry implementation
class ArpNotificationType
{
public:
	ArpNotificationType(int32);
	virtual ~ArpNotificationType();

	void SetType(int32);
	int32 Type();

	bool AddDependent(BHandler*);
	bool RemoveDependent(BHandler*);
	void NotifyDependents(BMessage *msg,
						  bool deliverAsynchronously = true);

private:
	BList	dependents;		// stores BHandlers*
	int32	type;
};


#endif
