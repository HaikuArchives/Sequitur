/* AmSongRef.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmPublic/AmSongRef.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTransport.h"
#include "ArpKernel/ArpDebug.h"

/*************************************************************************
 * AM-SONG-REF
 *************************************************************************/
AmSongRef::AmSongRef()
		: mSong(NULL)
{
}

AmSongRef::AmSongRef(const AmSong* song)
		: mSong(const_cast<AmSong*>(song))
{
	if ( mSong ) mSong->AddRef();
}

AmSongRef::AmSongRef(const AmSongRef& ref)
		: mSong(0)
{
	mSong = ref.mSong;
	if ( mSong ) mSong->AddRef();
}

AmSongRef::~AmSongRef()
{
	if ( mSong ) mSong->RemoveRef();
}

AmSongRef& AmSongRef::operator=(const AmSongRef &ref)
{
	SetTo( ref.mSong );
	return *this;
}

bool AmSongRef::SetTo(const AmSong* song)
{
	if ( mSong ) mSong->RemoveRef();
	mSong = const_cast<AmSong*>(song);
	if ( mSong ) mSong->AddRef();
	
	return IsValid();
}

bool AmSongRef::IsValid() const
{
	return mSong != 0;
}

song_id AmSongRef::SongId() const
{
	if ( !IsValid() ) return 0;
	return mSong->Id();
}

const AmSong* AmSongRef::ReadLock() const
{
	if( !mSong ) {
		return NULL;
	}
	if( !mSong->ReadLock() ) return NULL;
	return mSong;
}

void AmSongRef::ReadUnlock(const AmSong* song) const
{
	if( song ) {
		if( song != mSong ) debugger("Bad song returned to ReadUnlock()");
		else song->ReadUnlock();
	}
}

void AmSongRef::ReadUnlock(const AmPipelineMatrixI* matrix) const
{
	if (matrix) {
		if (matrix != mSong) debugger("Bad matrix returned to ReadUnlock()");
		else {
			const AmSong*	song = dynamic_cast<const AmSong*>(matrix);
			if (!song) debugger("Error casting matrix in ReadUnlock()");
			else song->ReadUnlock();
		}
	}
}

AmSong* AmSongRef::WriteLock(const char* name)
{
	if( !mSong ) {
		return 0;
	}
	if( !mSong->WriteLock(name) ) return NULL;
	return mSong;
}

void AmSongRef::WriteUnlock(AmSong* song)
{
	if( song ) {
		if( song != mSong ) debugger("Bad song returned to WriteUnlock()");
		else song->WriteUnlock();
	}
}

status_t AmSongRef::StartPlaying(AmTime startTime, AmTime stopTime) const
{
	if ( !IsValid() ) return B_NO_INIT;
	return mSong->DoStartPlaying(startTime, stopTime);
}

status_t AmSongRef::StartPlaying(const AmTrackRef& solo, AmTime startTime, AmTime stopTime) const
{
	if ( !IsValid() ) return B_NO_INIT;
	return mSong->DoStartPlaying(solo, startTime, stopTime);
}

status_t AmSongRef::StartRecording(AmTime startTime, AmTime stopTime)
{
	if ( !IsValid() ) return B_NO_INIT;
	return mSong->DoStartRecording(startTime, stopTime);
}

status_t AmSongRef::StartRecording(const AmTrackRef& solo, AmTime startTime, AmTime stopTime)
{
	if ( !IsValid() ) return B_NO_INIT;
	return mSong->DoStartRecording(solo, startTime, stopTime);
}

void AmSongRef::StopTransport() const
{
	if ( !IsValid() ) return;
	mSong->DoStopTransport(AmFilterI::TRANSPORT_CONTEXT);
}

void AmSongRef::PanicStop() const
{
	if ( !IsValid() ) return;
	mSong->DoStopTransport(AmFilterI::PANIC_CONTEXT);
}

bool AmSongRef::IsPlaying() const
{
	if( !IsValid() ) return false;
	return mSong->Transport().IsPlaying();
}

bool AmSongRef::IsRecording() const
{
	if( !IsValid() ) return false;
	return mSong->IsRecording();
}

AmTransport* AmSongRef::Transport() const
{
	if( !IsValid() ) return NULL;
	return &(mSong->Transport());
}

status_t AmSongRef::WindowMessage(BMessage* msg)
{
	if (!mSong) return B_ERROR;
	return mSong->WindowMessage(msg);
}

status_t AmSongRef::AddRangeObserver(BHandler* handler, uint32 code, AmRange range)
{
	if( !mSong ) return B_NO_INIT;
	return mSong->AddRangeObserver( handler, code, range );
}

status_t AmSongRef::AddRangeObserverAll(BHandler* handler, AmRange range)
{
	if( !mSong ) return B_NO_INIT;
	return mSong->AddRangeObserverAll( handler, range );
}

status_t AmSongRef::AddObserver(BHandler* handler, uint32 code)
{
	if( !mSong ) return B_NO_INIT;
	return mSong->AddObserver( handler, code );
}

status_t AmSongRef::RemoveObserverAll(BHandler* handler)
{
	if( !mSong ) return B_NO_INIT;
	return mSong->RemoveObserverAll( handler );
}

void AmSongRef::ReportMsgChange(BMessage* msg, BMessenger sender)
{
	if( !mSong ) return;
	return mSong->ReportMsgChange( msg, sender );
}

void AmSongRef::ReportChange(uint32 code, BMessenger sender)
{
	if( !mSong ) return;
	return mSong->ReportChange( code, sender );
}
