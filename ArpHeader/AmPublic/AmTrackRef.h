/* AmTrackRef.h
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
#ifndef AMKERNEL_AMTRACKREF_H
#define AMKERNEL_AMTRACKREF_H

#include <vector>
#include <support/SupportDefs.h>
#include <app/Messenger.h>
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmRange.h"
class BHandler;
class AmSong;
class AmTrack;
class AmTrackPtr;

/***************************************************************************
 * AM-TRACK-REF
 * This class represents a reference to a single track object.
 ****************************************************************************/
class AmTrackRef
{
public:
	AmTrackRef(const AmTrack* track = NULL);
	AmTrackRef(const AmTrackRef& ref);
	virtual ~AmTrackRef();

	AmTrackRef&		operator=(const AmTrackRef& ref);
	bool			operator==(const AmTrackRef& ref) const;
	bool			operator!=(const AmTrackRef& ref) const;
	
	bool			operator==(const AmTrackPtr& ref) const;
	bool			operator!=(const AmTrackPtr& ref) const;
	
	/* Set this object to the supplied track impl and answer the
	 * result of IsValid().
	 */
	bool			SetTo(const AmTrack* trackImpl);
	/* Answer true if this ref can create AmTrack objects for reading and
	 * writing.
	 */
	bool			IsValid() const;
	/* Answer a unique value for the track.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	track_id		TrackId() const;
	/* Quick access to transient record mode, without having to lock
	 * song.  (Note that this is out-of-date as soon as you get it.)
	 */
	uint32			RecordMode() const;
	
	/* CHANGE NOTIFICATION
	 */
	/* Add the supplied handler as an observer to all events of type code that
	 * occur during the supplied start and end times.  The handler will never
	 * receive any message with a what of the code supplied here -- that is more
	 * of a 'meta what.'  See the track for a list of what messages the handler
	 * might receive as a result of the what.
	 *
	 * Multiple calls to this method with the same handler and code will result in
	 * the previous range being replaced, not appended to.
	 */
	status_t		AddRangeObserver(BHandler* handler, uint32 code, AmRange range);
	status_t		AddRangeObserverAll(BHandler* handler, AmRange range);
	status_t		AddObserver(BHandler* handler, uint32 code);
	status_t		RemoveObserverAll(BHandler* handler);
	void			ReportChange(uint32 code, BMessenger sender);
	void			ReportMsgChange(BMessage* msg, BMessenger sender);

private:
	// Songs are allowed to directly access the track implementation,
	// because the song is what holds the actual lock.
	friend class	AmSong;
	AmTrack*		mTrack;
};

/***************************************************************************
 * AM-TRACK-PTR
 * This class represents a pointer to a single track object, without a reference.
 ****************************************************************************/
class AmTrackPtr
{
public:
	AmTrackPtr(const AmTrack* track = NULL);
	AmTrackPtr(const AmTrackPtr& ref);
	AmTrackPtr(const AmTrackRef& ref);
	virtual ~AmTrackPtr();

	AmTrackPtr&		operator=(const AmTrackPtr& ref);
	bool			operator==(const AmTrackPtr& ref) const;
	bool			operator!=(const AmTrackPtr& ref) const;
	
	AmTrackPtr&		operator=(const AmTrackRef& ref);
	bool			operator==(const AmTrackRef& ref) const;
	bool			operator!=(const AmTrackRef& ref) const;
	
	/* Set this object to the supplied track impl and answer the
	 * result of IsValid().
	 */
	bool			SetTo(const AmTrack* trackImpl);
	bool			SetTo(track_id id);
	/* Answer true if this ref can create AmTrack objects for reading and
	 * writing.
	 */
	bool			IsValid() const;
	/* Answer a unique value for the track.  If this ref isn't valid,
	 * this will answer with 0.
	 */
	track_id		TrackId() const;
	
private:
	// Songs are allowed to directly access the track implementation,
	// because the song is what holds the actual lock.
	friend class	AmSong;
	track_id		mId;
};

typedef std::vector<AmTrackRef>		trackref_vec;

#endif 
