/* AmSelections.h
 * Copyright (c)1997 - 2000 by Angry Red Planet and Eric Hackborn.
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
 
#ifndef AMKERNEL_AMSELECTIONS_H
#define AMKERNEL_AMSELECTIONS_H

#include "AmPublic/AmSelectionsI.h"

#include <vector>

/***************************************************************************
 * _AM-EVENT-SELECTION-ENTRY
 * A class to store a single cached, selected event.
 ****************************************************************************/
class _AmEventSelectionEntry
{
public:
	_AmEventSelectionEntry();
	_AmEventSelectionEntry(const _AmEventSelectionEntry& o);
	_AmEventSelectionEntry(AmEvent* event, int32 extraData);
	virtual ~_AmEventSelectionEntry();

	AmEvent*			mEvent;
	int32				mExtraData;

	_AmEventSelectionEntry&	operator=(const _AmEventSelectionEntry& o);

	void				Print() const;
};

/***************************************************************************
 * _AM-PHRASE-SELECTION-ENTRY
 * A class to store a cached AmPhrase, along with all of its selected events.
 ****************************************************************************/
class _AmPhraseSelectionEntry
{
public:
	_AmPhraseSelectionEntry();
	_AmPhraseSelectionEntry(const _AmPhraseSelectionEntry& o);
	_AmPhraseSelectionEntry(AmPhraseEvent* topPhrase, AmEvent* event, int32 extraData);
	virtual ~_AmPhraseSelectionEntry();

	AmPhraseEvent*		mTopPhrase;
	std::vector<_AmEventSelectionEntry>	mEntries;

	_AmPhraseSelectionEntry&	operator=(const _AmPhraseSelectionEntry& o);

	AmRange				TimeRange() const;
	uint32				CountEvents() const;
	status_t			EventAt(uint32* index,
								AmPhraseEvent** topPhrase,
								AmEvent** event,
								int32* extraData = NULL) const;
	void				AddEvent(AmEvent* event, int32 extraData);

	status_t			EntryFor(	const AmEvent* event,
									_AmEventSelectionEntry** outEventEntry,
									int32* outExtraData = NULL) const;
	void				Scrub();
	void				Sync(AmSong* song);
	AmEvent*			AsPlaybackList(	AmEvent* mergeEvent,
										AmTime start,
										AmFilterHolderI* output) const;
	AmEvent*			AsPhraseChain(	AmPhraseEvent** topPhrase,
										AmFilterHolderI* filter) const;
	void				Print() const;

private:
	bool		VerifyChain(AmEvent* head) const;
	bool		VerifyEvent(AmEvent* event) const;
	bool		VerifyChainEvent(AmEvent* chain, AmEvent* event) const;
};

/***************************************************************************
 * _AM-TRACK-SELECTION-ENTRY
 * A class to store an association of a track with all of its selection entries.
 ****************************************************************************/
class _AmTrackSelectionEntry
{
public:
	_AmTrackSelectionEntry();
	_AmTrackSelectionEntry(track_id tid);
	_AmTrackSelectionEntry(	track_id tid, AmPhraseEvent* topPhrase,
							AmEvent* event, int32 extraData);
//	_AmTrackSelectionEntry(	track_id tid, AmPhraseEvent* topPhrase,
//							AmPhraseEvent* container);
	_AmTrackSelectionEntry(const _AmTrackSelectionEntry& o);

	_AmTrackSelectionEntry&	operator=(const _AmTrackSelectionEntry& o);

	AmRange				TimeRange() const;
	uint32				CountEvents() const;

