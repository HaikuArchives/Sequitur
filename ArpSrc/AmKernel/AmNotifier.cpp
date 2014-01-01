/* AmNotifier.cpp
 */
#define _BUILDING_AmKernel 1

#include "AmKernel/AmNotifier.h"

#include <vector.h>
#include <stdio.h>
#include <be/app/Message.h>
#include <be/support/Autolock.h>
#include "AmPublic/AmEvents.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpSafeDelivery.h"
#include "AmKernel/AmPhrase.h"

const char*		RANGE_ALL_EVENT_STR		= "ra_event";

/***************************************************************************
 * _AM-FILTER-ENTRY
 ****************************************************************************/
class _AmFilterEntry : public AmRange
{
public:
	_AmFilterEntry();
	_AmFilterEntry(pipeline_id id, AmPipelineType type);
	_AmFilterEntry(const _AmFilterEntry& o);

	_AmFilterEntry&	operator=(const _AmFilterEntry& o);

	bool			Matches(pipeline_id id) const;
	void			Merge(AmPipelineType type);
	status_t		AddTo(BMessage& msg) const;

private:
	pipeline_id		mId;
	bool			mTypes[_NUM_PIPELINE];
};

/***************************************************************************
 * _AM-RANGE-ENTRY
 * This class is a simple association of a handler with a range.
 ****************************************************************************/
class _AmRangeEntry : public AmRange
{
public:
	_AmRangeEntry(BMessenger messenger);
	
	BMessenger			mMessenger;
};
typedef vector<_AmRangeEntry>	range_vec;

/***************************************************************************
 * _AM-OBSERVERS-ENTRY
 * This class maintains a collection of range objects.
 ****************************************************************************/
class _AmObserversEntry
{
public:
	_AmObserversEntry();

	/* Remove all range entries with the supplied messenger.
	 */
	void RemoveObserver(BMessenger messenger);

	/* Answer the range at the supplied messenger.  If one doesn't exist,
	 * create a new one and answer that.
	 */
	_AmRangeEntry*	RangeEntryFor( BMessenger messenger );
	/* Send out notices to all my messengers about the area change.
	 */
	void			ReportChange(uint32 code, AmRange range, BMessenger sender);
	/* This is a special method just for when sending out an all notices
	 * change.  It's remotely possible I'll fold the previous ReportChange()
	 * method into this one if I go to ONLY having the all message -- it
	 * certainly would provide a huge potential performance increase, but
	 * it would also be somewhat dangerous for change-receiving clients.
	 * (dangerous in the sense that I'd need to make sure I got them all
	 * updated)
	 */
	void			ReportAllChange(BMessage* msg, AmRange range, BMessenger sender,
									bool includesSignature);
	/* Send out a general change notice.
	 */
	void			ReportChange(BMessage* msg, BMessenger sender);

private:
	range_vec		mRanges;

	bool do_RemoveObserver(BMessenger messenger);
};

/***************************************************************************
 * AM-NOTIFIER
 ****************************************************************************/
AmNotifier::AmNotifier()
		: mAllObserver(0)
{
}

AmNotifier::~AmNotifier()
{
	observers_map::iterator i;
	for (i = mObserversMap.begin(); i != mObserversMap.end(); i++) {
		if ( (*i).second != 0 ) delete (*i).second;
	}
	mObserversMap.clear();

	delete mAllObserver;
}

uint32 AmNotifier::CodeFor(AmEvent::EventType type)
{
	if( type == AmEvent::NOTEON_TYPE )			return NOTE_OBS;
	if( type == AmEvent::NOTEOFF_TYPE )			return NOTE_OBS;
	if( type == AmEvent::CONTROLCHANGE_TYPE )	return CONTROL_CHANGE_OBS;
	if( type == AmEvent::PITCHBEND_TYPE )		return PITCH_OBS;
	if( type == AmEvent::SIGNATURE_TYPE )		return SIGNATURE_OBS;
	if( type == AmEvent::TEMPOCHANGE_TYPE )	return TEMPO_OBS;
	return OTHER_EVENT_OBS;
}

status_t AmNotifier::AddRangeObserver(BHandler* handler, uint32 code, AmRange range)
{
	BAutolock l(&mLock);
	_AmRangeEntry*		entry = RangeEntryFor( BMessenger(handler), code );
	if( !entry ) return B_NO_MEMORY;
	entry->Set( range.start, range.end );
	return B_OK;
}

status_t AmNotifier::AddRangeObserverAll(BHandler* handler, AmRange range)
{
	BAutolock l(&mLock);
	if( !mAllObserver ) mAllObserver = new _AmObserversEntry();
	if( !mAllObserver ) return B_NO_MEMORY;

	_AmRangeEntry*		entry = mAllObserver->RangeEntryFor( BMessenger(handler) );
	if( !entry ) return B_NO_MEMORY;
	entry->Set( range.start, range.end );
	return B_OK;
}

