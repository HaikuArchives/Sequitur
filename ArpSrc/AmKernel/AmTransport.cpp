/* AmTransport.cpp
*/

#define _BUILDING_AmKernel 1

#include "AmKernel/AmTransport.h"

#include <ArpKernel/ArpDebug.h>
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmTrackLookahead.h"

#include <Autolock.h>
#include <Messenger.h>
#include <MessageRunner.h>
#include <scheduler.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

ArpMOD();

const char* ArpTransportState = "transport_state";
const char* ArpTransportTime = "transport_time";
const char* ArpTransportTempo = "transport_tempo";
const char* ArpTransportNextTime = "transport_next_time";

//#define SHOW_TRANSPORT_MSGS 1

namespace ArpPrivate {

class TransportLooper : public BLooper
{
public:
	enum {
		kCheckState	= 0x0001,
		kSendAll	= 0x0002
	};
	
	TransportLooper(AmTransport* transport,
					const char *name = "ARP™ Midi Transport Looper",
					int32 priority = B_NORMAL_PRIORITY,
					int32 port_capacity = B_LOOPER_PORT_DEFAULT_CAPACITY)
		: BLooper(name, priority, port_capacity),
		  mTransport(transport),
		  mState(TS_STOPPED),
		  mCurLoop(0), mNextID(0), mHasStarted(false),
		  mPulse(NULL), mPulseTime(0)
	{
	}
	
	virtual ~TransportLooper()
	{
		delete mPulse;
		mPulse = NULL;
	}

	virtual void MessageReceived(BMessage *msg)
	{
printf("MessageReceived 1\n");
		if (!mTransport) {
			BLooper::MessageReceived(msg);
			return;
		}
printf("MessageReceived 2\n");
		
		ArpD(cdb << ADH << "Received: " << *msg << endl);
		if (mState != TS_STOPPED && SendReport(kCheckState, msg) == B_OK) {
			return;
		} else if (msg->what == 'puls') {
			delete mPulse;
			mPulse = NULL;
			if (mState != TS_STOPPED) SendReport(kCheckState);
			return;
		}
		
		BLooper::MessageReceived(msg);
	}
	
	void Shutdown()
	{
		mTransport = NULL;
		Quit();
	}
	
	status_t StartWatching(BMessenger who)
	{
		for (size_t i=0; i<mObservers.size(); i++) {
			if (mObservers[i].target == who) return B_NAME_IN_USE;
		}
		observer obs;
		obs.target = who;
		obs.sendTime = obs.nextTime = AM_TIME_MIN;
		obs.id = mNextID++;
		obs.sequence = 0;
		obs.updateSent = false;
		mObservers.push_back(obs);
		return B_OK;
	}
	
	status_t StopWatching(BMessenger who)
	{
		size_t i, j;
		for (i=0, j=0; i<mObservers.size(); i++) {
			if (j < i) mObservers[j] = mObservers[i];
			if (mObservers[i].target != who) j++;
		}
		if (j < i) {
			mObservers.resize(j);
			return B_OK;
		}
		return B_NAME_NOT_FOUND;
	}
	
	status_t SetState(transport_state state)
	{
printf("SetState 1\n");
		if (state == mState) return B_OK;
		
		mState = state;
		mHasStarted = false;
		delete mPulse;
		mPulse = NULL;
		for (size_t i=0; i<mObservers.size(); i++) {
			mObservers[i].sendTime = mObservers[i].nextTime = AM_TIME_MIN;
			mObservers[i].sequence = 0;
			mObservers[i].updateSent = false;
		}
		
		return SendReport(kSendAll);
	}
	
	transport_state State() const
	{
		return mState;
	}
	
