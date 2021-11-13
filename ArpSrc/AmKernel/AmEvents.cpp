/* AmEvents.cpp
 */
#define _BUILDING_AmKernel 1

#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmFilterI.h"
#include "AmKernel/AmMotion.h"
#include "AmKernel/AmPhrase.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmEventInspectors.h"

#include <MidiDefs.h>

#if defined(ArpDEBUG)
static int32 counter = 0;
#endif

#define NOISY 0


/***************************************************************************
 * AM-EVENT-PARENT
 * This abstracts the parent interface solely so the AmTrack can be the
 * parent of all the events it contains.
 ***************************************************************************/
AmEventParent::AmEventParent()
{
}

/***************************************************************
 * AM-EVENT
 ***************************************************************/
event_id AmEvent::Id() const
{
	return (void*)this;
}

#if 0
	/* The event may or may not have a track ID set.
	 */
	track_id	TrackId() const;
	void		SetTrackId(track_id tid);

track_id AmEvent::TrackId() const
{
	return mTrackId;
}

void AmEvent::SetTrackId(track_id tid)
{
	mTrackId = tid;
}
#endif

AmEvent::EventSubtype AmEvent::Subtype() const
{
	return NO_SUBTYPE;
}

int32 AmEvent::PersistentStateID() const
{
	return -1;
}

AmEventParent* AmEvent::Parent() const
{
	return mParent;
}

void AmEvent::SetParent(AmEventParent* parent)
{
	mParent = parent;
}

void AmEvent::TimeRangeDirty()
{
}

AmRange AmEvent::TimeRange() const
{
	return AmRange( StartTime(), EndTime() );
}

AmTime AmEvent::StartTime() const
{
	return mTime;
}

void AmEvent::SetStartTime(AmTime newTime)
{
	/* Actually, sometimes the time can go below
	 * zero -- for example, when adding a note to
	 * a track that has an unquantize in the output
	 * pipeline.  But sometimes it's illegal, like
	 * adding an event to a phrase, so this check
	 * remains.
	 */
	ASSERT(newTime >= 0);
	if (mTime == newTime) return;
	mTime = newTime;
	if (mParent) {
		mFlags |= NEEDS_SYNC_FLAG;
		mParent->TimeRangeDirty();
	}
}

AmTime AmEvent::EndTime() const
{
	return mTime;
}

AmTime AmEvent::Duration() const
{
	return EndTime() - StartTime();
}

void AmEvent::SetDuration(AmTime newDuration)
{
}

void AmEvent::SetEndTime(AmTime newTime)
{
}

status_t AmEvent::GetAsMessage(BMessage& msg) const
{
	msg.what = 'evnt';
	msg.AddInt32( "type", Type() );
	msg.AddInt32( "subtype", Subtype() );
	add_time(msg, "start", StartTime() );
	msg.AddPointer( "event_id", Id() );
	return B_OK;
}

AmEvent::AmEvent()
		: trackId(0), mParent(NULL)
{
	// debug...
#if defined(ArpDEBUG)
	int oldVal = atomic_add(&counter, 1);
	ArpDL(__FILE__, 3, cdb << ADH << "creating event #" << oldVal << std::endl);
#endif
	Initialize(0);
}

AmEvent::AmEvent(AmTime timeArg)
		: trackId(0), mParent(NULL)
{
	// debug...
#if defined(ArpDEBUG)
	int oldVal = atomic_add(&counter, 1);
	ArpDL(__FILE__, 3, cdb << ADH << "creating event #" << oldVal << std::endl);
#endif
	Initialize(timeArg);
}

AmEvent::AmEvent(const AmEvent& o)
	: trackId(o.trackId), mParent(NULL), mTime(o.mTime), mFilter(o.mFilter),
	  mNext(o.mNext), mPrev(o.mPrev), mFlags(0)
{
	ASSERT(mTime >= 0);
	
	if (mFilter) mFilter->IncRefs();
	
	// debug...
#if defined(ArpDEBUG)
	int oldVal = atomic_add(&counter, 1);
	ArpDL(__FILE__, 3, cdb << ADH << "creating event #" << oldVal << std::endl);
#endif
}

AmEvent::AmEvent(const BMessage& flatEvent)
		: trackId(0), mParent(NULL), mTime(0), mFilter(NULL),
		  mNext(NULL), mPrev(NULL), mFlags(0)
{
	int32	start;
	if (flatEvent.FindInt32( "start", &start) == B_OK)
		mTime = start;

	ASSERT(mTime >= 0);
	
	// debug...
#if defined(ArpDEBUG)
	int oldVal = atomic_add(&counter, 1);
	ArpDL(__FILE__, 3, cdb << ADH << "creating event #" << oldVal << std::endl);
#endif
}

AmEvent::~AmEvent() {
	if (mNext || mPrev) {
		Print();
		debugger("AmEvent destructor called while still in chain");
	}
	if (mFilter) mFilter->DecRefs();
	mFilter = NULL;
	
	// debug...
#if defined(ArpDEBUG)
	int last;
	if ( (last=atomic_add(&counter,-1)) == 1 ) {
//		ArpDL(__FILE__, 2, cdb << ADH << "deleted FINAL event #"
//								<< last-1 << " time: " << time << std::endl);
	} else {
//		ArpDL(__FILE__, 3, cdb << ADH << "deleted event #"
//								<< last-1 << " time: " << time << std::endl);
	}
#endif
}

void AmEvent::SetNextFilter(AmFilterHolderI* next)
{
	if (next) next->IncRefs();
	if (mFilter) mFilter->DecRefs();
	mFilter = next;
}

AmFilterHolderI* AmEvent::NextFilter() const
{
	return mFilter;
}

BView* AmEvent::NewView(ViewType type, BRect frame) const
{
	return NULL;
}

void AmEvent::AddedToPhrase()
{
}

void AmEvent::RemovedFromPhrase()
{
}

void AmEvent::Invalidate(	AmEvent* changedEvent,
							AmRange oldRange, AmRange newRange)
{
	if (mParent) mParent->Invalidate(changedEvent, oldRange, newRange);
}

const AmEvent* AmEvent::NextEvent() const
{
	return mNext;
}

const AmEvent* AmEvent::PrevEvent() const
{
	return mPrev;
}

AmEvent* AmEvent::NextEvent()
{
	return mNext;
}

AmEvent* AmEvent::PrevEvent()
{
	return mPrev;
}

const AmEvent* AmEvent::HeadEvent() const
{
	const AmEvent*	event = this;
	const AmEvent*	prev = event->PrevEvent();
	while (prev) {
		event = prev;
		prev = event->PrevEvent();
	}
	return event;
}

const AmEvent* AmEvent::TailEvent() const
{
	const AmEvent*	event = this;
	const AmEvent*	next = event->NextEvent();
	while (next) {
		event = next;
		next = event->NextEvent();
	}
	return event;
}

AmEvent* AmEvent::HeadEvent()
{
	AmEvent*	event = this;
	AmEvent*	prev = event->PrevEvent();
	while (prev) {
		event = prev;
		prev = event->PrevEvent();
	}
	return event;
}

AmEvent* AmEvent::TailEvent()
{
	AmEvent*	event = this;
	AmEvent*	next = event->NextEvent();
	while (next) {
		event = next;
		next = event->NextEvent();
	}
	return event;
}

void AmEvent::AppendEvent(AmEvent* event)
{
	AmEvent* after = this;
	ArpVALIDATE(after != NULL && event != NULL && after != event, return);
	#if NOISY
	ArpD(cdb << ADH << "AppendEvent: after=" << after
			<< ", event=" << event << std::endl);
	#endif
	AmEvent* const afterNext = after->NextEvent();
	#if NOISY
	ArpD(cdb << ADH << "AppendEvent: afterNext=" << afterNext << std::endl);
	#endif
	event->SetNextEvent(afterNext);
	event->SetPrevEvent(after);
	#if NOISY
	ArpD(cdb << ADH << "AppendEvent: linked event to next=" << afterNext
					<< ", prev=" << after << std::endl);
	#endif
	if( afterNext ) {
		afterNext->SetPrevEvent(event);
		#if NOISY
		ArpD(cdb << ADH << "AppendEvent: linked afterNext to prev="
						<< event << std::endl);
		#endif
	}
	after->SetNextEvent(event);
	#if NOISY
	ArpD(cdb << ADH << "AppendEvent: linked after to next=" << event << std::endl);
	#endif
}

