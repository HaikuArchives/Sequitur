/* AmSongObserver.cpp
 */
#define _BUILDING_AmKernel 1

#include "AmPublic/AmSongObserver.h"

#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmSong.h"

/**********************************************************************
 * AM-SONG-OBSERVER
 **********************************************************************/
AmSongObserver::AmSongObserver( AmSongRef songRef )
		: mSongRef(songRef), mSongId(0), mNesting(0)
{
}

AmSongObserver::AmSongObserver( song_id songId )
		: mSongRef(), mSongId(songId), mNesting(0)
{
}

AmSongObserver::~AmSongObserver()
{
}

const AmSong* AmSongObserver::ReadLock() const
{
	if (!mSongRef.IsValid() && mSongId != 0) {
		mSongRef = AmGlobals().SongRef(mSongId);
		if (!mSongRef.IsValid())
			return NULL;
	}
	const AmSong* song = mSongRef.ReadLock();
	if (song)
		mNesting++;
	return song;
}

void AmSongObserver::ReadUnlock(const AmSong* song) const
{
	if (song) {
		mSongRef.ReadUnlock(song);
		mNesting--;
		if (mNesting == 0 && mSongId != 0) {
			mSongRef = AmSongRef();
		}
	}
}

void AmSongObserver::ReadUnlock(const AmPipelineMatrixI* matrix) const
{
	const AmSong*	song = dynamic_cast<const AmSong*>(matrix);
	ArpASSERT(song);
	ReadUnlock(song);
}

AmSong* AmSongObserver::WriteLock(const char* name)
{
	if (!mSongRef.IsValid() && mSongId != 0) {
		mSongRef = AmGlobals().SongRef(mSongId);
		if (!mSongRef.IsValid())
			return NULL;
	}
	AmSong* song = mSongRef.WriteLock();
	if (song)
		mNesting++;
	return song;
}

void AmSongObserver::WriteUnlock(AmSong* song)
{
	if (song) {
		mSongRef.WriteUnlock(song);
		mNesting--;
		if (mNesting == 0 && mSongId != 0) {
			mSongRef = AmSongRef();
		}
	}
}

void AmSongObserver::WriteUnlock(AmPipelineMatrixI* matrix)
{
	AmSong*	song = dynamic_cast<AmSong*>(matrix);
	ArpASSERT(song);
	WriteUnlock(song);
}

AmSongRef AmSongObserver::SongRef() const
{
	if (mSongRef.IsValid())
		return mSongRef;
	if (mSongId != 0)
		return AmGlobals().SongRef(mSongId);
	return AmSongRef();
}

void AmSongObserver::SetSongRef( AmSongRef songRef )
{
	mSongRef = songRef;
	if (mSongRef.IsValid())
		mSongId = 0;
}