	status_t SendReport(uint32 flags, const BMessage* reply = NULL)
	{
printf("SendReport 1\n");
		// If called with a reply message, first check to see that it is the kind
		// of reply we want.  If not, immediately return with an error.
		const BMessage* prev;
		if (reply) {
			prev = reply->Previous();
			if (!prev || prev->what != TRANSPORT_CHANGE_MSG) return B_ERROR;
		} else {
			prev = NULL;
		}
		
printf("SendReport 2\n");
		if (!mTransport) return B_CANCELED;
printf("SendReport 3\n");
		
		//printf("Before report: state=%d, playing=%d, running=%d\n",
		//		(int)mState, mTransport->IsPlaying(), mTransport->IsRunning());
				
		if (flags&kCheckState) {
			ArpD(cdb << ADH << "Before report, checking: state="
				<< (int)mState << ", playing=" << mTransport->IsPlaying() << endl);
			if (mState != TS_STOPPED && !mTransport->IsRunning()) {
				return SetState(TS_STOPPED);
			}
		}
		
printf("SendReport 4\n");
		if (mTransport->IsPlaying() || mHasStarted) {
			mHasStarted = true;
			
			const AmTime curTime = mTransport->CurrentTime();
			const AmTime curLoop = mTransport->LoopOffset();
			AmTime nextTime = AM_TIME_MAX;
			
			int32 replyID, replySequence;
			if (!prev || prev->FindInt32("id", &replyID) != B_OK) replyID = -1;
			if (!prev || prev->FindInt32("sequence", &replySequence) != B_OK) replySequence = -1;
			
			BMessage msg(TRANSPORT_CHANGE_MSG);
			msg.AddInt32(ArpTransportState, mState);
			add_time(msg, ArpTransportTime, curTime);
			msg.AddFloat(ArpTransportTempo,mTransport->BPM());
			msg.AddInt32("id", -1);
			msg.AddInt32("sequence", -1);
			msg.AddBool("play_to_end", mTransport->PlayToEnd());
			ArpD(cdb << ADH << "Sending: " << msg << endl);
			
			size_t i, j;
			status_t err;
			for (i=0, j=0; i<mObservers.size(); i++) {
				if (j < i) mObservers[j] = mObservers[i];
				observer& obs = mObservers[j];
				if (obs.id == replyID && obs.sequence == replySequence) {
					obs.updateSent = false;
					if (find_time(*reply, ArpTransportNextTime, &obs.nextTime) != B_OK) {
						obs.nextTime = obs.sendTime + (PPQN/4);
					}
				}
				if ((flags&kSendAll) || curLoop != mCurLoop
						|| (!obs.updateSent && obs.nextTime <= curTime)) {
					msg.ReplaceInt32("id", obs.id);
					msg.ReplaceInt32("sequence", ++(obs.sequence));
					obs.sendTime = curTime;
					obs.updateSent = true;
					err = obs.target.SendMessage(&msg, this, 100*1000);
				} else {
					err = B_OK;
				}
				if (err == B_OK) {
					j++;
					if (!obs.updateSent) {
						if (nextTime > obs.nextTime) nextTime = obs.nextTime;
					}
				} else {
					printf("*** Failure sending observation message!\n");
				}
			}
			if (j < i) mObservers.resize(j);
			ArpD(cdb << ADH << "Sent " << j << " transport messages" << endl);
			
			mCurLoop = curLoop;
			
			if (nextTime != AM_TIME_MAX) {
				bigtime_t sysrealtime = system_time();
				bigtime_t nextrealtime = mTransport->FutureTime(nextTime);
				ArpD(cdb << ADH << "Next report time in "
						<< (nextrealtime-sysrealtime) << " us "
						<< "(current time " << sysrealtime
						<< ", next time " << nextrealtime << ")" << endl);
				if (!mPulse || nextrealtime < mPulseTime) {
					delete mPulse;
					mPulse = NULL;
					BMessage pulse('puls');
					mPulse = new BMessageRunner(BMessenger(this), &pulse,
												nextrealtime-sysrealtime, 1);
					mPulseTime = nextrealtime;
				}
			}
		} else if (mTransport->IsRunning()) {
			ArpD(cdb << ADH << "Skipping this report, setting up pulse for next"
					<< endl);
			delete mPulse;
			mPulse = NULL;
			BMessage pulse('puls');
			mPulse = new BMessageRunner(BMessenger(this), &pulse,
										10*1000, 1);
			mPulseTime = system_time() + 10*1000;
		} else if ((flags&kSendAll) && !mTransport->IsRunning()) {
			const AmTime curTime = mTransport->CurrentTime();
			const AmTime curLoop = mTransport->LoopOffset();
			
			BMessage msg(TRANSPORT_CHANGE_MSG);
			msg.AddInt32(ArpTransportState, mState);
			add_time(msg, ArpTransportTime, curTime);
			msg.AddFloat(ArpTransportTempo,mTransport->BPM());
			msg.AddInt32("id", -1);
			msg.AddInt32("sequence", -1);
			msg.AddBool("play_to_end", mTransport->PlayToEnd());
			ArpD(cdb << ADH << "Sending: " << msg << endl);
			size_t i;
			for (i=0; i<mObservers.size(); i++) {
				observer& obs = mObservers[i];
				msg.ReplaceInt32("id", obs.id);
				msg.ReplaceInt32("sequence", ++(obs.sequence));
				obs.sendTime = AM_TIME_MIN;
				obs.updateSent = false;
				obs.target.SendMessage(&msg, (BHandler*)NULL, 100*1000);
			}
			mCurLoop = curLoop;
			delete mPulse;
			mPulse = NULL;
		}
		
		return B_OK;
	}
	