void AmEvent::InsertEvent(AmEvent* event)
{
	AmEvent* before = this;
	ArpVALIDATE(before != NULL && event != NULL && before != event, return);
	AmEvent* const beforePrev = before->PrevEvent();
	event->SetPrevEvent(beforePrev);
	event->SetNextEvent(before);
	if( beforePrev ) beforePrev->SetNextEvent(event);
	before->SetPrevEvent(event);
}

AmEvent* AmEvent::RemoveEvent()
{
	AmEvent* event = this;
	ArpVALIDATE(event != NULL, return event);
	if( event ) {
		AmEvent* const prev = event->PrevEvent();
		AmEvent* const next = event->NextEvent();
		if( prev ) prev->SetNextEvent(next);
		if( next ) next->SetPrevEvent(prev);
		event->SetNextEvent(NULL);
		event->SetPrevEvent(NULL);
		if( next ) return next;
		return prev;
	}
	
	return NULL;
}

static inline int compare_events(const AmTime firstTime, const AmEvent::AmEvent::EventType firstType,
								 const AmTime secondTime,const  AmEvent::AmEvent::EventType secondType)
{
	if (firstTime < secondTime) return -1;
	else if (firstTime > secondTime) return 1;
	// if times are the same, place events with higher type codes
	// after of those with lower ones.
	else if (firstType < secondType) return -1;
	else if (firstType > secondType) return 1;
	return 0;
}

int32 AmEvent::QuickCompare(const AmEvent* other) const
{
	return compare_events(StartTime(), Type(), other->StartTime(), other->Type());
}

AmEvent* AmEvent::MergeEvent(AmEvent* src)
{
	AmEvent* destPos = this;
	
	#if NOISY
	ArpD(cdb << ADH << "Merging " << src << " into " << destPos << std::endl);
	#endif
	if( !destPos ) return src;
	const AmTime srcTime = src->StartTime();
	const AmEvent::AmEvent::EventType srcType = src->Type();
	#if NOISY
	ArpD(cdb << ADH << "Source time is " << srcTime << std::endl);
	#endif
	AmEvent* tmp=NULL;
	bool searched = false;
	
	// If src time is after current position time, look forward for
	// the place to insert it.
	while( compare_events(destPos->StartTime(), destPos->Type(), srcTime, srcType) <= 0 ) {
		#if NOISY
		ArpD(cdb << ADH << "Dest time is " << destPos->StartTime()
				<< ", going forward." << std::endl);
		#endif
		tmp = destPos->NextEvent();
		if( !tmp ) {
			// Whoops, ran to end of list -- place source at end.
			#if NOISY
			ArpD(cdb << ADH << "This is the last event; appending src.\n");
			#endif
			destPos->AppendEvent(src);
			return src;
		}
		destPos = tmp;
		searched = true;
	}
	
	if( searched ) {
		// We moved forward at least one event in the dest list, so
		// we know this is where to put it.
		#if NOISY
		ArpD(cdb << ADH << "Found dest time " << destPos->Time()
						<< ", inserting here." << std::endl);
		#endif
		destPos->InsertEvent(src);
		return src;
	}
	
	// That didn't work -- src time is before current position, so look
	// back in the list for where this goes.
	while( compare_events(destPos->StartTime(), destPos->Type(), srcTime, srcType) > 0 ) {
		#if NOISY
		ArpD(cdb << ADH << "Dest time is " << destPos->StartTime()
						<< ", going backward." << std::endl);
		#endif
		tmp = destPos->PrevEvent();
		if( !tmp ) {
			// Whoops, ran to end of list -- place source at front.
			#if NOISY
			ArpD(cdb << ADH << "This is the first event; inserting src.\n");
			#endif
			destPos->InsertEvent(src);
			return src;
		}
		destPos = tmp;
	}
	
	// Okay, we absolutely positive know that this is the place to
	// put it.
	#if NOISY
	ArpD(cdb << ADH << "Found dest time " << destPos->Time()
					<< ", appending here." << std::endl);
	#endif
	destPos->AppendEvent(src);
	return src;
}

AmEvent* AmEvent::MergeList(AmEvent* source, bool ordered)
{
	AmEvent* dest = this;
	if (!dest) return source;
	if (!source) return dest;
	
	// make sure we are at the front of the list.
	AmEvent* tmp;
	while( (tmp=source->PrevEvent()) != NULL ) source = tmp;
	
	while( source != NULL ) {
		// because we know we are chewing through the entire list,
		// we don't need to do an actual "RemoveEvent()" on 'source'.
		AmEvent* srcNext = source->NextEvent();
		dest = dest->MergeEvent(source);
		if (ordered && source->NextEvent() == srcNext) {
			return source;
		}
		source = srcNext;
	}
	
	return dest;
}

AmEvent* AmEvent::CutForwardAtTime(const AmTime time, AmEvent** outTail)
{
	AmEvent* pos = this;
	while (pos && pos->StartTime() < time)
		pos = pos->NextEvent();
	
	if (pos == this) {
		*outTail = pos;
		return NULL;
	} else if (!pos) {
		*outTail = NULL;
		return this;
	}
	
	AmEvent* prev = pos->PrevEvent();
	if (prev) {
		prev->SetNextEvent(NULL);
		pos->SetPrevEvent(NULL);
	}
	*outTail = pos;
	return this;
}

AmEvent* AmEvent::CutBackwardAtTime(const AmTime time, AmEvent** outTail)
{
ArpASSERT(mTime >= 0);
	AmEvent* pos = this;
	while (pos && pos->StartTime() >= time)
		pos = pos->PrevEvent();
	
	if (pos == this) {
		*outTail = NULL;
		return this;
	} else if (!pos) {
		*outTail = pos;
		return NULL;
	}
	
	AmEvent* next = pos->NextEvent();
	if (next) {
		next->SetPrevEvent(NULL);
		pos->SetNextEvent(NULL);
	}
	*outTail = this;
	return pos;
}

AmEvent* AmEvent::CutBefore()
{
	if (!mPrev) return this;
	AmEvent*		ans = mPrev;
	mPrev->SetNextEvent(0);
	SetPrevEvent(0);
	return ans;	
}

void AmEvent::SetNextEvent(AmEvent* event)
{
	mNext = event;
}

void AmEvent::SetPrevEvent(AmEvent* event)
{
	mPrev = event;
}

void AmEvent::Delete()
{
	if( mNext || mPrev ) RemoveEvent();
	AmSafeDelete::Delete();
}

void AmEvent::DeleteChain()
{
	AmEvent* event = this;
	while( event ) {
		#if NOISY
		ArpD(cdb << ADH << "Delete chain: removing event " << event << std::endl);
		#endif
		AmEvent* next = event->RemoveEvent();
		event->Delete();
		event = next;
	}
}

AmEvent* AmEvent::CopyChain() const
{
	AmEvent* head = NULL;
	AmEvent* tail = NULL;
	const AmEvent* pos = this;
	while (pos) {
		AmEvent* e = pos->Copy();
		if (e) {
			if (head) {
				tail->SetNextEvent(e);
				e->SetPrevEvent(tail);
				tail = e;
			} else {
				head = tail = e;
			}
		}
		pos = pos->NextEvent();
	}
	return head;
}

bool AmEvent::AssertTimeOrder() const
{
	AmEvent* e = const_cast<AmEvent*>(this);
	if (e) {
		AmEvent* head = e->HeadEvent();
		e = head;
		while (e) {
			AmEvent* n = e->NextEvent();
			if (n && e->StartTime() > n->StartTime()) {
				printf("Event chain out of order:\n");
				head->PrintChain(PRINT_ADDRESS);
				debugger("Event chain out of order");
				return false;
			}
			e = n;
		}
	}
	return true;
}

bool AmEvent::Equals(AmEvent* event) const
{
	assert( event );
	if( Type() != event->Type() ) return false;
	if( StartTime() != event->StartTime() ) return false;
	if( EndTime() != event->EndTime() ) return false;
	return true;
}

void AmEvent::SetTo(AmEvent* event, bool setTimes)
{
	ArpASSERT(event);
	if (event->Type() == Type() && setTimes) {
		SetStartTime( event->StartTime() );
		SetEndTime( event->EndTime() );
	}
}

