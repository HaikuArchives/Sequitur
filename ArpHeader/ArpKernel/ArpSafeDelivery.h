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
 * ArpSafeDelivery.h
 *
 * Gauranteed non-blocking delivery of messages.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
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
 * 7/24/2000:
 *	• Created this file.
 *
 */

#ifndef ARPKERNEL_ARPSAFEDELIVERY_H
#define ARPKERNEL_ARPSAFEDELIVERY_H

#include <Messenger.h>

class ArpSafeDeliveryLooper;

class ArpSafeDelivery {
public:
	ArpSafeDelivery();
	virtual ~ArpSafeDelivery();
	
	static ArpSafeDelivery* Default();
	
	// This copies the message -- it does not take ownership.
	status_t SendMessage(const BMessenger& target, BMessage* msg);

private:
	friend class ArpSafeDeliveryLooper;
	
	void LooperShuttingDown();
	
	int32 mAllocated;
	ArpSafeDeliveryLooper* mLooper;
};

status_t SafeSendMessage(const BMessenger& target, BMessage* msg);

#endif
