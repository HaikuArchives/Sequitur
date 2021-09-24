#include "AmKernel/AmInputQueue.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmTransport.h"

#include "ArpKernel/ArpDebug.h"

#include <Autolock.h>

AmInputQueue::AmInputQueue(song_id song)
	: mSong(song), 
	  mLock("AmInputQueue Lock"),
	  mPerformLock("AmInputQueue Perform Lock"),
	  mRecordLock("AmInputQueue Record Lock"),
	  mRecordAvail(B_BAD_SEM_ID), mRecordThread(B_BAD_THREAD_ID)
{
}

AmInputQueue::~AmInputQueue()
{
	StopRecording();
	StopPerforming();
}

bool AmInputQueue::Lock()
{
	return mLock.Lock();
}

void AmInputQueue::Unlock()
{
	mLock.Unlock();
}

// ------------------------------------------------------------------

status_t AmInputQueue::StartPerforming()
{
	BAutolock _l(mLock);
	
	status_t result = B_ERROR;
	
	// Start up record thread.
	if (mPerformLock.Lock()) {
		mPerformAvail = create_sem(0, "AmInputQueue Perform Avail");
		if (mPerformAvail >= B_OK) {
			mPerformThread = spawn_thread(PerformThreadEntry, "AmInputQueue Performer",
										 B_REAL_TIME_PRIORITY, this);
			if (mPerformThread >= B_OK) {
				resume_thread(mPerformThread);
				result = B_OK;
			} else {
				result = mPerformThread;
				delete_sem(mPerformAvail);
				mPerformAvail = B_BAD_SEM_ID;
			}
		} else {
			result = mPerformAvail;
		}
		mPerformLock.Unlock();
	}
	
	return result;
}

bool AmInputQueue::IsPerforming() const
{
	BAutolock _l(mPerformLock);
	return mPerformAvail >= B_OK;
}

void AmInputQueue::StopPerforming()
{
	BAutolock _l(mLock);
	
	mPerformLock.Lock();
	if (mPerformThread >= B_OK) {
		sem_id sem = mPerformAvail;
		mPerformAvail = B_BAD_SEM_ID;
		delete_sem(sem);
		
		mPerformLock.Unlock();
		status_t ret;
		wait_for_thread(mPerformThread, &ret);
		mPerformLock.Lock();
	}
	
	for (int32 i=0; i<mPerformItems.CountItems(); i++) {
		record_item* ri = (record_item*)mPerformItems.ItemAt(i);
		if (ri) {
			if (ri->events) ri->events->DeleteChain();
			delete ri;
		}
	}
	mPerformItems.MakeEmpty();
	mPerformLock.Unlock();
}

void AmInputQueue::PerformEvents(track_id track, AmEvent* events)
{
	mPerformLock.Lock();
	if (mPerformThread >= B_OK) {
		record_item* ri = new record_item;
		ri->track = track;
		ri->events = events;
		mPerformItems.AddItem(ri);
		release_sem(mPerformAvail);
	} else {
		events->DeleteChain();
	}
	mPerformLock.Unlock();
}

int32 AmInputQueue::PerformThreadEntry(void* arg)
{
	ArpD(cdb << ADH << "Enter the performer." << std::endl);
	AmInputQueue *obj = (AmInputQueue *)arg;

	int32 ret = obj->PerformLoop();
	obj->mPerformLock.Lock();
	if (obj->mPerformAvail >= B_OK) {
		delete_sem(obj->mPerformAvail);
		obj->mPerformAvail = B_BAD_SEM_ID;
	}
	obj->mPerformThread = B_BAD_THREAD_ID;
	obj->mPerformLock.Unlock();
	
	ArpD(cdb << ADH << "Exit the recorder." << std::endl);
	return ret;
}

/* EEAAAAAAAGHH!  My new code uses the current time from the transport
 * to add signature information to the params.  God I hope this is OK.
 */
