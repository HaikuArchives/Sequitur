/* AmSelections.cpp
 */
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <app/Messenger.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSelections.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

/*************************************************************************
 * AM-SELECTIONS-I
 *************************************************************************/
AmSelectionsI* AmSelectionsI::NewSelections()
{
	return new AmSelections();
}

/*************************************************************************
 * AM-SELECTIONS
 *************************************************************************/
AmSelections::AmSelections()
{
}

AmSelections::AmSelections(const AmSelections& o)
{
	for (uint32 k = 0; k < o.mSelections.size(); k++)
		mSelections.push_back( o.mSelections[k] );
}

AmSelections::~AmSelections()
{
}

AmRange AmSelections::TimeRange() const
{
	AmRange		range;
	for (uint32 k = 0; k < mSelections.size(); k++)
		range += mSelections[k].TimeRange();
	return range;
}

uint32 AmSelections::CountTracks() const
{
	return mSelections.size();
}

uint32 AmSelections::CountEvents() const
{
	uint32		count = 0;
	for (uint32 k = 0; k < mSelections.size(); k++)
		count += mSelections[k].CountEvents();
	return count;
}

track_id AmSelections::TrackAt(uint32 trackIndex) const
{
	if (trackIndex >= mSelections.size() ) return 0;
	return mSelections[trackIndex].mTrackId;
}

status_t AmSelections::EventAt(	uint32 trackIndex, uint32 eventIndex,
								track_id* outTrackId,
								AmPhraseEvent** outTopPhrase,
								AmEvent** outEvent,
								int32* outExtraData) const
{
	if (trackIndex >= mSelections.size() ) return B_ERROR;
	*outTrackId = mSelections[trackIndex].mTrackId;
	return mSelections[trackIndex].EventAt(eventIndex, outTopPhrase, outEvent, outExtraData);
}

status_t AmSelections::EventAt(	track_id trackId, uint32 eventIndex,
								AmPhraseEvent** outTopPhrase,
								AmEvent** outEvent,
								int32* outExtraData) const
{
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].EventAt(eventIndex, outTopPhrase, outEvent, outExtraData);
	}
	return B_BAD_INDEX;
}

bool AmSelections::IncludesEvent(	track_id trackId,
									const AmPhraseEvent* topPhrase,
									const AmEvent* event,
									int32* outExtraData) const
{
	ArpASSERT(topPhrase && event);
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].IncludesEvent(topPhrase, event, outExtraData);
	}
	return false;
}

status_t AmSelections::AddEvent(track_id trackId,
								AmPhraseEvent* topPhrase,
								AmEvent* event,
								int32 extraData)
{
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].AddEvent(topPhrase, event, extraData);
	}
	mSelections.push_back(_AmTrackSelectionEntry(trackId, topPhrase, event, extraData));
	return B_OK;
}

status_t AmSelections::AddPhrase(	track_id trackId,
									AmPhraseEvent* topPhrase,
									AmPhraseEvent* container)
{
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].AddPhrase(topPhrase, container);
	}
	mSelections.push_back(trackId);
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].AddPhrase(topPhrase, container);
	}
	return B_NO_MEMORY;
}

status_t AmSelections::RemoveTrack(track_id trackId)
{
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId) {
			mSelections.erase(mSelections.begin() + k);
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}

status_t AmSelections::SetExtraData(track_id trackId,
									AmPhraseEvent* topPhrase,
									AmEvent* event,
									int32 extraData)
{
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].SetExtraData(topPhrase, event, extraData);
	}
	uint32		count = mSelections.size();
	mSelections.push_back(trackId);
	if (count <= mSelections.size()) return B_NO_MEMORY;
	return mSelections[count].SetExtraData(topPhrase, event, extraData);
}

AmSelectionsI* AmSelections::Copy() const
{
	return new AmSelections(*this);
}

void AmSelections::Scrub()
{
	for (uint32 k = 0; k < mSelections.size(); k++)
		mSelections[k].Scrub();
}

void AmSelections::Sync(AmSong* song)
{
	for (uint32 k = 0; k < mSelections.size(); k++)
		mSelections[k].Sync(song);
}

/* FIX:  I assume this will have problems if the first event in the
 * selections does NOT have the lowest time in the selections.  Need
 * to check with dianne about what to do with that -- can I 'rewind'
 * the merged list and answer the head?
 */
