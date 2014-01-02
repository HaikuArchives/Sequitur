/* AmNotifier.h
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
 * 05.16.00		hackborn
 * Created this file
 */
#ifndef AMKERNEL_AMNOTIFIER_H
#define AMKERNEL_AMNOTIFIER_H

#include <map.h>
#include <vector.h>
#include <app/Handler.h>
#include <app/Messenger.h>
#include <support/Locker.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmRange.h"

// Forward references
class AmPhrase;
class AmPhraseEvent;
class _AmFilterEntry;
class _AmRangeEntry;
class _AmObserversEntry;
typedef map< uint32, _AmObserversEntry*, less<uint32> >	observers_map;
typedef vector<_AmFilterEntry>		notifier_filter_vec;

extern const char*	RANGE_ALL_EVENT_STR;

/***************************************************************************
 * AM-NOTIFIER
 * This class stores invalidation areas and a list of observers, and
 * notifiers its observers about the changed area.  It currently can store
 * invalidated AmRanges and invalidated pipelines.  It also provides a
 * generic change notification mechanism.
 ****************************************************************************/
class AmNotifier : public BHandler
{
public:
	AmNotifier();
	virtual ~AmNotifier();

	/* These are the events a client can register to hear about
	 * via the add observer methods.
	 */
	enum {
		NOTE_OBS				= 'Onot',
		CONTROL_CHANGE_OBS		= 'Ocnt',
		PITCH_OBS				= 'Opit',
		SIGNATURE_OBS			= 'Osig',
		TEMPO_OBS				= 'Otpo',
		OTHER_EVENT_OBS			= 'Ooth',
		/* Whenever a client registers as a RangeObserverAll, it
		 * will receive a RANGE_OBS message whose range is the
		 * combined range of all event types that have changed.  The
		 * mesage includes a series of int32 fields named RANGE_ALL_EVENT_STR
		 * which lists the specifical event types that have changed.
		 */
		RANGE_OBS				= 'Oral',
		
		PIPELINE_CHANGE_OBS		= 'Omfi',	// Changes to the structure of a pipeline
											// -- filters added, removed, etc.
		FILTER_CHANGE_OBS		= 'Ompi'	// Changes to an actual filter
	};
	/* Answer my observer message appropriate for the supplied event type.
	 */
	static uint32		CodeFor(AmEvent::EventType type);

	/* Register a handler to listen to a specific code or all codes, respectively.
	 */
	/* Register a handler for all changes to code within the supplied range.
	 */
	virtual status_t	AddRangeObserver(BHandler* handler, uint32 code, AmRange range);
	/* Register a handler for all changes, period, within the supplied range.
	 */
	virtual status_t	AddRangeObserverAll(BHandler* handler, AmRange range);
	/* Register a handler for all changes to code, regardless of range.
	 */
	virtual status_t	AddObserver(BHandler* handler, uint32 code);
	/* Completely remove the observer.
	 */
	virtual status_t	RemoveObserverAll(BHandler* handler);

	/* Report a change to the supplied range
	 */
	virtual void		ReportRangeChange(uint32 code, AmRange range, BMessenger sender);
	/* Report a generic change.  Ths variant simply sends the change out.  Whomever
	 * calls this should know enough to supply a change with a valid what.
	 */
	virtual void		ReportMsgChange(BMessage* msg, BMessenger sender);
	/* Report a generic change.  This variant calls AnnotateMsg() on the supplied
	 * message.
	 */
	virtual void		ReportChange(uint32 code, BMessenger sender);
	
	/* A change has been made to an event I am watching.  Merge it in with my
	 * other changes.  If an event is supplied, then the change notification
	 * can be optimized to that type of event.  If it isn't, everyone will
	 * receive notification.
	 */
	virtual void		MergeRangeChange(	AmRange oldRange, AmRange newRange,
											AmEvent* event = NULL);
	/* A change has been made to the structure of a pipeline I am watching --
	 * i.e. a filter has been added, removed, etc.  Merge the change
	 * in with my other changes.  In the current incarnation, all I track is
	 * the pipeline segments containing changed filters, I don't actually
	 * worry about knowing the filters themselves.
	 */
	virtual void		MergePipelineChange(pipeline_id id, AmPipelineType type);
	/* A change has been made to the state of a filter I am watching.
	 */
	virtual void		MergeFilterChange(pipeline_id id, AmPipelineType type);
	
	/* Invalidate all the ranges and pipeline segments that have been merged,
	 * and clear them out.
	 */
	virtual void		FlushChanges();

protected:
	/* Subclasses must implement to answer whatever phrase the event
	 * currently resides in (or should reside in).
	 */
	virtual AmPhrase*	PhraseFor(AmEvent* event) const			{ return NULL; }
	/* This is an optional place for subclasses to add any additional
	 * info before a message is sent out as a change notice.
	 */
	virtual void		AnnotateMessage(BMessage& msg)	const	{ }
	uint32				CodeForEvent(AmEvent::EventType type) const;

private:
	/* All calls to this notification mechanism are atomic, so each
	 * call has a simple lock around it.
	 */
	BLocker 			mLock; 
	/* Store a map of codes to observer entries.  Each entry has any number
	 * of range objects (a simple class that maps a handler to a range of
	 * time).
	 */
	observers_map		mObserversMap;
	/* A list of all the pipeline segments that have changed.
	 */
	notifier_filter_vec	mPipelines;
	/* A list of all the pipeline segments containing filters that have changed.
	 */
	notifier_filter_vec	mFilters;
	/* This is a special object that stores the handlers that are listening
	 * to all my codes.
	 */
	_AmObserversEntry*	mAllObserver;
	
	/* Answer the range at the supplied code and messenger.  If it doesn't
	 * exist, create it.
	 */
	_AmRangeEntry*		RangeEntryFor( BMessenger messenger, uint32 code );
	/* Answer the observers entry at the supplied code.  If it doesn't
	 * exist, create it.
	 */
	_AmObserversEntry*	ObserversEntryFor( uint32 code );

	/* Store the ranges for each event that comes through me.  Flush them
	 * out as change messages when the lock is done.
	 */
	AmRange		mNoteRange, mControlRange, mPitchRange, mSignatureRange,
				mTempoRange, mOtherRange;
};

#endif 
