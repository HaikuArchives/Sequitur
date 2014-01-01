#define _BUILDING_AmKernel 1

#ifndef AMKERNEL_AMPERFORMER_H
#include "AmKernel/AmPerformer.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <Autolock.h>
#include <scheduler.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

ArpMOD();

//#define SHOW_ERROR 1
//#define PRINT_CHAINS 1

//#include <iostream>

static inline AmTime do_realtime_to_pulse(bigtime_t time, float bpm,
										  AmTime baseBeat, bigtime_t baseTime)
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const double PPM = double(PPQN)*bpm;
	
	return AmTime( baseBeat + ( ((time-baseTime)*PPM) / (60*1000000) ) );
}

static inline bigtime_t do_pulse_to_realtime(AmTime pulse, float bpm,
											 AmTime baseBeat, bigtime_t baseTime)
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const double PPM = double(PPQN)*bpm;
	
	// This is the number of microseconds from the start of
	// the song that the next event should be performed.
	return bigtime_t( ((double(pulse-baseBeat) * 60*1000000) / PPM) + baseTime);
}

#if 0
static inline AmTime do_realtime_to_pulse(bigtime_t time, int32 bpm,
										  AmTime baseBeat, bigtime_t baseTime)
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const bigtime_t PPM = bigtime_t(PPQN)*bpm;
	
	return AmTime( baseBeat + ( ((time-baseTime)*PPM) / (60*1000000) ) );
}

static inline bigtime_t do_pulse_to_realtime(AmTime pulse, int32 bpm,
											 AmTime baseBeat, bigtime_t baseTime)
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const bigtime_t PPM = bigtime_t(PPQN)*bpm;
	
	// This is the number of microseconds from the start of
	// the song that the next event should be performed.
	return ((bigtime_t(pulse-baseBeat) * 60*1000000) / PPM) + baseTime;
}

#endif

AmTime realtime_to_pulse(bigtime_t time, int32 bpm, AmTime baseBeat, bigtime_t baseTime)
{
	return do_realtime_to_pulse(time, bpm, baseBeat, baseTime);
}

bigtime_t pulse_to_realtime(AmTime pulse, int32 bpm, AmTime baseBeat, bigtime_t baseTime)
{
	return do_pulse_to_realtime(pulse, bpm, baseBeat, baseTime);
}

/* ----------------------------------------------------------------
   AmPerformer Implementation
   ---------------------------------------------------------------- */

AmPerformer::AmPerformer(bool continuous_running)
	: mContinuousRunning(continuous_running),
	  mAccess("ARP™ Performer Access"),
	  mDataSem(B_BAD_SEM_ID), mPlayThread(B_BAD_THREAD_ID),
	  
	  mEventAccess("ARP™ Performer Event Access"),
	  mPosition(NULL), mTail(NULL), mStartTime(0),
	  mRestart(false), mPlaying(false), mFinished(false),
	  mCleanup(false),
	  
	  mTimeAccess("ARP™ Performer Time Access"),
	  mClockTarget(NULL), mNextClock(0),
	  mSharedBPM(120), mSharedTempoBeat(0), mSharedTempoTime(0),
	  mSharedBeatOffset(0),
	  mValidTime(false), mSetBPM(true),
	  
	  mBPM(120), mTempoBeat(0), mTempoTime(0), mBaseTime(0),
	  mBeatOffset(0),
	  mNewEvents(NULL)
{
	if (mContinuousRunning) {
		Play(NULL, 0);		// prime the pump
		Stop();
	}
}

AmPerformer::~AmPerformer()
{
	StopForReal();
}

float AmPerformer::BPM() const
{
	return mBPM;
}

void AmPerformer::SetBPM(float BPM)
{
	BAutolock l(mTimeAccess);
	mSharedBPM = BPM;
	mSetBPM = true;
}

void AmPerformer::SetClockTarget(AmClockTarget* target)
{
	BAutolock l(mTimeAccess);
	//printf("Setting performer %p clock target to %p\n", this, target);
	mClockTarget = target;
	if (target) {
		mNextClock = 0;
		release_sem(mDataSem);
	}
}

