/* AmSongObserver.h
 * Copyright (c)2000 by Dianne and Eric Hackborn.
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
 * 05.21.00			hackborn
 * Created this file
 */

#ifndef AMKERNEL_AMSONGOBSERVER_H
#define AMKERNEL_AMSONGOBSERVER_H

#include <be/support/SupportDefs.h>
#include "AmPublic/AmSongRef.h"
class AmPipelineMatrixI;
class AmSong;

/**********************************************************************
 * AM-SONG-OBSERVER
 **********************************************************************/
class AmSongObserver
{
public:
	AmSongObserver( AmSongRef songRef );
	AmSongObserver( song_id songId );
	virtual ~AmSongObserver();

	const AmSong*	ReadLock() const;
	void			ReadUnlock(const AmSong* song) const;
	void			ReadUnlock(const AmPipelineMatrixI* matrix) const;
	AmSong*			WriteLock(const char* name = NULL);
	void			WriteUnlock(AmSong* song);
	void			WriteUnlock(AmPipelineMatrixI* matrix);

	AmSongRef		SongRef() const;
	void			SetSongRef( AmSongRef songRef );
	
private:
	mutable	AmSongRef		mSongRef;
			song_id			mSongId;
	mutable	int32			mNesting;
};

#endif 