int32 AmInputQueue::PerformLoop()
{
	while (mPerformAvail >= B_OK) {
		while (acquire_sem(mPerformAvail) == B_INTERRUPTED) ;
		BList items;
		if (mPerformLock.Lock()) {
			items = mPerformItems;
			mPerformItems.MakeEmpty();
			mPerformLock.Unlock();
		}

		am_filter_params			params;
		am_filter_params*			p = 0;

		AmSongRef songRef = AmGlobals().SongRef(mSong);
		if (songRef.IsValid()) {
			for (int32 i=0; i<items.CountItems(); i++) {
				record_item* ri = (record_item*)items.ItemAt(i);
				if (!ri) continue;
				
				{
					// READ SONG BLOCK
					#ifdef AM_TRACE_LOCKS
					printf("AmInputQueue::PerformLoop() read lock\n"); fflush(stdout);
					#endif
					const AmSong* song = songRef.ReadLock();
					const AmTrack* track = song ? song->Track(ri->track) : NULL;
					if (track) {
						p = 0;
						AmEvent*		signatures = 0;
						AmTime			currentTime = song->Transport().CurrentTime();
						if (currentTime >= 0) {
							signatures = song->PlaybackList(currentTime, currentTime, PLAYBACK_NO_PERFORMANCE | PLAYBACK_NO_TEMPO | PLAYBACK_RAW_CONTEXT);
							if (signatures) {
//								printf("Sigs: \n"); signatures->PrintChain();
								params.cur_signature = dynamic_cast<AmSignature*>(signatures);
							} else params.cur_signature = 0;
							p = &params;
						}
						// Run events through input filters
						ri->events = ArpExecFilters(ri->events, REALTIME_EXEC_TYPE, false, p, 0, 0, 0, signatures);
						if (ri->events) ri->events = ri->events->HeadEvent();
						
						// Run result through output filters.
						AmFilterHolderI* h = track->Filter(OUTPUT_PIPELINE);
						if (h) {
							AmEvent* pos = ri->events;
							while (pos) {
								if (!pos->NextFilter()) pos->SetNextFilter(h);
								pos = pos->NextEvent();
							}
							ri->events = ArpExecFilters(ri->events, REALTIME_EXEC_TYPE, false, p, 0, 0, 0, signatures);
							if (ri->events) ri->events = ri->events->HeadEvent();
						}
						song->Transport().Merge(ri->events);
						ri->events = NULL;
					}
					params.cur_signature = 0;

					songRef.ReadUnlock(song);
					// END READ SONG BLOCK
				}
				
				if (ri->events) ri->events->DeleteChain();
				delete ri;
			}
		}
	}
	
	return B_OK;
}

#if 0
int32 AmInputQueue::PerformLoop()
{
	while (mPerformAvail >= B_OK) {
		while (acquire_sem(mPerformAvail) == B_INTERRUPTED) ;
		
		BList items;
		if (mPerformLock.Lock()) {
			items = mPerformItems;
			mPerformItems.MakeEmpty();
			mPerformLock.Unlock();
		}
		
		AmSongRef songRef = AmGlobals().SongRef(mSong);
		if (songRef.IsValid()) {
			for (int32 i=0; i<items.CountItems(); i++) {
				record_item* ri = (record_item*)items.ItemAt(i);
				if (!ri) continue;
				
				{
					// READ SONG BLOCK
					#ifdef AM_TRACE_LOCKS
					printf("AmInputQueue::PerformLoop() read lock\n"); fflush(stdout);
					#endif
					const AmSong* song = songRef.ReadLock();
					const AmTrack* track = song ? song->Track(ri->track) : NULL;
					if (track) {
printf("CurrentTime is %lld\n", song->Transport().CurrentTime());
						// Run events through input filters
						ri->events = ArpExecFilters(ri->events, REALTIME_EXEC_TYPE, false);
						if (ri->events) ri->events = ri->events->HeadEvent();
						
						// Run result through output filters.
						AmFilterHolderI* h = track->Filter(OUTPUT_PIPELINE);
						if (h) {
							AmEvent* pos = ri->events;
							while (pos) {
								if (!pos->NextFilter()) pos->SetNextFilter(h);
								pos = pos->NextEvent();
							}
							ri->events = ArpExecFilters(ri->events, REALTIME_EXEC_TYPE, false);
							if (ri->events) ri->events = ri->events->HeadEvent();
						}
						song->Transport().Merge(ri->events);
						ri->events = NULL;
					}
					songRef.ReadUnlock(song);
					// END READ SONG BLOCK
				}
				
				if (ri->events) ri->events->DeleteChain();
				delete ri;
			}
		}
	}
	
	return B_OK;
}
#endif