	status_t			EventAt(uint32 index,
								AmPhraseEvent** topPhrase,
								AmEvent** event,
								int32* extraData = NULL) const;
	bool				IncludesEvent(	const AmPhraseEvent* topPhrase,
										const AmEvent* event,
										int32* outExtraData = NULL) const;
	status_t			AddEvent(	AmPhraseEvent* topPhrase,
									AmEvent* event,
									int32 extraData = 0);
	status_t			AddPhrase(	AmPhraseEvent* topPhrase,
									AmPhraseEvent* container);
	status_t			SetExtraData(	AmPhraseEvent* topPhrase,
										AmEvent* event,
										int32 extraData);
	void				Scrub();
	void				Sync(AmSong* song);
	AmEvent*			AsPlaybackList(const AmSong* song, AmEvent* mergeEvent, AmTime start) const;
	AmEvent*			AsChain(AmFilterHolderI* filter) const;
	AmEvent*			AsPhraseChain(	uint32 phraseIndex, AmPhraseEvent** topPhrase,
										AmFilterHolderI* filter) const;
	void				Print() const;

	track_id			mTrackId;
	std::vector<_AmPhraseSelectionEntry>	mEntries;

private:
	status_t			EntryFor(	const AmPhraseEvent* topPhrase,
									const AmEvent* event,
									_AmPhraseSelectionEntry** outPhraseEntry,
									_AmEventSelectionEntry** outEventEntry,
									int32* outExtraData = NULL) const;

	/* Answer true if the chain contains all and only the events I contain.
	 */
	bool				VerifyChain(AmEvent* head) const;
	/* Answer true if one of my selections objects contains the event.
	 */
	bool				VerifyEvent(AmEvent* event) const;
	/* Answer true if the supplied chain contains the event.
	 */
	bool				VerifyChainEvent(AmEvent* chain, AmEvent* event) const;
};

/***************************************************************************
 * AM-SELECTIONS
 * NOTE:  Right now there are a lot of inefficiencies in places that could
 * cause problems.  IncludesEvent(), for example, is called for every single
 * event that gets drawn, and we might want to think about making this fast.
 ****************************************************************************/
class AmSelections  : public AmSelectionsI
{
public:
	AmSelections(); 
	AmSelections(const AmSelections& o); 
	virtual ~AmSelections();

	virtual AmRange		TimeRange() const;
	virtual uint32		CountTracks() const;
	virtual uint32		CountEvents() const;
	virtual track_id	TrackAt(uint32 trackIndex) const;
	virtual status_t	EventAt(uint32 trackIndex, uint32 eventIndex,
								track_id* outTrackId,
								AmPhraseEvent** outTopPhrase,
								AmEvent** outEvent,
								int32* outExtraData = NULL) const;
	virtual status_t	EventAt(track_id trackId, uint32 eventIndex,
								AmPhraseEvent** outTopPhrase,
								AmEvent** outEvent,
								int32* outExtraData = NULL) const;
	virtual bool		IncludesEvent(	track_id trackId,
										const AmPhraseEvent* topPhrase,
										const AmEvent* event,
										int32* outExtraData = NULL) const;
	virtual status_t	AddEvent(	track_id trackId,
									AmPhraseEvent* topPhrase,
									AmEvent* event,
									int32 extraData = 0);
	virtual status_t	AddPhrase(	track_id trackId,
									AmPhraseEvent* topPhrase,
									AmPhraseEvent* container);
	virtual status_t	RemoveTrack(track_id trackId);
	virtual status_t	SetExtraData(	track_id trackId,
										AmPhraseEvent* topPhrase,
										AmEvent* event,
										int32 extraData);
	virtual AmSelectionsI* Copy() const;
	virtual void		Scrub();
	virtual void		Sync(AmSong* song);
	virtual AmEvent*	AsPlaybackList(const AmSong* song) const;
	virtual AmEvent*	AsChain(track_id trackId, AmTrack* track,
								AmFilterHolderI* filter = NULL) const;
	virtual AmEvent*	AsPhraseChain(	AmTrack* track, uint32 phraseIndex,
										AmPhraseEvent** topPhrase, AmFilterHolderI* filter = NULL) const;

	void				Print() const;

private:
	std::vector<_AmTrackSelectionEntry>	mSelections;
};

#endif 