AmEvent& AmEvent::operator=(const AmEvent& o)
{
	mTime = o.mTime;
	ASSERT(mTime >= 0);
	SetNextFilter(o.mFilter);
	mNext = NULL;
	mPrev = NULL;
	return *this;
}

void AmEvent::Initialize(AmTime timeArg) {
	ASSERT(timeArg >= 0);
	mTime = timeArg;
	mFilter = NULL;
	mNext = NULL;
	mPrev = NULL;
	mFlags = 0;
}

AmEvent* AmEvent::mFreeList[AmEvent::_NUM_TYPE] = { NULL };

void* AmEvent::GetEvent(AmEvent::EventType type, size_t size)
{
	// It turns out that on the x86 platform, it is actually faster to just
	// new and free the object...!  My guess is because this helps program
	// locality and thus improves cache hits.
	// And on the PPC, not using new is only marginally faster, so I think
	// it's best just to get rid of it.
	return malloc(size);
	
#if 0
	if( type < 0 || type >= _NUM_TYPE ) return new char[size];
	
	ArpD(cdb << ADH << "First check for free note ons...\n");
	if( !mFreeList[type] ) return new char[size];
	
	AmEvent* ev;
	BAutolock l(listLock);
	ArpD(cdb << ADH << "Second check for free note ons...\n");
	if( !mFreeList[type] ) return new char[size];
	
	ArpD(cdb << ADH << "Getting a free note on...\n");
	// We have an available object -- take it off the free list and
	// re-initialize it with the new constructor arguments.
	ev = mFreeList[type];
	mFreeList[type] = (AmEvent*)ev->mNext;
	return ev;
#endif
}

void AmEvent::SaveEvent(AmEvent::EventType type, void* ev)
{
	free(ev);
#if 0
	if( type < 0 || type >= _NUM_TYPE ) delete[] (char*)ev;
	
	BAutolock l(listLock);
	((AmEvent*)ev)->mNext = mFreeList[type];
	mFreeList[type] = (AmEvent*)ev;
#endif
}

/***************************************************************************
* Public - utility
*-------------------------------------------------------------------------*/

void AmEvent::Print(void) const {
	printf("AmEvent time %lld\n", mTime);
}

void AmEvent::PrintChain(uint32 flags, const char* prefix) const
{
	const AmEvent* pos = this;
	while (pos) {
		if (prefix) printf("%s", prefix);
		if (flags&PRINT_ADDRESS) printf("%p ", pos);
		pos->Print();
		pos = pos->NextEvent();
	}
}

void AmEvent::_ReservedAmEvent1() { }
void AmEvent::_ReservedAmEvent2() { }
void AmEvent::_ReservedAmEvent3() { }
void AmEvent::_ReservedAmEvent4() { }
void AmEvent::_ReservedAmEvent5() { }
void AmEvent::_ReservedAmEvent6() { }
void AmEvent::_ReservedAmEvent7() { }
void AmEvent::_ReservedAmEvent8() { }
void AmEvent::_ReservedAmEvent9() { }
void AmEvent::_ReservedAmEvent10() { }
void AmEvent::_ReservedAmEvent11() { }
void AmEvent::_ReservedAmEvent12() { }
void AmEvent::_ReservedAmEvent13() { }
void AmEvent::_ReservedAmEvent14() { }
void AmEvent::_ReservedAmEvent15() { }
void AmEvent::_ReservedAmEvent16() { }

// #pragma mark -

