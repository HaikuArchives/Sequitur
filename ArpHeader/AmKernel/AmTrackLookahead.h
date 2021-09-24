/* AmTrackLookahead.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.05.19		hackborn@angryredplanet.co,
 * First version
 */

#ifndef AMKERNEL_AMTRACKLOOKAHEAD_H
#define AMKERNEL_AMTRACKLOOKAHEAD_H


#include <vector>
#include "AmPublic/AmDefs.h"
class AmEvent;
class AmFilterHolderI;
class AmSong;
class AmTrack;

class _LookaheadEntry
{
public:
	_LookaheadEntry();
	_LookaheadEntry(const _LookaheadEntry& o);
	_LookaheadEntry(track_id t, uint32 g, AmTime l);

	_LookaheadEntry&	operator=(const _LookaheadEntry& o);
	
	track_id		tid;
	uint32			groups;
	AmEvent*		event;
	AmTime			length;
};

/*************************************************************************
 * AM-TRACK-LOOKAHEAD
 * I am essentially an array, indexed by track ID.  For each track, I
 * store an event list.  Clients can access the event list either by
 * requesting for a given track, or requesting for a given filter, which
 * will answer with the lookahead for any sibling tracks if the primary
 * doesn't exist.  This is essentially a big hack to deal with the
 * fact that sequitur doesn't have multi-channel tracks.
 *
 * I own all my event lists and will delete when done.
 *************************************************************************/
class AmTrackLookahead
{
public:
	AmTrackLookahead();
	virtual ~AmTrackLookahead();
	
	virtual	const AmEvent*	Lookahead(track_id tid) const;
	virtual const AmEvent*	Lookahead(AmFilterHolderI* holder) const;

	/* Set up all the tracks that need lookahead.  Don't
	 * actually generate the lookahead chains here.
	 */
	status_t				SetTracks(const AmSong* song);
	/* Wipe out any chains for the supplied track(s) and make new ones.
	 */
	status_t				MakeChains(const AmSong* song, const AmTime start);
	status_t				MakeChains(const AmTrack* track, const AmTime start);

private:
	std::vector<_LookaheadEntry>	mEntries;	

	_LookaheadEntry*		EntryFor(track_id tid);
	const _LookaheadEntry*	EntryFor(track_id tid) const;

	bool					HasSibling(uint32 groups, AmTime* length) const;
	void					DeleteAll();

/* Debugging
 */
public:
	void					Print() const;
};


#endif