AmEvent* AmSelections::AsPlaybackList(const AmSong* song) const
{
	if (!song) return NULL;
	AmEvent*	head = NULL;
	AmTime		start = TimeRange().start;
	for (uint32 k = 0; k < mSelections.size(); k++) {
		head = mSelections[k].AsPlaybackList(song, head, 0);
	}
	if (!head) return NULL;
	am_filter_params params;
	AmTime			endTime = song->CountEndTime();
	AmEvent*		tempos = song->PlaybackList(0, endTime, PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_SIGNATURE);
	AmEvent*		signatures = song->PlaybackList(0, endTime, PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_TEMPO);
	params.AddMotionChanges(song);
	if (tempos) tempos = tempos->HeadEvent();
	if (signatures) signatures = signatures->HeadEvent();
	params.cur_tempo = dynamic_cast<AmTempoChange*>(tempos);
	params.cur_signature = dynamic_cast<AmSignature*>(signatures);
	head = ArpExecFilters(head->HeadEvent(), NORMAL_EXEC_TYPE, false, &params, NULL, NULL, tempos, signatures);
	if (signatures) signatures->DeleteChain();
	if (tempos) tempos->DeleteChain();
	params.DeleteMotionChanges();
	if (head) head = head->HeadEvent();
	/* This is a little trick -- when the events went through the filters, we wanted
	 * them at their actual time in the song, so that any track motions are properly
	 * applied.  But in reality, we want them to play immediately, so now we run
	 * through and offset the start times starting at 0.
	 */
	AmEvent*	e = head;
	while (e) {
		e->SetStartTime(e->StartTime() - start);
		e = e->NextEvent();
	}

	return head;
}

AmEvent* AmSelections::AsChain(	track_id trackId, AmTrack* track,
								AmFilterHolderI* filter) const
{
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == trackId)
			return mSelections[k].AsChain(filter);
	}
	return NULL;
}

AmEvent* AmSelections::AsPhraseChain(	AmTrack* track, uint32 phraseIndex,
										AmPhraseEvent** topPhrase, AmFilterHolderI* filter) const
{
	ArpVALIDATE(track, return NULL);
	for (uint32 k = 0; k < mSelections.size(); k++) {
		if (mSelections[k].mTrackId == track->Id() )
			return mSelections[k].AsPhraseChain(phraseIndex, topPhrase, filter);
	}
	return NULL;
}

void AmSelections::Print() const
{
	printf("AmSelections (%p)\n", this);
	for (uint32 k = 0; k < mSelections.size(); k++)
		mSelections[k].Print();
}

// #pragma mark -

/***************************************************************************
 * _AM-EVENT-SELECTION-ENTRY
 ****************************************************************************/
_AmEventSelectionEntry::_AmEventSelectionEntry()
		: mEvent(NULL), mExtraData(0)
{
}

_AmEventSelectionEntry::_AmEventSelectionEntry(const _AmEventSelectionEntry& o)
		: mEvent(o.mEvent), mExtraData(o.mExtraData)
{
	ArpASSERT(mEvent);
	mEvent->IncRefs();
}

_AmEventSelectionEntry::_AmEventSelectionEntry(	AmEvent* event,
												int32 extraData)
		: mEvent(event), mExtraData(extraData)
{
	ArpASSERT(mEvent);
	mEvent->IncRefs();
}

_AmEventSelectionEntry::~_AmEventSelectionEntry()
{
	if (mEvent) mEvent->DecRefs();
}

_AmEventSelectionEntry& _AmEventSelectionEntry::operator=(const _AmEventSelectionEntry& o)
{
	if (o.mEvent) o.mEvent->IncRefs(); 
	if (mEvent) mEvent->DecRefs(); 
	mEvent = o.mEvent;
	mExtraData = o.mExtraData;
	return *this;
}

void _AmEventSelectionEntry::Print() const
{
	printf("\t\t\tevent: ");
	if (mEvent) mEvent->Print();
	else printf("NULL\n");
}

// #pragma mark -

/***************************************************************************
 * _AM-PHRASE-SELECTION-ENTRY
 ****************************************************************************/
_AmPhraseSelectionEntry::_AmPhraseSelectionEntry()
		: mTopPhrase(NULL)
{
}

_AmPhraseSelectionEntry::_AmPhraseSelectionEntry(const _AmPhraseSelectionEntry& o)
		: mTopPhrase(o.mTopPhrase)
{
	ArpASSERT(mTopPhrase);
	mTopPhrase->IncRefs();
	for (uint32 k = 0; k < o.mEntries.size(); k++) mEntries.push_back(o.mEntries[k]);
}