status_t AmNotifier::AddObserver(BHandler* handler, uint32 code)
{
	BAutolock l(&mLock);
	_AmRangeEntry*		entry = RangeEntryFor(BMessenger(handler), code);
	if (!entry) return B_NO_MEMORY;
	return B_OK;
}

status_t AmNotifier::RemoveObserverAll(BHandler* handler)
{
	BAutolock l(&mLock);
	BMessenger	messenger(handler);
	
	observers_map::iterator i;
	for (i = mObserversMap.begin(); i != mObserversMap.end(); i++) {
		if ( (*i).second != 0 ) (*i).second->RemoveObserver(messenger);
	}
	if (mAllObserver) mAllObserver->RemoveObserver(messenger);
	return B_OK;
}

void AmNotifier::ReportRangeChange(uint32 code, AmRange range, BMessenger sender)
{
	BAutolock l(&mLock);
	_AmObserversEntry*	obs = ObserversEntryFor(code);
	if (obs) obs->ReportChange(code, range, sender);

	if (mAllObserver) {
		BMessage		msg(RANGE_OBS);
		msg.AddInt32(RANGE_ALL_EVENT_STR, code);
		mAllObserver->ReportAllChange(&msg, range, sender, code == SIGNATURE_OBS);
	}
}

void AmNotifier::ReportMsgChange(BMessage* msg, BMessenger sender)
{
	ArpASSERT(msg);
	BAutolock l(&mLock);
	_AmObserversEntry*	obs = ObserversEntryFor(msg->what);
	if (obs) obs->ReportChange(msg, sender);
	if (mAllObserver) mAllObserver->ReportChange(msg, sender);
}

void AmNotifier::ReportChange(uint32 code, BMessenger sender)
{
	BMessage	msg(code);
	AnnotateMessage(msg);

	BAutolock l(&mLock);
	_AmObserversEntry*	obs = ObserversEntryFor(code);
	if (obs) obs->ReportChange(&msg, sender);
	if (mAllObserver) mAllObserver->ReportChange(&msg, sender);
}

void AmNotifier::MergeRangeChange(	AmRange oldRange, AmRange newRange,
									AmEvent* event)
{
	uint32		code = NOTE_OBS;
	if (event) code = CodeForEvent( event->Type() );
	AmRange		range = oldRange + newRange;
	if (!event || code == NOTE_OBS)				mNoteRange += range;
	if (!event || code == CONTROL_CHANGE_OBS)	mControlRange += range;
	if (!event || code == PITCH_OBS)			mPitchRange += range;
	if (!event || code == SIGNATURE_OBS)		mSignatureRange += range;
	if (!event || code == TEMPO_OBS)			mTempoRange += range;
	if (!event || code == OTHER_EVENT_OBS)		mOtherRange += range;
}

void AmNotifier::MergePipelineChange(pipeline_id id, AmPipelineType type)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (mPipelines[k].Matches(id) ) {
			mPipelines[k].Merge(type);
			return;
		}
	}
	mPipelines.push_back( _AmFilterEntry(id, type) );
}

void AmNotifier::MergeFilterChange(pipeline_id id, AmPipelineType type)
{
	for (uint32 k = 0; k < mFilters.size(); k++) {
		if (mFilters[k].Matches(id) ) {
			mFilters[k].Merge(type);
			return;
		}
	}
	mFilters.push_back( _AmFilterEntry(id, type) );
}