/***************************************************************
 * AM-CHANNEL-PRESSURE
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmChannelPressure::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmChannelPressure));
	return GetEvent(CHANNELPRESSURE_TYPE, size);
}

void AmChannelPressure::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmChannelPressure));
	SaveEvent(CHANNELPRESSURE_TYPE, ptr);
}

AmChannelPressure::AmChannelPressure(uint8 pressureArg, AmTime timeArg)
		: AmEvent(timeArg)
{
	mPressure = pressureArg;
}

AmChannelPressure::AmChannelPressure(const AmChannelPressure& o)
	: AmEvent(o),
	  mPressure(o.mPressure)
{
}

void AmChannelPressure::RealDelete()
{
	delete this;
}

AmEvent* AmChannelPressure::Copy() const
{
	return new AmChannelPressure(*this);
}

status_t AmChannelPressure::GetAsMessage(BMessage& msg) const
{
	if (AmEvent::GetAsMessage(msg) != B_OK) return B_ERROR;
	msg.AddInt16( "pressure", mPressure );
	return B_OK;
}

BView* AmChannelPressure::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmChannelPressureInspector(frame);
	else return 0;
}

bool AmChannelPressure::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	return mPressure == ((AmChannelPressure*)event)->Pressure();
}

void AmChannelPressure::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmChannelPressure*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmChannelPressure*>( event )) ) {
		SetPressure( cp->Pressure() );
	}
}

AmChannelPressure& AmChannelPressure::operator=(const AmChannelPressure& o)
{
	AmEvent::operator=(o);
	mPressure = o.mPressure;
	return *this;
}
	
// ----------- Channel Pressure Specific Interface -----------

uint8 AmChannelPressure::Pressure() const
{
	return(mPressure);
}

void AmChannelPressure::SetPressure(uint8 pressureArg)
{
	if (mPressure == pressureArg) return;
	mPressure = pressureArg;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

void AmChannelPressure::Print() const {
	printf("AmChannelPressure pressure %d, time %lld\n",
			mPressure, mTime);
}

AmEvent::AmEvent::EventType AmChannelPressure::Type() const
{
	return AmEvent::CHANNELPRESSURE_TYPE;
}

int32 AmChannelPressure::PersistentStateID() const
{
	return 0;
}

// #pragma mark -

/***************************************************************
 * AM-NOTE-ON
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmNoteOn::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmNoteOn));
	return GetEvent(NOTEON_TYPE, size);
}

void AmNoteOn::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmNoteOn));
	SaveEvent(NOTEON_TYPE, ptr);
}

AmNoteOn::AmNoteOn(uint8 noteArg, uint8 velocityArg, AmTime timeArg)
		: AmEvent(timeArg),
		mNote(noteArg), mVelocity(velocityArg), mRelVelocity(127), mDuration(20)
{
}

AmNoteOn::AmNoteOn(const AmNoteOn& o)
	: AmEvent(o),
	  mNote(o.mNote), mVelocity(o.mVelocity),
	  mRelVelocity(o.mRelVelocity), mDuration(o.mDuration)
{
}

AmNoteOn::AmNoteOn(const BMessage& flatEvent)
		: AmEvent(flatEvent),
		mNote(64), mVelocity(100), mRelVelocity(127), mDuration(20)
{
	int16	val16;
	int32	val32;
	if (flatEvent.FindInt16( "note", &val16) == B_OK)
		mNote = val16;
	if (flatEvent.FindInt16( "vel", &val16) == B_OK)
		mVelocity = val16;
	if (flatEvent.FindInt16( "rel_vel", &val16) == B_OK)
		mRelVelocity = val16;
	if (flatEvent.FindInt32( "end", &val32) == B_OK)
		SetEndTime( val32 );
}

void AmNoteOn::RealDelete()
{
	delete this;
}

AmEvent* AmNoteOn::Copy() const
{
	return new AmNoteOn(*this);
}

status_t AmNoteOn::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt16("note", mNote);
	msg.AddInt16("vel", mVelocity);
	msg.AddInt16("rel_vel", mRelVelocity);
	add_time(msg, "end", EndTime() );
	return B_OK;
}

bool AmNoteOn::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mNote == ((AmNoteOn*)event)->Note() ) ) return false;
	if( !( mVelocity == ((AmNoteOn*)event)->Velocity() ) ) return false;
	return mRelVelocity == ((AmNoteOn*)event)->ReleaseVelocity();
}

void AmNoteOn::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmNoteOn*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmNoteOn*>( event )) ) {
		SetNote( cp->Note() );
		SetVelocity( cp->Velocity() );
		SetReleaseVelocity( cp->ReleaseVelocity() );
	}
}

BView* AmNoteOn::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmNoteOnInspector(frame);
	else return 0;
}

AmNoteOn& AmNoteOn::operator=(const AmNoteOn& o)
{
	AmEvent::operator=(o);
	mNote = o.mNote;
	mVelocity = o.mVelocity;
	mRelVelocity = o.mRelVelocity;
	mDuration = o.mDuration;
	return *this;
}
	
// ----------- Note On Specific Interface -----------

uint8 AmNoteOn::Note() const
{
	return(mNote);
}

void AmNoteOn::SetNote(uint8 noteArg)
{
	if (noteArg > 127) noteArg = 127;
	if (mNote == noteArg) return;
	mNote = noteArg;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

uint8 AmNoteOn::Velocity() const
{
	return mVelocity;
}

void AmNoteOn::SetVelocity(uint8 velocityArg)
{
	if (mVelocity == velocityArg) return;
	mVelocity = velocityArg;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

uint8 AmNoteOn::ReleaseVelocity() const
{
	return mRelVelocity;
}

void AmNoteOn::SetReleaseVelocity(uint8 velocityArg)
{
	if (mRelVelocity == velocityArg) return;
	mRelVelocity = velocityArg;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

AmTime AmNoteOn::Duration() const
{
	return mDuration;
}

void AmNoteOn::SetDuration(AmTime timeArg)
{
	if (mDuration != 0) {
		AmTime		newDur = timeArg > 1 ? timeArg : 1;
		if (mDuration == newDur) return;
		mDuration = newDur;
		if (mParent) {
			mFlags |= NEEDS_SYNC_FLAG;
			mParent->TimeRangeDirty();
		}
	}
}

AmEvent::AmEvent::EventType AmNoteOn::Type() const
{
	return NOTEON_TYPE;
}


AmFilterHolderI* AmNoteOn::NextFilter() const
{
	return mFilter;
}

void AmNoteOn::SetEndTime(AmTime newTime)
{
	if (mDuration != 0) {
		AmTime		newDur = 1;
		if (newTime > mTime) newDur = newTime - mTime;
		if (mDuration == newDur) return;
		mDuration = newDur;
		if (mParent) {
			mFlags |= NEEDS_SYNC_FLAG;
			mParent->TimeRangeDirty();
		}
	}
}

void AmNoteOn::Print() const
{
	printf("AmNoteOn note %d, velocity %d, duration %lld, time %lld, end time %lld (%p) (filter %p)\n",
			mNote, mVelocity, mDuration, mTime, EndTime(), this, mFilter);
}

AmTime AmNoteOn::EndTime() const
{
	return( mTime + mDuration );
}

// A quick hack to let us know whether or not the note is sharp

static bool note_is_sharp[12] = {
	false, true,	// C, C#
	false, true,	// D, D#
	false,			// E
	false, true,	// F, F#
	false, true,	// G, G#
	false, true,	// A, A#
	false,			// B
};

bool AmNoteOn::IsSharp() const {
	return(note_is_sharp[mNote%12]);
}

void AmNoteOn::SetHasDuration(bool state) {
	if (!state) mDuration = 0;
	else mDuration = 1;
}

bool AmNoteOn::HasDuration() const {
	return mDuration != 0;
}

AmNoteOn::~AmNoteOn() {}

// #pragma mark -

/***************************************************************
 * AM-NOTE-OFF
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmNoteOff::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmNoteOff));
	return GetEvent(NOTEOFF_TYPE, size);
}

void AmNoteOff::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmNoteOff));
	SaveEvent(NOTEOFF_TYPE, ptr);
}

AmEvent::EventType AmNoteOff::Type() const
{
	return NOTEOFF_TYPE;
}

AmNoteOff::AmNoteOff(uint8 noteArg, uint8 velocityArg, AmTime timeArg)
		: AmEvent(timeArg)
{
	mNote = noteArg;
	mVelocity = velocityArg;
}

AmNoteOff::AmNoteOff(const AmNoteOff& o)
	: AmEvent(o),
	  mNote(o.mNote), mVelocity(o.mVelocity)
{
}

void AmNoteOff::RealDelete()
{
	delete this;
}

AmEvent* AmNoteOff::Copy() const
{
	return new AmNoteOff(*this);
}

status_t AmNoteOff::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt16( "note", mNote );
	msg.AddInt16( "vel", mVelocity );
	return B_OK;
}

bool AmNoteOff::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mNote == ((AmNoteOff*)event)->Note() ) ) return false;
	if( !( mVelocity == ((AmNoteOff*)event)->Velocity() ) ) return false;
	return true;
}

void AmNoteOff::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmNoteOff*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmNoteOff*>( event )) ) {
		SetNote( cp->Note() );
		SetVelocity( cp->Velocity() );
	}
}

AmNoteOff& AmNoteOff::operator=(const AmNoteOff& o)
{
	AmEvent::operator=(o);
	mNote = o.mNote;
	mVelocity = o.mVelocity;
	return *this;
}

AmNoteOff::~AmNoteOff()
{
	
}

// ----------- Note Off Specific Interface -----------

uint8 AmNoteOff::Note() const {
	return(mNote);
}

void AmNoteOff::SetNote(uint8 noteArg)
{
	if (noteArg > 127) noteArg = 127;
	if (mNote == noteArg) return;
	mNote = noteArg;
}

uint8 AmNoteOff::Velocity() const {
	return(mVelocity);
}

void AmNoteOff::SetVelocity(uint8 velocityArg) {
	if (mVelocity == velocityArg) return;
	mVelocity = velocityArg;
}

void AmNoteOff::Print() const {
	printf("AmNoteOff note %d, velocity %d, at time %lld\n", mNote, mVelocity, mTime);
}

bool AmNoteOff::IsSharp() const {
	return(note_is_sharp[mNote%12]);
}

// #pragma mark -

/***************************************************************
 * AM-TEMPO-CHANGE
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmTempoChange::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmTempoChange));
	return GetEvent(TEMPOCHANGE_TYPE, size);
}

void AmTempoChange::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmTempoChange));
	SaveEvent(TEMPOCHANGE_TYPE, ptr);
}

AmEvent::EventType AmTempoChange::Type() const
{
	return TEMPOCHANGE_TYPE;
}

int32 AmTempoChange::PersistentStateID() const
{
	return 0;
}

AmTempoChange::AmTempoChange(float tempo, AmTime time)
		: AmEvent(time), mTempo(tempo)
{
}

AmTempoChange::AmTempoChange(const AmTempoChange& o)
	: AmEvent(o),
	  mTempo(o.mTempo)
{
}

float AmTempoChange::Tempo() const
{
	return mTempo;
}

void AmTempoChange::SetTempo(float tempo)
{
	if (mTempo == tempo) return;
	mTempo = tempo;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

AmEvent* AmTempoChange::Copy() const
{
	return new AmTempoChange(*this);
}

status_t AmTempoChange::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddFloat("tempo", mTempo);
	return B_OK;
}

bool AmTempoChange::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mTempo == ((AmTempoChange*)event)->Tempo() ) ) return false;
	return true;
}

void AmTempoChange::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmTempoChange*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmTempoChange*>( event )) ) {
		SetTempo( cp->Tempo() );
	}
}

BView* AmTempoChange::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmTempoChangeInspector(frame);
	else return NULL;
}

void AmTempoChange::Print() const
{
	printf("AmTempoChange tempo %f, time %lld\n", mTempo, mTime);
}

AmTempoChange& AmTempoChange::operator=(const AmTempoChange& o)
{
	AmEvent::operator=(o);
	mTempo = o.mTempo;
	return *this;
}
	
void AmTempoChange::RealDelete()
{
	delete this;
}

AmTempoChange::~AmTempoChange()
{
	
}

// #pragma mark -

/***************************************************************
 * AM-CONTROL-CHANGE
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmControlChange::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmControlChange));
	return GetEvent(CONTROLCHANGE_TYPE, size);
}

void AmControlChange::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmControlChange));
	SaveEvent(CONTROLCHANGE_TYPE, ptr);
}

AmEvent::EventType AmControlChange::Type() const
{
	return CONTROLCHANGE_TYPE;
}

int32 AmControlChange::PersistentStateID() const
{
	return mControlNumber;
}

AmControlChange::AmControlChange(uint8 controlNumber,
										   uint8 controlValue,
										   AmTime time)
		: AmEvent(time)
{
	mControlNumber = controlNumber;
	mControlValue = controlValue;
}

AmControlChange::AmControlChange(const AmControlChange& o)
		: AmEvent(o),
		  mControlNumber(o.mControlNumber), mControlValue(o.mControlValue)
{
}

AmControlChange::AmControlChange(const BMessage& flatEvent)
		: AmEvent(flatEvent), mControlNumber(0), mControlValue(0)
{
	int16	i;
	if (flatEvent.FindInt16("number", &i) == B_OK) mControlNumber = i;
	if (flatEvent.FindInt16("value", &i) == B_OK) mControlValue = i;
}

uint8 AmControlChange::ControlNumber() const
{
	return mControlNumber;
}

void AmControlChange::SetControlNumber(uint8 controlNumber)
{
	if (mControlNumber == controlNumber) return;
	mControlNumber = controlNumber;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

uint8 AmControlChange::ControlValue() const
{
	return mControlValue;
}

void AmControlChange::SetControlValue(uint8 controlValue)
{
	if (mControlValue == controlValue) return;
	mControlValue = controlValue;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

void AmControlChange::RealDelete()
{
	delete this;
}

AmEvent* AmControlChange::Copy() const
{
	return new AmControlChange(*this);
}

status_t AmControlChange::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt16( "number", mControlNumber );
	msg.AddInt16( "value", mControlValue );
	return B_OK;
}

bool AmControlChange::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mControlNumber == ((AmControlChange*)event)->ControlNumber() ) ) return false;
	if( !( mControlValue == ((AmControlChange*)event)->ControlValue() ) ) return false;
	return true;
}

void AmControlChange::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmControlChange*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmControlChange*>( event )) ) {
		SetControlNumber( cp->ControlNumber() );
		SetControlValue( cp->ControlValue() );
	}
}

BView* AmControlChange::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmCcInspector(frame);
	else return 0;
}

AmControlChange& AmControlChange::operator=(const AmControlChange& o)
{
	AmEvent::operator=(o);
	mControlNumber = o.mControlNumber;
	mControlValue = o.mControlValue;
	return *this;
}

AmControlChange::~AmControlChange() { }

// ----------- Tempo Change Specific Interface -----------

void AmControlChange::Print() const
{
	printf("AmControlChange number %d, value %d, time %lld\n", mControlNumber, mControlValue, mTime);
}

// #pragma mark -

/***************************************************************
 * AM-PITCH-BEND
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmPitchBend::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmPitchBend));
	return GetEvent(PITCHBEND_TYPE, size);
}

void AmPitchBend::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmPitchBend));
	SaveEvent(PITCHBEND_TYPE, ptr);
}

AmPitchBend::AmPitchBend(int16 pitch, AmTime time)
		: AmEvent(time)
{
	SetValue(pitch);
}

AmPitchBend::AmPitchBend(uint8 lsb, uint8 msb, AmTime time)
		: AmEvent(time), mLsb(lsb), mMsb(msb)
{
}

AmPitchBend::AmPitchBend(const AmPitchBend& o)
	: AmEvent(o),
	  mLsb(o.mLsb), mMsb(o.mMsb)
{
}

AmEvent::EventType AmPitchBend::Type() const
{
	return PITCHBEND_TYPE;
}

int32 AmPitchBend::PersistentStateID() const
{
	return 0;
}

void AmPitchBend::RealDelete()
{
	delete this;
}

AmEvent* AmPitchBend::Copy() const
{
	return new AmPitchBend(*this);
}

status_t AmPitchBend::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt16( "value", Value() );
	return B_OK;
}

bool AmPitchBend::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mLsb == ((AmPitchBend*)event)->Lsb() ) ) return false;
	if( !( mMsb == ((AmPitchBend*)event)->Msb() ) ) return false;
	return true;
}

void AmPitchBend::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmPitchBend*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmPitchBend*>( event )) ) {
		SetValue( cp->Value() );
	}
}

BView* AmPitchBend::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmPitchBendInspector(frame);
	else return 0;
}

AmPitchBend& AmPitchBend::operator=(const AmPitchBend& o)
{
	AmEvent::operator=(o);
	mLsb = o.mLsb;
	mMsb = o.mMsb;
	return *this;
}

AmPitchBend::~AmPitchBend() {}

// ----------- Pitch Bend Specific Interface -----------

int16 AmPitchBend::Value() const
{
	return ( (mMsb<<7) | (mLsb&0x7f) ) - 8192;
}

void AmPitchBend::SetValue(int16 pitch)
{
	uint8		msb, lsb;
	if (pitch <= AM_PITCH_MIN) msb = lsb = 0;
	else if (pitch >= AM_PITCH_MAX) msb = lsb = 127;
	else {
		pitch += 8192;
		lsb = (pitch&0x7f);
		msb = (pitch>>7);
	}
	if (mMsb == msb && mLsb == lsb) return;
	mMsb = msb;
	mLsb = lsb;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

uint8 AmPitchBend::Lsb() const
{
	return mLsb;
}

void AmPitchBend::SetLsb(uint8 lsb)
{
	if (mLsb == lsb) return;
	mLsb = lsb;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

uint8 AmPitchBend::Msb() const
{
	return mMsb;
}

void AmPitchBend::SetMsb(uint8 msb)
{
	if (mMsb == msb) return;
	mMsb = msb;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

void AmPitchBend::Print() const
{
	printf("AmPitchBend lsb %d, msb %d, time %lld\n",
			mLsb, mMsb, mTime);
}

// #pragma mark -

/***************************************************************
 * AM-PROGRAM-CHANGE
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmProgramChange::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmProgramChange));
	return GetEvent(PROGRAMCHANGE_TYPE, size);
}

void AmProgramChange::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmProgramChange));
	SaveEvent(PROGRAMCHANGE_TYPE, ptr);
}

AmProgramChange::AmProgramChange()
		: AmEvent(0),
		mProgramNumber(0)
{
}

AmEvent::EventType AmProgramChange::Type() const
{
	return PROGRAMCHANGE_TYPE;
}

int32 AmProgramChange::PersistentStateID() const
{
	return 0;
}

AmProgramChange::AmProgramChange(uint8 programNumber, AmTime time)
		: AmEvent(time)
{
	mProgramNumber = programNumber;
}

AmProgramChange::AmProgramChange(const AmProgramChange& o)
	: AmEvent(o),
	  mProgramNumber(o.mProgramNumber)
{
}

AmProgramChange::AmProgramChange(const BMessage& flatEvent)
		: AmEvent(flatEvent), mProgramNumber(0)
{
	int16	i;
	if (flatEvent.FindInt16("program", &i) == B_OK) mProgramNumber = i;
}

AmProgramChange::~AmProgramChange()
{
	
}

AmTime AmProgramChange::EndTime() const
{
	return mTime;
}

uint8 AmProgramChange::ProgramNumber() const
{
	return mProgramNumber;
}

void AmProgramChange::SetProgramNumber(uint8 pn)
{
	if (mProgramNumber == pn) return;
	mProgramNumber = pn;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

void AmProgramChange::RealDelete()
{
	delete this;
}

AmEvent* AmProgramChange::Copy() const
{
	return new AmProgramChange(*this);
}

status_t AmProgramChange::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt16( "program", mProgramNumber );
	return B_OK;
}

bool AmProgramChange::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mProgramNumber == ((AmProgramChange*)event)->ProgramNumber() ) ) return false;
	return true;
}

void AmProgramChange::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmProgramChange*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmProgramChange*>( event )) ) {
		SetProgramNumber( cp->ProgramNumber() );
	}
}

BView* AmProgramChange::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmProgramChangeInspector(frame);
	else return 0;
}

AmProgramChange& AmProgramChange::operator=(const AmProgramChange& o)
{
	AmEvent::operator=(o);
	mProgramNumber = o.mProgramNumber;
	return *this;
}

// ----------- Program Change Specific Interface -----------

void AmProgramChange::Print() const {
	printf("AmProgramChange program %d, time %lld\n",
			mProgramNumber, mTime);
}

// #pragma mark -

/***************************************************************
 * AM-SIGNATURE
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmSignature::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmSignature));
	return GetEvent(SIGNATURE_TYPE, size);
}

void AmSignature::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmSignature));
	SaveEvent(SIGNATURE_TYPE, ptr);
}

AmSignature::AmSignature()
		: AmEvent(0),
		  mMeasure(1), mBeats(4), mBeatValue(4)
{
	CalculateEndTime();
}

AmEvent::EventType AmSignature::Type() const
{
	return SIGNATURE_TYPE;
}

int32 AmSignature::PersistentStateID() const
{
	return 0;
}

AmSignature::~AmSignature() {}

AmSignature::AmSignature(AmTime time, int32 measure, uint32 beats, uint32 beatValue)
		: AmEvent(time),
		  mMeasure(measure), mBeats(beats), mBeatValue(beatValue)
{
	CalculateEndTime();
}

AmSignature::AmSignature(AmTime time)
		: AmEvent(time)
{
	mMeasure = 1;
	mBeats = 4;
	mBeatValue = 4;
	CalculateEndTime();
}

AmSignature::AmSignature(const AmSignature& o)
	: AmEvent(o),
	  mMeasure(o.mMeasure), mBeats(o.mBeats), mBeatValue(o.mBeatValue)
{
	CalculateEndTime();
}

AmTime AmSignature::EndTime() const
{
	return mEndTime;
}

/* FIX:  The signature needs to add one to the duration for it to
 * be accurate.  Is the same true for the other events?
 */
