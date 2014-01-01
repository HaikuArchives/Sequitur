/* AmSongRef.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * Created this file
 */
#ifndef AMPUBLIC_AMSONGREF_H
#define AMPUBLIC_AMSONGREF_H

#include <vector.h>
#include <be/support/SupportDefs.h>
#include <be/app/Messenger.h>
#include "AmPublic/AmRange.h"
class BHandler;
class BMessage;
class AmEvent;
class AmGlobalsImpl;
class AmPipelineMatrixI;
class AmPipelineMatrixRef;
class AmSong;
class AmSongObserver;
class AmTrackRef;
class AmTransport;

/***************************************************************************
 * AM-SONG-REF
 * This class represents a reference to a single song object.
 ****************************************************************************/
class AmSongRef
{
public:
	AmSongRef();
	AmSongRef(const AmSong* song);
	AmSongRef(const AmSongRef& ref);
	virtual ~AmSongRef();

	AmSongRef&		operator=(const AmSongRef& ref);
	/* Set this object to the supplied track impl and answer the
	 * result of IsValid().
	 */
	bool			SetTo(const AmSong* song);
	/* Answer true if this ref can create AmTrack objects for reading and
	 * writing.
	 */
	bool			IsValid() const;
	/* Answer a unique value for the song.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	song_id			SongId() const;
	
	/* LOCKING
	 */
	const AmSong*	ReadLock() const;
	void			ReadUnlock(const AmSong* song) const;
	void			ReadUnlock(const AmPipelineMatrixI* matrix) const;
	AmSong*			WriteLock(const char* name = NULL);
	void			WriteUnlock(AmSong* song);
	void			WriteUnlock(AmPipelineMatrixI* matrix);

	/* TRANSPORT CONTROL
	 */
	/* These must be called WITHOUT a read or write lock held
	 * on the song; the transport will read- or write-lock the
	 * song as needed.
	 */
	status_t		StartPlaying(AmTime startTime,
								 AmTime stopTime = -1) const;
	status_t		StartPlaying(const AmTrackRef& solo, AmTime startTime,
								 AmTime stopTime = -1) const;
	status_t		StartRecording(AmTime startTime,
								   AmTime stopTime = -1);
	status_t		StartRecording(const AmTrackRef& solo, AmTime startTime,
								   AmTime stopTime = -1);
	void			StopTransport() const;
	void			PanicStop() const;
	bool			IsPlaying() const;
	bool			IsRecording() const;
	AmTransport*	Transport() const;
	/* Pass a message to the window that owns this song.
	 * Added for MMC.
	 */
	status_t		WindowMessage(BMessage* msg);
	
	/* CHANGE NOTIFICATION
	 */
	/* Add the supplied handler as an observer to all events of type code that
	 * occur during the supplied start and end times.  See AmSong.h for a list
	 * of the available observer messages clients can subscribe to.
	 *
	 * Multiple calls to this method with the same handler and code will result in
	 * the previous range being replaced, not appended to.
	 */
	status_t		AddRangeObserver(BHandler* handler, uint32 code, AmRange range);
	status_t		AddRangeObserverAll(BHandler* handler, AmRange range);
	status_t		AddObserver(BHandler* handler, uint32 code);
	status_t		RemoveObserverAll(BHandler* handler);
	void			ReportMsgChange(BMessage* msg, BMessenger sender);
	void			ReportChange(uint32 code, BMessenger sender);
	
private:
	friend class AmGlobalsImpl;
	friend class AmPipelineMatrixRef;
	AmSong*			mSong;
};

typedef vector<AmSongRef>		songref_vec;

#endif 
