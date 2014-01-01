/* AmEvents.h
 * These classes represent the various Midi Objects that can be
 * in the system, based on the 1.0 MMA Midi Specification.  (if
 * you don't have the spec you really should get it!)  All
 * events subclass from AmEvent, which gives a time for the
 * event and provides a common interface.
 */

#ifndef AMPUBLIC_AMEVENTS_H
#define AMPUBLIC_AMEVENTS_H

#include <Message.h>
#include <GraphicsDefs.h>
#include <Rect.h>
#include <BlockCache.h>
#include <Debug.h>
#include <SupportDefs.h>
#include <ArpBuild.h>
#include "ArpKernel/ArpString.h"
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmRange.h"
#include "AmPublic/AmSafeDelete.h"

// Forward reference.
class BView;
class AmEvent;
class AmFilterHolderI;
class AmPhrase;

/***************************************************************************
 * AM-EVENT-PARENT
 * This abstracts the parent interface solely so the AmTrack can be the
 * parent of all the events it contains.
 ***************************************************************************/
class AmEventParent
{
public:
	virtual AmEventParent*	Parent() const = 0;
	virtual void			SetParent(AmEventParent* parent) = 0;
	/* Whenever an event has a property changed, it will pass this method
	 * up through the parent hierarchy.  If oldStart == newStart and
	 * oldEnd == newEnd, then no time change occurred in the event.  Parents
	 * can use that to optimize the notification.
	 */
	virtual void			Invalidate(	AmEvent* changedEvent,
										AmRange oldRange, AmRange newRange) = 0;

	/* Anything that caches time information has just been informed that its
	 * cache is not valid.
	 */
	virtual void			TimeRangeDirty() = 0;
	
protected:
	AmEventParent();
};

/***************************************************************************
 * AM-EVENT
 * This class defines the abstract superclass for all midi events.
 ***************************************************************************/
#define STR_AMEVENT			"AmEvent"

class AmEvent : public AmSafeDelete
{
public:
	/* This is a hack for recording.  The last filter that process an
	 * event in the input pipeline sets the track ID, so the event knows
	 * which track to record into.  This would not be necessary if we
	 * had a 'full' design where the tracks are actually filters, and just
	 * record events as they receive them.
	 */
	track_id				trackId;

	event_id				Id() const;
	
	// --------------------------------------------------------
	// EVENT TYPES.
	// --------------------------------------------------------
	
	/* These are all of the types of events that are currently
	 * defined.  You can determine the type of a particular event
	 * object by calling Type() and checking its returned value
	 * against these codes.  To access information about a particular
	 * type of event, you will need to dynamic_cast<T>() this object
	 * into the corresponding subclass interface.
	 *
	 * For example, given some event object:
	 *    AmEvent* event;
	 *
	 * If this is a NOTEON_TYPE, you can get information about the
	 * note by doing the following:
	 *    AmNoteOn* myNode = dynamic_cast<AmNoteOn*>(event);
	 *
	 * Note that the ordering between these IS IMPORTANT.  In particular,
	 * we want note-off events to always be placed before note-on events,
	 * and various other events should appear before note-on so that they
	 * can set up state before a note is played.  This allows the user
	 * to, for example, place a program change at the same time as a note,
	 * and have it always apply to that note.
	 */
	enum EventType {
		NO_TYPE						= 0x00000001,
		NOTEOFF_TYPE				= 0x00000010,
		SYSTEMCOMMON_TYPE			= 0x00000020,
		SYSTEMEXCLUSIVE_TYPE		= 0x00000021,
		SYSTEMREALTIME_TYPE			= 0x00000022,
		PROGRAMCHANGE_TYPE			= 0x00000030,
		CONTROLCHANGE_TYPE			= 0x00000031,
		PITCHBEND_TYPE				= 0x00000032,
		NOTEON_TYPE					= 0x00000040,
		CHANNELPRESSURE_TYPE		= 0x00000050,
		KEYPRESSURE_TYPE			= 0x00000051,
		TEMPOCHANGE_TYPE			= 0x00000060,
		SONGPOSITION_TYPE			= 0x00000061,
		SIGNATURE_TYPE				= 0x00000070,
		MOTION_TYPE					= 0x00000072,
		LYRIC_TYPE					= 0x00000080,
		PHRASE_TYPE					= 0x00001000,
		TRANSPORT_TYPE				= 0x00002000,
		PERFORMER_TYPE				= 0x00003000,
		_NUM_TYPE
	};
	virtual EventType		Type() const = 0;