AmTime AmSignature::Duration() const
{
	return EndTime() - StartTime() + 1;
}

int32 AmSignature::Measure() const
{
	return mMeasure;
}

void AmSignature::SetMeasure(int32 measure)
{
	mMeasure = measure;
}

uint32 AmSignature::Beats() const
{
	return mBeats;
}

uint32 AmSignature::BeatValue() const
{
	return mBeatValue;
}

void AmSignature::RealDelete()
{
	delete this;
}

AmEvent* AmSignature::Copy() const
{
	return new AmSignature(*this);
}

status_t AmSignature::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt32( "measure", mMeasure );
	msg.AddInt32( "beats", mBeats );
	msg.AddInt32( "beat_value", mBeatValue );
	return B_OK;
}

bool AmSignature::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mMeasure == ((AmSignature*)event)->Measure() ) ) return false;
	if( !( mBeats == ((AmSignature*)event)->Beats() ) ) return false;
	if( !( mBeatValue == ((AmSignature*)event)->BeatValue() ) ) return false;
	return true;
}

void AmSignature::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmSignature*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmSignature*>( event )) ) {
		SetMeasure( cp->Measure() );
		SetBeats( cp->Beats() );
		SetBeatValue( cp->BeatValue() );
	}
}

AmSignature& AmSignature::operator=(const AmSignature& o)
{
	AmEvent::operator=(o);
	mMeasure = o.mMeasure;
	mBeats = o.mBeats;
	mBeatValue = o.mBeatValue;
	CalculateEndTime();
	return *this;
}

