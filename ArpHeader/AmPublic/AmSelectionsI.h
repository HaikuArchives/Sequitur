/* AmSelectionsI.h
 * Copyright (c)2000 by Angry Red Planet and Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2000.05.31		hackborn
 * Created this file.
 */ 
 
#ifndef AMPUBLIC_AMSELECTIONSI_H
#define AMPUBLIC_AMSELECTIONSI_H

#include <support/SupportDefs.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmRange.h"
class AmFilterHolderI;
class AmPhrase;
class AmPhraseEvent;
class AmSong;
class AmTrack;

/***************************************************************************
 * AM-SELECTIONS-I
 * This object stores a collection of selected events.  Since each event
 * needs to store a container in addition to the actual event, a standard
 * AmPhrase could not be used.  Also, an AmPhrase would be misleading:  Since
 * this collection doesn't hear about changes in events, then the events
 * could have their times changed, but that wouldn't be reflected here.
 ****************************************************************************/
class AmSelectionsI
{
public:
	virtual ~AmSelectionsI()	{ }

	/* Use this to get a new instance of a selections object.
	 */
	static AmSelectionsI* NewSelections();

	/* Answer the time range of the selections.  These values are probably being
	 * computed, so use them sparingly.  The range will not be valid if I have
	 * no events.
	 */
	virtual AmRange		TimeRange() const = 0;
	virtual uint32		CountTracks() const = 0;
	virtual uint32		CountEvents() const = 0;
	/* Answer the track Id at the given index, or 0 if none.
	 */	
	virtual track_id	TrackAt(uint32 trackIndex) const = 0;
	/* Answer the event and container at the supplied index.  Return B_OK if
	 * everything went well, otherwise B_ERROR.  The extra data can be
	 * supplied if events have any specific piece of information
	 * they want retained.  Extra data can be generated, for example,
	 * from the the InterestingEventAt() method in the tool target:
	 * A note on event could specify whether the user clicked at
	 * the start or end of the note.
	 */
	virtual status_t	EventAt(uint32 trackIndex, uint32 eventIndex,
								track_id* outTrackId,
								AmPhraseEvent** outTopPhrase,
								AmEvent** outEvent,
								int32* outExtraData = NULL) const = 0;
	virtual status_t	EventAt(track_id trackId, uint32 eventIndex,
								AmPhraseEvent** outTopPhrase,
								AmEvent** outEvent,
								int32* outExtraData = NULL) const = 0;
	/* Answer true if I currently contain the supplied event.
	 */
	virtual bool		IncludesEvent(	track_id trackId,
										const AmPhraseEvent* topPhrase,
										const AmEvent* event,
										int32* outExtraData = NULL) const = 0;
	/* Add a new event to this collection.  Adding a phrase event will
	 * automatically cause all phrase events it contains, recursively,
	 * to be added to the selection:  That's just the current rule,
	 * users can't select an event at the top of a nest without implicitly
	 * selecting everything in the nest.
	 * 
	 * The extra data can be supplied if events have any specific
	 * piece of information they want retained.  Extra data can be
	 * generated, for example, from the the InterestingEventAt() method
	 * in the tool target:  A note on event could specify whether the
	 * user clicked at the start or end of the note.
	 */
	virtual status_t	AddEvent(	track_id trackId,
									AmPhraseEvent* topPhrase,
									AmEvent* event,
									int32 extraData = 0) = 0;
	/* Add every event in the phrase to this collection.
	 */
	virtual status_t	AddPhrase(	track_id trackId,
									AmPhraseEvent* topPhrase,
									AmPhraseEvent* container) = 0;
	/* Remove all events from the supplied track.
	 */
	virtual status_t	RemoveTrack(track_id trackId) = 0;
	/* Clients can change the extra data associated with an existing
	 * selected event.  If the event doesn't exist, B_ERROR is answered.
	 */
	virtual status_t	SetExtraData(	track_id trackId,
										AmPhraseEvent* topPhrase,
										AmEvent* event,
										int32 extraData) = 0;

	virtual AmSelectionsI* Copy() const = 0;

	/* This is used only in special situations -- the list might contain
	 * events that have been deleted, this method will remove them.  Generally,
	 * only classes that actually store the selections need to do this.
	 */
	virtual void		Scrub() = 0;
	/* This is used only in special situations -- if a client knows
	 * the AmPhrase end time info might be incorrect (for example, a filter
	 * has been processing the events in this list) -- then this method
	 * can be used to make sure everyone is sync'ed up again.
	 */
	virtual void		Sync(AmSong* song) = 0;

	/*---------------------------------------------------------
	 * CONVENIENCE
	 * These methods are not strictly related to the business of
	 * being a selections object, but are based on data the
	 * selections hold and are operations that multiple clients
	 * use.
	 *---------------------------------------------------------*/
	/* Convert myself into a list suitable for playback and answer
	 * the head.  Typically, a client would just pass this list off to
	 * a transport for playback.  If a client does something else,
	 * though, then they're responsible for deleting these events.
	 */
	virtual AmEvent*	AsPlaybackList(const AmSong* song) const = 0;
	/* Take all events in the supplied track and place them into a
	 * chain (i.e. the NextEvent() and PrevEvent() methods of each
	 * event are filled properly).  Answer the head of this chain.
	 * Call SetNextFilter() on each event with the supplied filter.
	 */
	virtual AmEvent*	AsChain(track_id trackId, AmTrack* track,
								AmFilterHolderI* filter = NULL) const = 0;
	/* Take all events in the supplied track and phrase place them
	 * into a chain (i.e. the NextEvent() and PrevEvent() methods of
	 * each event are filled properly).  Answer the head of this chain.
	 * Call SetNextFilter() on each event with the supplied filter.
	 */
	virtual AmEvent*	AsPhraseChain(	AmTrack* track, uint32 phraseIndex,
										AmPhraseEvent** topPhrase, AmFilterHolderI* filter = NULL) const = 0;
};

#endif 