_AmPhraseSelectionEntry::_AmPhraseSelectionEntry(	AmPhraseEvent* topPhrase,
													AmEvent* event,
													int32 extraData)
		: mTopPhrase(topPhrase)
{
	ArpASSERT(mTopPhrase);
	mTopPhrase->IncRefs();
	mEntries.push_back( _AmEventSelectionEntry(event, extraData) );
}

_AmPhraseSelectionEntry::~_AmPhraseSelectionEntry()
{
	if (mTopPhrase) mTopPhrase->DecRefs();
}

_AmPhraseSelectionEntry& _AmPhraseSelectionEntry::operator=(const _AmPhraseSelectionEntry& o)
{
	if (o.mTopPhrase) o.mTopPhrase->IncRefs(); 
	if (mTopPhrase) mTopPhrase->DecRefs(); 
	mTopPhrase = o.mTopPhrase;
	mEntries.resize(0);
	for (uint32 k = 0; k < o.mEntries.size(); k++) mEntries.push_back(o.mEntries[k]);
	return *this;
}

AmRange _AmPhraseSelectionEntry::TimeRange() const
{
	ArpASSERT(mTopPhrase);
	AmRange		range;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mEvent) range += mTopPhrase->EventRange(mEntries[k].mEvent);
	}
	return range;
}

uint32 _AmPhraseSelectionEntry::CountEvents() const
{
	return mEntries.size();
}

status_t _AmPhraseSelectionEntry::EventAt(	uint32* index,
											AmPhraseEvent** topPhrase,
											AmEvent** event,
											int32* extraData) const
{
	ArpASSERT(index);
	if (*index >= mEntries.size() ) {
		*index = *index - mEntries.size();
		return B_ERROR;
	}
	*topPhrase = mTopPhrase;
	*event = mEntries[*index].mEvent;
	if (extraData) *extraData = mEntries[*index].mExtraData;
	return B_OK;
}

void _AmPhraseSelectionEntry::AddEvent(AmEvent* event, int32 extraData)
{
	ArpASSERT(event);
	mEntries.push_back( _AmEventSelectionEntry(event, extraData) );
}

status_t _AmPhraseSelectionEntry::EntryFor(	const AmEvent* event,
											_AmEventSelectionEntry** outEventEntry,
											int32* outExtraData) const
{
	ArpASSERT(event);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mEvent == event) {
			*outEventEntry = const_cast<_AmEventSelectionEntry*>(&(mEntries[k]));
			ArpASSERT(*outEventEntry);
			if (outExtraData) *outExtraData = mEntries[k].mExtraData;
			return B_OK;
		}
	}
	return B_ERROR;
}

void _AmPhraseSelectionEntry::Scrub()
{
	if (mTopPhrase && mTopPhrase->IsDeleted() ) {
		mEntries.resize(0);
		return;
	}
#if 0
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mEvent && mEntries[k].mEvent->IsDeleted()) {
			mEntries.erase(mEntries.begin() + k);
			k = 0;
		}
	}
#endif
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mEvent) {
			if (mEntries[k].mEvent->IsDeleted() || mEntries[k].mEvent->Parent() == NULL) {
				mEntries.erase(mEntries.begin() + k);
				k = 0;
			}
		}
	}
		/* In certain situations, the event and container are both
		 * valid, but the event is no longer in the given container.
		 * One check that could be performed here to definitely take
		 * care of this case is to erase the selection if
		 *	!mContainer->Phrase().Includes( mEvent )
		 * Time-wise, however, that's pretty horrendous.  I *think*
		 * a feasible alternative is to erase the selection if the
		 * container can't contain the event -- i.e., the event's time
		 * lies outside of the container.  If this turns out to be a
		 * false assumption, the previous method can be reverted to.
		 *
		 * Unfortunately, while I know this is a problem, I didn't write
		 * down a test case for it, and I can't remember what exactly
		 * gives rise to the problem.  I know it's something about either
		 * the separate or merge commands, but whenever I've tried either
		 * they seem to work fine.
		 */
}

void _AmPhraseSelectionEntry::Sync(AmSong* song)
{
	if (mTopPhrase && mTopPhrase->Phrase() ) mTopPhrase->Phrase()->Sync(this);
}

