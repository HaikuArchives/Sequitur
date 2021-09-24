/* AmTrack.h
 * Copyright (c)1998-2000 by Eric Hackborn.
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
 * Created this file from its original incarnation as AmTrack.
 */
#ifndef AMKERNEL_AMTRACK_H
#define AMKERNEL_AMTRACK_H

#include <BeExp/UndoContext.h>
#include "AmPublic/AmDeviceI.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPhrase.h"
#include "AmKernel/AmPipelineSegment.h"
#include "AmPublic/AmSongRef.h"
#include "AmKernel/AmViewProperty.h"
#include "ArpKernel/ArpRef.h"
class AmSong;
class AmTrack;
class AmFilterHolder;
class AmFilterAddOn;
class AmInputTarget;

#define SZ_UNTITLED_TRACK			"Untitled track"

#define STR_SENDER					"sender"	// A pointer to the object that
												// invoked the method that caused
												// the pre/post change to be sent

/* Tracks can be identified through a unique ID.  This is used in
 * various parts of the system, for example:  Positively identifying
 * a track in a song, passing references to the track via BMessages,
 * etc.
 */
#define SZ_TRACK_ID		"track_id"
/* The title is supplied in the TITLE_CHANGE_OBS notification message.
 */
#define SZ_TRACK_TITLE	"track_title"

/***************************************************************************
 * AM-TRACK
 * The actual implementation that the AmTrack object provides access to.
 * All public methods are commented in AmTrack.h.
 ****************************************************************************/