AmTime AmPerformer::CurrentTime() const
{
	BAutolock l(mTimeAccess);
	
	if (mDataSem < B_OK) return -1;
	
	// This is the real time that has elapsed since the last tempo change.
	const bigtime_t realtime = system_time() - mSharedTempoTime;
	
	// This is the number of sequencer pulses that are
	// running every minute.
//	const bigtime_t PPM = bigtime_t(PPQN)*mSharedBPM;
	const double PPM = double(PPQN)*mSharedBPM;

	// This is the number of sequencer pulses that have elapsed
	// since mSharedTempoTime.
	return AmTime(((realtime*PPM)/(60*1000000)) + mSharedTempoBeat - mSharedBeatOffset);
}

bigtime_t AmPerformer::FutureTime(AmTime pulse) const
{
	BAutolock l(mTimeAccess);
	
	if (mDataSem < B_OK) return -1;
	if (!mValidTime) return -1;
	
	return do_pulse_to_realtime(pulse, mSharedBPM,
								mSharedTempoBeat-mSharedBeatOffset, mSharedTempoTime);
}

AmTime AmPerformer::RealtimeToPulse(bigtime_t time, bool includeFuture) const
{
	if (!includeFuture && time > system_time()) return -1;
	
	BAutolock l(mTimeAccess);
	
	if (mDataSem < B_OK) return -1;
	if (!mValidTime) return -1;
	if (time < mSharedTempoTime) return -1;
	
	return do_realtime_to_pulse(time, mSharedBPM,
								mSharedTempoBeat-mSharedBeatOffset, mSharedTempoTime);
}

bigtime_t AmPerformer::PulseToRealtime(AmTime pulse, bool includeFuture) const
{
	BAutolock l(mTimeAccess);
	
	if (mDataSem < B_OK) return -1;
	if (!mValidTime) return -1;
	if (pulse < (mSharedTempoBeat-mSharedBeatOffset)) return -1;
	
	const bigtime_t time = do_pulse_to_realtime(pulse, mSharedBPM,
												mSharedTempoBeat-mSharedBeatOffset,
												mSharedTempoTime);
	
	if (!includeFuture && time > system_time()) return -1;
	
	return time;
}

AmTime AmPerformer::LoopOffset() const
{
	return mSharedBeatOffset;
}

bool AmPerformer::IsPlaying() const
{
	return mPlaying;
}

bool AmPerformer::IsRunning() const
{
	return mDataSem >= B_OK;
}

enum {
	PRIVATE_RESTART = 1<<16
};

status_t AmPerformer::Restart(AmEvent* song, uint32 flags)
{
	BAutolock _l(mAccess);
	if (!mContinuousRunning) {
		while (IsRunning()) {
			mAccess.Unlock();
			Stop();
			mAccess.Lock();
		}
	}
	
	status_t ret = B_OK;
	
	bool threadRunning = mPlayThread >= B_OK;
	
	if (!threadRunning) {
		#if ArpDEBUG
		int32 sugPri = suggest_thread_priority(B_AUDIO_PLAYBACK,
											   1000000/32, 2000, 100);
		ArpD(cdb << ADH << "Suggested thread pri=" << sugPri << endl);
		#endif
		
		ASSERT(mDataSem == B_BAD_SEM_ID);
		ASSERT(mPlayThread == B_BAD_THREAD_ID);
		
		mDataSem = create_sem(0, "ARP™ Midi Performer Data Queue");
		if( mDataSem < B_OK ) {
			ret = (status_t)mDataSem;
			Stop();
			return ret;
		}
	}
	
	ret = Play(song, (flags&~AMPF_RESTART)|PRIVATE_RESTART);
	if( ret < B_OK ) {
		Stop();
		return ret;
	}
	
	if (!threadRunning) {
		mPlayThread = spawn_thread(PlayThreadEntry, "ARP™ Midi Performer",
								   B_REAL_TIME_PRIORITY, this);
		if( mPlayThread < B_OK ) {
			ret = (status_t)mPlayThread;
			Stop();
			return ret;
		}
		
		ret = resume_thread(mPlayThread);
		if( ret < B_OK ) {
			kill_thread(mPlayThread);
			mPlayThread = B_BAD_THREAD_ID;
			Stop();
		}
	}
	
	return ret >= B_OK ? B_OK : ret;
}