	int32 IndexOfObserver(const BMessenger& obs) const
	{
		size_t N = mObservers.size();
		for (size_t i=0; i<N; i++) {
			if (mObservers[i].target == obs) return i;
		}
		return -1;
	}
	
private:
	AmTransport*		mTransport;
	
	struct observer {
		BMessenger target;
		AmTime sendTime;
		AmTime nextTime;
		int32 id;
		int32 sequence;
		bool updateSent;
	};
	vector<observer>	mObservers;
	
	transport_state		mState;
	AmTime				mCurLoop;
	int32				mNextID;
	bool				mHasStarted;
	
	BMessageRunner*		mPulse;
	bigtime_t			mPulseTime;
};

// ----------------------------------------------------------------

class TransportEvent : public AmEvent
{
public:
						TransportEvent(AmTime time, sem_id semaphore, bool final=false)
							: AmEvent(time), mSemaphore(semaphore), mFinal(final) { }
						TransportEvent(const TransportEvent& o)
							: AmEvent(o), mSemaphore(o.mSemaphore), mFinal(o.mFinal) { }

	virtual EventType	Type() const  		{ return TRANSPORT_TYPE; }

	virtual AmEvent*	Copy() const		{ return new TransportEvent(*this); }
	virtual void		Print(void) const
	{
		printf("TransportEvent at %lld sem: %ld, final=%d\n",
				mTime, mSemaphore, mFinal);
	}

	virtual void		Delete()
	{
		//printf("*** TransportEvent deleted final=%d\n", mFinal);
		if (mFinal)
			delete_sem(mSemaphore);
		else
			release_sem(mSemaphore);
		AmEvent::Delete();
	}
	
	void*				operator new(size_t size)
	{
		ArpASSERT( size == sizeof(TransportEvent) );
		return GetEvent( TRANSPORT_TYPE, size );
	}
	void				operator delete(void* ptr, size_t size)
	{
		ArpASSERT( size == sizeof(TransportEvent) );
		SaveEvent( TRANSPORT_TYPE, ptr );
	}

protected:
	virtual				~TransportEvent()		{ }
	TransportEvent&		operator=(const TransportEvent& o)
	{
		AmEvent::operator=(o);
		mSemaphore = o.mSemaphore;
		return *this;
	}

private:
	sem_id				mSemaphore;
	bool				mFinal;
	
