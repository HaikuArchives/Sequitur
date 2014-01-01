/* GlNotifier.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2004.03.23			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLNOTIFIER_H
#define GLPUBLIC_GLNOTIFIER_H

#include <be/app/Messenger.h>
#include <be/support/Locker.h>
class _GlNotifierData;

/***************************************************************************
 * GL-NOTIFIER
 * I store a list of messenger targets and supply notification when
 * something changes.
 ***************************************************************************/
class GlNotifier
{
public:
	virtual ~GlNotifier();

	enum {
		CHANGE_MSG		= 'GChn',
	};
	enum {
		INFO_CODE		= 0x00000001,
		NODE_CODE		= 0x00000002,	// Receive notification whenever node
										// params change
		CHAIN_CODE		= 0x00000004	// Receive notification whenever nodes
										// are added or removed
	};
	
	status_t			AddObserver(uint32 code, const BMessenger& m);
	status_t			RemoveObserver(uint32 code, const BMessenger& m);

	void				RemoveAll();

	/* The sender, if any, will be included as a pointer in the change reports.
	 */
	void				SetSender(void* sender);

	/* Report a generic change.  Send the supplied message to appropriate
	 * observers.  The code is one or more of the _CODE flags.  The msg
	 * what should currently be CHANGE_MSG; that might change though I
	 * don't know why it would.
	 */
	void				ReportMsgChange(uint32 code, BMessage* msg, BMessenger sender);

protected:
	GlNotifier();

private:
	_GlNotifierData*	mData;
	void*				mSender;
	BLocker				mNotifierLock;
};

#endif