static AmEvent* phrase_as_playback_list(AmPhraseEvent* phrase,
										AmEvent* mergeEvent,
										AmTime start,
										AmFilterHolderI* output)
{
	if (!phrase || !phrase->Phrase() ) return mergeEvent;
	AmNode*		n = phrase->Phrase()->HeadNode();
	AmEvent*	head = mergeEvent;
	while (n) {
		if (n->Event()->Type() == n->Event()->PHRASE_TYPE) {
			head = phrase_as_playback_list(dynamic_cast<AmPhraseEvent*>(n->Event()), head, start, output);
		} else {
			AmEvent*	e = n->Event()->Copy();
			if (e) {
				e->SetParent(NULL);
				e->SetStartTime(e->StartTime() - start);
				e->SetNextFilter(output);
				if (head) {
					head->MergeEvent(e);
					head = head->HeadEvent();
				} else head = e;
			}
		}
		n = n->next;
	}
	return head;
}

AmEvent* _AmPhraseSelectionEntry::AsPlaybackList(	AmEvent* mergeEvent,
													AmTime start,
													AmFilterHolderI* output) const
{
	if (!mTopPhrase) return mergeEvent;
	AmEvent*	head = mergeEvent;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mEvent) {
			if (mEntries[k].mEvent->Type() == AmEvent::PHRASE_TYPE)
				head = phrase_as_playback_list(dynamic_cast<AmPhraseEvent*>(mEntries[k].mEvent), head, start, output);
			else {
				AmEvent*	e = mEntries[k].mEvent->Copy();
				if (e) {
					e->SetParent(NULL);
					e->SetStartTime(e->StartTime() - start);
					e->SetNextFilter(output);
					if (head) {
						head->MergeEvent(e);
						head = head->HeadEvent();
					} else head = e;
				}
			}
		}
	}
	return head;
}

AmEvent* _AmPhraseSelectionEntry::AsPhraseChain(AmPhraseEvent** topPhrase,
												AmFilterHolderI* filter) const
{
	*topPhrase = mTopPhrase;
	AmEvent*		cur = NULL;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		AmEvent*	e = mEntries[k].mEvent;
		if (e) {
			e->RemoveEvent();
			if (filter) e->SetNextFilter(filter);
			if (!cur) cur = e;
			else if (e->StartTime() > cur->StartTime() ) {
				cur->AppendEvent(e);
				cur = e;
			} else {
				cur->MergeEvent(e);
				cur = cur->TailEvent();
			}
		}
	}
	if (cur) cur = cur->HeadEvent();
	ArpASSERT(VerifyChain(cur) );
	return cur;
}

void _AmPhraseSelectionEntry::Print() const
{
	printf("\t\tTop phrase: ");
	if (mTopPhrase) mTopPhrase->Print();
	else printf("NULL\n");

	printf("There are %ld event selection entries\n", mEntries.size());
	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].Print();
}

bool _AmPhraseSelectionEntry::VerifyChain(AmEvent* head) const
{
	if (!head || mEntries.size() < 1) {
		if (head) return false;
		if (mEntries.size() > 0) return false;
	}

	AmEvent*	cur = head;
	while (cur) {
		if (!VerifyEvent(cur) ) {
			printf("CHAIN HAS WRONG EVENT "); cur->Print();
			return false;
		}
		cur = cur->NextEvent();
	}

	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (!VerifyChainEvent(head, mEntries[k].mEvent) ) {
			printf("CHAIN IS MISSING EVENT "); mEntries[k].mEvent->Print();
			return false;
		}
	}
	return true;
}

bool _AmPhraseSelectionEntry::VerifyEvent(AmEvent* event) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (event == mEntries[k].mEvent) return true;
	}
	return false;
}

bool _AmPhraseSelectionEntry::VerifyChainEvent(AmEvent* chain, AmEvent* event) const
{
	AmEvent*	cur = chain;
	while (cur) {
		if (cur == event) return true;
		cur = cur->NextEvent();
	}
	return false;
}

// #pragma mark -

/*************************************************************************
 * _AM-TRACK-SELECTION-ENTRY
 *************************************************************************/
_AmTrackSelectionEntry::_AmTrackSelectionEntry()
		: mTrackId(0)
{
}

_AmTrackSelectionEntry::_AmTrackSelectionEntry(track_id tid)
		: mTrackId(tid)
{
}