	enum EventSubtype {
		NO_SUBTYPE					= 0x00000001,
		ROOT_SUBTYPE				= 0x00000002,
		LINK_SUBTYPE				= 0x00000003,
		BANK_SUBTYPE				= 0x00000004,
		INNER_SUBTYPE				= 0x00000005,
		_NUM_SUBTYPE
	};
	virtual EventSubtype	Subtype() const;
	/* If your event represents a persistant state value (i.e.,
	 * a controller change), you should override this.  Return
	 * an identifier for the state of this event's type, starting
	 * at zero.  For example, a program change event would always
	 * return zero, since it represents one persistent state.
	 * A control change event would return a value from 0 to 127
	 * representing one of the different control change states
	 * that it modifiers.
	 */
	virtual int32			PersistentStateID() const;

	AmEventParent*			Parent() const;
	void					SetParent(AmEventParent* parent);
	virtual void			TimeRangeDirty();

	// --------------------------------------------------------
	// EVENT ATTRIBUTES.
	// --------------------------------------------------------
	
	virtual AmRange		TimeRange() const;
	/* Clients need to be knowledgeable about getting and setting
	 * the start time.  If the event exists in a phrase, then
	 * the get / set must always occur via the methods available
	 * on the top level phrase event that the event exists in.  If
	 * the event is not in a phrase (for example, it's being processed
	 * by a filter), then these methods are fine.
	 */
	virtual AmTime		StartTime() const;
	virtual void		SetStartTime(AmTime newTime);
	/* Interface for retrieving the duration of an event.  If the
	 * event doesn't implement this interface, its duration is
	 * always zero.
	 */
	virtual AmTime		EndTime() const;
	virtual AmTime		Duration() const;
	/* Clients need to be knowledgeable about getting and setting
	 * the end time.  If the event exists in a phrase, then
	 * the get / set must always occur via the methods available
	 * on the top level phrase event that the event exists in.  If
	 * the event is not in a phrase (for example, it's being processed
	 * by a filter), then these methods are fine.  These methods only
	 * apply to event types that have duration.
	 */
	virtual void		SetDuration(AmTime newDuration);
	virtual void		SetEndTime(AmTime newTime);
	/* When performing playback or applying filters to a sequence,
	 * this is the next filter object that this event will be piped
	 * through.  You can change this filter, if you would like, to
	 * change where the event is going.
	 */
	virtual AmFilterHolderI* NextFilter() const			{ return mFilter; }
	virtual void		SetNextFilter(AmFilterHolderI* next);
	// --------------------------------------------------------
	// USER INTERFACE.
	// --------------------------------------------------------
	/* These are the types of views that events should know how
	 * to construct.
	 *
	 * The inspector view should be one line of controls that
	 * expose for editing the numerical properties of the event.
	 */
	enum ViewType {
		INSPECTOR_VIEW,
		
		_NUM_VIEW
	};
	/* Answer a new view of the requested type and size.
	 */
	virtual BView*		NewView(ViewType type, BRect frame) const
														{ return 0; }

	/* Events are notified when they are added to or removed from a phrase.
	 * Events are currently in the phrase when they receive both these
	 * methods.  The default implementation does nothing.
	 */
	virtual void		AddedToPhrase();
	virtual void		RemovedFromPhrase();

	// --------------------------------------------------------
	// EVENT CHAINS.
	// --------------------------------------------------------
	void				Invalidate(	AmEvent* changedEvent,
									AmRange oldRange, AmRange newRange);
	/* Events can be chained together to perform operations on
	 * a set of events.  These event chains are used primarily
	 * during playback and filtering operations -- an event list
	 * is generated, run through a series of filters, and either
	 * sent to an output device or inserted back into a sequence.
	 */
	
