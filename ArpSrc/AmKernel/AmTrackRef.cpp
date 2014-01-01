/* AmTrackRef.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include "AmPublic/AmTrackRef.h"
#include "AmKernel/AmTrack.h"
#include "ArpKernel/ArpDebug.h"

/*************************************************************************
 * AM-TRACK-REF
 *************************************************************************/
AmTrackRef::AmTrackRef(const AmTrack* track)
		: mTrack(const_cast<AmTrack*>(track))
{
	if ( mTrack ) mTrack->AddRef();
}

AmTrackRef::AmTrackRef(const AmTrackRef& ref)
		: mTrack(ref.mTrack)
{
	if ( mTrack ) mTrack->AddRef();
}

AmTrackRef::~AmTrackRef()
{
	if ( mTrack ) mTrack->RemoveRef();
}

AmTrackRef& AmTrackRef::operator=(const AmTrackRef &ref)
{
	SetTo( ref.mTrack );
	return *this;
}

bool AmTrackRef::operator==(const AmTrackRef& ref) const
{
	return mTrack == ref.mTrack;
}

bool AmTrackRef::operator!=(const AmTrackRef& ref) const
{
	return mTrack != ref.mTrack;
}

bool AmTrackRef::operator==(const AmTrackPtr& ref) const
{
	return TrackId() == ref.TrackId();
}

bool AmTrackRef::operator!=(const AmTrackPtr& ref) const
{
	return TrackId() != ref.TrackId();
}

bool AmTrackRef::SetTo(const AmTrack* track)
{
	if ( mTrack ) mTrack->RemoveRef();
	mTrack = const_cast<AmTrack*>( track );
	if ( mTrack ) mTrack->AddRef();
	
	return IsValid();
}

bool AmTrackRef::IsValid() const
{
	return mTrack != 0;
}

track_id AmTrackRef::TrackId() const
{
	if ( !IsValid() ) return 0;
	return mTrack->Id();
}

uint32 AmTrackRef::RecordMode() const
{
	if ( !IsValid() ) return AmTrack::RECORD_OFF_MODE;
	return mTrack->RecordMode();
}

status_t AmTrackRef::AddRangeObserver(BHandler* handler, uint32 code, AmRange range)
{
	if( !mTrack ) return B_NO_INIT;
	return mTrack->AddRangeObserver( handler, code, range );
}

status_t AmTrackRef::AddRangeObserverAll(BHandler* handler, AmRange range)
{
	if( !mTrack ) return B_NO_INIT;
	return mTrack->AddRangeObserverAll( handler, range );
}

status_t AmTrackRef::AddObserver(BHandler* handler, uint32 code)
{
	if( !mTrack ) return B_NO_INIT;
	return mTrack->AddObserver( handler, code );
}

status_t AmTrackRef::RemoveObserverAll(BHandler* handler)
{
	if( !mTrack ) return B_NO_INIT;
	return mTrack->RemoveObserverAll( handler );
}

void AmTrackRef::ReportChange(uint32 code, BMessenger sender)
{
	if( !mTrack ) return;
	return mTrack->ReportChange( code, sender );
}

void AmTrackRef::ReportMsgChange(BMessage* msg, BMessenger sender)
{
	if( !mTrack ) return;
	return mTrack->ReportMsgChange( msg, sender );
}

/*************************************************************************
 * AM-TRACK-PTR
 *************************************************************************/
AmTrackPtr::AmTrackPtr(const AmTrack* track)
		: mId(track ? track->Id() : 0)
{
}

AmTrackPtr::AmTrackPtr(const AmTrackPtr& ref)
		: mId(ref.mId)
{
}

AmTrackPtr::AmTrackPtr(const AmTrackRef& ref)
		: mId(ref.TrackId())
{
}

AmTrackPtr::~AmTrackPtr()
{
}

AmTrackPtr& AmTrackPtr::operator=(const AmTrackPtr &ref)
{
	SetTo( ref.mId );
	return *this;
}

bool AmTrackPtr::operator==(const AmTrackPtr& ref) const
{
	return mId == ref.mId;
}

bool AmTrackPtr::operator!=(const AmTrackPtr& ref) const
{
	return mId != ref.mId;
}

AmTrackPtr& AmTrackPtr::operator=(const AmTrackRef &ref)
{
	SetTo( ref.TrackId() );
	return *this;
}

bool AmTrackPtr::operator==(const AmTrackRef& ref) const
{
	return mId == ref.TrackId();
}

bool AmTrackPtr::operator!=(const AmTrackRef& ref) const
{
	return mId != ref.TrackId();
}

bool AmTrackPtr::SetTo(const AmTrack* track)
{
	mId = (track ? track->Id() : 0);
	return IsValid();
}

bool AmTrackPtr::SetTo(track_id id)
{
	mId = id;
	return IsValid();
}

bool AmTrackPtr::IsValid() const
{
	return mId != 0;
}

track_id AmTrackPtr::TrackId() const
{
	return mId;
}