// ----------- Program Change Specific Interface -----------

void AmSignature::SetStartTime(AmTime time)
{
	AmEvent::SetStartTime(time);
	CalculateEndTime();
}

static bool valid_beat_value(uint32 beatValue)
{
	return beatValue == 1
			|| beatValue == 2
			|| beatValue == 4
			|| beatValue == 8
			|| beatValue == 16
			|| beatValue == 32
			|| beatValue == 64;
}

static float ticks_per_beat_value(uint32 beatValue)
{
	ArpASSERT( valid_beat_value(beatValue) );
	if (beatValue == 1)			return PPQN * 4;
	else if (beatValue == 2)	return PPQN * 2;
	else if (beatValue == 4)	return PPQN;
	else if (beatValue == 8)	return PPQN * 0.5;
	else if (beatValue == 16)	return PPQN * 0.25;
	else if (beatValue == 32)	return PPQN * 0.125;
	else if (beatValue == 64)	return PPQN * 0.0625;
	else return PPQN;
}

void AmSignature::SetBeats(uint32 beats)
{
	mBeats = beats;
	CalculateEndTime();
}

void AmSignature::SetBeatValue(uint32 beatValue)
{
	mBeatValue = beatValue;
	CalculateEndTime();
}

void AmSignature::Set( const AmSignature& sig )
{
	Set( sig.StartTime(), sig.Measure(), sig.Beats(), sig.BeatValue() );
}

void AmSignature::Set(AmTime time, int32 measure, uint32 beats, uint32 beatValue)
{
	ArpASSERT( valid_beat_value(beatValue) );
	mTime = time;
	mMeasure = measure;
	mBeats = beats;
	mBeatValue = beatValue;
	CalculateEndTime();
}

void AmSignature::Set(AmTime time, uint32 beats, uint32 beatValue)
{
	ArpASSERT( valid_beat_value(beatValue) );
	mTime = time;
	mBeats = beats;
	mBeatValue = beatValue;
	CalculateEndTime();
}

void AmSignature::Set(AmTime time, int32 measure)
{
	mTime = time;
	mMeasure = measure;
	CalculateEndTime();
}

AmTime AmSignature::TicksPerBeat() const
{
	return (AmTime)ticks_per_beat_value(mBeatValue);
}

AmTime AmSignature::BeatForTime(AmTime time) const
{
	if ( time < StartTime() || time > EndTime() ) return time;
	AmTime	ticksPerBeat = TicksPerBeat();
	AmTime	start = StartTime();
	while ( (start + ticksPerBeat) < time ) start += ticksPerBeat;
	return start;
}

void AmSignature::Print() const
{
	printf("[%li] at %lli,  %li \\ %li\t end: %lli\n",
			mMeasure, mTime, mBeats, mBeatValue, mEndTime);
	fflush(stdout);
}

void AmSignature::CalculateEndTime()
{
	mEndTime = mTime + ((( (AmTime)ticks_per_beat_value(mBeatValue) ) * mBeats) -1);
}

// #pragma mark -

/***************************************************************
 * AM-MOTION-CHANGE
 ***************************************************************/

AmMotionChange::AmMotionChange(	const AmMotionI* motion,
								int32 measure, AmTime time)
		: AmEvent(time), mMotion(NULL), mMeasure(measure),
		  mTrackId(0)
{
	if (motion) mMotion = new AmMotion(*motion);
}

AmMotionChange::AmMotionChange(const AmMotionChange& o)
	: AmEvent(o), mMotion(NULL), mMeasure(o.mMeasure),
	  mTrackId(o.mTrackId)
{
	if (o.mMotion) mMotion = new AmMotion(*o.mMotion);
}


int32 AmMotionChange::PersistentStateID() const
{
	return 0;
}

AmEvent::EventType AmMotionChange::Type() const
{
	return MOTION_TYPE;
}

AmMotionChange::~AmMotionChange() {}

AmMotionChange::AmMotionChange(const BMessage& msg)
		: AmEvent(0), mMotion(NULL), mMeasure(1),
		  mTrackId(0)
{
	bool		noMotion = false;
	if (msg.FindBool("no_motion", &noMotion) != B_OK) noMotion = false;
	if (!noMotion) mMotion = new AmMotion(msg, false, NULL);
	if (msg.FindInt32("measure", &mMeasure) != B_OK) mMeasure = 1;

	AmTime		t;
	if (find_time(msg, "start", &t) == B_OK) mTime = t;
}

int32 AmMotionChange::Measure() const
{
	return mMeasure;
}

void AmMotionChange::SetMeasure(int32 measure)
{
	mMeasure = measure;
}

bool AmMotionChange::HasMotion() const
{
	return mMotion != NULL;
}

BString AmMotionChange::Label() const
{
	if (!mMotion) return BString();
	return mMotion->Label();
}

uint32 AmMotionChange::CountHits() const
{
	if (!mMotion) return 0;
	return mMotion->CountHits();
}

uint32 AmMotionChange::CountMeasures() const
{
	if (!mMotion) return 0;
	return mMotion->CountMeasures();
}

status_t AmMotionChange::GetHit(uint32 number,
								BPoint* pt, float* end) const
{
	if (!mMotion) return B_ERROR;
	return mMotion->GetHit(number, pt, end);
}

status_t AmMotionChange::GetHitY(float x, float* outY) const
{
	if (!mMotion) return B_ERROR;
	return mMotion->GetHitY(x, outY);
}

bool AmMotionChange::IsEmpty() const
{
	if (!mMotion) return true;
	return mMotion->IsEmpty();
}

AmMotionMode AmMotionChange::EditingMode() const
{
	if (!mMotion) return RHYTHM_MODE;
	return mMotion->EditingMode();
}