	/* These functions return the next and previous event in a
	 * chain.  If this is the last event in a chain, NextEvent()
	 * returns NULL; if it is the first, PrevEvent() returns NULL.
	 */
	const AmEvent*		NextEvent() const				{ return mNext; }
	const AmEvent*		PrevEvent() const				{ return mPrev; }
	AmEvent*			NextEvent()						{ return mNext; }
	AmEvent*			PrevEvent()						{ return mPrev; }
	/* Read the head and tail event in a list.  Note that if the
	 * given event is in the middle of the list, this will require
	 * traversing up or down the list to find the end.
	 */
	const AmEvent*		HeadEvent() const;
	const AmEvent*		TailEvent() const;
	AmEvent*			HeadEvent();
	AmEvent*			TailEvent();
	/* Add and remove events from a chain:
	 *   AppendEvent() places 'event' immediately after 'this' one.
	 *   InsertEvent() places 'event' immediately before 'this' one.
	 *   RemoveEvent() removes 'this' event from its chain, returning
	 *     either the event before or after it, or NULL.
	 *     NOTE: This does NOT deallocate its memory!
	 *
	 * These functions do not try to keep your chain in time
	 * order; it is your responsibility to do so.
	 */
	void				AppendEvent(AmEvent* event);
	void				InsertEvent(AmEvent* event);
	AmEvent*			RemoveEvent();
	/* Return the relative ordering between two events.  If -1,
	 * 'this' is before 'other; if 1, 'other' is before 'this'.
	 * If 0, the ordering is indeterminant.  (0 does NOT mean that
	 * they are the same.)
	 */
	int32				QuickCompare(const AmEvent* other) const;
	
	/* Merge a single event into another list. The 'sourceEvent'
	 * is placed into 'this' event's chain so that 'this' is kept
	 * in time-sorted order.  Note that 'this' event's chain must already
	 * be in time-sorted order for this function to work.
	 */
	AmEvent*			MergeEvent(AmEvent* sourceEvent);
	/* Merge two event lists.  The events in the 'source' list are
	 * merged into 'this' list, and the combined list returned.
	 * The 'dest' and 'this' events can be any event in their list,
	 * although it is of course more efficient if 'source' is the first
	 * event and 'this' is some time close to the first 'source' time.
	 * The function returns some event in the resulting combined list;
	 * exactly which event this might be is not defined.
	 *
	 * The 'dest' list must be sorted in time order for this function
	 * to work.  While the 'source' list does -not- need to be sorted,
	 * you will of course get much better performance if it is.  And
	 * you can get even better performance by passing 'ordered' true
	 * if you know that the source list is ordered.
	 */
	AmEvent*			MergeList(AmEvent* sourceList, bool ordered=false);
	/* Split the current event chain at the given time.  The return
	 * value is either 'this' if there are events before 'time', or
	 * NULL if not.  The argument 'outTail' is filled in with the
	 * first event at or after 'time'.  The event chain is split
	 * at this point into two separate chains.
	 *
	 * The input event list must be in time order for this function
	 * to work.
	 */
	AmEvent*			CutForwardAtTime(const AmTime time, AmEvent** outTail);
	/* Like above, but look backwards from current position for events
	 * at or after 'time'.  Returns the first event found less than 'time'
	 * (after which the cut occurs) and 'outTail' is filled with 'this if
	 * there are any events >= 'time'.
	 */
	AmEvent*			CutBackwardAtTime(const AmTime time, AmEvent** outTail);
	/* Cut the chain between this event and the one before it.  Answer
	 * the preceding event.
	 */
	AmEvent*			CutBefore();
	/* These functions are needed to actually implement the
	 * above.  You should not normally need to call them.
	 */
	void				SetNextEvent(AmEvent* event)	{ mNext = event; }
	void				SetPrevEvent(AmEvent* event)	{ mPrev = event; }

	// --------------------------------------------------------
	// MEMORY MANAGEMENT AND UTILITIES.
	// --------------------------------------------------------
	
	/* Deallocate the memory for an entire chain of events.
	 * The event 'this' as well as any events connected to it
	 * are deallocated.
	 */
	virtual void		Delete();
	void				DeleteChain();
	virtual AmEvent*	Copy() const				= 0;
	AmEvent*			CopyChain() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	
	bool				AssertTimeOrder() const;
	
	virtual bool		Equals(AmEvent* event) const;
	
	enum {
		PRINT_ADDRESS	= 1<<0
	};
	virtual void		Print(void) const;
	void				PrintChain(uint32 flags = 0, const char* prefix = NULL) const;
	