class AmTrack : public AmNotifier,
				public AmEventParent
{
public:
	AmTrack(AmSong* song, const char* title = SZ_UNTITLED_TRACK);

	void AddRef() const;
	void RemoveRef() const;
	
	/*---------------------------------------------------------
	 * CHANGE NOTIFICATION
	 *---------------------------------------------------------*/
	/* These are the events a client can register to hear about
	 * via the notifier's add observer methods.  Note that all
	 * the MIDI events are defined in the notifier.
	 */
	enum {
		TITLE_CHANGE_OBS		= '#tic',
		MODE_CHANGE_OBS			= '#mic',
		GROUP_CHANGE_OBS		= '#grp'
	};
	
	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	/* Answer a unique identifier for this track.  This value is
	 * immutable, and so can be called without first locking the
	 * track (provided you are positive the track won't be deleted
	 * underneath you).
	 */
	track_id		Id() const;

	/* Answer and set the title of this track.  Setting the title
	 * causes an TITLE_CHANGE_OBS message to be sent to all observers.
	 */
	const char*		Title() const;
	void			SetTitle(const char* title, void* sender = 0);
	/* Answer my song.
	 */
	const AmSong*	Song() const;
	AmSong*			Song();
	/* Answer the index at which I exist in my song.  If I'm not
	 * in a song, answer 0 (although that's a severe error condition.
	 */
	int32			SongIndex() const;
	/* Answer my device, or NULL if none.
	 */
	virtual ArpCRef<AmDeviceI> Device() const;
	/* The groups are stored as inidividual bits in a mask, starting
	 * with group 1 at 0x00000001.
	 */
	uint32			Groups() const;
	void			SetGroups(uint32 groups);
	/* Answer the highest lookahead time of any filter in my
	 * output pipeline.
	 */
	AmTime			LookaheadTime() const;
	/* The following methods provide read access to the various
	 * events in this track.  All events in a track reside in
	 * an AmPhrase object, which in turn resides in the Phrases()
	 * object.
	 *
	 * The one exception to this is signatures, which reside in
	 * their own phrase object.  Signatures should be added to
	 * the track the same way that phrases are added to the track,
	 * via the single-argument AddEvent() method.
	 */
	const AmPhrase& Phrases() const;
	const AmPhrase&	Signatures() const;
	status_t		SetSignature(int32 measure, uint32 beats, uint32 beatValue);
	/* Fill in the signature with the sig info for whatever signature contains
	 * the supplied time.
	 */
	status_t		GetSignature(AmTime time, AmSignature& signature) const;
	status_t		GetSignatureForMeasure(int32 measure, AmSignature& signature) const;
	void			LockSignaturesToSong();
	/* Answer the phrase containing my motions.
	 */
	const AmPhrase&	Motions() const;
	status_t		SetMotion(int32 measure, const AmMotionI* motion);
	status_t		ClearMotion(int32 measure);
	
	/* Answer the end time of the last event in the track.  This
	 * does not include signature events, which are considered
	 * beneath notice.
	 */
	AmTime			EndTime2() const;
	/* Answer and set my current mode flags.
	 */
	enum {
		MUTE_MODE				= 0x00000001,
		SOLO_MODE				= 0x00000002
	};
	uint32			ModeFlags() const;
	void			SetModeFlags(uint32 flags);
	/* Only one record mode can be active at a time.
	 */
	enum {
		RECORD_OFF_MODE			= 0,
		RECORD_REPLACE_MODE,
		RECORD_MERGE_MODE
	};
	uint32			RecordMode() const;
	void			SetRecordMode(uint32 mode);

	/* Write my current state to the supplied message.
	 */
	status_t		GetSettings(BMessage* settings) const;
	/* Read my current state from the supplied message.
	 */
	status_t		PutSettings(const BMessage* settings);

	/*---------------------------------------------------------
	 * EDITING
	 *---------------------------------------------------------*/
	/* Supply NULL to phrase if the event should be added directly
	 * into this track's phrase list.
	 */
	status_t		AddEvent(	AmPhrase* phrase, AmEvent* event, const char* undoName = "Add Event");
	status_t		RemoveEvent(AmPhrase* phrase, AmEvent* event);

	/* These are inherited from AmEventParent, but meaningless here.
	 */
	virtual AmEventParent*	Parent() const;
	virtual void			SetParent(AmEventParent* parent);
	virtual void			TimeRangeDirty();
	/* There shouldn't be a reason for clients to call this message,
	 * it should always be bubbled up from changes to the events I
	 * contain.
	 */
	virtual void	Invalidate(	AmEvent* changedEvent,
								AmRange oldRange, AmRange newRange);
	virtual void	ChangeEvent(AmEvent* child,
								AmTime oldStart, AmTime newStart,
								AmTime oldEnd, AmTime newEnd);

	/* Low-level event manipulation. */
	status_t		AddEvent(	AmPhrase* phrase, AmEvent* event,
								BUndoContext* undo, const char* undoName = "Add Event");
	status_t		RemoveEvent(AmPhrase* phrase, AmEvent* event,
								BUndoContext* undo, const char* undoName = "Remove Event");

	/*---------------------------------------------------------
	 * USER INTERFACE
	 *---------------------------------------------------------*/
	/* Answer the number of properties at the given type.
	 */
	uint32					CountProperties(TrackViewType type) const;
	/* These methods get and set a specific view property.  Any
	 * particular view type might have 0 to n view properties,
	 * accessed through the index argument.  Obviously, as soon
	 * as this track object has been deleted, any properties it
	 * has returned are invalid.
	 */
	const AmViewPropertyI*	Property(TrackViewType type, uint32 index = 0) const;
	/* Set the property at the supplied index.  If index is greater
	 * then my current number of properties of that type, then add
	 * the property to the end.  If property is 0, then erase the
	 * property at that index.  Note that some view types can't be
	 * erased -- there's always a fixed number -- so those ignore
	 * erase commands.
	 */
	void					SetProperty(const AmViewPropertyI* property,
										TrackViewType type,
										uint32 index = 0);
	/* Read and write my window state stuff -- frame, saturation levels, etc.
	 * Note that the set method doesn't actually set those properties --
	 * they are stored by the window, not me.  It just copies the supplied
	 * BMessage into my mConfiguration.
	 */
	status_t				GetConfiguration(BMessage* config) const;
	status_t				SetConfiguration(const BMessage* config);
	/* Answer the pixel height of this track when represented in
	 * a phrase view (such as the song window).  It would be nice
	 * if we could stick this in a better place, like the arrange property.
	 */
	float					PhraseHeight() const;
	void					SetPhraseHeight(float height);
	
	/*---------------------------------------------------------
	 * FILTER MANIPULATION -- this needs to be updated, probably
	 * to a scheme similar to what the events are doing.
	 *---------------------------------------------------------*/

	/* Answer the first filter in my list for the requested type.  All other
	 * filters of this type can be found by traversing this one.  If the
	 * type is invalid or no filter has been set, answer 0.
	 */
	AmFilterHolderI*	Filter(AmPipelineType type) const;
	/* Track down the requested filter in the supplied pipeline.
	 */
	AmFilterHolderI*	Filter(AmPipelineType type, filter_id id) const;
	/* Insert the filter into the filter list before the given index (i.e., a
	 * beforeIndex value of 0 will insert at the head, 1 will insert between
	 * the head and the second filter, etc).  If the index is invalid (-1, for
	 * example), add the filter at the end of the list.  If the filter is an output
	 * filter, ignore the position and replace the current output filter.  The
	 * filter is generated from the addon.  If the filter is a copy of an existing
	 * filter, the client should request the config from the existing filter and
	 * supply that here.
	 */
	status_t			InsertFilter(	AmFilterAddOn* addon,
										AmPipelineType type,
										int32 beforeIndex = -1,
										const BMessage* config = 0);
	/* Replace the filter at the given index with a new instance of addon.  If
	 * the pipeline type is a single filter endpoint -- either a SOURCE or
	 * DESTINATION, and the addon matches, then always succeed.  Otherwise, this
	 * will only succeed if the specified pipeline actually has a filter at the
	 * index supplied, otherwise B_ERROR will be returned.
	 */
	status_t			ReplaceFilter(	AmFilterAddOn* addon,
										AmPipelineType type,
										int32 atIndex = 0,
										const BMessage* config = 0);
	/* Track down the filter at filter ID and remove it.  Send out a notification
	 * that I've changed.  If I don't actually have the filter, send out no notification
	 * and respond with B_ERROR.
	 */
	status_t			RemoveFilter(AmPipelineType type, filter_id id);
	
	/* Fill in the given BMessage with a flattened representation of the
	 * filter chain in the track.
	 */
	status_t			FlattenFilters(BMessage* out, AmPipelineType type) const;
	/* Instantiate filter objects from the message, and append to this track.
	 */
	status_t			UnflattenFilters(const BMessage* archive,
										 AmPipelineType type, bool append=false);
	
	/*---------------------------------------------------------
	 * PLAYBACK
	 *---------------------------------------------------------*/
	/* This is the object into which you should place events that you
	 * would like to have recorded.
	 */
	ArpRef<AmInputTarget> InputTarget() const;
	
	/* While recording, perform given event list into the track.
	 * The events should already have been run through all of their
	 * input filters.  Note that you shouldn't normally call this by
	 * yourself -- it will be called by the record queue.
	 */
	void				RecordEvents(AmEvent* events);
	
	/* Start and stop generation of a phrase while recording.  Called
	 * automatically for you by AmSong, but you can call it yourself
	 * to control the generation of phrases.  StopRecordingPhrase()
	 * returns false if no events were added, otherwise true.
	 */
	void				StartRecordingPhrase();
	bool				StopRecordingPhrase();
	
	/* Answer a list -- a copy of all of my events -- whose sole
	 * purpose in life is to be eaten by the playback routines.  The list
	 * answered will be all the events found on or after startTime, and
	 * -before- stopTime.  If stopTime = -1, then we will play until
	 * the last event in the track.  This method should be protected,
	 * accessible only by the AmTransport.
	 */
	AmEvent*			PlaybackList(AmTime startTime = 0, AmTime stopTime = -1,
									 uint32 flags = 0) const;

	/* Answer the playback list containing -only- events in this track
	 * (i.e., not including tempo events).
	 */
	AmEvent*			RawPlaybackList(AmTime startTime = 0, AmTime stopTime = -1,
										uint32 flags = 0) const;
	
	/* Part of the support for generating events (pulsing).
	 */
	status_t			AddPulseFilters(AmSong* song);	
	
	/*---------------------------------------------------------
	 * AM-NOTIFIER INTERFACE
	 *---------------------------------------------------------*/
	virtual void		MergeRangeChange(	AmRange oldRange, AmRange newRange,
											AmEvent* event = NULL);
	/* A little convenience method that sends out a notice that a filter
	 * in the filter list has changed.
	 */
	void				MergePipelineChange(AmPipelineSegment* pipeline);
	virtual void		MergePipelineChange(pipeline_id id, AmPipelineType type);
	virtual void		MergeFilterChange(pipeline_id id, AmPipelineType type);
	/* If the track has any range changes noted, it should send out
	 * the notification and clear the changes.
	 */
	virtual void		FlushChanges();

	/*---------------------------------------------------------
	 * LOW-LEVEL TRACK MANIPULATION
	 *---------------------------------------------------------*/
	void				SetTitle(const char* title, void* sender, BUndoContext* undo);
	AmPipelineSegment*	PipelineSegment(AmPipelineType type);

protected:
	virtual ~AmTrack();
	
	virtual void Delete()			{ delete this; }

	/*---------------------------------------------------------
	 * AM-EVENT-CONTAINER-I INTERFACE
	 *---------------------------------------------------------*/
	virtual AmPhrase*	PhraseFor(AmEvent* event) const;
	virtual void		AnnotateMessage(BMessage& msg) const;

private:
	int32				mRefCount;
	/* The track implementation holds onto a direct pointer to the song
	 * implementation -- this means that all track methods access the
	 * song as if a read or write lock has already been acquired.  If it
	 * hasn't, that would be a bit of a problem.
	 */
	AmSong* const		mSong;
	AmPhrase			mPhrases;
	/* The signatures are a pointer because it's not a given that a track
	 * has signatures.  Typically, the track just forwards on whatever
	 * signatures are in the song.  However, if a client has specifically
	 * added a signature to the track, then a signatures phrase will be
	 * constructed and used from then on.
	 */
	mutable AmPhrase*	mSignatures;
	/* Always use this method to create a new signatures phrase (if it's
	 * needed and the current one is NULL).  It adds the proper default data.
	 */
	status_t			NewSignatures() const;
	/* The phrase to store my motions.
	 */
	AmPhrase			mMotions;
	uint32				mGroups;
	
	typedef std::vector<AmViewProperty*>		viewproperty_vec;
	/* There are several view property objects which I've just hardcoded,
	 * then a vector for the secondaries, which might have any number.
	 */
	AmViewProperty		mArrangeView;
	AmViewProperty		mPriView;
	viewproperty_vec	mSecViews;
	BMessage			mConfiguration;		// Window state stuff -- frame, etc.
	float				mPhraseHeight;
	
	static const int32	PIPELINE_SIZE = 2;
	AmPipelineSegment	mPipelines[PIPELINE_SIZE];
	
	/* The flags can be in certain modes, like solo, etc.  See the enum
	 * and the accessing methods for the supported modes.
	 */
	uint32				mModeFlags;
	bool				mThroughMode;
	uint32				mRecordMode;
	enum {
		HAS_NOTIFICATION	=	(1<<0)
	};
	uint32				mFlags;
	
	/* This track's target for recording events into the song's record queue.
	 */
	const ArpRef<AmInputTarget> mInputTarget;
	
	/* This is true if the song has asked us to record.  We may not
	 * actually be recording, depending on the record mode.
	 */
	bool				mRecording;
	
	/* Current phrase being recorded into.
	 */
	AmPhraseEvent*		mRecordingPhrase;
	
	/* Last time that was recorded, for extending phrase.
	 */
	AmTime				mLastRecordedTime;
	
	/* Low-level function for managing the mRecordPhrase.
	 */
	void				AttachRecordingPhrase();
	/* Answer true if anything was actually recorded, false otherwise.
	 */
	bool				DetachRecordingPhrase();
};