status_t AmPerformer::Play(AmEvent* song, uint32 flags)
{
	if (mFinished && !mContinuousRunning && (flags&(AMPF_RESTART|PRIVATE_RESTART)) == 0) {
		if (song) {
			printf("Song is fininished.  Eating:\n");
			song->PrintChain();
			song->DeleteChain();
		}
		return B_OK;
	}
	
	if (mDataSem < B_OK || (flags&AMPF_RESTART) != 0) {
		if (song || mContinuousRunning) return Restart(song, flags|AMPF_RESTART);
		return B_OK;
	}
	
	BAutolock l(mAccess);
	
	if (song) song = song->HeadEvent();
	
	// If we are restarting, just replace any existing events.
	
	if (flags&(AMPF_RESTART|PRIVATE_RESTART)) {
		mTimeAccess.Lock();
		
		mSharedTempoBeat = 0;
		mSharedBeatOffset = 0;
		mSharedTempoTime = system_time();
		mValidTime = false;
		
		// Make sure the song starts out with the current tempo.  If this
		// is a song, that is accomplished by inserting a tempo event at
		// the front.  Otherwise, we set the 'mSetBPM' flag to tell it to
		// pull the tempo from us directly.  We can't always use the latter
		// method because if the song already starts with a tempo, we may
		// reset it after playing that first tempo event.
		if (song) {
			AmTempoChange* head = new AmTempoChange(mSharedBPM, song->StartTime());
			song->InsertEvent(head);
			song = head;
		} else {
			mSetBPM = true;
		}
		
		mTimeAccess.Unlock();
		
		mEventAccess.Lock();
		
		if (mPosition) mPosition->DeleteChain();
		mPosition = song;
		mTail = song ? song->TailEvent() : NULL;
		song = NULL;
		//printf("Starting song:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
		
		mFinished = false;
		mStartTime = mPosition ? mPosition->StartTime() : 0;
		mRestart = true;
		
		mEventAccess.Unlock();
	}
	
	// Merge given song into existing event chain.  This is
	// complicated because we want to minimize the amount of time
	// we are holding mEventAccess, which blocks the performance
	// thread.
	
	if (song) {
		// An optimization: if some of the new events appear close
		// to the current head event of the song, then pull those
		// out and merge them immediately.
		mEventAccess.Lock();
		if (mPosition) {
			const AmTime limit = mPosition->StartTime()+(PPQN/64);
			if (song->StartTime() < limit) {
				mEventAccess.Unlock();
				AmEvent* head = song->CutForwardAtTime(limit, &song);
				mEventAccess.Lock();
				#if PRINT_CHAINS
				printf("-------------------------------------\n");
				printf("Prepending event chain:\n"); head->PrintChain(AmEvent::PRINT_ADDRESS);
				printf("To:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
				#endif

				mPosition = mPosition->MergeList(head, true)->HeadEvent();
				#if PRINT_CHAINS
				printf("Final chain:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
				#endif
				release_sem(mDataSem);
			}
		}
		mEventAccess.Unlock();
	}
	
	if (song) {
		// First extract out the section of the performance chain
		// that the new events will be merged into.
		mEventAccess.Lock();
		AmEvent* updateChain;
		AmEvent* tail = mTail;
		
		#if PRINT_CHAINS
		printf("-------------------------------------\n");
		printf("Appending event chain:\n"); song->PrintChain(AmEvent::PRINT_ADDRESS);
		printf("To:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
		
		if (mTail) {
			mTail = mTail->CutBackwardAtTime(song->StartTime(), &updateChain);
		} else {
			mPosition = mPosition->CutForwardAtTime(song->StartTime(), &updateChain);
		}
		if (!updateChain) {
			// We didn't cut anything -- so the tail we currently have is not
			// in the part we cut.
			tail = NULL;
		}
		
		#if PRINT_CHAINS
		printf("Cut chain:\n"); updateChain->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
		
		mEventAccess.Unlock();
		
		// Now merge our new events into the corresponding section of
		// the song.
		if (updateChain)
			updateChain->MergeList(song, true);
		else
			updateChain = song;
		updateChain = updateChain->HeadEvent();
		tail = tail ? tail->TailEvent() : updateChain->TailEvent();
		
		// Finally, append the merged event back into the performance data.
		mEventAccess.Lock();
		
		#if PRINT_CHAINS
		printf("Merged chain:\n"); updateChain->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
		
		if (mPosition && !mTail)
			mTail = mPosition->TailEvent();
		if (mTail) {
			mTail->SetNextEvent(updateChain);
			updateChain->SetPrevEvent(mTail);
		} else {
			mPosition = updateChain;
		}
		mTail = tail;
		
		#if PRINT_CHAINS
		printf("Final chain:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
		printf("mPosition=%p, mTail=%p\n", mPosition, mTail);
		printf("-------------------------------------\n");
		#endif
		
		mEventAccess.Unlock();
	}
	
	if ((flags&AMPF_FINISHED) != 0) {
		BAutolock _l(mEventAccess);
		mFinished = true;
	}
	
	// Make sure the playback thread is running.
	ArpD(cdb << ADH << "Resuming performer thread" << endl);
	return release_sem(mDataSem);
}

void AmPerformer::Stop()
{
	if (!mContinuousRunning) {
		StopForReal();
		return;
	}
	
	// If we are continuously running, just remove any existing events
	// from the performer and then let it continue.
	mEventAccess.Lock();
	if (mPosition) {
		mPosition->DeleteChain();
		mPosition = mTail = NULL;
	}
	mRestart = false;
	mFinished = true;
	mPlaying = false;
	mCleanup = true;
	release_sem(mDataSem);
	mEventAccess.Unlock();
}

void AmPerformer::StopForReal()
{
	mAccess.Lock();
	
	ArpD(cdb << ADH << "Stopping performer." << endl);
	
	mEventAccess.Lock();
	sem_id dataSem = mDataSem;
	mDataSem = B_BAD_SEM_ID;
	if (dataSem > B_OK) delete_sem(dataSem);
	thread_id playThread = mPlayThread;
	mRestart = false;
	mPlaying = false;
	mEventAccess.Unlock();
	
	mAccess.Unlock();
	
	status_t ret;
	if (playThread >= B_OK) wait_for_thread(playThread, &ret);
	
	mAccess.Lock();
	if (mPlayThread == playThread) mPlayThread = B_BAD_THREAD_ID;
	
	ArpD(cdb << ADH << "Performer is stopped." << endl);
	
	// All resources should have been cleaned up by the thread.
	// Did it do the right thing?
	ASSERT(mDataSem == B_BAD_SEM_ID);
	ASSERT(mPlayThread == B_BAD_THREAD_ID);
	
	mFinished = true;
	
	mAccess.Unlock();
}

// ---------------- The Performer ----------------

int32 AmPerformer::PlayThreadEntry(void* arg)
{
	ArpD(cdb << ADH << "Enter the performer." << endl);
	AmPerformer *obj = (AmPerformer *)arg;
	
	int32 ret = B_OK;
	
	bool repeat = true;
	while (repeat) {
		repeat = false;
		ret = obj->RunPerformance();
		{
			BAutolock _l(obj->mAccess);
			
			obj->mEventAccess.Lock();
			if( obj->mPosition ) obj->mPosition->DeleteChain();
			obj->mPosition = NULL;
			obj->mTail = NULL;
			obj->mPlaying = false;
			obj->mEventAccess.Unlock();
			
			if( obj->mNewEvents ) obj->mNewEvents->DeleteChain();
			obj->mNewEvents = NULL;
			obj->mPlayThread = B_BAD_THREAD_ID;
			
			if (obj->mDataSem >= B_OK) {
				delete_sem(obj->mDataSem);
				obj->mDataSem = B_BAD_SEM_ID;
			}
		}
	}
	
	ArpD(cdb << ADH << "Exit the performer." << endl);
	return ret;
}

bigtime_t AmPerformer::EventTimeToRealTime(AmTime event) const
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const double PPM = double(PPQN)*mBPM;
	
	// This is the number of microseconds from the start of
	// the song that the next event should be performed.
	return bigtime_t( (bigtime_t(event-mTempoBeat) * 60*1000000) / PPM );
}

AmTime AmPerformer::RealTimeToEventTime(bigtime_t real) const
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const double PPM = double(PPQN)*mBPM;
	
	return AmTime( mTempoBeat + ( (real*PPM) / (60*1000000) ) );
}

#if 0
bigtime_t AmPerformer::EventTimeToRealTime(AmTime event) const
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const bigtime_t PPM = bigtime_t(PPQN)*mBPM;
	
	// This is the number of microseconds from the start of
	// the song that the next event should be performed.
	return (bigtime_t(event-mTempoBeat) * 60*1000000) / PPM;
}

AmTime AmPerformer::RealTimeToEventTime(bigtime_t real) const
{
	// This is the number of sequencer pulses that are
	// running every minute.
	const bigtime_t PPM = bigtime_t(PPQN)*mBPM;
	
	return AmTime( mTempoBeat + ( (real*PPM) / (60*1000000) ) );
}
#endif

void AmPerformer::ClearNewEvents()
{
	am_filter_params params;
	params.performance_time = system_time();
	while( mNewEvents ) {
		AmEvent* next = mNewEvents;
		mNewEvents = mNewEvents->RemoveEvent();
		ExecuteEvent(next, &params);
	}
}

AmTime AmPerformer::NextEventBeat(bool* out_found)
{
	BAutolock _l(mEventAccess);
	
	if( mCleanup ) {
		ClearNewEvents();
		mCleanup = false;
	}
	
	const bool restarting = mRestart;
	if( mRestart ) {
		// Send any waiting events.
		ClearNewEvents();
		#if 1
		mTempoBeat = mStartTime;
		mBaseTime = system_time();
		#else
		mTempoBeat = 0;
		mBaseTime = system_time() - EventTimeToRealTime(mStartTime);
		#endif
		mTempoTime = mBaseTime;
		mBeatOffset = 0;
		mRestart = false;
		mPlaying = true;
	}
	
	{
		// Propogate current song location up to callers.
		BAutolock l(mTimeAccess);
		if (mSetBPM) {
			if (mBPM != mSharedBPM) {
				bigtime_t cur = system_time();
				mTempoBeat = RealTimeToEventTime(cur-mTempoTime);
				mTempoTime = cur;
			}
			mBPM = mSharedBPM;
		} else mSharedBPM = mBPM;
		mSharedTempoBeat = mTempoBeat;
		mSharedTempoTime = mTempoTime;
		mSharedBeatOffset = mBeatOffset;
		if (restarting) mNextClock = 0;
		mValidTime = true;
		mSetBPM = false;
	}
	
	#if ArpDEBUG
	if (mPosition) mPosition->AssertTimeOrder();
	if (mNewEvents) mNewEvents->AssertTimeOrder();
	#endif
	
	AmTime nextBeat = B_INFINITE_TIMEOUT;
	if( mPosition && (!mNewEvents || (mPosition->QuickCompare(mNewEvents) <= 0)) ) {
		*out_found = true;
		nextBeat = mPosition->StartTime();
	} else if( mNewEvents ) {
		*out_found = true;
		nextBeat = mNewEvents->StartTime();
	} else {
		*out_found = false;
	}
	
	if (mFinished)
		mPlaying = false;
	
	//printf("Returning next beat: event=%Ld, clock=%Ld\n", nextBeat, mNextClock);
	return nextBeat < mNextClock ? nextBeat : mNextClock;
}

AmEvent* AmPerformer::NextEvents()
{
	bool haveEvent = false;
	AmTime nextBeat = NextEventBeat(&haveEvent);
	
	bigtime_t nextTime;
	if (nextBeat < B_INFINITE_TIMEOUT) {
		nextTime = EventTimeToRealTime(nextBeat) + mTempoTime;
	} else if (mFinished && !mContinuousRunning) {
		return NULL;
	} else {
		nextTime = B_INFINITE_TIMEOUT;
	}
	
	ArpD(cdb << ADH << "Next note beat " << nextBeat << " at " << nextTime
				<< " us (" << (float(nextTime)/1000000) << " secs)"
				<< ", mBaseTime=" << mBaseTime << endl);
	
	bigtime_t curTime = system_time();
	
	#if PRINT_CHAINS
	printf("Next event beat %Ld, time from now is %Ld\n",
			nextBeat, nextTime-curTime);
	#endif
	if( nextTime > curTime ) {
		// Wait until the desired system time.
		status_t err = mDataSem;
		while( mDataSem >= B_OK &&
			   (err=acquire_sem_etc(mDataSem, 1, B_ABSOLUTE_TIMEOUT, nextTime)) == B_INTERRUPTED ) {
		}
	}
	
	ArpD(cdb << ADH << "Done waiting, time now is "
				<< (system_time()-mBaseTime)
				<< " us (" << (float(system_time()-mBaseTime)/1000000) << " secs)"
				<< ", mBaseTime=" << mBaseTime << endl);
	
	if (mDataSem < B_OK) {
		// Bail now if we are shutting down.
		ArpD(cdb << ADH << "NextEvents(): Shutting down." << endl);
		return NULL;
	}
	
	if (mRestart || mCleanup) {
		// Whoops, we are a victim of premature execution.  Don't make a mess!
		ArpD(cdb << ADH << "NextEvents(): Premature execution!" << endl);
		return NULL;
	}
	
	// After all the time that has gone by, this is the last beat that
	// should have already been played.
	curTime = system_time();
	const AmTime endBeat = RealTimeToEventTime(curTime-mTempoTime)+1;
	ArpD(cdb << ADH << "NextEvents(): Collecting up to beat " << endBeat << endl);
	
	// Send any requested clock right now.  Yes, the locking here is gross.
	// Deal or clean it up yourself.
	mTimeAccess.Lock();
	if (mClockTarget) {
		// If the previously requested clock is outside of the current position,
		// reset it to where we are now.
		if (mNextClock <= (endBeat-(PPQN/12)) || mNextClock >= (endBeat+(PPQN/12))) {
			//printf("NextClock is %Ld, endBeat is %Ld, reset clock to %Ld\n",
			//		mNextClock, nextBeat, (endBeat/(PPQN/24))*(PPQN/24));
			mNextClock = (endBeat/(PPQN/24))*(PPQN/24);
		}
		if (mNextClock <= endBeat) {
			AmTime thisClock = mNextClock;
			AmClockTarget* target = mClockTarget;
			mNextClock += PPQN/24;
			mTimeAccess.Unlock();
			target->Clock(thisClock);
			//printf("Performer %p sent clock %Ld to %p\n", this, thisClock, target);
		} else {
			mTimeAccess.Unlock();
		}
	} else {
		mNextClock = B_INFINITE_TIMEOUT;
		mTimeAccess.Unlock();
	}
	
	// Now collect all of the events to be performed at this time.  In the
	// case of premature executation, we just won't find any events and return
	// NULL to try again.
	AmEvent* result = NULL;
	mEventAccess.Lock();
	
	#if PRINT_CHAINS
	printf("-------------------------------------------------\n");
	printf("Collecting up to beat: %Ld\n", endBeat);
	#endif
	
	if (mPosition) {
		#if PRINT_CHAINS
		printf("Extracting performance events:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
		
		result = mPosition->CutForwardAtTime(endBeat, &mPosition);
		if (mPosition == NULL)
			mTail = NULL;
		
		#if PRINT_CHAINS
		printf("Returned events:\n"); result->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
	}
	
	#if !PRINT_CHAINS
	mEventAccess.Unlock();
	#endif
	
	if (mNewEvents) {
		#if PRINT_CHAINS
		printf("Extracting new evets:\n"); mPosition->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
		
		AmEvent* extra = mNewEvents->CutForwardAtTime(endBeat, &mNewEvents);
		
		#if PRINT_CHAINS
		printf("Returned events:\n"); extra->PrintChain(AmEvent::PRINT_ADDRESS);
		#endif
		
		if (extra) {
			if (!result) {
				ArpD(cdb << ADH << "NextEvents(): Using mNewEvents." << endl);
				result = extra;
			} else {
				result = result->MergeList(extra, true)->HeadEvent();
				ArpD(cdb << ADH << "NextEvents(): head of merge=" << result << endl);
			}
		}
	}
	
	#if PRINT_CHAINS
	printf("Final events:\n"); result->PrintChain(AmEvent::PRINT_ADDRESS);
	printf("-------------------------------------------------\n");
	mEventAccess.Unlock();
	#endif
	
	//printf("Now performing:\n"); result->PrintChain(AmEvent::PRINT_ADDRESS);
	
	return result;
}

void AmPerformer::ExecuteEvent(AmEvent* event, am_filter_params* params)
{
	// Run the event through its next filter, which it presumably
	// will in some way "perform".  (Since any filters that reach
	// this place will be marked as an output filter.)
	AmFilterHolderI* filter = event->NextFilter();
	if (filter) {
		event->SetNextFilter(filter->FirstConnection());
		event = filter->Filter()->HandleRealtimeEvent(event);
		AmEvent* pos = event;
		while (pos) {
			if (pos->NextFilter()) pos = pos->NextEvent();
			else {
				AmEvent* last = pos;
				if (pos == event) event = pos = pos->NextEvent();
				else pos = pos->NextEvent();
				last->Delete();
			}
		}
		if (event) {
			ArpD(cdb << ADH << "Filter generated events:"
						<< endl << ADH; event->Print());
			event = event->HeadEvent();
			if( mNewEvents ) mNewEvents = mNewEvents->MergeList(event)->HeadEvent();
			else mNewEvents = event;
		}
	} else {
		// Just delete this event.
		ArpD(cdb << ADH << "Dropping event without filter:"
					<< endl << ADH; event->Print());
		event->DeleteChain();
	}
}

static bigtime_t lastTimer, thisTimer;

int32 AmPerformer::RunPerformance()
{
	mBPM = mSharedBPM;
	mTempoBeat = 0;
	mBaseTime = system_time();
	mTempoTime = mBaseTime;
	mBeatOffset = 0;
	mNewEvents = NULL;
	ArpD(lastTimer = system_time());
	
	am_filter_params params;
	#if SHOW_ERROR
	bigtime_t error = 0;
	#endif
//printf("Run 1\n");
	while (mDataSem >= B_OK &&
			(mPosition || mNewEvents || !mFinished || mContinuousRunning) ) {
//printf("Run 2\n");	-- Here's the loop
		
		ArpD(cdb << ADH << "Ready for next: mPosition="
					<< mPosition << ", mNewEvents=" << mNewEvents << endl);
		
		ArpD(thisTimer = system_time());
		
		ArpD(cdb << ADH << "Start=" << lastTimer << ", end=" << thisTimer
						<< ", taken=" << (thisTimer-lastTimer) << endl);
		
		#if SHOW_ERROR
		if( error > 0 ) {
			printf("Error: %Ld\n", error);
			error = 0;
		}
		#endif
		
		AmEvent* events = NextEvents();
		AmTime curBeat = -100000;
		while (events) {
			if (curBeat != events->StartTime()) {
				curBeat = events->StartTime();
				params.performance_time = EventTimeToRealTime(curBeat)+mTempoTime;
			}
			
			AmEvent* cur = events;
			events = events->NextEvent();
			cur->SetPrevEvent(NULL);
			cur->SetNextEvent(NULL);
			
			// Directly process tempo events.
			if( cur->Type() == AmEvent::TEMPOCHANGE_TYPE ) {
				AmTempoChange* tempo =
					dynamic_cast<AmTempoChange*>(cur);
				if( tempo ) {
					const float newTempo = tempo->Tempo();
					if (newTempo != mBPM) {
						mBPM = newTempo;
						mTempoTime = params.performance_time;
						mTempoBeat = curBeat;
						printf("New tempo is %f bpm, at time %lld and beat %lld\n",
								mBPM, params.performance_time, curBeat);
					}
				}
		
			} else if( cur->Type() == AmEvent::PERFORMER_TYPE ) {
				AmPerformerEvent* pe =
					dynamic_cast<AmPerformerEvent*>(cur);
				if( pe ) {
					if( pe->BeatOffset() >= 0 )
						mBeatOffset = pe->BeatOffset();
				}
			}
			
			ExecuteEvent(cur, &params);
			#if SHOW_ERROR
			bigtime_t delta = system_time()-params.performance_time;
			if( delta > error ) error = delta;
			#endif
		}
	}
	
	// Send any waiting events.
	ClearNewEvents();
	
	return 0;
}
