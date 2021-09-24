/* AmPerformer.h
 * Copyright (c)1998 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Dianne Hackborn
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Angry Red Planet,
 * at <hackborn@angryredplanet.com> or <hackbod@angryredplanet.com>.
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
 * 1998/09/25		hackbod
 * Created this file
 */

#ifndef AMKERNEL_AMPERFORMER_H
#define AMKERNEL_AMPERFORMER_H

#ifndef AMPUBLIC_AMFILTERI_H
#include <AmPublic/AmFilterI.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _LOOPER_H
#include <app/Looper.h>
#endif

#ifndef _LOCKER_H
#include <support/Locker.h>
#endif

/*****************************************************************************
 *
 *	ARP-MIDI-PERFORMER CLASS
 *
 *	AmPerformer encapsulates a thread that "performs" a chain of
 *	events.
 *
 *****************************************************************************/

// Play mode flags

enum {
	AMPF_FINISHED	= 1<<0,		// There is no more data in this song.
	AMPF_RESTART	= 1<<1		// Start a new song, reseting time base.
};

AmTime realtime_to_pulse(bigtime_t time, int32 bpm, AmTime baseBeat, bigtime_t baseTime);
bigtime_t pulse_to_realtime(AmTime pulse, int32 bpm, AmTime baseBeat, bigtime_t baseTime);

class AmPerformerEvent : public AmEvent
{
public:
						AmPerformerEvent(AmTime time);
						AmPerformerEvent(const AmPerformerEvent& o);

	virtual EventType	Type() const;

	virtual AmEvent*	Copy() const;
	virtual void		Print(void) const;

	void				SetBeatOffset(AmTime t);
	AmTime				BeatOffset() const;
	
	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual				~AmPerformerEvent();
	AmPerformerEvent&		operator=(const AmPerformerEvent& o);

private:
	AmTime				mBeatOffset;
};

class AmClockTarget
{
public:
	virtual						~AmClockTarget();
	
	virtual	void				Clock(AmTime time) = 0;
};

class AmPerformer
{
public:
	/* Constructor and destructor. */
	AmPerformer(bool continuous_running = false);
	virtual ~AmPerformer();
	
	// --------------------------------------------------------
	// ATTRIBUTES AND INFORMATIONAL METHODS.
	// --------------------------------------------------------
	
	/* Get and set the playback beats per minute.  The default
	 * is 120 BPM.  Changing this will cause an effect when the
	 * next event is performed.
	 */
	float		BPM() const;
	void		SetBPM(float BPM);
	
	/* Set target for MIDI clock reports.  Only one at a time.
	 */
	void		SetClockTarget(AmClockTarget* target);
	
	/* Return the current time being played in the song, or -1
	 * if not currently playing.
	 */
	AmTime		CurrentTime() const;
	
	/* Return the system time at which a future song time will
	 * occur.
	 */
	bigtime_t	FutureTime(AmTime time) const;
	
	/* Transformations between real-time and beat-time when playing.
	 * If not currently performing, or the given input is out of
	 * the performer's range of knowledge (before the last tempo
	 * change or after the current time), then -1 is returned.
	 */
	AmTime RealtimeToPulse(bigtime_t time, bool includeFuture=false) const;
	bigtime_t PulseToRealtime(AmTime pulse, bool includeFuture=false) const;
	
	/* Return the current loop offset.  You can use this to transform
	 * a song beat time to the beat time in the transport.
	 */
	AmTime LoopOffset() const;
	
	/* Return true if the object is currently performing a song.
	 * Returns false if it has run out of events to perform.
	 */
	bool IsPlaying() const;
	
	/* Return true if the performer thread is currently running.
	 */
	bool IsRunning() const;
	
	// --------------------------------------------------------
	// PLAYBACK CONTROL.
	// --------------------------------------------------------
	
	/* Play the given event chain.  How this is accomplished depends
	 * on the flags passed in:
	 *
	 * 0
	 *	The events are merged into the currently playing events.
	 *	For good results, they should have time stamps corresponding
	 *	in some way to the existing events.
	 *
	 * AMPF_FINISHED
	 *	Like above, but once all of the events now in the performer
	 *	are played, it will stop and shut down.
	 *
	 * AMPF_RESTART
	 *	If the performer is currently running, it is stopped and
	 *	its current events cleared.  The given events are then
	 *	placed into it, and it restarts the performance at the
	 *	time of these new events.
	 *
	 */
	status_t Play(AmEvent* chain, uint32 flags = AMPF_FINISHED|AMPF_RESTART);
	
	/* Stop any running performance, right now dammit. */
	void Stop();
	
private:
	// copying constructor and assignment are not allowed.
	AmPerformer(const AmPerformer&);
	AmPerformer& operator=(const AmPerformer&);
	
	// caller functions.
	
	status_t Restart(AmEvent* chain, uint32 flags);
	void StopForReal();
	
	// read thread functions.
	
	static int32 PlayThreadEntry(void* arg);
	bigtime_t EventTimeToRealTime(AmTime event) const;
	AmTime RealTimeToEventTime(bigtime_t real) const;
	AmTime NextEventBeat(bool* out_found);
	AmEvent* NextEvents();
	void ExecuteEvent(AmEvent* event, am_filter_params* params);
	void ClearNewEvents();
	int32 RunPerformance();
	
	bool mContinuousRunning;
	
	// general data accessed by read thread and callers.
	
	mutable BLocker mAccess;
	
	sem_id mDataSem;			// Released when new data is available
	thread_id mPlayThread;		// Thread that is playing the song
	
	// events queued into read thread by callers.
	
	mutable BLocker mEventAccess;
	AmEvent* mPosition;			// Current event chain being played
	AmEvent* mTail;				// Cached last event in chain
	AmTime mStartTime;			// Starting time of this song
	bool mRestart;				// Reset performance time to next event
	bool mPlaying;				// Does mPlayThread have events to play?
	bool mFinished;				// Stop playing once out of events?
	bool mCleanup;				// Ask performer thread to remove all events.
	
	// data to report song timing.
	
	mutable BLocker mTimeAccess;
	AmClockTarget* mClockTarget;
	AmTime mNextClock;			// Time at which next clock should go out.
	double mSharedBPM;
	AmTime mSharedTempoBeat;
	bigtime_t mSharedTempoTime;
	bigtime_t mSharedBeatOffset;
	bool mValidTime;
	bool mSetBPM;
	
	// data only accessed by read thread.
	
	double mBPM;				// Current beats per minute being played
	AmTime mTempoBeat;			// Beat of last tempo change
	bigtime_t mTempoTime;		// Time of last tempo change
	bigtime_t mBaseTime;		// Time when song started
	bigtime_t mBeatOffset;		// Difference: (performance beat) - (song beat)
	
	AmEvent* mNewEvents;		// Extra events generated by filters
};

#endif