	/* Set myself to the supplied event (only if our types match).
	 * If setTimes is false, don't set my start and end times (which
	 * typically needs to be done through the phrase containing the event).
	 * This simply sets MIDI event data, like velocities and what not,
	 * it ignores the event's parent, next and prev, etc.
	 */
	virtual void		SetTo(AmEvent* event, bool setTimes = false);

protected:
	AmEvent();
	AmEvent(AmTime);
	AmEvent(const AmEvent& o);
	AmEvent(const BMessage& flatEvent);
	virtual ~AmEvent();

	AmEvent& operator=(const AmEvent& o);
		
	void Initialize(AmTime);
		
	// Memory management
	static void* GetEvent(EventType type, size_t size);
	static void SaveEvent(EventType type, void* ev);
	
	AmEventParent*		mParent;
	AmTime				mTime;
	AmFilterHolderI*	mFilter;
	AmEvent*			mNext;
	AmEvent*			mPrev;
	/* This is a special case for when the event is being processed
	 * by a filter -- in that case, the filter might change the start
	 * or end time of the event.  If it does, the change gets noted
	 * by setting the NEEDS_SYNC_FLAG, so clients know that they need
	 * to rehash the event in the phrase that contains it.
	 */
	enum {
		NEEDS_SYNC_FLAG		= 0x00000001
	};
	uint32				mFlags;
	
	friend class AmPhrase;
	
private:
	virtual	void		_ReservedAmEvent1();
	virtual	void		_ReservedAmEvent2();
	virtual	void		_ReservedAmEvent3();
	virtual	void		_ReservedAmEvent4();
	virtual	void		_ReservedAmEvent5();
	virtual	void		_ReservedAmEvent6();
	virtual	void		_ReservedAmEvent7();
	virtual	void		_ReservedAmEvent8();
	virtual	void		_ReservedAmEvent9();
	virtual	void		_ReservedAmEvent10();
	virtual	void		_ReservedAmEvent11();
	virtual	void		_ReservedAmEvent12();
	virtual	void		_ReservedAmEvent13();
	virtual	void		_ReservedAmEvent14();
	virtual	void		_ReservedAmEvent15();
	virtual	void		_ReservedAmEvent16();
	size_t				_reserved_data[2];
	
	static AmEvent* mFreeList[AmEvent::_NUM_TYPE];
};

/***************************************************************************
 * AM-CHANNEL-PRESSURE
 ***************************************************************************/
class AmChannelPressure : public AmEvent
{
public:
	AmChannelPressure(uint8, AmTime);
	AmChannelPressure(const AmChannelPressure& o);
	
	uint8				Pressure() const;
	void				SetPressure(uint8);

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------		
	virtual EventType	Type() const 				{ return CHANNELPRESSURE_TYPE; }
	virtual int32		PersistentStateID() const	{ return 0; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual void		Print(void) const;
	
	void*			operator new(size_t size);
	void			operator delete(void* ptr, size_t size);

protected:
	virtual ~AmChannelPressure() { }
	AmChannelPressure& operator=(const AmChannelPressure& o);
	virtual void		RealDelete();
	
private:
	uint8	mPressure;
};

/***************************************************************************
 * AM-NOTE-ON
 ***************************************************************************/
class AmNoteOn : public AmEvent
{
public:
	AmNoteOn(uint8 noteArg, uint8 velocityArg, AmTime timeArg);
	AmNoteOn(const AmNoteOn& o);
	AmNoteOn(const BMessage& flatEvent);
	
	uint8				Note() const;
	void				SetNote(uint8);
	uint8				Velocity() const;
	void				SetVelocity(uint8);
	uint8				ReleaseVelocity() const;
	void				SetReleaseVelocity(uint8);
	virtual void		SetEndTime(AmTime newTime);
	
	bool				IsSharp() const;
	
