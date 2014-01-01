/* GlProcessStatus.h
 * Copyright (c)2003 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.12.31			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLPROCESSSTATUS_H
#define GLPUBLIC_GLPROCESSSTATUS_H

#include <ArpCore/StlVector.h>
#include <be/app/Messenger.h>
#include <be/support/Locker.h>
#include <be/support/SupportDefs.h>
class _GlProcessPartition;

/***************************************************************************
 * GL-PROCESS-STATUS
 * An object held by the app that provides info to nodes such as whether
 * the process is cancelled, and lets nodes provide status updates.
 ****************************************************************************/
class GlProcessStatus
{
public:
	/* Should be called before this object is used.
	 */
	status_t		Init();

	/* Status works by partitioning the bar into segments -- each segment
	 * then can be partitioned, etc.  PushPartition() sets the current
	 * segment to 0; step through each segment with IncSegment().  To
	 * set the completion amount of the current operation with SetComplete().
	 * If the object has been cancelled all these methods will return B_CANCELED.
	 */
	status_t		PushPartition(int32 segments);
	status_t		IncSegment();
	status_t		SetComplete(float percent);
	status_t		PopPartition();
	
	bool			Canceled();
	void			SetCanceled();
	
	
protected:
	friend class GlPerformer;
	
	GlProcessStatus();
	~GlProcessStatus();

	void			SetTarget(BMessenger target, int32 code);
	void			SetProgressWidth(int32 pixels);

private:
	BLocker 		mLock; 
	vector<_GlProcessPartition*>	mPartitions;
	BMessenger		mTarget;
	uint32			mCode;
	int32			mProgressWidth;
	int32			mLastPix;
	bool			mCanceled;
	
	void			SetPixel(float percent);	

	void			FreeParititonStack();

public:
	void			Print() const;
};

#endif