	void Initialize();
};

enum which_hook {
	start_hook,
	finish_hook,
	stop_hook,
	panic_hook
};

static AmEvent* call_track_hooks(const AmTrack* track,
								 AmTime firstTime, AmTime lastTime,
								 which_hook hook, am_filter_params* params)
{
	AmEvent* result = NULL;
	for (int32 i=0; i<2; i++) {
		AmFilterHolderI* f = track->Filter(i==0 ? INPUT_PIPELINE
												: OUTPUT_PIPELINE);
		while (f) {
			if (!f->IsBypassed()) {
				AmEvent* ev = NULL;
				switch (hook) {
					case start_hook:
						ev = f->Filter()->StartSection(firstTime, lastTime, params);
						break;
					case finish_hook:
						ev = f->Filter()->FinishSection(firstTime, lastTime, params);
						break;
					case stop_hook:
						f->Filter()->Stop(AmFilterI::TRANSPORT_CONTEXT);
						break;
					case panic_hook:
						f->Filter()->Stop(AmFilterI::PANIC_CONTEXT);
						break;
				}
				if (ev) {
					AmEvent* e = ev;
					while (e) {
						if (!e->NextFilter()) e->SetNextFilter(f);
						e = e->NextEvent();
					}
					result = result->MergeList(ev, true);
				}
			}
			f = f->NextInLine();
		}
	}
	return result;
}

static AmEvent* call_hooks(const AmSong* song,
							AmTime firstTime, AmTime lastTime,
							which_hook hook, am_filter_params* params)
{
	AmEvent* result = NULL;
	const uint32 N = song->CountTracks();
	for (uint32 i=0; i<N; i++) {
		const AmTrack* t = song->Track(i);
		if (!t) continue;
		AmEvent* ev = call_track_hooks(t, firstTime, lastTime, hook, params);
		result = result->MergeList(ev, true);
	}
	return result;
}

}	// namespace ArpPrivate

using namespace ArpPrivate;

/* ----------------------------------------------------------------
   AmTransport Implementation
   ---------------------------------------------------------------- */
   
AmTransport::AmTransport(AmSongRef songRef, bool continuous_running)
	: AmSongObserver(songRef),
	  mFeedThread(B_BAD_THREAD_ID), mSyncSem(B_BAD_SEM_ID),
	  mLoopStart(-1), mLoopStop(-1), mLooping(false), mRunning(false), mExiting(false),
	  mPerformer(continuous_running)
{
	mLooper = new TransportLooper(this);
	mLooper->Run();
}

AmTransport::AmTransport(song_id songId, bool continuous_running)
	: AmSongObserver(songId),
	  mFeedThread(B_BAD_THREAD_ID), mSyncSem(B_BAD_SEM_ID),
	  mLoopStart(-1), mLoopStop(-1), mLooping(false), mRunning(false), mExiting(false), mPlayToEnd(false),
	  mPerformer(continuous_running)
{
	mLooper = new TransportLooper(this);
	mLooper->Run();
}

AmTransport::~AmTransport()
{
	Stop(AmFilterI::TRANSPORT_CONTEXT);
	if (mLooper->Lock()) mLooper->Shutdown();
}

AmTime AmTransport::CurrentTime() const
{
	BAutolock l(mAccess);
	
	AmTime time = mPerformer.CurrentTime();
	if (!Looping() || !IsRunning()) return time;
	
	return ( (time-mStartTime) % (mStopTime-mStartTime) ) + mStartTime;
}
	
bigtime_t AmTransport::FutureTime(AmTime time) const
{
	BAutolock l(mAccess);
	return mPerformer.FutureTime(time);
}

AmTime AmTransport::LoopOffset() const
{
	BAutolock l(mAccess);
	return mPerformer.LoopOffset();
}

bool AmTransport::IsPlaying() const
{
	return mPerformer.IsPlaying();
}

AmTime AmTransport::RealtimeToPulse(bigtime_t time, bool includeFuture) const
{
	BAutolock l(mAccess);
	AmTime pulse = mPerformer.RealtimeToPulse(time, includeFuture);
	return pulse >= 0 ? pulse : -1;
}