_AmTrackSelectionEntry::_AmTrackSelectionEntry(	track_id tid, AmPhraseEvent* topPhrase,
												AmEvent* event, int32 extraData)
		: mTrackId(tid)
{
	AddEvent(topPhrase, event, extraData);
}

_AmTrackSelectionEntry::_AmTrackSelectionEntry(const _AmTrackSelectionEntry& o)
		: mTrackId(o.mTrackId)
{
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back( o.mEntries[k] );
}

_AmTrackSelectionEntry& _AmTrackSelectionEntry::operator=(const _AmTrackSelectionEntry& o)
{
	mEntries.resize(0);
	mTrackId = o.mTrackId;
	for (uint32 k = 0; k < o.mEntries.size(); k++)
		mEntries.push_back( o.mEntries[k] );
	return *this;
}

AmRange _AmTrackSelectionEntry::TimeRange() const
{
	AmRange		range;
	for (uint32 k = 0; k < mEntries.size(); k++)
		range += mEntries[k].TimeRange();
	return range;
}

uint32 _AmTrackSelectionEntry::CountEvents() const
{
	uint32	count = 0;
	for (uint32 k = 0; k < mEntries.size(); k++)
		count += mEntries[k].CountEvents();
	return count;
}

status_t _AmTrackSelectionEntry::EventAt(	uint32 index,
											AmPhraseEvent** topPhrase,
											AmEvent** event,
											int32* extraData) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].EventAt(&index, topPhrase, event, extraData) == B_OK)
			return B_OK;
	}
	return B_ERROR;
}

bool _AmTrackSelectionEntry::IncludesEvent(	const AmPhraseEvent* topPhrase,
											const AmEvent* event,
											int32* outExtraData) const
{
	ArpASSERT(topPhrase && event);
	_AmPhraseSelectionEntry*	pe;
	_AmEventSelectionEntry*		ee;
	return EntryFor(topPhrase, event, &pe, &ee, outExtraData) == B_OK;
}

status_t _AmTrackSelectionEntry::AddEvent(	AmPhraseEvent* topPhrase,
											AmEvent* event,
											int32 extraData)
{
	ArpASSERT(topPhrase && event);
	ArpASSERT(IncludesEvent(topPhrase, event) == false);

	bool		added = false;	
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mTopPhrase == topPhrase) {
			mEntries[k].AddEvent(event, extraData);
			added = true;
			break;
		}
	}
	if (!added) mEntries.push_back( _AmPhraseSelectionEntry(topPhrase, event, extraData) );
	/* If this is a phrase event, then add in all phrase events
	 * it contains.  That's the rule.
	 */
	if (event->Type() == event->PHRASE_TYPE) {
		AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(event);
		if (pe && pe->Phrase()) {
			AmNode*		n = pe->Phrase()->HeadNode();
			while (n) {
				if (n->Event()->Type() == n->Event()->PHRASE_TYPE)
					AddEvent(topPhrase, n->Event(), extraData);
				n = n->next;
			}
		}
	}
	return B_OK;
}

status_t _AmTrackSelectionEntry::AddPhrase(AmPhraseEvent* topPhrase, AmPhraseEvent* container)
{
	ArpASSERT(topPhrase && topPhrase->Phrase() );
	ArpASSERT(container && container->Phrase() );
	if (!topPhrase || !topPhrase->Phrase() ) return B_ERROR;
	if (!container || !container->Phrase() ) return B_ERROR;
	AmNode*		node = container->Phrase()->HeadNode();
	while (node) {
		status_t	err = AddEvent(topPhrase, node->Event());
		if (err != B_OK) return err;
		node = node->next;
	}
	return B_OK;
}

status_t _AmTrackSelectionEntry::SetExtraData(	AmPhraseEvent* topPhrase,
												AmEvent* event,
												int32 extraData)
{
	ArpASSERT(topPhrase && event);
	_AmPhraseSelectionEntry*	pe;
	_AmEventSelectionEntry*		ee;
	if (EntryFor(topPhrase, event, &pe, &ee) != B_OK) return B_ERROR;
	ee->mExtraData = extraData;
	return B_OK;
}

void _AmTrackSelectionEntry::Scrub()
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		mEntries[k].Scrub();
		if (mEntries[k].mEntries.size() < 1) {
			mEntries.erase(mEntries.begin() + k);
			k = 0;
		}
	}
}

void _AmTrackSelectionEntry::Sync(AmSong* song)
{
	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].Sync(song);
}

