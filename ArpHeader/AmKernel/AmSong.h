/* AmSong.h
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.15.00			hackborn
 * Mutatated from its original incarnation.
 */

#ifndef AMKERNEL_AMSONG_H
#define AMKERNEL_AMSONG_H

#include "AmPublic/AmPipelineMatrixI.h"
#include "AmPublic/AmTrackRef.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPhrase.h"
#include "AmKernel/MultiLocker.h"

#include "ArpKernel/ArpRef.h"

#include <UndoContext.h>
class AmFilterHolder;
class AmPipelineSegment;
class AmTransport;
class AmInputQueue;
class _AmPulseFilters;

#define SZ_UNTITLED_SONG	"Untitled song"

/* Songs can be identified through a unique ID.  This is used in
 * various parts of the system, for example:  Positively identifying
 * a song, passing references to the song via BMessages, etc.
 */
#define SZ_SONG_ID		"song_id"
/* The title is supplied in the TITLE_CHANGE_OBS notification message.
 */
#define SZ_SONG_TITLE	"song_title"

/* These are the flags that can be passed into PlaybackList() of
 * both AmSong and AmTrack.
 */
enum {
	// If set, the returned list does not include any contextual
	// events (e.g., tempo) from before the start time.
	PLAYBACK_NO_CONTEXT		= (1<<0),
	// If set, any returned contextual events will not have their
	// time adjusted to match the start time of the playback list.
	PLAYBACK_RAW_CONTEXT	= (1<<1),
	// If set, the playback list will not include tempo events.
	PLAYBACK_NO_TEMPO		= (1<<2),
	// If set, the playback list will not include signature events.
	PLAYBACK_NO_SIGNATURE	= (1<<3),
	// If set, the playback list will not include any performance
	// events (notes, control changes, and other things in regular
	// tracks).
	PLAYBACK_NO_PERFORMANCE	= (1<<4),
	// If set, a list will be returned even if muted.
	PLAYBACK_IGNORE_MUTE	= (1<<5)
};

/**********************************************************************
 * AM-SONG-IMPL
 * This class represents one complete song, including all of its tracks
 * and their associated data, tempo and signature objects, etc.
 **********************************************************************/