bigtime_t AmTransport::PulseToRealtime(AmTime pulse, bool includeFuture) const
{
	BAutolock l(mAccess);
	return mPerformer.PulseToRealtime(pulse, includeFuture);
}

status_t AmTransport::StartWatching(BMessenger observer)
{
	BAutolock _el(mLooper);
	return mLooper->StartWatching(observer);
}

status_t AmTransport::StopWatching(BMessenger observer)
{
	BAutolock _el(mLooper);
	return mLooper->StopWatching(observer);
}

void AmTransport::SetLooping(bool state, AmTime start, AmTime stop)
{
	BAutolock _el(mLooper);
	mLooping = state;
	mLoopStart = start;
	mLoopStop = stop;
}

bool AmTransport::Looping() const
{
	return mLooping;
}
	
status_t AmTransport::Start(AmTime startTime, AmTime stopTime)
{
	return Start(AmTrackRef(), startTime, stopTime);
}

status_t AmTransport::Start(AmTrackRef solo,
							AmTime startTime, AmTime stopTime)
{
	mPlayToEnd = false;
	if (stopTime < startTime) {
		#ifdef AM_TRACE_LOCKS
		printf("AmTransport::Start() read lock\n"); fflush(stdout);
		#endif
		const AmSong* song = ReadLock();
		if (!song) return B_NO_INIT;
		if (!solo.IsValid()) stopTime = song->CountEndTime();
		else {
			const AmTrack* track = song->Track(solo);
			if (track) stopTime = track->EndTime2();
		}
		song->StartFiltersHack();
		ReadUnlock(song);
		mPlayToEnd = true;
	}
	
	ArpD(cdb << "transport start " << startTime
			<< " stop " << stopTime << endl);
	ArpASSERT(startTime >= 0 && stopTime >= 0);
	
	if (startTime >= stopTime) return B_OK;
	
	BAutolock _el(mLooper);
	
	Stop(AmFilterI::TRANSPORT_CONTEXT);
	
	#if ArpDEBUG
	int32 sugPri = suggest_thread_priority(B_AUDIO_PLAYBACK,
										   1000000/32, 2000, 100);
	ArpD(cdb << ADH << "Suggested thread pri=" << (int) sugPri << endl);
	#endif
	
	BAutolock l(mAccess);
	
	ASSERT(mPlayThread == B_BAD_THREAD_ID);
	ASSERT(mSyncSem == B_BAD_SEM_ID);
	
	mFeedThread = spawn_thread(FeedThreadEntry, "ARP™ Midi Transport Feeder",
							   B_URGENT_DISPLAY_PRIORITY, this);
	if( mFeedThread < 0 ) {
		Stop(AmFilterI::TRANSPORT_CONTEXT);
		return (status_t)mFeedThread;
	}
	
	mSoloTrack = solo;
	mStartTime = startTime;
	mStopTime = stopTime;
	mLoopTime = 0;
	mRunning = true;
	mExiting = false;
	
	status_t ret = resume_thread(mFeedThread);
	if( ret != B_OK ) Stop(AmFilterI::TRANSPORT_CONTEXT);
	else {
		while (!IsRunning()) snooze(1000);
		mLooper->SetState(TS_PLAYING);
	}
	
	return ret;
}

status_t AmTransport::Merge(AmEvent* events)
{
	if (events) mPerformer.Play(events, 0);
	return B_OK;
}

