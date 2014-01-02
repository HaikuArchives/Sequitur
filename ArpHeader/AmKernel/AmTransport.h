/* AmTransport.h
 * Copyright (c)1998 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Eric Hackborn
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
 *	- I'm not currently enforcing the singleton status of this class.
 *
 *	- Currently, there is globaly information that should be included in
 * each song, such as Tempo information.  This information is not returned
 * by either the version or the track as part of a supplied playlist, so I
 * will need to request that information and mix it in, probably in my
 * PreparedList() method.  The reason it should be done this way is because
 * this information is global, so we want it regardless of whether we are
 * playing a whole version or just a track.  If tracks mixed the information
 * in, we would get it redundantly when playing a version, if versions mixed
 * it in we would miss it when playing a track.  It makes sense just to handle
 * it here.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 1998/12/05		hackborn
 * Created this file
 */

#ifndef AMKERNEL_AMTRANSPORT_H
#define AMKERNEL_AMTRANSPORT_H

#include <AmPublic/AmSongObserver.h>
#include <AmPublic/AmTrackRef.h>
#include <AmKernel/AmPerformer.h>

#include <Locker.h>

namespace ArpPrivate {
class TransportLooper;
class TransportEvent;
}

enum transport_state {
	TS_STOPPED = 0,
	TS_PLAYING = 1
};

extern const char* ArpTransportState;		// "transport_state"
extern const char* ArpTransportTime;		// "transport_time"
extern const char* ArpTransportTempo;		// "transport_tempo"

extern const char* ArpTransportNextTime;	// "transport_next_time"

enum {
	TRANSPORT_CHANGE_MSG	= 'trch'
};

/*****************************************************************************
 *
 *	ARP-MIDI-TRANSPORT CLASS
 *
 *****************************************************************************/

class AmTransport : public AmSongObserver
{
public:
	AmTransport(AmSongRef songRef, bool continuous_running = false);
	AmTransport(song_id songId, bool continuous_running = false);
	virtual ~AmTransport();
		
	// --------------------------------------------------------
	// SETUP AND INFORMATIONAL METHODS
	// --------------------------------------------------------
		
	/* Return the current time being played in the song, or -1
	 * if not currently playing.
	 */
	AmTime CurrentTime() const;
		
	/* Return the system time at which a future song time will
	 * occur.
	 */
	bigtime_t FutureTime(AmTime time) const;
	
	/* Return the current loop offset.  You can use this to transform
	 * a song beat time to the beat time in the transport.
	 */
	AmTime LoopOffset() const;
	
	/* Get and set current performance tempo.
	 */
	float BPM() const;
	void SetBPM(float value);
	
	/* Set target for MIDI clock reports.  Only one at a time.
	 */
	void SetClockTarget(AmClockTarget* target);
	
	/* Return true if the object is currently performing a song.
	 * Returns false if it has run out of events to perform.
	 */
	bool IsPlaying() const;
	
	/* Time conversions from AmPerformer.
	 */
	AmTime RealtimeToPulse(bigtime_t time, bool includeFuture=false) const;
	bigtime_t PulseToRealtime(AmTime pulse, bool includeFuture=false) const;
	
	/* Return true if the transport is running.
	 */
	bool IsRunning() const;
	
	/* Return true if the transport is waiting for playback to finish.
	 */
	bool IsExiting() const;

	/* Return true if the transport has been requested a 'soft' end time --
	 * i.e., whatever the song's end time happens to be.  Return false if
	 * the transport Start() method was called with an actual stop time.
	 */
	bool PlayToEnd() const;
	
	/* Start and stop watching the current transport state.
	 */
	status_t StartWatching(BMessenger observer);
	status_t StopWatching(BMessenger observer);
	
	// --------------------------------------------------------
	// PLAYBACK CONTROL.
	// --------------------------------------------------------
	
	/* Control whether song playback is looped.
	 */
	void SetLooping(bool state, AmTime start=-1, AmTime stop=-1);
	bool Looping() const;
	
	/* Start a song performance.  Start at the first event found
	 * at startTime, and stop at the last event found at or before
	 * stopTime (unless the song ends first, in which case stop at
	 * that point).  If stopTime is < 0, then play until the end of
	 * the song.
	 */
	status_t Start(AmTime startTime, AmTime stopTime = -1);
	status_t Start(AmTrackRef solo, AmTime startTime, AmTime stopTime = -1);
	
	/* Add more events to song that is currently playing.  These
	 * must be in time of the song (without looping).  Note that
	 * since the transport's AmPerformer is in continuous_running
	 * mode, you can call this even when a song isn't playing.
	 */
	status_t Merge(AmEvent* events);
	
	/* Stop any running performance. */
	void Stop(uint32 context);
	
private:
	// copying constructor and assignment are not allowed.
	AmTransport(const AmTransport&);
	AmTransport& operator=(const AmTransport&);
	
	static int32 FeedThreadEntry(void* arg);
	int32 FeedPerformance();
	
	// data only accessed by public interface.
	
	ArpPrivate::TransportLooper* mLooper;
	
	// data accessed by feed thread and callers.
	
	mutable BLocker mAccess;
	
	AmTrackRef mSoloTrack;	// If valid, this is a single track to play
	
	thread_id mFeedThread;	// feeds blocks of events into the performer
	sem_id mSyncSem;		// synchronization with performer
	AmTime mStartTime;		// place in song to start feeding from
	AmTime mStopTime;		// place in song to stop feeding from, or 0
	AmTime mLoopTime;		// offset for loop
	AmTime mLoopStart;		// beginning of loop section
	AmTime mLoopStop;		// ending of loop section
	bool mLooping;			// Is this playback range looped?
	bool mRunning;			// Is mFeedThread running?
	bool mExiting;			// Waiting for performer to finish?
	bool mPlayToEnd;		// When Start() was called, was it with -1 stop time?
	
	// data only accessed by feed thread.
	
	// Store a single performer object to handle all of my data actualization
	AmPerformer			mPerformer;
};

#endif