// ------------------------------------------------------------------

status_t AmInputQueue::StartRecording()
{
	BAutolock _l(mLock);
	
	status_t result = B_ERROR;
	
	// Start up record thread.
	if (mRecordLock.Lock()) {
		mRecordAvail = create_sem(0, "AmInputQueue Record Avail");
		if (mRecordAvail >= B_OK) {
			mRecordThread = spawn_thread(RecordThreadEntry, "AmInputQueue Recorder",
										 B_NORMAL_PRIORITY, this);
			if (mRecordThread >= B_OK) {
				resume_thread(mRecordThread);
				result = B_OK;
			} else {
				result = mRecordThread;
				delete_sem(mRecordAvail);
				mRecordAvail = B_BAD_SEM_ID;
			}
		} else {
			result = mRecordAvail;
		}
		mRecordLock.Unlock();
	}
	
	return result;
}

bool AmInputQueue::IsRecording() const
{
	BAutolock _l(mRecordLock);
	return mRecordAvail >= B_OK;
}

void AmInputQueue::StopRecording()
{
	BAutolock _l(mLock);
	
	mRecordLock.Lock();
	if (mRecordThread >= B_OK) {
		sem_id sem = mRecordAvail;
		mRecordAvail = B_BAD_SEM_ID;
		delete_sem(sem);
		
		mRecordLock.Unlock();
		status_t ret;
		wait_for_thread(mRecordThread, &ret);
		mRecordLock.Lock();
	}
	
	for (int32 i=0; i<mRecordItems.CountItems(); i++) {
		record_item* ri = (record_item*)mRecordItems.ItemAt(i);
		if (ri) {
			if (ri->events) ri->events->DeleteChain();
			delete ri;
		}
	}
	mRecordItems.MakeEmpty();
	mRecordLock.Unlock();
}

void AmInputQueue::RecordEvents(track_id track, AmEvent* events)
{
	mRecordLock.Lock();
	if (mRecordThread >= B_OK) {
//debugger("Dsds");
		record_item* ri = new record_item;
		ri->track = track;
		ri->events = events;
		mRecordItems.AddItem(ri);
		release_sem(mRecordAvail);
	} else {
		events->DeleteChain();
	}
	mRecordLock.Unlock();
}

int32 AmInputQueue::RecordThreadEntry(void* arg)
{
	ArpD(cdb << ADH << "Enter the recorder." << std::endl);
	AmInputQueue *obj = (AmInputQueue *)arg;
	
	int32 ret = obj->RecordLoop();
	obj->mRecordLock.Lock();
	if (obj->mRecordAvail >= B_OK) {
		delete_sem(obj->mRecordAvail);
		obj->mRecordAvail = B_BAD_SEM_ID;
	}
	obj->mRecordThread = B_BAD_THREAD_ID;
	obj->mRecordLock.Unlock();
	
	ArpD(cdb << ADH << "Exit the recorder." << std::endl);
	return ret;
}

static inline AmEvent* _get_record_t_and_e(	AmEvent* head, AmSong* song,
											AmEvent** newHead, AmTrack** track)
{
	if (!head) return 0;
	track_id		tid = head->trackId;
	if (!tid) {
		*newHead = head->NextEvent();
		head->RemoveEvent();
		head->Delete();
		return 0;
	}
	
	AmEvent*		n = head->NextEvent();
	while (n && n->trackId == tid) n = n->NextEvent();
	if (n) {
		*newHead = n;
		n->CutBefore();
	} else *newHead = 0;

	if (!(*track) || tid != (*track)->Id())
		*track = song->Track(tid);
	if (!(*track)) return 0;

	return head;
}