void AmMotionChange::SetMotion(const AmMotionI* motion)
{
	if (mMotion) delete mMotion;
	mMotion = NULL;
	if (motion) mMotion = new AmMotion(*motion);
}

AmEvent* AmMotionChange::Copy() const
{
	return new AmMotionChange(*this);
}

status_t AmMotionChange::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	if (mMotion) err = mMotion->WriteTo(msg);
	else err = msg.AddBool("no_motion", true);
	msg.AddInt32("measure", mMeasure);
	return err;
}

bool AmMotionChange::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
//	if( !( mLsb == ((AmMotionChange*)event)->Lsb() ) ) return false;
//	if( !( mMsb == ((AmPitchBend*)event)->Msb() ) ) return false;
	return false;
}

void AmMotionChange::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmMotionChange*		mc;
	if (event->Type() == Type() && (mc = dynamic_cast<AmMotionChange*>(event)) ) {
		SetMotion(mc->mMotion);
		mMeasure = mc->mMeasure;
	}
}

BView* AmMotionChange::NewView(ViewType type, BRect frame) const
{
	return NULL;
}

void AmMotionChange::Print() const
{
	printf("AmMotionChange at %lld measure %ld, motion is ", mTime, mMeasure);
	if (!mMotion) printf("NULL\n");
	else mMotion->Print();
}

void* AmMotionChange::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmMotionChange));
	return GetEvent(MOTION_TYPE, size);
}

void AmMotionChange::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmMotionChange));
	SaveEvent(MOTION_TYPE, ptr);
}

AmMotionChange& AmMotionChange::operator=(const AmMotionChange& o)
{
	AmEvent::operator=(o);
	SetMotion(o.mMotion);
	mMeasure = o.mMeasure;
	return *this;
}

track_id AmMotionChange::TrackId() const
{
	return mTrackId;
}

void AmMotionChange::SetTrackId(track_id trackId)
{
	mTrackId = trackId;
}

void AmMotionChange::RealDelete()
{
	delete this;
}

// #pragma mark -

/***************************************************************
 * AM-SYSTEM-COMMON
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmSystemCommon::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmSystemCommon));
	return GetEvent(SYSTEMCOMMON_TYPE, size);
}

void AmSystemCommon::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmSystemCommon));
	SaveEvent(SYSTEMCOMMON_TYPE, ptr);
}

uint8 AmSystemCommon::Status() const {return mStatus;}
void AmSystemCommon::SetStatus(uint8 status)  {mStatus = status;}

uint8 AmSystemCommon::Data1() const {return mData1;}
void AmSystemCommon::SetData1(uint8 status)  {mData1 = status;}

uint8 AmSystemCommon::Data2() const {return mData2;}
void AmSystemCommon::SetData2(uint8 status)  {mData2 = status;}

AmEvent::EventType AmSystemCommon::Type() const {return SYSTEMCOMMON_TYPE; }

AmSystemCommon::~AmSystemCommon() {}

AmSystemCommon::AmSystemCommon(uint8 status, uint8 data1,
										 uint8 data2, AmTime time)
		: AmEvent(time)
{
	mStatus = status;
	mData1 = data1;
	mData2 = data2;
}

AmSystemCommon::AmSystemCommon(const AmSystemCommon& o)
	: AmEvent(o),
	  mStatus(o.mStatus), mData1(o.mData1), mData2(o.mData2)
{
}

void AmSystemCommon::RealDelete()
{
	delete this;
}

AmEvent* AmSystemCommon::Copy() const
{
	return new AmSystemCommon(*this);
}

status_t AmSystemCommon::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	msg.AddInt16( "status", mStatus );
	msg.AddInt16( "data1", mData1 );
	msg.AddInt16( "data2", mData2 );
	return B_OK;
}

bool AmSystemCommon::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	if( !( mStatus == ((AmSystemCommon*)event)->Status() ) ) return false;
	if( !( mData1 == ((AmSystemCommon*)event)->Data1() ) ) return false;
	if( !( mData2 == ((AmSystemCommon*)event)->Data2() ) ) return false;
	return true;
}

void AmSystemCommon::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmSystemCommon*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmSystemCommon*>( event )) ) {
		SetStatus( cp->Status() );
		SetData1( cp->Data1() );
		SetData2( cp->Data2() );
	}
}

AmSystemCommon& AmSystemCommon::operator=(const AmSystemCommon& o)
{
	AmEvent::operator=(o);
	mStatus = o.mStatus;
	mData1 = o.mData1;
	mData2 = o.mData2;
	return *this;
}

// ----------- System Common Specific Interface -----------

void AmSystemCommon::Print() const
{
	printf("System Common time %lli\n", mTime);
	fflush(stdout);
}

// #pragma mark -

/***************************************************************
 * AM-SYSTEM-EXCLUSIVE
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------

void* AmSystemExclusive::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmSystemExclusive));
	return GetEvent(SYSTEMEXCLUSIVE_TYPE, size);
}

void AmSystemExclusive::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmSystemExclusive));
	SaveEvent(SYSTEMEXCLUSIVE_TYPE, ptr);
}

AmSystemExclusive::AmSystemExclusive(const uint8* data, size_t length, AmTime time)
		: AmEvent(time),
		  mData((const ichar*)data, (int32)length),
		  mChannelStart(-1), mChannelEnd(-1)
{
	if (mData.Length() <= 0 || mData[mData.Length()-1] != (char)B_SYS_EX_END) {
		mData.Append((const ichar*)&B_SYS_EX_END, 1);
	}
}

AmSystemExclusive::AmSystemExclusive(const AmSystemExclusive& o)
	: AmEvent(o),
//	  mData(o.mData)
	 /* I assume dianne's ArpString does a copy on write.  However,
	  * something seems to be messing it up -- in the tool seeds, events
	  * are copied before changed, then the two events are compared to
	  * see if there are any changes (and to see if a note should sound),
	  * but currently the two events are always identical.
	  */
	  mData(o.Data(), o.Length(), true),
	  mChannelStart(o.mChannelStart), mChannelEnd(o.mChannelEnd)
{
}

void AmSystemExclusive::RealDelete()
{
	delete this;
}

const uint8* AmSystemExclusive::Data() const {
	return (const uint8*)(const char*)mData;
}

size_t AmSystemExclusive::Length() const {
	return mData.Length();
}

AmEvent::EventType AmSystemExclusive::Type() const {
	return SYSTEMEXCLUSIVE_TYPE;
}

AmSystemExclusive::~AmSystemExclusive() {}

void AmSystemExclusive::SetData(const uint8* data, size_t length)
{
	mData.Set((const ichar*)data, (int32)length);
	if (mData.Length() <= 0 || mData[mData.Length()-1] != (char)B_SYS_EX_END) {
		mData.Append((const ichar*)&B_SYS_EX_END, 1);
	}
}
												
AmEvent* AmSystemExclusive::Copy() const
{
	return new AmSystemExclusive(*this);
}

status_t AmSystemExclusive::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	if (mData.Length() > 0) {
		msg.AddData( "data", B_RAW_TYPE, (const ichar*)mData, mData.Length() );
	}
	return B_OK;
}

#if 1
static void print_hex(const uint8* bytes, size_t len)
{
	if (!bytes) {
		printf("No bytes\n");
		return;
	}
	for (size_t k = 0; k < len; k++) {
		printf("%x ", bytes[k]);
	}
	printf("\n"); fflush(stdout);
}
#endif

bool AmSystemExclusive::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
#if 0
printf("COMPARING SYSEX 1: "); 
print_hex(Data(), Length());
printf("COMPARING SYSEX 2: "); 
print_hex(((AmSystemExclusive*)event)->Data(), ((AmSystemExclusive*)event)->Length());
#endif
	if( !( mData == ((AmSystemExclusive*)event)->mData ) ) return false;
	return true;
}

void AmSystemExclusive::SetChannelBytes(int32 start, int32 end)
{
	mChannelStart = start;
	mChannelEnd = end;
}

void AmSystemExclusive::GetChannelBytes(int32* start, int32* end) const
{
	*start = mChannelStart;
	*end = mChannelEnd;
}