	// A note-on that has a duration also implicitly defines a note-off.
	// AmNoteOn by default has a durection.  You will typically only
	// see AmNoteOn events without a duration when recordingg, in which
	// case the note duration isn't known at the time it is played.
	// When HasDuration() is false, Duration() always returns zero and
	// it can not be set.
	void				SetHasDuration(bool state);
	bool				HasDuration() const;
	
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const  { return NOTEON_TYPE; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual void		Print(void) const;
	
	virtual AmTime		EndTime() const;
	virtual AmTime		Duration() const;
	virtual void		SetDuration(AmTime);
	
	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);
	
protected:
	virtual ~AmNoteOn() { }
	AmNoteOn& operator=(const AmNoteOn& o);
	virtual void		RealDelete();
		
private:
	uint8	mNote, mVelocity, mRelVelocity;
	AmTime 	mDuration;
};

/***************************************************************************
 * AM-NOTE-OFF
 ***************************************************************************/
class AmNoteOff : public AmEvent
{
public:
	AmNoteOff(uint8 noteArg, uint8 velocityArg, AmTime timeArg);
	AmNoteOff(const AmNoteOff& o);
	
	uint8				Note() const;
	void				SetNote(uint8);
	uint8				Velocity() const;
	void				SetVelocity(uint8);
	
	bool 				IsSharp() const;
	
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const		{ return NOTEOFF_TYPE; };
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual void		Print(void) const;
	
	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmNoteOff() { }
	AmNoteOff& operator=(const AmNoteOff& o);
	virtual void		RealDelete();
		
private:
	uint8	mNote, mVelocity;
};

/***************************************************************************
 * AM-TEMPO-CHANGE
 ***************************************************************************/
#define AM_TEMPO_MIN		(1)
#define AM_TEMPO_MAX		(400)

class AmTempoChange : public AmEvent
{
public:
	AmTempoChange(float tempo, AmTime time);
	AmTempoChange(const AmTempoChange& o);
	
	float				Tempo() const;
	void				SetTempo(float tempo);
	
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const				{ return TEMPOCHANGE_TYPE; }
	virtual int32		PersistentStateID() const	{ return 0; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmTempoChange() 				{ }
	AmTempoChange& operator=(const AmTempoChange& o);
	virtual void		RealDelete();

private:
	float	mTempo;
};

/***************************************************************************
 * AM-CONTROL-CHANGE
 ***************************************************************************/
class AmControlChange : public AmEvent
{
public:
	AmControlChange(uint8 controlNumber,
					uint8 controlValue,
					AmTime time);
	AmControlChange(const AmControlChange& o);
	AmControlChange(const BMessage& flatEvent);

	uint8				ControlNumber() const;
	void				SetControlNumber(uint8 controlNumber);
	uint8				ControlValue() const;
	void				SetControlValue(uint8 controlValue);
	 
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const 				{ return CONTROLCHANGE_TYPE; }
	virtual int32		PersistentStateID() const	{ return mControlNumber; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmControlChange() { }
	AmControlChange& operator=(const AmControlChange& o);
	virtual void		RealDelete();

private:
	uint8	mControlNumber, mControlValue;
};

/***************************************************************************
 * AM-PITCH-BEND
 ***************************************************************************/
#define AM_PITCH_MIN		(-8192)
#define AM_PITCH_MAX		(8191)

class AmPitchBend : public AmEvent
{
public:
	AmPitchBend(int16 pitch, AmTime time);
	AmPitchBend(uint8 lsb, uint8 msb, AmTime time);
	AmPitchBend(const AmPitchBend& o);

	int16				Value() const;
	void				SetValue(int16 pitch);
	uint8				Lsb() const;
	void				SetLsb(uint8 lsb);
	uint8				Msb() const;
	void				SetMsb(uint8 msb);

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const 				{ return PITCHBEND_TYPE; }
	virtual int32		PersistentStateID() const	{ return 0; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmPitchBend()						{ }
	AmPitchBend& operator=(const AmPitchBend& o);
	virtual void		RealDelete();

private:
	uint8	mLsb, mMsb;
};

/***************************************************************************
 * AM-PROGRAM-CHANGE
 ***************************************************************************/
class AmProgramChange : public AmEvent
{
public:
	AmProgramChange();
	AmProgramChange(uint8 programNumber, AmTime time);
	AmProgramChange(const AmProgramChange& o);
	AmProgramChange(const BMessage& flatEvent);