/***************************************************************************
 * AM-TRACK-EVENT-UNDO
 * An undo operation for adding / removing events in tracks.
 ****************************************************************************/
class AmTrackEventUndo : public BUndoOperation
{
public:
	AmTrackEventUndo(	AmTrack* track, bool added,
						AmPhrase* phrase, AmEvent* event);
	virtual ~AmTrackEventUndo();
	
	virtual const void*	Owner() const;
	virtual bool		MatchOwnerContext(const void* owner) const;
	
	virtual void		Commit();
	virtual void		Undo();
	virtual void		Redo();

	void				Print() const;

private:
	AmTrack* const	mTrack;
	AmPhrase*		mPhrase;
	AmEvent*		mEvent;
	bool			mAdded;

	void			swap_data();
};

/***************************************************************************
 * AM-CHANGE-EVENT-UNDO
 * An undo operation for generic changes made to event.
 ****************************************************************************/
class _AmCeoEntry;

class AmChangeEventUndo : public BUndoOperation
{
public:
	AmChangeEventUndo(const AmTrack* track);
	virtual ~AmChangeEventUndo();

	virtual const void*	Owner() const;
	virtual bool		MatchOwnerContext(const void* owner) const;
	virtual void		Commit();
	virtual void		Undo();
	virtual void		Redo();

	track_id			TrackId() const;
	/* Answer the number of event's I have listed as changing.
	 */	
	int32				CountEvents() const;
	AmEvent*			EventAt(int32 index, AmPhrase** outContainer = NULL);
	/* Answer true if any of my changed events are actually different
	 * from the original events.
	 */
	bool				HasChanges() const;
	/* Call this before an event changes, so I can store the original
	 * values.
	 */
	void				EventChanging(AmPhrase* phrase, AmEvent* event);
	/* Call this on any events that have been added to the undo once they
	 * have changed.
	 */
	void				EventChanged(AmEvent* event);
	void				RemoveEvent(AmEvent* event);
	/* Run through and find every entry that has an event tagged for
	 * changing, but no actual changed event, and remove those entries.
	 */
	void				ScrubUnchanged();
	/* Answer a string that describes the contents of this operation.
	 * For example, if I contain 10 notes, answer "10 Notes".
	 */
	BString				StringContents() const;
	 
	void				Print() const;
	
private:
	const AmTrack*		mTrack;
	BList				mEntries;

	void				swap_data();
	_AmCeoEntry*		EntryForId(event_id id) const;
};

#endif 