int32 AmInputQueue::RecordLoop()
{
	while (mRecordAvail >= B_OK) {
		while (acquire_sem(mRecordAvail) == B_INTERRUPTED) ;
		
		BList items;
		if (mRecordLock.Lock()) {
			items = mRecordItems;
			mRecordItems.MakeEmpty();
			mRecordLock.Unlock();
		}
		
		AmSongRef songRef = AmGlobals().SongRef(mSong);
		if (songRef.IsValid()) {
			for (int32 i=0; i<items.CountItems(); i++) {
				record_item* ri = (record_item*)items.ItemAt(i);
				if (!ri) continue;
				
				{
					// READ SONG BLOCK
					#ifdef AM_TRACE_LOCKS
					printf("AmInputQueue::RecordLoop() read lock\n"); fflush(stdout);
					#endif
					const AmSong* song = songRef.ReadLock();
					if (song) {
						ri->events = ArpExecFilters(ri->events, REALTIME_EXEC_TYPE, false);
						if (ri->events) ri->events = ri->events->HeadEvent();
						songRef.ReadUnlock(song);
					}
					// END READ SONG BLOCK
				}
				
				{
					// WRITE SONG BLOCK
					AmSong* song = songRef.WriteLock();
					if (song) {
#if 1
						/* New mechanism -- all events should have been supplied
						 * a track by the last filter they went through.  So,
						 * record to the requested track.  This wouldn't be
						 * necessary if we had a full architecture -- that is,
						 * the tracks were just another filter in the pipeline.
						 */
						AmTrack*		track = 0;
						AmEvent*		e = 0;
						AmEvent*		head = ri->events;
						while ((e = _get_record_t_and_e(head, song, &head, &track)) != 0) {
							if (track) track->RecordEvents(e);
							else e->DeleteChain();
						}
						if (head) head->DeleteChain();
						ri->events = 0;
#endif
#if 0
						AmTrack*		track = song->Track(ri->track);
						if (track && ri->events) {
							track->RecordEvents(ri->events);
							ri->events = NULL;
						}
#endif
						songRef.WriteUnlock(song);
					}
					// END WRITE SONG BLOCK
				}
				
				if (ri->events) ri->events->DeleteChain();
				delete ri;
			}
		}
	}
	
	return B_OK;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------

AmInputTarget::AmInputTarget(track_id track, AmInputQueue* queue)
	: mTrack(track), mQueue(queue), mPerforming(false), mRecording(false)
{
}

void AmInputTarget::SetPerforming(bool state)
{
	if (mQueue->Lock()) {
		mPerforming = state;
		mQueue->Unlock();
	}
}

bool AmInputTarget::IsPerforming() const
{
	bool state = false;
	if (mQueue->Lock()) {
		state = mPerforming;
		mQueue->Unlock();
	}
	return state;
}

void AmInputTarget::SetRecording(bool state)
{
	if (mQueue->Lock()) {
		mRecording = state;
		mQueue->Unlock();
	}
}

bool AmInputTarget::IsRecording() const
{
	bool state = false;
	if (mQueue->Lock()) {
		state = mRecording;
		mQueue->Unlock();
	}
	return state;
}

void AmInputTarget::HandleEvents(AmEvent* events)
{
	mQueue->Lock();
	if (mQueue->IsPerforming() && IsPerforming()) {
		mQueue->Unlock();
		mQueue->PerformEvents(mTrack, events->CopyChain());
		mQueue->Lock();
	}
	
	if (!mQueue->IsRecording() || !IsRecording()) {
		mQueue->Unlock();
		//printf("Dropping record:\n"); events->PrintChain();
		events->DeleteChain();
		return;
	}
	
	mQueue->Unlock();
	mQueue->RecordEvents(mTrack, events);
}

AmInputTarget::~AmInputTarget()
{
}