	AmTime				EndTime() const;
	uint8				ProgramNumber() const;
	void				SetProgramNumber(uint8 pn);

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------	
	virtual EventType	Type() const 				{ return PROGRAMCHANGE_TYPE; }
	virtual int32		PersistentStateID() const	{ return 0; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmProgramChange() { }
	AmProgramChange& operator=(const AmProgramChange& o);
	virtual void		RealDelete();

private:
	uint8	mProgramNumber;
};

/***************************************************************************
 * AM-SIGNATURE
 ***************************************************************************/
class AmSignature : public AmEvent
{
public:
	AmSignature();
	AmSignature(AmTime time);
	AmSignature(AmTime time, int32 measure, uint32 beats, uint32 beatValue);
	AmSignature(const AmSignature& o);

	virtual ~AmSignature()							{ }
	
	virtual void		SetStartTime(AmTime newTime);
	AmTime				EndTime() const;
	virtual AmTime		Duration() const;

	int32				Measure() const;
	void				SetMeasure(int32 measure);
	uint32				Beats() const;
	void				SetBeats(uint32 beats);
	uint32				BeatValue() const;
	void				SetBeatValue(uint32 beatValue);
	void				Set( const AmSignature& sig );
	void				Set(AmTime time,
							uint32 beats,
							uint32 beatValue);
	void				Set(AmTime time,
							int32 measure,
							uint32 beats,
							uint32 beatValue);
	void				Set(AmTime time, int32 measure);

	AmTime				TicksPerBeat() const;
	/* Answer the beat, in ticks, which the supplied time is in.  If
	 * anything goes wrong, just return the supplied time.
	 */
	AmTime				BeatForTime(AmTime time) const;
	
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const 		 		{ return SIGNATURE_TYPE; }
	virtual int32		PersistentStateID() const	{ return 0; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	AmSignature& operator=(const AmSignature& o);
	virtual void		RealDelete();
		
private:
	int32		mMeasure;
	uint32		mBeats, mBeatValue;

	AmTime		mEndTime;		// This is a constructed value,
								// but we cache it for speed purposes.

	void CalculateEndTime();
};

/***************************************************************************
 * AM-MOTION-CHANGE
 ***************************************************************************/
class AmMotion;
class AmMotionI;

class AmMotionChange : public AmEvent
{
public:
	AmMotionChange(const AmMotionI* motion, int32 measure, AmTime time);
	AmMotionChange(const AmMotionChange& o);
	AmMotionChange(const BMessage& msg);

	int32				Measure() const;
	void				SetMeasure(int32 measure);
	bool				HasMotion() const;
		
	// --------------------------------------------------------
	// MOTION INTERFACE
	// --------------------------------------------------------
	BString				Label() const;
	uint32				CountHits() const;
	uint32				CountMeasures() const;
	status_t			GetHit(	uint32 number,
								BPoint* pt, float* end = NULL) const;
	status_t			GetHitY(float x, float* outY) const;
	bool				IsEmpty() const;
	AmMotionMode		EditingMode() const;
	void				SetMotion(const AmMotionI* motion);

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const 				{ return MOTION_TYPE; }
	virtual int32		PersistentStateID() const	{ return 0; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual BView*		NewView(ViewType type, BRect frame) const;
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

	/* This is a hack made use of by some internals.  Ignore it.
	 */
	track_id			TrackId() const;
	void				SetTrackId(track_id trackId);

protected:
	virtual ~AmMotionChange()						{ }
	AmMotionChange& operator=(const AmMotionChange& o);
	virtual void		RealDelete();

private:
	AmMotion*			mMotion;
	int32				mMeasure;
	track_id			mTrackId;
};

/***************************************************************************
 * AM-SYSTEM-EXCLUSIVE
 ***************************************************************************/
class AmSystemExclusive : public AmEvent
{
public:
	AmSystemExclusive(const uint8* data, size_t size, AmTime time);
	AmSystemExclusive(const AmSystemExclusive& o);

	const uint8*		Data() const			{ return (const uint8*)(const char*)mData; }
	size_t				Length() const			{ return mData.Length(); }
	void				SetData(const uint8* data, size_t length);
	
	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const			{ return SYSTEMEXCLUSIVE_TYPE; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);
	virtual bool		Equals(AmEvent* event) const;