void AmNotifier::FlushChanges()
{
	{
		AmRange			allRange;
		BAutolock		l(&mLock);
		BMessage		msg(RANGE_OBS);
		bool			includesSignature = false;
		if (mNoteRange.IsValid() )	{
			_AmObserversEntry*	obs = ObserversEntryFor(NOTE_OBS);
			if (obs) obs->ReportChange(NOTE_OBS, mNoteRange, BMessenger() );
			allRange += mNoteRange;
			msg.AddInt32(RANGE_ALL_EVENT_STR, NOTE_OBS);
		}
		if (mControlRange.IsValid() ) {
			_AmObserversEntry*	obs = ObserversEntryFor(CONTROL_CHANGE_OBS);
			if (obs) obs->ReportChange(CONTROL_CHANGE_OBS, mControlRange, BMessenger() );
			allRange += mControlRange;
			msg.AddInt32(RANGE_ALL_EVENT_STR, CONTROL_CHANGE_OBS);
		}
		if (mPitchRange.IsValid() )	{
			_AmObserversEntry*	obs = ObserversEntryFor(PITCH_OBS);
			if (obs) obs->ReportChange(PITCH_OBS, mPitchRange, BMessenger() );
			allRange += mPitchRange;
			msg.AddInt32(RANGE_ALL_EVENT_STR, PITCH_OBS);
		}
		if (mSignatureRange.IsValid() ) {
			_AmObserversEntry*	obs = ObserversEntryFor(SIGNATURE_OBS);
			if (obs) obs->ReportChange(SIGNATURE_OBS, mSignatureRange, BMessenger() );
			allRange += mSignatureRange;
			msg.AddInt32(RANGE_ALL_EVENT_STR, SIGNATURE_OBS);
			includesSignature = true;
		}
		if (mTempoRange.IsValid() ) {
			_AmObserversEntry*	obs = ObserversEntryFor(TEMPO_OBS);
			if (obs) obs->ReportChange(TEMPO_OBS, mTempoRange, BMessenger() );
			allRange += mTempoRange;
			msg.AddInt32(RANGE_ALL_EVENT_STR, TEMPO_OBS);
		}
		if (mOtherRange.IsValid() ) {
			_AmObserversEntry*	obs = ObserversEntryFor(OTHER_EVENT_OBS);
			if (obs) obs->ReportChange(OTHER_EVENT_OBS, mOtherRange, BMessenger() );
			allRange += mOtherRange;
			msg.AddInt32(RANGE_ALL_EVENT_STR, OTHER_EVENT_OBS);
		}

		if (allRange.IsValid() && mAllObserver)
			mAllObserver->ReportAllChange(&msg, allRange, BMessenger(), includesSignature);
	}

	if (mPipelines.size() > 0) {
		BMessage		msg(PIPELINE_CHANGE_OBS);
		for (uint32 k = 0; k < mPipelines.size(); k++) mPipelines[k].AddTo(msg);
		AnnotateMessage(msg);
		ReportMsgChange(&msg, BMessenger() );
		mPipelines.resize(0);
	}
	if (mFilters.size() > 0) {
		BMessage		msg(FILTER_CHANGE_OBS);
		for (uint32 k = 0; k < mFilters.size(); k++) mFilters[k].AddTo(msg);
		AnnotateMessage(msg);
		ReportMsgChange(&msg, BMessenger() );
		mFilters.resize(0);
	}

	mNoteRange.MakeInvalid();
	mControlRange.MakeInvalid();
	mPitchRange.MakeInvalid();
	mSignatureRange.MakeInvalid();
	mTempoRange.MakeInvalid();
	mOtherRange.MakeInvalid();
}

uint32 AmNotifier::CodeForEvent(AmEvent::EventType type) const
{
	if( type == AmEvent::NOTEON_TYPE )			return NOTE_OBS;
	if( type == AmEvent::NOTEOFF_TYPE )			return NOTE_OBS;
	if( type == AmEvent::CONTROLCHANGE_TYPE )	return CONTROL_CHANGE_OBS;
	if( type == AmEvent::PITCHBEND_TYPE )		return PITCH_OBS;
	if( type == AmEvent::SIGNATURE_TYPE )		return SIGNATURE_OBS;
	if( type == AmEvent::TEMPOCHANGE_TYPE )		return TEMPO_OBS;
	return OTHER_EVENT_OBS;
}

_AmRangeEntry* AmNotifier::RangeEntryFor(BMessenger messenger, uint32 code)
{
	_AmObserversEntry*	obs = ObserversEntryFor( code );
	if( !obs ) return 0;

	return obs->RangeEntryFor( messenger );
}

_AmObserversEntry* AmNotifier::ObserversEntryFor( uint32 code )
{
	observers_map::const_iterator	i = mObserversMap.find( code );
	if( i != mObserversMap.end() ) return (*i).second;

	// The observers entry at the requested code doesn't exist.  Create a new one.
	_AmObserversEntry*	entry = new _AmObserversEntry();
	if( !entry ) return 0;
	
	pair<observers_map::iterator, bool>	p;
	p = mObserversMap.insert( observers_map::value_type( code, entry ) );
	
	return entry;
}

/***************************************************************************
 * _AM-FILTER-ENTRY
 ****************************************************************************/
_AmFilterEntry::_AmFilterEntry()
		: mId(0)
{
	for (uint32 k = 0; k < _NUM_PIPELINE; k++) mTypes[k] = false;
}

_AmFilterEntry::_AmFilterEntry(pipeline_id id, AmPipelineType type)
		: mId(id)
{
	for (uint32 k = 0; k < _NUM_PIPELINE; k++) mTypes[k] = false;
	Merge(type);
}

_AmFilterEntry::_AmFilterEntry(const _AmFilterEntry& o)
		: mId(o.mId)
{
	for (uint32 k = 0; k < _NUM_PIPELINE; k++) mTypes[k] = o.mTypes[k];
}