class AmSong : public AmNotifier,
			   public AmPipelineMatrixI
{
public:
	AmSong(const char* title = SZ_UNTITLED_SONG);

	// --------------------------------------------------------
	// AM-PIPELINE-MATRIX-I INTERFACE
	// --------------------------------------------------------
	virtual void				AddRef() const;
	virtual void				RemoveRef() const;
	virtual bool				ReadLock() const;
	virtual bool				WriteLock(const char* name = NULL);
	virtual bool				ReadUnlock() const;
	virtual bool				WriteUnlock();

	virtual void				SetDirty(bool dirty = true);

	virtual song_id				Id() const;
	virtual uint32				CountPipelines() const;
	virtual pipeline_id			PipelineId(uint32 pipelineIndex) const;
	virtual status_t			PipelineHeight(uint32 pipelineIndex, float* outHeight) const;

	virtual const BUndoContext*	UndoContext() const;
	virtual BUndoContext*		UndoContext();

	virtual AmFilterHolderI*	Filter(	pipeline_id id,
										AmPipelineType type,
										filter_id filterId = 0) const;
	virtual status_t			InsertFilter(	AmFilterAddOn* addon,
												pipeline_id id,
												AmPipelineType type,
												int32 beforeIndex = -1,
												const BMessage* config = 0);
	virtual status_t			ReplaceFilter(	AmFilterAddOn* addon,
												pipeline_id id,
												AmPipelineType type,
												int32 atIndex = 0,
												const BMessage* config = 0);
	virtual status_t			RemoveFilter(	pipeline_id id,
												AmPipelineType type,
												filter_id filterId);
	virtual status_t			MakeConnection(	pipeline_id fromPid,
												AmPipelineType fromType,
												filter_id fromFid,
												pipeline_id toPid,
												AmPipelineType toType,
												filter_id toFid);
	virtual status_t			BreakConnection(pipeline_id fromPid,
												AmPipelineType ownerType,
												filter_id fromFid,
												pipeline_id toPid,
												AmPipelineType toType,
												filter_id toFid);
	virtual void				PipelineChanged(pipeline_id id, AmPipelineType type);
	virtual void				FilterChanged(pipeline_id id, AmPipelineType type);

	virtual status_t			FlattenConnections(BMessage* into, AmPipelineType type) const;
	virtual status_t			UnflattenConnections(const BMessage* into, AmPipelineType type);

	virtual status_t			AddMatrixPipelineObserver(pipeline_id id, BHandler* handler);
	virtual status_t			AddMatrixFilterObserver(pipeline_id id, BHandler* handler);
	virtual status_t			RemoveMatrixObserver(pipeline_id id, BHandler* handler);

	/*---------------------------------------------------------
	 * CHANGE NOTIFICATION
	 *---------------------------------------------------------*/
	/* These are the events a client can register to hear about
	 * via the notifier's add observer methods.  Note that all
	 * the MIDI events are defined in the notifier.
	 */
	enum ObserverMessages {
		TRACK_CHANGE_OBS		= '#trc',	// Sent whenever tracks are added or removed.
											// SZ_STATUS, An int32 of AM_ADDED or AM_REMOVED
											// SZ_TRACK_ID, A pointer to a track_id
											// SZ_POSITION, An int32, the position of the track (optional)
		TITLE_CHANGE_OBS		= '#tic',
		END_TIME_CHANGE_OBS		= '#etc',
		STATUS_OBS				= '#sns'	// Sent when the song status is changed -- typically, this
											// means the song is removed from the system.  Currently, nothing
											// in the song sends this out -- it's the responsibility of whatever
											// system is removing the song to notify everyone.
											// SZ_SONG_ID, A pointer which is the song_id
											// SZ_STATUS, An int32 of AM_REMOVED (with others potentially in the future)
	};
	/* This is a macro that sends out a notice to all observers that
	 * the song's end time has changed.  Polite clients will supply
	 * the current end time, since they should already have it, but
	 * if the default is supplied, it will be generated.
	 */
	void			EndTimeChangeNotice(AmTime endTime = -1);
	
	/* Answer with the end time of the last notification.  This makes
	 * it easier for you to figure out if data you have added has
	 * extended the length of the song.
	 */
	AmTime			LastEndTime() const;
	
	/* Dirty state.  The ClearDirty() method is for the sitation when you
	 * read the song to save it, and then want to mark that it is no longer
	 * dirty.  You shouldn't need to have a write lock to do this, so
	 * ClearDirty() is const.  (It doesn't change the song itself.)
	 */
	bool			IsDirty() const;
	void			ClearDirty() const;
	
	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* These methods are convenience accessors to the song's initial tempo.  If
	 * you want access to anything but the first tempo, you need to access the
	 * tempo list.  If you actually want to make changes, you need to
	 * go through the modification framework.
	 */
	float			BPM() const;
	void			SetBPM(float BPM);
	const AmPhrase& Signatures() const;
	/* Fill in the signature with the sig info for whatever signature contains
	 * the supplied time.
	 */
	status_t		GetSignature(AmTime time, AmSignature& signature) const;
	status_t		GetSignatureForMeasure(int32 measure, AmSignature& signature) const;
	/* FIX: Signature editing is not currently undoable, fix that.
	 */
	status_t		SetSignature(int32 measure, uint32 beats, uint32 beatValue);
	/* Move signatures either left or right from the start point.
	 */
	status_t		OffsetSignatures(AmTime start, AmTime offset);
		
	AmTrackRef		TempoRef() const;
	const AmTrack*	TempoTrack() const;
	AmTrack*		TempoTrack();
	/* Things are kind of gross right now -- the tempo track's phrases should
	 * have a single phrase event which contains all the tempos.  It's done that
	 * way because it makes it much easier on the tool target, although clearly
	 * this should be rethought.  This method is a convenience accessor.
	 */
	const AmPhrase*	TempoPhrase() const;

	/* Answer and set the title of this song.  Setting the title
	 * causes an TITLE_CHANGE_OBS message to be sent to all observers.
	 */
	const char*		Title() const;
	void			SetTitle(const char* title, void* sender = 0);
	AmTime			CountEndTime() const;

	AmRange			RecordRange() const;
	void			SetRecordRange(AmRange range);
	
	/*---------------------------------------------------------
	 * EDITING
	 * Clients are responsible for sending out change notices
	 * after an edit operation.
	 *---------------------------------------------------------*/
	/* In certain situations, clients need to operate on the
	 * song and its tracks without creating undo state.
	 */
	bool				IsSuppressingUndo() const;
	void				SetSuppressUndo(bool suppress);
	
	virtual status_t	AddEvent(AmEvent* event);
	/* Clear out this song -- remove tracks, events, etc.
	 */
	status_t			Clear();

	/* Song loading.  StartLoad() removes all data from the song.
	 * FinishLoad() sets up any initial values that the load did
	 * not set.  Clear() is actually a StartLoad() and FinishLoad().
	 */
	void				StartLoad();
	void				FinishLoad();

	/* When a change is	made to a track, the track informs the
	 * song so the song can send out a change notice if it likes.
	 * Some clients listen to the song because they want to hear
	 * about changes made to all tracks -- in this case, they
	 * want information about who the change is coming from, i.e.
	 * the track id, etc.  This lets the song annotate the change
	 * message appropriately.
	 */
	void				SetChangedTrack(track_id id);

	void				SetWindow(BMessenger m);
	/*---------------------------------------------------------
	 * TRACK ACCESSING
	 *---------------------------------------------------------*/
	/* Various ways to access the tracks.  Each track has a unique ID,
	 * as well as a position in the collection of tracks, both of which
	 * allow access.  Answer B_OK if the track ref object is valid, otherwise
	 * B_ERROR if the track doesn't exist.  Any track the ref was referring
	 * to already is now lost.
	 */
	status_t TrackRefForId( track_id id, AmTrackRef& ref ) const;
	status_t TrackRefForIndex( uint32 index, AmTrackRef& ref ) const;

	/* Answer readable and writeable tracks at the supplied track ref, index, or id.
	 */
	const AmTrack*	Track(AmTrackRef trackRef) const;
	AmTrack* 		Track(AmTrackRef trackRef);
	const AmTrack*	Track(uint32 index) const;
	AmTrack* 		Track(uint32 index);
	const AmTrack*	Track(track_id id) const;
	AmTrack* 		Track(track_id id);
	/* Answer the index of the supplied track.  If the song doesn't contain
	 * the track then the answer will be -1.
	 */
	int32			TrackIndex(track_id id) const;
	
	uint32			CountTracks() const;

	/* Create a new track object and notify my dependents about it.  Place
	 * the object at position index in my collection -- i.e. 0 would place it
	 * at the head.  -1 is shorthand for the end of the collection.
	 */
	status_t		AddTrack(int32 height, int32 index = -1);
	status_t		AddTrack(AmTrack* track, int32 index = -1);
	status_t		RemoveTrack(track_id id);
	status_t		RemoveTrack(uint32 index);
	/* Reposition this track in my list.
	 */
	status_t		MoveTrackBy(track_id tid, int32 delta);
	
	// ---- Low level add / remove API ----
	status_t		AddTrack(AmTrack* track, BUndoContext* undo, int32 index = -1 );
	status_t		RemoveTrack(int32 index, BUndoContext* undo);
	status_t		MoveTrackBy(track_id tid, int32 delta, BUndoContext* undo);

	/*---------------------------------------------------------
	 * PLAYBACK
	 *---------------------------------------------------------*/
	/* Transformations between real-time and beat-time when playing.
	 */
	AmTime RealtimeToPulse(bigtime_t time) const;
	bigtime_t PulseToRealtime(AmTime pulse) const;
	
	/* Return the transport.  You can do anything you want with the
	 * transport, EXCEPT starting or stopping playback.  For that,
	 * use the functions in AmSongRef.  Note that you can retrieve and
	 * manpulate the transport without locking the song, since it
	 * has its own protection.
	 */
	AmTransport& Transport() const;
	
	/* Answer true if the song if playback is currently running in
	 * record mode.
	 */
	bool IsRecording() const;
	
	/* Return the input queue.  This is where you place events that
	 * you would like to have recorded into the song.
	 */
	ArpRef<AmInputQueue> InputQueue() const;
	
	/* Answer a list -- a copy of all of my events -- whose sole
	 * purpose in life is to be eaten by the playback routines.  The list
	 * answered will be all the events found on or after startTime, and
	 * or -before- stopTime.  If stopTime = -1, then we will play until
	 * the last event in the track.  This method should be protected,
	 * accessible only by the AmTransport.
	 */
	AmEvent* PlaybackList(AmTime startTime = 0, AmTime stopTime = -1,
						   uint32 flags = 0) const;

	/* Pass a message to the window that owns me.  This was added
	 * to support MMC.
	 */
	status_t			WindowMessage(BMessage* msg);

	/*---------------------------------------------------------
	 * AM-NOTIFIER INTERFACE
	 *---------------------------------------------------------*/
	virtual void		FlushChanges();

protected:
	virtual ~AmSong();
	
	virtual void Delete()			{ delete this; }

	/*---------------------------------------------------------
	 * AM-NOTIFIER INTERFACE
	 *---------------------------------------------------------*/
	virtual AmPhrase*	PhraseFor(AmEvent* event) const;
	virtual void		AnnotateMessage(BMessage& msg) const;

private:
	friend class AmSongRef;
	friend class AmTrack;
	friend class AmTransport;
	
	int32				mRefCount;
	mutable MultiLocker	mLock;
	trackref_vec		mTrackRefs;
	AmPhrase			mSignatures;
	AmTrack*			mTempoTrack;
	mutable AmTransport*mTransport;
	mutable bool		mDirty;
	int32				mFlags;
	/* When I annotate messages that come from the track, if
	 * this is set, I can set the appropriate track info in the
	 * message.
	 */
	track_id			mChangedTrack;
	/* When tracks are added or removed, I note that in this message,
	 * which gets sent out when I FlushChanges().  If there are a lot
	 * of track changes (like during load), this cuts down on a ton
	 * of notification.
	 */
	BMessage			mTrackChangeMsg;
	
	BUndoContext		mUndoContext;
	
	// Last end time notification.
	AmTime				mLastEndTime;
	
	// Recording.
	const ArpRef<AmInputQueue> mInputQueue;
	// The range during which I should accept recorded events.  If the
	// range is invalid, I should accept all recorded events.
	AmRange				mRecordRange;

	BMessenger			mWindow;

	// These are accessed through an AmSongRef.
	status_t DoStartPlaying(AmTime startTime, AmTime stopTime = -1) const;
	status_t DoStartPlaying(const AmTrackRef& solo, AmTime startTime, AmTime stopTime = -1) const;
	status_t DoStartRecording(AmTime startTime, AmTime stopTime = -1);
	status_t DoStartRecording(const AmTrackRef& solo, AmTime startTime, AmTime stopTime = -1);
	void DoStopTransport(uint32 context) const;
	/* A hack to get all the filters to receive a Start() command
	 * before the song starts playing.
	 */
	void StartFiltersHack() const;
	
	// Merge all tempos in given playback range into eventList.
	void MergeTemposInto(AmEvent** eventList,
						 AmTime startTime, AmTime stopTime, uint32 flags) const;
	
	// Merge all signatures in given playback range into eventList.
	void MergeSignaturesInto(AmEvent** eventList,
							 AmTime startTime, AmTime stopTime, uint32 flags) const;
	
	// Playback temp -- take every event in track, prepare it for playback, and
	// merge it into event;
	void MergeTrackInto(AmTrack* track,
						AmEvent** event,
						AmTime startTime,
						AmTime stopTime,
						uint32 flags) const;
	/* A position of -1 means to not send out position information with this
	 * change notice.  Typically a position is not sent out only when the track
	 * was removed.
	 */
	void TrackChangeNotice( track_id id, AmStatus status, int32 position = -1 );

	const AmTrack*		TrackForPipeline(pipeline_id id) const;
	AmTrack*			TrackForPipeline(pipeline_id id);

	status_t			AddConnectionInfo(	AmFilterHolderI* connection, BMessage& msg,
											AmPipelineType type) const;
	status_t			UnflattenConnection(int32 source_pi, int32 source_fi,
											int32 dest_pi, int32 dest_fi,
											AmPipelineType type);

	AmPipelineSegment*	PipelineSegment(pipeline_id pid, AmPipelineType type);

	/* A cache of all the filters I contain that want pulse notice.
	 */
	BLocker				mPulseAccess;
	_AmPulseFilters*	mPulseFilters;
	bool				mPulseFiltersChanged;
	thread_id			mPulseThread;

	status_t			CachePulseFilters();
	bool				SendPulse();

	static int32		PulseThreadEntry(void *arg);

public:
	/* Only ever called by the track as a result during AddPulseFilters()
	 */
	status_t			AddPulseFilter(AmFilterHolderI* holder);
};

#endif 