static void start_filter(AmFilterHolderI* holder)
{
	if (!holder || !holder->Filter() ) return;
	holder->Filter()->Start(holder->Filter()->TRANSPORT_CONTEXT);
	uint32					count = holder->CountConnections();
	for (uint32 k = 0; k < count; k++) {
		AmFilterHolderI*	n = holder->ConnectionAt(k);
		if (n) start_filter(n);
	}
}

AmEvent* _AmTrackSelectionEntry::AsPlaybackList(const AmSong* song, AmEvent* mergeEvent, AmTime start) const
{
	const AmTrack*		track = song->Track(mTrackId);
	if (!track) return mergeEvent;
	AmFilterHolderI*	output = track->Filter(OUTPUT_PIPELINE);
	if (!output) return mergeEvent;
	
	AmEvent*	head = mergeEvent;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		head = mEntries[k].AsPlaybackList(mergeEvent, start, output);
	}
	start_filter(output);
	
	return head;
}

AmEvent* _AmTrackSelectionEntry::AsChain(AmFilterHolderI* filter) const
{
	AmEvent*		cur = NULL;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		for (uint32 j = 0; j < mEntries[k].mEntries.size(); j++) {
			AmEvent*	e = mEntries[k].mEntries[j].mEvent;
			if (e) {
				e->RemoveEvent();
				if (filter) e->SetNextFilter(filter);
				if (!cur) cur = e;
				else if (e->StartTime() > cur->StartTime() ) {
					cur->AppendEvent(e);
					cur = e;
				} else {
					cur->MergeEvent(e);
					cur = cur->TailEvent();
				}
			}
		}
	}
	if (cur) cur = cur->HeadEvent();
	ArpASSERT(VerifyChain(cur) );
	return cur;
}

AmEvent* _AmTrackSelectionEntry::AsPhraseChain(	uint32 phraseIndex, AmPhraseEvent** topPhrase,
												AmFilterHolderI* filter) const
{
	if (phraseIndex >= mEntries.size() ) return NULL;
	return mEntries[phraseIndex].AsPhraseChain(topPhrase, filter);
}

void _AmTrackSelectionEntry::Print() const
{
	printf("\tAmTrackSelections %p, %ld phrase selections\n", mTrackId, mEntries.size());
	for (uint32 k = 0; k < mEntries.size(); k++)
		mEntries[k].Print();
	fflush(stdout);
}

status_t _AmTrackSelectionEntry::EntryFor(	const AmPhraseEvent* topPhrase,
											const AmEvent* event,
											_AmPhraseSelectionEntry** outPhraseEntry,
											_AmEventSelectionEntry** outEventEntry,
											int32* outExtraData) const
{
	ArpASSERT(topPhrase && event);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].mTopPhrase == topPhrase) {
			*outPhraseEntry = const_cast<_AmPhraseSelectionEntry*>(&(mEntries[k]));
			ArpASSERT(*outPhraseEntry);
			return (*outPhraseEntry)->EntryFor(event, outEventEntry, outExtraData);
		}
	}
	return B_ERROR;
}

bool _AmTrackSelectionEntry::VerifyChain(AmEvent* head) const
{
	if (!head || mEntries.size() < 1) {
		if (head) return false;
		if (mEntries.size() > 0) return false;
	}
	AmEvent*	cur = head;
	while (cur) {
		if (!VerifyEvent(cur) ) {
			printf("CHAIN HAS WRONG EVENT "); cur->Print();
			return false;
		}
		cur = cur->NextEvent();
	}

	for (uint32 k = 0; k < mEntries.size(); k++) {
		for (uint32 j = 0; j < mEntries[k].mEntries.size(); j++) {
			if (!VerifyChainEvent(head, mEntries[k].mEntries[j].mEvent) ) {
				printf("CHAIN IS MISSING EVENT "); mEntries[k].mEntries[j].mEvent->Print();
				return false;
			}
		}
	}
	return true;
}

bool _AmTrackSelectionEntry::VerifyEvent(AmEvent* event) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		for (uint32 j = 0; j < mEntries[k].mEntries.size(); j++) {
			if (event == mEntries[k].mEntries[j].mEvent) return true;
		}
	}
	return false;
}

bool _AmTrackSelectionEntry::VerifyChainEvent(AmEvent* chain, AmEvent* event) const
{
	AmEvent*	cur = chain;
	while (cur) {
		if (cur == event) return true;
		cur = cur->NextEvent();
	}
	return false;
}
