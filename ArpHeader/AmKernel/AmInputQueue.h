/* AmRecordQueue.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 05.14.00		hackborn
 * Created this file
 */
#ifndef AMKERNEL_AMINPUTQUEUE_H
#define AMKERNEL_AMINPUTQUEUE_H

#include "AmPublic/AmDefs.h"

#include "ArpKernel/ArpRef.h"

#include <List.h>
#include <Locker.h>

class AmEvent;

/**********************************************************************
 * AM-INPUT-QUEUE
 * Object that you can record events into.
 **********************************************************************/
class AmInputQueue : public ArpRefable {
public:
						AmInputQueue(song_id song);
	
	bool				Lock();
	void				Unlock();
	
	status_t			StartPerforming();
	bool				IsPerforming() const;
	void				StopPerforming();
	
	void				PerformEvents(track_id track, AmEvent* events);
	
	status_t			StartRecording();
	bool				IsRecording() const;
	void				StopRecording();
	
	void				RecordEvents(track_id track, AmEvent* events);
	
protected:
	virtual				~AmInputQueue();

private:
	const song_id		mSong;
	
	mutable BLocker		mLock;
	
	struct record_item {
		track_id track;
		AmEvent* events;
	};
	
	static int32		PerformThreadEntry(void* arg);
	int32				PerformLoop();
	
	mutable BLocker		mPerformLock;
	
	sem_id				mPerformAvail;
	thread_id			mPerformThread;
	BList				mPerformItems;	// of record_item objects
	
	static int32		RecordThreadEntry(void* arg);
	int32				RecordLoop();
	
	mutable BLocker		mRecordLock;
	
	sem_id				mRecordAvail;
	thread_id			mRecordThread;
	BList				mRecordItems;	// of record_item objects
};

/**********************************************************************
 * AM-INPUT-TARGET
 * Object that you can record events into.
 **********************************************************************/
class AmInputTarget : public ArpRefable {
public:
						AmInputTarget(track_id track, AmInputQueue* queue);

	void				SetPerforming(bool state);
	bool				IsPerforming() const;
	
	void				SetRecording(bool state);
	bool				IsRecording() const;
	
	void				HandleEvents(AmEvent* events);
	
protected:
	virtual				~AmInputTarget();

private:
	const track_id		mTrack;
	const ArpRef<AmInputQueue> mQueue;
	bool				mPerforming;
	bool				mRecording;
};


#endif