	/* This is a hack addon for the sysex commands.  There SHOULD be
	 * subclass of sysex, the command, that handles this, but I didn't
	 * feel like hacking all that into the file IO, so I took the cheap
	 * way out.
	 */
	void				SetChannelBytes(int32 start, int32 end);
	void				GetChannelBytes(int32* start, int32* end) const;
	void				SetChannel(uchar channel);
	
protected:
	virtual ~AmSystemExclusive()					{ }
	AmSystemExclusive& operator=(const AmSystemExclusive& o);
	virtual void		RealDelete();
		
private:
	ArpString			mData;
	int32				mChannelStart, mChannelEnd;
};

/***************************************************************************
 * AM-SYSTEM-COMMON
 ***************************************************************************/
class AmSystemCommon : public AmEvent
{
public:
	AmSystemCommon(uint8 status, uint8 data1, uint8 data2, AmTime time);
	AmSystemCommon(const AmSystemCommon& o);

	uint8				Status() const			{ return mStatus; }
	void				SetStatus(uint8 status)	{ mStatus = status; }
	uint8				Data1() const			{ return mData1; }
	void				SetData1(uint8 data1)	{ mData1 = data1; }
	uint8				Data2() const			{ return mData2; }
	void				SetData2(uint8 data2)	{ mData2 = data2; }

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const			{ return SYSTEMCOMMON_TYPE; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual void		Print(void) const;

	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmSystemCommon()					{ }
	AmSystemCommon& operator=(const AmSystemCommon& o);
	virtual void		RealDelete();
		
private:
	uint8	mStatus, mData1, mData2;
};

/***************************************************************************
 * AM-SONG-POSITION
 ***************************************************************************/
class AmSongPosition : public AmSystemCommon
{
public:
	AmSongPosition();
	AmSongPosition(AmTime time);
	AmSongPosition(const AmSongPosition& o);

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------
	virtual EventType	Type() const	{ return SONGPOSITION_TYPE; }
	virtual AmEvent*	Copy() const;
	virtual void		Print(void) const;

	void*			operator new(size_t size);
	void			operator delete(void* ptr, size_t size);

protected:
	virtual ~AmSongPosition()			{ }
	AmSongPosition& operator=(const AmSongPosition& o);
	virtual void		RealDelete();
};

/***************************************************************************
 * AM-LYRIC
 ***************************************************************************/
class AmLyric : public AmEvent
{
public:
	AmLyric(const BString& lyric, AmTime time);
	AmLyric(const AmLyric& o);
	
	BString				Lyric() const;
	void				SetLyric(const BString& lyric);

	// --------------------------------------------------------
	// AM-EVENT INTERFACE
	// --------------------------------------------------------		
	virtual EventType	Type() const  { return LYRIC_TYPE; }
	virtual AmEvent*	Copy() const;
	virtual status_t	GetAsMessage(BMessage& msg) const;
	virtual bool		Equals(AmEvent* event) const;
	virtual void		SetTo(AmEvent* event, bool setTimes = false);
	virtual void		Print(void) const;
	
	void*				operator new(size_t size);
	void				operator delete(void* ptr, size_t size);

protected:
	virtual ~AmLyric();
	AmLyric&			operator=(const AmLyric& o);
	virtual void		RealDelete();
	
private:
	BString		mLyric;
};



// *****************************
// *****************************
// The rest are not implemented.
#if 0

/***************************************************************************
* class AmKeyPressure
***************************************************************************/

class AmKeyPressure : public AmEvent,
						   public AmKeyPressureI
{
	public:
		AmKeyPressure(uint8, uint8, AmTime);

		virtual EventType Type() const  { return KEYPRESSURE_TYPE; };
		uint8 Note();
		void SetNote(uint8);
		uint8 Pressure();
		void SetPressure(uint8);
		
		virtual bool IsKeyPressure() {return true; };
		
		virtual void Play(uint8, AmTime, BMidi*);

		// Utility functions
		virtual AmEvent* Copy();

	private:
		uint8	note, pressure;
};

#endif

/* This little tucked-away method is a convenience for turning an EventType
 * into a string.
 */
void string_for_event_type(	BString& answer, AmEvent::EventType type,
							AmEvent::EventSubtype subtype = AmEvent::NO_SUBTYPE,
							bool plural = false);
AmEvent* am_get_as_event(const BMessage& msg);

#endif
