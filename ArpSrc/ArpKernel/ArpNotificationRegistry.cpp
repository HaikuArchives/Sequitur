/*
	
	ArpNotificationRegistry.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef ARPKERNEL_ARPNOTIFICATIONREGISTRY_H
#include "ArpKernel/ArpNotificationRegistry.h"
#endif
#ifndef _LOOPER_H
#include <app/Looper.h>
#endif
#include <cstdio>
#include <cassert>

/*
		void ExpressInterest(BHandler*);
		void ExpressInterest(int, BHandler*);
		void RetractInterest(BHandler*);
		void RetractInterest(int, BHandler*);
*/

/**********************************************************************
* class ArpNotificationRegistry
**********************************************************************/

ArpNotificationRegistry::ArpNotificationRegistry() {
}

ArpNotificationRegistry::~ArpNotificationRegistry() {
	ArpNotificationType		*ade;
	while ( (ade = (ArpNotificationType*)entries.RemoveItem((int32)0)) != NULL) 
		delete ade;
}


/**********************************************************************
* Registering - notifying
**********************************************************************/

int32 ArpNotificationRegistry::AddDependent(BHandler *v)
{
	ArpNotificationType		*ade;
	int32				count = 0;

	for (int32 k=0; (ade = (ArpNotificationType*)entries.ItemAt(k)) != NULL; k++)
		if (ade->AddDependent(v)) count++;
	return count;
}

int32 ArpNotificationRegistry::AddDependent(int32 type, BHandler *v)
{
	ArpNotificationType		*ade;
	int32				count = 0;

	for (int32 k=0; (ade = (ArpNotificationType*)entries.ItemAt(k)) != NULL; k++) {
		if (ade->Type() == type) {
			if (ade->AddDependent(v)) count++;
		}
	}
	return count;
}

int32 ArpNotificationRegistry::RemoveDependent(BHandler *v) {
	ArpNotificationType		*ade;
	int32				count = 0;
	for (int32 k=0; (ade = (ArpNotificationType*)entries.ItemAt(k)) != NULL; k++)
		if (ade->RemoveDependent(v)) count++;
	return(count);
}

int32 ArpNotificationRegistry::RemoveDependent(int32 type, BHandler *v) {
	ArpNotificationType		*ade;
	int32				count = 0;
	for (int32 k=0; (ade = (ArpNotificationType*)entries.ItemAt(k)) != NULL; k++)
		if (ade->Type() == type) {
			if (ade->RemoveDependent(v)) count++;
		}
	return(count);
}

void ArpNotificationRegistry::NotifyDependents(int32 type, BMessage *m,
											   bool deliverAsynchronously)
{
	ArpNotificationType		*ade;
	for (int32 k=0; (ade = (ArpNotificationType*)entries.ItemAt(k)) != 0; k++)
		if (ade->Type() == type) ade->NotifyDependents(m, deliverAsynchronously);
}


/**********************************************************************
* Public - registering
**********************************************************************/

bool ArpNotificationRegistry::RegisterType(int32 type)
{
	ArpNotificationType		*ade = new ArpNotificationType(type);
	if (ade == NULL) return false;
	return entries.AddItem((void*)ade);
}


/**********************************************************************
* class ArpNotificationType
* This class stores a list of dependents and notifies them with the
* supplied message.
**********************************************************************/

ArpNotificationType::ArpNotificationType(int32 typeArg) {
	type = typeArg;
}

ArpNotificationType::~ArpNotificationType() {
}


/**********************************************************************
* Public - accessing
**********************************************************************/

void ArpNotificationType::SetType(int32 t) {
	type = t;
}

int32 ArpNotificationType::Type() {
	return(type);
}


/**********************************************************************
* Public - registering / notifying
**********************************************************************/

bool ArpNotificationType::AddDependent(BHandler *h) {
	//assert(h && h->Looper());
	return(dependents.AddItem((void*)h));
}

bool ArpNotificationType::RemoveDependent(BHandler *h) {
	return(dependents.RemoveItem((void*)h));
}

void ArpNotificationType::NotifyDependents(BMessage *msg, bool deliverAsynchronously)
{
	BHandler	*h;
	BLooper		*looper;
	for (int32 i=0; (h = (BHandler*)dependents.ItemAt(i)) != 0; i++) {
		if (((looper = h->Looper()) != 0)
				&& (looper->Lock())) {
			// FIX: need to send out the message, and let the views
			// invalidate themselves!  So be aware that the Update
			// implementation will ALWAYS be called from within this
			// loop, meaning within a lock/unlock
#if 0	// Uncomment this to debug where messages are going
printf("SENDING MESSAGE TO %s\n", h->Name());  fflush(stdout);
#endif
			h->MessageReceived(msg);
			looper->Unlock();
		}
	}
}