void AmTransport::Stop(uint32 context)
{
	BAutolock _el(mLooper);
	
	int32 ret = B_OK;
	
	mAccess.Lock();
	
	mRunning = false;
	delete_sem(mSyncSem);
	wait_for_thread(mFeedThread,&ret);
	#if 0
	while( mFeedThread >= 0 ) {
		ArpD(cdb << ADH << "Interrupt feeder " << mFeedThread << endl);
		suspend_thread(mFeedThread);
		while ((ret=resume_thread(mFeedThread)) == B_INTERRUPTED) ;
		if (ret != B_OK) mFeedThread = -1;
		
		mAccess.Unlock();
		ArpD(cdb << ADH << "Waiting for feeder." << endl);
		//snooze(20000);
		wait_for_thread(mFeedThread,&ret);
		mAccess.Lock();
		
		ArpD(cdb << ADH << "Checking feeder again." << endl);
	}
	#endif
	
	ArpD(cdb << ADH << "Feeder is stopped." << endl);
	
	mPerformer.Stop();
	
	#ifdef AM_TRACE_LOCKS
	printf("AmTransport::Stop() read lock\n"); fflush(stdout);
	#endif
	const AmSong* song = ReadLock();
	if (song) {
		call_hooks(song, 0, 0, context == AmFilterI::PANIC_CONTEXT ? panic_hook : stop_hook, NULL);
		ReadUnlock(song);
	}
	
	mAccess.Unlock();
	
	mLooper->SetState(TS_STOPPED);
}

// ---------------- The Feeder ----------------

int32 AmTransport::FeedThreadEntry(void* arg)
{
	ArpD(cdb << ADH << "Enter the feeder." << endl);
	AmTransport *obj = (AmTransport *)arg;
	obj->mSyncSem = create_sem(2, "ARP™ Midi Transport Sync");
	if (obj->mSyncSem < B_OK) return obj->mSyncSem;
	int ret = obj->FeedPerformance();
	if (obj->mSyncSem >= B_OK) {
		delete_sem(obj->mSyncSem);
		obj->mSyncSem = B_BAD_SEM_ID;
	}
	obj->mFeedThread = B_BAD_THREAD_ID;
	obj->mRunning = false;
	ArpD(cdb << ADH << "Exit the feeder." << endl);
	return ret;
}

static const AmTime		gBufferTime = PPQN;
//static const AmTime		gBufferTime = PPQN / 128;

