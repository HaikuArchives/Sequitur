/* SeqSongSelections.h
 * Copyright (c)1997 - 2001 by Angry Red Planet and Eric Hackborn.
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
 
#ifndef SEQUITUR_SEQSONGSELECTIONS_H
#define SEQUITUR_SEQSONGSELECTIONS_H

#include <vector.h>
#include <be/support/SupportDefs.h>
#include "AmPublic/AmRange.h"

/***************************************************************************
 * SEQ-SONG-SELECTIONS
 * This object stores a rectangular range area -- a time range and a series
 * of track indexes -- for a single song.
 ****************************************************************************/
class SeqSongSelections
{
public:
	SeqSongSelections();
	SeqSongSelections(const SeqSongSelections& o);
	virtual ~SeqSongSelections();

	static SeqSongSelections* New();

	virtual AmRange		TimeRange() const;
	virtual void		SetTimeRange(AmRange range);

	virtual void		AddTrack(track_id trackId);
	virtual void		RemoveTrack(track_id trackId);
	virtual bool		IncludesTrack(track_id trackId) const;
	virtual uint32		CountTracks() const;
	virtual track_id	TrackAt(uint32 index) const;

	virtual bool		IsEmpty() const;

	virtual SeqSongSelections* Copy() const;

private:
	AmRange				mRange;
	vector<track_id>	mTracks;
};

/******************************************************************
 * SEQ-SONG-WIN-PROPERTIES-I
 * A window which will contain overview objects is expected to
 * implement this interface.
 ******************************************************************/
class SeqSongWinPropertiesI
{
public:
	/* Clients should never delete the answered selections object.  If
	 * they want to clear the selections, they should send 0 to SetSelections().
	 */
	virtual SeqSongSelections*	Selections() const = 0;
	/* Clients should never get the selections, modify them, and do nothing
	 * else -- they should always call set if they've changed them.
	 */
	virtual void				SetSelections(SeqSongSelections* selections) = 0;

	virtual bool				IsRecording() const = 0;
};

#endif 