_AmFilterEntry& _AmFilterEntry::operator=(const _AmFilterEntry& o)
{
	mId = o.mId;
	for (uint32 k = 0; k < _NUM_PIPELINE; k++) mTypes[k] = o.mTypes[k];
	return *this;
}

bool _AmFilterEntry::Matches(pipeline_id id) const
{
	return mId == id;
}

void _AmFilterEntry::Merge(AmPipelineType type)
{
	if (type < _NUM_PIPELINE) mTypes[type] = true;
}

status_t _AmFilterEntry::AddTo(BMessage& msg) const
{
	ArpASSERT(mId);
	if (!mId) return B_ERROR;
	msg.AddPointer("pipeline_id", mId);
	for (int32 k = 0; k < _NUM_PIPELINE; k++)
		if (mTypes[k]) msg.AddInt32("pipeline_type", k);
	return B_OK;
}

/***************************************************************************
 * AM-RANGE-ENTRY
 ****************************************************************************/
_AmRangeEntry::_AmRangeEntry(BMessenger messenger)
		: mMessenger(messenger)
{
}

/***************************************************************************
 * AM-OBSERVERS-ENTRY
 ****************************************************************************/
_AmObserversEntry::_AmObserversEntry()
{
}

void _AmObserversEntry::RemoveObserver(BMessenger messenger)
{
	while( do_RemoveObserver( messenger ) ) ;
}

_AmRangeEntry* _AmObserversEntry::RangeEntryFor( BMessenger messenger )
{
// FIX:  Sometimes messengers are becoming invalid, I don't know why and I'm not sure
// what to do about that.
	uint32		size = mRanges.size();
	for( uint32 k = 0; k < size; k++ ) {
//printf("\t\tmatching %ld, is it valid? %d\n", k, mRanges[k].mMessenger.IsValid() );
		if( mRanges[k].mMessenger == messenger ) {
//printf("\tmessenger matched at %ld\n", k);
			return &(mRanges[k]);
		}
	}
	mRanges.push_back( _AmRangeEntry( messenger ) );
	if( !(mRanges.size() > size) ) return 0;
//printf("\tADDED A RANGE ENTRY, now size is %ld\n", mRanges.size());
	return &(mRanges[size]);
}

void _AmObserversEntry::ReportChange(uint32 code, AmRange range, BMessenger sender)
{
	BMessage	msg(code);
	add_time(msg, "start_time", range.start );
	add_time(msg, "end_time", range.end );
	for (uint32 k = 0; k < mRanges.size(); k++) {
		if (mRanges[k].mMessenger.IsValid()
				&& (mRanges[k].mMessenger != sender)
				&& mRanges[k].Overlaps(range) ) {
			SafeSendMessage(mRanges[k].mMessenger, &msg);
		/* Special case for the signatures -- we don't care whether or
		 * not they overlap, just send out a notice.
		 */
		} else if (mRanges[k].mMessenger.IsValid()
				&& (mRanges[k].mMessenger != sender)
				&& code == AmNotifier::SIGNATURE_OBS) {
			SafeSendMessage(mRanges[k].mMessenger, &msg);
		}
	}
}

void _AmObserversEntry::ReportAllChange(BMessage* msg, AmRange range, BMessenger sender,
										bool includesSignature)
{
	ArpASSERT(msg);
	add_time(*msg, "start_time", range.start);
	add_time(*msg, "end_time", range.end);
	for (uint32 k = 0; k < mRanges.size(); k++) {
		if (mRanges[k].mMessenger.IsValid()
				&& (mRanges[k].mMessenger != sender)
				&& mRanges[k].Overlaps(range) ) {
			SafeSendMessage(mRanges[k].mMessenger, msg);
		/* Special case for the signatures -- we don't care whether or
		 * not they overlap, just send out a notice.
		 */
		} else if (mRanges[k].mMessenger.IsValid()
				&& (mRanges[k].mMessenger != sender)
				&& includesSignature) {
			SafeSendMessage(mRanges[k].mMessenger, msg);
		}
	}
}

void _AmObserversEntry::ReportChange(BMessage* msg, BMessenger sender)
{
	ArpASSERT(msg);
	for (uint32 k = 0; k < mRanges.size(); k++) {
		if (mRanges[k].mMessenger.IsValid()
				&& (mRanges[k].mMessenger != sender) ) {
			SafeSendMessage(mRanges[k].mMessenger, msg);
		}
	}
}

bool _AmObserversEntry::do_RemoveObserver(BMessenger messenger)
{
	range_vec::iterator		i;
	for (i = mRanges.begin(); i != mRanges.end(); i++) {
		if ( (*i).mMessenger == messenger ) {
			mRanges.erase( i );
			return true;
		}
	}
	return false;
}