int32 AmTransport::FeedPerformance()
{
	// retrieve reference to song, so that we don't have to
	// lock the transport while working with it
	AmTrackRef soloTrack;
	AmTime startTime, stopTime;
	{
		BAutolock l(&mAccess);
		soloTrack = mSoloTrack;
		startTime = mStartTime;
		stopTime = mStopTime;
	}
	
	AmTrackLookahead		lookahead;
	
	ArpD(cdb << ADH << "Playing song: from " << startTime
			<< " to " << stopTime << endl);
	ArpASSERT(startTime >= 0 && stopTime >= 0);
	
	bool				firstTime = true;
	bool				firstNote = true;
	status_t			err = B_OK;
	
	am_filter_params	params;
	AmEvent*			tempos = NULL;
	AmEvent*			signatures = NULL;
	bool				madeMotions = false;
	
	while (mRunning && err == B_OK && startTime <= stopTime) {
		const bool wasFirstTime = firstTime;
		const bool wasFirstNote = firstNote;
		AmTime nextTime = startTime + (gBufferTime*(firstNote ? 2 : 1));
		if (nextTime > stopTime) nextTime = stopTime+1;
		firstTime = false;
		firstNote = false;
		
		#if SHOW_TRANSPORT_MSGS
		ArpD(cdb << ADH << "Playing from "
						<< startTime << " to " << nextTime << endl);
		#endif
		ArpASSERT(startTime >= 0 && nextTime >= 0);
		
		#ifdef AM_TRACE_LOCKS
		printf("AmTransport::FeedPerformance() read lock\n"); fflush(stdout);
		#endif
		const AmSong* song = ReadLock();
		const AmTrack* track = NULL;
		
		#if SHOW_TRANSPORT_MSGS
		ArpD(cdb << ADH << "Reading song " << song << endl);
		#endif
		if (!song) return B_ERROR;
		if (!madeMotions) {
			params.AddMotionChanges(song);
			madeMotions = true;
		}
		const uint32 baseFlags = wasFirstTime ? 0 : PLAYBACK_NO_CONTEXT;
		
		AmEvent* lastTempo = tempos ? tempos->TailEvent() : NULL;
		if (lastTempo) {
			if (lastTempo != tempos) {
				lastTempo->RemoveEvent();
				tempos->DeleteChain();
			}
		}
		tempos = song->PlaybackList(startTime, nextTime-1,
									baseFlags|PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_SIGNATURE
									|PLAYBACK_RAW_CONTEXT);
		
		AmEvent* lastSignature = signatures ? signatures->TailEvent() : NULL;
		if (lastSignature) {
			if (lastSignature != signatures) {
				lastSignature->RemoveEvent();
				signatures->DeleteChain();
			}
		}
		signatures = song->PlaybackList(startTime, nextTime-1,
									baseFlags|PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_TEMPO
									|PLAYBACK_RAW_CONTEXT);
		
		AmEvent* event = NULL;
		AmEvent* ev = NULL;
		if (wasFirstTime) {
			lookahead.SetTracks(song);
		}

		if (!soloTrack.IsValid()) {
			event = song->PlaybackList(startTime, nextTime-1,
										baseFlags|PLAYBACK_NO_TEMPO|PLAYBACK_NO_SIGNATURE);
			lookahead.MakeChains(song, nextTime);
		} else {
			track = song->Track(soloTrack);
			if (track) {
				event = track->PlaybackList(startTime, nextTime-1,
											baseFlags|PLAYBACK_NO_TEMPO|PLAYBACK_NO_SIGNATURE);
				lookahead.MakeChains(track, nextTime);
			}
		}
		
		if (tempos) {
			tempos = tempos->HeadEvent();
			
			// Merge this section's tempo list into the complete performance
			// event chain, making sure the first tempo event does not occur
			// before this section's start time.
			AmEvent* performTempos = tempos->CopyChain();
			if (performTempos) {
				if (performTempos->StartTime() < startTime)
					performTempos->SetStartTime(startTime);
				if (event) {
					event->MergeList(performTempos);
					event = event->HeadEvent();
				} else {
					event = performTempos;
				}
			}
			
			// Insert the last section's final tempo event into the front
			// of this section's tempo chain.
			if (lastTempo) {
				if (tempos->StartTime() > startTime) {
					tempos->MergeEvent(lastTempo);
					tempos = tempos->HeadEvent();
				} else {
					// This chain has a tempo right at the beginning, so
					// we can completely forget about the previous context.
					lastTempo->DeleteChain();
					lastTempo = NULL;
				}
			}
			
		} else {
			tempos = lastTempo;
		}
		
		if (signatures) {
			signatures = signatures->HeadEvent();
			// We don't merge these into the playback list, because
			// nothing else is going to make use of them.
			if (lastSignature) {
				if (signatures->StartTime() > startTime) {
					signatures->MergeEvent(lastSignature);
					signatures = signatures->HeadEvent();
				} else {
					// This chain has a signature right at the beginning, so
					// we can completely forget about the previous context.
					lastSignature->DeleteChain();
					lastSignature = NULL;
				}
			}
		} else {
			signatures = lastSignature;
		}
		
		#if SHOW_TRANSPORT_MSGS
		ArpD(cdb << ADH << "Retrieved events " << event << endl);
		#endif
		
		params.flags = (song->IsRecording() ? AMFF_RECORDING : 0)
					 | (wasFirstNote ? AMFF_STARTING : 0);
		params.performance_time = 0;
		params.cur_tempo = dynamic_cast<AmTempoChange*>(tempos);
		params.cur_signature = dynamic_cast<AmSignature*>(signatures);
	
		if (!soloTrack.IsValid())
			ev = call_hooks(song, startTime, nextTime-1, start_hook, &params);
		else if (track)
			ev = call_track_hooks(track, startTime, nextTime-1, start_hook, &params);
		event = event->MergeList(ev, true);
		
		//printf("Tempo events:\n"); tempos->PrintChain();
		//printf("Signature events:\n"); signatures->PrintChain();
		
		event = ArpExecFilters(event, NORMAL_EXEC_TYPE, false, &params, NULL, NULL, tempos, signatures, &lookahead);
		
		params.cur_tempo = dynamic_cast<AmTempoChange*>(tempos);
		params.cur_signature = dynamic_cast<AmSignature*>(signatures);
		if (!soloTrack.IsValid())
			ev = call_hooks(song, startTime, nextTime-1, finish_hook, &params);
		else if (track)
			ev = call_track_hooks(track, startTime, nextTime-1, finish_hook, &params);
		event = event->MergeList(ev, true);
		
		#if SHOW_TRANSPORT_MSGS
		ArpD(cdb << ADH << "Filtered events " << event << endl);
		#endif
		
		ReadUnlock(song);
	
		#if SHOW_TRANSPORT_MSGS
		#if ArpDEBUG
		int32 count=0;
		ev = event;
		while (ev) {
			count++;
			ev = ev->NextEvent();
		}
		ArpD(cdb << ADH << "Result: " << count << " events, pred="
				<< (event ? event->PrevEvent() : NULL) << endl);
		#endif
		#endif
		
		if (wasFirstNote) {
			// If this is the first time through, insert a "dummy"
			// event at the front of the list to be sure the
			// performer starts playing at the requested time.
			// It has no next filter, so nothing is actually done
			// with it.
			ev = new AmSongPosition(startTime);
			if (ev) {
				if (event) event->MergeEvent(ev);
				else event = ev;
			}
		}
		
		// If this is the first time through playback, we doubled
		// the amount of time and so need to insert a transport event
		// in the middle.
		if (wasFirstNote) {
			ev = new TransportEvent(startTime+(nextTime-startTime)/2, mSyncSem);
			if (ev) {
				if (event) event->MergeEvent(ev);
				else event = ev;
			}
		}
		
		// Insert our special transport event at the end of the
		// list, to know when to feed more data.
		ev = new TransportEvent(nextTime-1, mSyncSem);
		if (ev) {
			if (event) event->MergeEvent(ev);
			else event = ev;
		}
		
		if (wasFirstTime) {
			// If we are looping, tell the performer the offset
			// from the performance beat to the actual song location.
			AmPerformerEvent* pe = new AmPerformerEvent(startTime);
			if (pe) {
				pe->SetBeatOffset(mLoopTime);
				if (event) event->MergeEvent(pe);
				else event = pe;
			}
		}
		
		startTime = nextTime;
		ArpASSERT(startTime >= 0);
		
		// If didn't get any data, just skip to next part of song.
		if (!event) continue;
		
		// Truncate any note durations that are past our stop time;
		// add offset if looping.
		ev = event = event->HeadEvent();
		while (ev) {
			if (ev->EndTime() > stopTime) ev->SetEndTime(stopTime);
			if (mLoopTime > 0) ev->SetStartTime(ev->StartTime()+mLoopTime);
			ev = ev->NextEvent();
		}
		
		status_t err = mPerformer.Play(event, wasFirstNote ? AMPF_RESTART : 0);
		
		#if SHOW_TRANSPORT_MSGS
		ArpD(cdb << ADH << "Performer append = " << strerror(err) << endl);
		#endif
		if (err != B_OK) continue;

		if (mLooping && startTime >= mLoopStop) {
			mLoopTime += mLoopStop - mLoopStart;
			startTime = mStartTime = mLoopStart;
			firstTime = true;
		}
		
		while (mRunning && (err=acquire_sem(mSyncSem)) == B_INTERRUPTED) {
			;
		}
	}
	
	ArpD(cdb << ADH << "Exiting with err=" << strerror(err) << endl);
	
	mExiting = true;
	
	// now wait for the transport to stop
	if (mPerformer.IsPlaying()) {
		if (!mRunning || err != B_OK) {
			mPerformer.Stop();
		} else {
			mPerformer.Play(new TransportEvent(stopTime, mSyncSem, true), AMPF_FINISHED);
			status_t err;
			while ((err=acquire_sem(mSyncSem)) == B_OK || err == B_INTERRUPTED)
				;
		}
	}
	
	params.DeleteMotionChanges();
	return B_OK;
}
