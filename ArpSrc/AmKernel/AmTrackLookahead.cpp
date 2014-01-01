/* AmRange.cpp
*/
#define _BUILDING_AmKernel 1

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmTrackLookahead.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmFilterI.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

/***************************************************************************
 * AM-TRACK-LOOKAHEAD
 ****************************************************************************/
AmTrackLookahead::AmTrackLookahead()
{
}

AmTrackLookahead::~AmTrackLookahead()
{
	DeleteAll();
}

const AmEvent* AmTrackLookahead::Lookahead(track_id tid) const
{
	const _LookaheadEntry*	e = EntryFor(tid);
	if (!e) return NULL;
	return e->event;
}

const AmEvent* AmTrackLookahead::Lookahead(AmFilterHolderI* holder) const
{
	ArpVALIDATE(holder, return NULL);
	const _LookaheadEntry*	e = EntryFor(holder->TrackId());
	if (!e) return NULL;
	if (e->event) return e->event;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].tid != e->tid && mEntries[k].groups == e->groups) {
			if (mEntries[k].event) return mEntries[k].event;
		}
	}
	return NULL;
}

status_t AmTrackLookahead::SetTracks(const AmSong* song)
{
	DeleteAll();
	ArpVALIDATE(song, return B_ERROR);
	const AmTrack*				t = NULL;
	vector<const AmTrack*>		unadded;
	for (uint32 k = 0; (t = song->Track(k)) != NULL; k++) {
		AmTime		length = t->LookaheadTime();
		if (length > 0) mEntries.push_back(_LookaheadEntry(t->Id(), t->Groups(), length));
		else unadded.push_back(t);
	}
	/* Now add in all the tracks that weren't added but are grouped with
	 * tracks that were added.
	 */
	for (uint32 k = 0; k < unadded.size(); k++) {
		AmTime		length;
		if (HasSibling(unadded[k]->Groups(), &length)) 
			mEntries.push_back(_LookaheadEntry(unadded[k]->Id(), unadded[k]->Groups(), length));
	}
	return B_OK;
}

status_t AmTrackLookahead::MakeChains(const AmSong* song, const AmTime start)
{
	const AmTrack*	t = NULL;
	for (uint32 k = 0; (t = song->Track(k)) != NULL; k++) {
		MakeChains(t, start);
	}
	return B_OK;
}

status_t AmTrackLookahead::MakeChains(const AmTrack* track, const AmTime start)
{
	ArpVALIDATE(track, return B_ERROR);
	_LookaheadEntry*	e = EntryFor(track->Id());
	if (!e) return B_ERROR;
	if (e->event) e->event->Delete();
	e->event = track->PlaybackList(start, start + e->length, PLAYBACK_NO_TEMPO|PLAYBACK_NO_SIGNATURE);
	return B_OK;
}

_LookaheadEntry* AmTrackLookahead::EntryFor(track_id tid)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].tid == tid) return &(mEntries[k]);
	}
	return NULL;
}

const _LookaheadEntry* AmTrackLookahead::EntryFor(track_id tid) const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].tid == tid) return &(mEntries[k]);
	}
	return NULL;
}

bool AmTrackLookahead::HasSibling(uint32 groups, AmTime* length) const
{
	if (groups == 0) return false;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].groups == groups) {
			*length = mEntries[k].length;
			return true;
		}
	}
	return false;
}

void AmTrackLookahead::DeleteAll()
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].event) mEntries[k].event->Delete();
	}
	mEntries.resize(0);
}

void AmTrackLookahead::Print() const
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		printf("\t%ld: tid %p groups %ld length %lld\n", k,
				mEntries[k].tid, mEntries[k].groups, mEntries[k].length);
	}
}

/***************************************************************************
 * _LOOKAHEAD-ENTRY
 ****************************************************************************/
_LookaheadEntry::_LookaheadEntry()
		: tid(NULL), groups(0), event(NULL), length(0)
{
}

_LookaheadEntry::_LookaheadEntry(const _LookaheadEntry& o)
		: tid(o.tid), groups(o.groups), event(o.event), length(o.length)
{
}

#if 0
	_LookaheadEntry(track_id t, AmEvent* e);
_LookaheadEntry::_LookaheadEntry(track_id t, AmEvent* e)
		: tid(t), event(e), length(0)
{
}
#endif
		
_LookaheadEntry::_LookaheadEntry(track_id t, uint32 g, AmTime l)
		: tid(t), groups(g), event(NULL), length(l)
{
}
		
_LookaheadEntry& _LookaheadEntry::operator=(const _LookaheadEntry &o)
{
	tid = o.tid;
	groups = o.groups;
	event = o.event;
	length = o.length;
	return *this;
}