void AmSystemExclusive::SetChannel(uchar channel)
{
	if (mChannelStart < 0 || mChannelEnd != mChannelStart) return;
	if (mChannelStart >= mData.Length()) return;
	int32		len = mData.Length();
	char*		buf = mData.LockBuffer(len);
	if (buf) buf[mChannelStart] = channel;
	mData.UnlockBuffer(len);
}

AmSystemExclusive& AmSystemExclusive::operator=(const AmSystemExclusive& o)
{
	AmEvent::operator=(o);
	mData = o.mData;
	mChannelStart = o.mChannelStart;
	mChannelEnd = o.mChannelEnd;
	return *this;
}

// ----------- System Common Specific Interface -----------

void AmSystemExclusive::Print() const
{
	printf("System Exclusive (%p) time %lli: %ld chan start: %ld bytes: ", this, mTime, mData.Length(), mChannelStart);
	print_hex(Data(), Length());
//	printf("\n");
	fflush(stdout);
}

// #pragma mark -

/***************************************************************
 * AM-SONG-POSITION
 ***************************************************************/

// ----------- Event Management: Create, Delete, Duplicate. -----------
void* AmSongPosition::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmSongPosition));
	return GetEvent(SONGPOSITION_TYPE, size);
}

void AmSongPosition::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmSongPosition));
	SaveEvent(SONGPOSITION_TYPE, ptr);
}

AmEvent::EventType AmSongPosition::Type() const {
	return SONGPOSITION_TYPE;
}

AmSongPosition::~AmSongPosition() {}

AmSongPosition::AmSongPosition()
		: AmSystemCommon((0xF2), 0, 0, 0)
{
}

AmSongPosition::AmSongPosition(AmTime time)
		: AmSystemCommon((0xF2), 0, 0, time)
{
}

AmSongPosition::AmSongPosition(const AmSongPosition& o)
	: AmSystemCommon(o)
{
}

void AmSongPosition::RealDelete()
{
	delete this;
}

AmEvent* AmSongPosition::Copy() const
{
	return new AmSongPosition(*this);
}

AmSongPosition& AmSongPosition::operator=(const AmSongPosition& o)
{
	AmSystemCommon::operator=(o);
	return *this;
}

void AmSongPosition::Print() const
{
	printf("AmSongPosition time %lli\n", mTime);
	fflush(stdout);
}

// #pragma mark -

/***************************************************************
 * AM-LYRIC
 ***************************************************************/
AmLyric::AmLyric(const BString& lyric, AmTime time)
		: AmEvent(time),
		  mLyric(lyric)
{
}

AmLyric::AmLyric(const AmLyric& o)
	: AmEvent(o),
	  mLyric(o.mLyric)
{
}

AmEvent::EventType AmLyric::Type() const
{
	return LYRIC_TYPE;
}

BString AmLyric::Lyric() const
{
	return mLyric;
}

void AmLyric::SetLyric(const BString& lyric)
{
	mLyric = lyric;
}

AmEvent* AmLyric::Copy() const
{
	return new AmLyric(*this);
}

status_t AmLyric::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	if( mLyric.String()[0] > 0 && mLyric.Length() > 0 )
		msg.AddString( "lyric", mLyric.String() );
	return B_OK;
}

bool AmLyric::Equals(AmEvent* event) const
{
	if( !AmEvent::Equals(event) ) return false;
	return mLyric == ((AmLyric*)event)->Lyric();
}

void AmLyric::SetTo(AmEvent* event, bool setTimes)
{
	AmEvent::SetTo(event, setTimes);
	AmLyric*	cp;
	if (event->Type() == Type() && (cp = dynamic_cast<AmLyric*>( event )) ) {
		SetLyric( cp->Lyric() );
	}
}

void AmLyric::Print() const {
	printf("AmLyric lyric %s, time %lld\n", mLyric.String(), mTime);
}

void* AmLyric::operator new(size_t size)
{
	ArpASSERT(size == sizeof(AmLyric));
	return GetEvent(LYRIC_TYPE, size);
}

void AmLyric::operator delete(void* ptr, size_t size)
{
	ArpASSERT(size == sizeof(AmLyric));
	SaveEvent(LYRIC_TYPE, ptr);
}

AmLyric::~AmLyric()
{
}

AmLyric& AmLyric::operator=(const AmLyric& o)
{
	AmEvent::operator=(o);
	mLyric = o.mLyric;
	return *this;
}

void AmLyric::RealDelete()
{
	delete this;
}






// *****************************
// *****************************
// The rest are not implemented.
#if 0



// #pragma mark -

/***************************************************************
* class AmKeyPressure
***************************************************************/

AmKeyPressure::AmKeyPressure(uint8 noteArg, uint8 pressureArg, uint32 timeArg)
		: AmEvent(timeArg) {
	note = noteArg;
	pressure = pressureArg;
}

uint8 AmKeyPressure::Note() {
	return(note);
}

void AmKeyPressure::SetNote(uint8 noteArg) {
	note = noteArg;
}

uint8 AmKeyPressure::Pressure() {
	return(pressure);
}

void AmKeyPressure::SetPressure(uint8 pressureArg) {
	pressure = pressureArg;
}

AmEvent* AmKeyPressure::Copy() {
	return(new AmKeyPressure(note, pressure, time));
}

#endif

void string_for_event_type(	BString& answer, AmEvent::AmEvent::EventType type,
							AmEvent::EventSubtype subtype, bool plural)
{
	if (plural) {
		if (subtype == AmEvent::LINK_SUBTYPE) answer << "Links";
		else if (type == AmEvent::NOTEOFF_TYPE) answer << "Notes";
		else if (type == AmEvent::PROGRAMCHANGE_TYPE) answer << "Program Changes";
		else if (type == AmEvent::CONTROLCHANGE_TYPE) answer << "Control Changes";
		else if (type == AmEvent::PITCHBEND_TYPE) answer << "Pitch Bends";
		else if (type == AmEvent::NOTEON_TYPE) answer << "Notes";
		else if (type == AmEvent::CHANNELPRESSURE_TYPE) answer << "Channel Aftertouches";
		else if (type == AmEvent::KEYPRESSURE_TYPE) answer << "Key Aftertouches";
		else if (type == AmEvent::TEMPOCHANGE_TYPE) answer << "Tempo Changes";
		else if (type == AmEvent::SIGNATURE_TYPE) answer << "Signatures";
		else if (type == AmEvent::LYRIC_TYPE) answer << "Lyrics";
		else answer << "Events";
	} else {
		if (subtype == AmEvent::LINK_SUBTYPE) answer << "Link";
		else if (type == AmEvent::NOTEOFF_TYPE) answer << "Note";
		else if (type == AmEvent::PROGRAMCHANGE_TYPE) answer << "Program Change";
		else if (type == AmEvent::CONTROLCHANGE_TYPE) answer << "Control Change";
		else if (type == AmEvent::PITCHBEND_TYPE) answer << "Pitch Bend";
		else if (type == AmEvent::NOTEON_TYPE) answer << "Note";
		else if (type == AmEvent::CHANNELPRESSURE_TYPE) answer << "Channel Aftertouch";
		else if (type == AmEvent::KEYPRESSURE_TYPE) answer << "Key Aftertouch";
		else if (type == AmEvent::TEMPOCHANGE_TYPE) answer << "Tempo Change";
		else if (type == AmEvent::SIGNATURE_TYPE) answer << "Signature";
		else if (type == AmEvent::LYRIC_TYPE) answer << "Lyric";
		else answer << "Event";
	}
}

AmEvent* am_get_as_event(const BMessage& msg)
{
	int32		type, subtype;
	if (msg.FindInt32("type", &type) != B_OK
			|| msg.FindInt32("subtype", &subtype) != B_OK)
		return NULL;
	if (type == AmEvent::PHRASE_TYPE) {
		AmPhraseEvent*	pe = NULL;
		if (subtype == AmEvent::ROOT_SUBTYPE) pe = new AmRootPhraseEvent(msg);
		else if (subtype == AmEvent::BANK_SUBTYPE) pe = new AmBankChange(msg);
		else if (subtype == AmEvent::INNER_SUBTYPE) pe = new AmInnerPhraseEvent(msg);
		return pe;
	} else if (type == AmEvent::CONTROLCHANGE_TYPE) {
		return new AmControlChange(msg);
	} else if (type == AmEvent::PROGRAMCHANGE_TYPE) {
		return new AmProgramChange(msg);
	}
	return NULL;	
}
