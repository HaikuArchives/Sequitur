/* AmSelections.cpp
 */
#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpDebug.h"
#include "Sequitur/SeqSongSelections.h"

/***************************************************************************
 * SEQ-SONG-SELECTIONS
 ****************************************************************************/
SeqSongSelections* SeqSongSelections::New()
{
	return new SeqSongSelections();
}

SeqSongSelections::SeqSongSelections()
{
}

SeqSongSelections::SeqSongSelections(const SeqSongSelections& o)
		: mRange(o.mRange)
{
	for( uint32 k = 0; k < o.mTracks.size(); k++ )
		mTracks.push_back( o.mTracks[k] );
}

SeqSongSelections::~SeqSongSelections()
{
}

AmRange SeqSongSelections::TimeRange() const
{
	return mRange;
}

void SeqSongSelections::SetTimeRange(AmRange range)
{
	mRange = range;
}

void SeqSongSelections::AddTrack(track_id trackId)
{
	if (!IncludesTrack(trackId) ) mTracks.push_back(trackId);
}

void SeqSongSelections::RemoveTrack(track_id trackId)
{
	for (uint32 k = 0; k < mTracks.size(); k++) {
		if (mTracks[k] == trackId) {
			mTracks.erase(mTracks.begin() + k);
			ArpASSERT(!IncludesTrack(trackId));
			return;
		}
	}
}

bool SeqSongSelections::IncludesTrack(track_id trackId) const
{
	for (uint32 k = 0; k < mTracks.size(); k++)
		if (mTracks[k] == trackId) return true;
	return false;
}

uint32 SeqSongSelections::CountTracks() const
{
	return mTracks.size();
}

track_id SeqSongSelections::TrackAt(uint32 index) const
{
	if (index >= mTracks.size() ) return 0;
	return mTracks[index];
}

bool SeqSongSelections::IsEmpty() const
{
	if( mTracks.size() > 0 ) return false;
	return true;
//	return RangeIsEmpty() && TracksAreEmpty();
}
#if 0
bool SeqSongSelections::RangeIsEmpty() const
{
	return !mRange.IsValid();
}

bool SeqSongSelections::TracksAreEmpty() const
{
	if( mTracks.size() > 0 ) return false;
	return true;
}
#endif

SeqSongSelections* SeqSongSelections::Copy() const
{
	return new SeqSongSelections(*this);
}
