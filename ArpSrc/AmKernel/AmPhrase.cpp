/* AmPhrase.cpp
 */
#define _BUILDING_AmKernel 1

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmPhrase.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSelections.h"

// The number of nodes we allot for the chain heads -- every time the list needs
// more chains, this is the number of chains we add.
static const uint32 ARP_CHAIN_INCREMENT		= 10;

//#define _SET_DIRTY_TRACE	1

/***************************************************************************
 * AM-PHRASE
 ***************************************************************************/
AmPhrase::AmPhrase(uint32 chainSize)
		: mParent(NULL), mOldParent(0), mChainSize(chainSize),
		  mCachedEndTime(0), mEndTimeIsDirty(true),
		  mColorsNeedInit(true)
{
	ValidateHash(1);
}

AmPhrase::AmPhrase(const AmPhrase& o)
		: mParent(NULL), mOldParent(0), mChainSize(o.mChainSize),
		  mCachedEndTime(0), mEndTimeIsDirty(true),
		  mColorsNeedInit(true)
{
	ValidateHash( 1 );
	AmNode*		head = o.HeadNode();
	if( head ) SetList(head->Copy());
}

AmPhrase::~AmPhrase()
{
	for( uint32 k = 0; k < mChains.size(); k++ )
		delete mChains[k].mSpanNode;
}

AmPhrase& AmPhrase::operator=(const AmPhrase& o)
{
	AmNode*		head = o.HeadNode();
	SetList(head ? head->Copy() : NULL);
	return *this;
}

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/

AmRange	AmPhrase::TimeRange() const
{
	return AmRange( StartTime(), EndTime() );
}

AmTime AmPhrase::StartTime() const
{
	AmNode*		node = HeadNode();
	if (!node) return 0;
	return node->StartTime();
}

void AmPhrase::SetStartTime(AmTime newTime)
{
	ArpASSERT(newTime >= 0);
	#ifdef _SET_DIRTY_TRACE
	printf("AmPhrase::SetStartTime() set dirty to true\n");
	#endif
	SetEndTimeIsDirty();

	AmRange				oldRange = TimeRange();
	AmNode*				node = HeadNode();
	AmNode*				next = NULL;
	AmTime				delta = newTime - oldRange.start;
	vector<AmEvent*>	mEvents;
	
	while (node) {
		next = node->next;
		AmEvent*	event = node->Event();
		status_t	answer = RemoveNoHash(event, event->StartTime() );
		if (answer == B_OK) {
			ArpASSERT(event->StartTime() + delta >= 0);
			event->SetStartTime(event->StartTime() + delta);
			event->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
			mEvents.push_back(event);
		}
		node = next;
	}

	for (uint32 k = 0; k < mEvents.size(); k++)
		AddNoHash(mEvents[k]);

	AmRange		newRange = TimeRange();
	Invalidate(NULL, oldRange, newRange);
	if (mOldParent && oldRange.start != newRange.start) {
		mOldParent->RangeChange(this, oldRange.start, newRange.start);
	}

#if 0
	AmTime		oldStart = StartTime();
	status_t	answer = RemoveNoHash( event, event->StartTime() );
	if (answer != B_OK) return answer;
	AmRange		eRange = event->TimeRange();
	event->SetStartTime(newTime);
	ChangeEvent( event, eRange.start, event->StartTime(), eRange.end, event->EndTime() );
	answer = AddNoHash( event );
	if( mOldParent ) {
		AmTime		newStart = StartTime();
		if( oldStart != newStart ) mOldParent->RangeChange( this, oldStart, newStart );
	}
#endif

	ArpASSERT( CheckAll() );
}

AmTime AmPhrase::EndTime() const
{
	if (mEndTimeIsDirty) mCachedEndTime = CountEndTime();
	mEndTimeIsDirty = false;
	ArpASSERT( CheckEndTime() );
	return mCachedEndTime;
}

AmNode* AmPhrase::HeadNode() const
{
	for( uint32 k = 0; k < mChains.size(); k++ )
		if( mChains[k].HeadNode() ) return mChains[k].HeadNode();
	return 0;
}

AmNode* AmPhrase::TailNode() const
{
	for( uint32 k = mChains.size() - 1; k >= 0; k-- ) {
		if( mChains[k].HeadNode() )
			return mChains[k].HeadNode()->TailNode();
		/* FIX:  There's an inherent flaw with looping backwards through
		 * a vector:  I'm iterating over an unsigned variable, so the stopping
		 * condition won't actually be reached.  I should use an stl iterator
		 * for this.
		 */
		if( k == 0 ) return 0;
	}
	return 0;
}

AmNode* AmPhrase::ChainHeadNode(AmTime time, PhraseSearchType search) const
{
	ArpASSERT( time >= 0 );
	if( time < 0 ) return 0;
	uint32	hash = HashFromTime( time );
	if( hash >= mChains.size() ) return 0;
	return mChains[hash].HeadNode( search );
}

AmNode* AmPhrase::ChainSpanNode(AmTime time, PhraseSearchType search) const
{
	ArpASSERT( time >= 0 );
	if( time < 0 ) return 0;
	uint32	hash = HashFromTime( time );
	if( hash >= mChains.size() ) return 0;
	return mChains[hash].SpanNode( search );
}

AmNode* AmPhrase::FindNode(	AmTime time,
							PhraseSearchType search) const
{
	ArpASSERT(time >= 0);
	if( mChains.size() < 1 ) return 0;
	uint32		hash = HashFromTime( time );
	if( search == BACKWARDS_SEARCH && hash >= mChains.size() ) hash = mChains.size() - 1;
	if( hash >= mChains.size() ) return 0;
	return mChains[hash].FindNode( time, search );
}

AmNode* AmPhrase::FindNode(AmEvent* event) const
{
	ArpASSERT( event );
	AmNode*		n = FindNode( event->StartTime(), FORWARDS_SEARCH );
	if( !n ) return 0;
	while( n ) {
		if( n->Event() == event ) return n;
		n = n->next;
	}
	return 0;
}

bool AmPhrase::Includes(AmEvent* event) const
{
	ArpASSERT( event );
	AmNode*		n = ChainHeadNode( event->StartTime(), NO_SEARCH );
	while( n && n->StartTime() <= event->StartTime() ) {
		if( n->Event() == event ) return true;
		n = n->next;
	}
	return false;	
}

bool AmPhrase::IsEmpty() const
{
	ArpASSERT( CheckAll() );
	if( HeadNode() ) return false;
	else return true;
}

uint32 AmPhrase::CountNodes() const
{
	uint32		count = 0;
	AmNode*		node = HeadNode();
	while( node ) {
		count++;
		node = node->next;
	}
	return count;
}

bool AmPhrase::IncludesOnly(AmEvent::EventType type) const
{
	AmNode*		n = HeadNode();
	while (n) {
		if (n->Event() && n->Event()->Type() != type) return false;
		n = n->next;
	}
	return true;
}

void AmPhrase::MergeInto(	AmEvent** eventList, const AmPhraseEvent* topPhrase,
							AmFilterHolderI* initialFilter,
						 	AmTime startTime, AmTime stopTime) const
{
	ArpASSERT(startTime >= 0);
	if (stopTime >= 0) ArpASSERT(startTime <= stopTime);
	
	AmTime		offset = 0;
	if (topPhrase) offset = topPhrase->TimeOffset();
	AmNode*		node = FindNode(startTime + offset);
	while (node) {
		AmTime		eventStart = node->Event()->StartTime();
		if (topPhrase) eventStart = (topPhrase->EventRange(node->Event())).start;
		// A stopTime of -1 means we play to the end of the phrase, otherwise
		// stop if we're at the end of the requested play time.
		if (stopTime != -1) {
			if (eventStart > stopTime) return;
		}

		if (node->Event()->Type() == node->Event()->PHRASE_TYPE) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( node->Event() );
			if (pe && pe->Phrase() ) pe->Phrase()->MergeInto(eventList, topPhrase, initialFilter, startTime, stopTime);
		} else {
			AmEvent	*copy = node->Event()->Copy();
			if (copy) {
				if (topPhrase) copy->SetStartTime(eventStart);
				copy->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
				copy->SetNextFilter(initialFilter);
				if (*eventList == 0) {
					*eventList = copy;
				} else {
					(*eventList)->MergeEvent(copy);
				}
			}
		}
		node = node->next;
	}
}

uint32 AmPhrase::CountLinks() const
{
	return mLinks.size();
}

void AmPhrase::Link(AmLinkEvent* link)
{
	ArpASSERT(link);
	ArpASSERT(ContainsLink(link) == false);
	mLinks.push_back(link);
	/* If this is the first link created, I want to
	 * invalidate myself, as well, since my visual
	 * representation in the song window will now
	 * be different.
	 */
	AmRange		newRange = link->TimeRange();
	AmRange		oldRange = newRange;
	if (CountLinks() == 1) oldRange = TimeRange();
	Invalidate(link, oldRange, newRange);
}

void AmPhrase::Unlink(AmLinkEvent* link)
{
	ArpASSERT(link);
	for (uint32 k = 0; k < mLinks.size(); k++) {
		if (mLinks[k] == link) {
			link_vec::iterator		i = mLinks.begin() + k;
			mLinks.erase(i);
			break;
		}
	}
	ArpASSERT(ContainsLink(link) == false);
	/* If I just unlinked my last link, invalidate myself
	 * as well as the link, because now my visual representation
	 * will have changed.
	 */
	AmRange		newRange = link->TimeRange();
	AmRange		oldRange = newRange;
	if (CountLinks() == 0) oldRange = TimeRange();
	Invalidate(link, oldRange, newRange);
}

rgb_color AmPhrase::Color(ColorCode code) const
{
	if (mColorsNeedInit) InitializeColors();
	if (code == BACKGROUND_C) return mBgC;
	else if (code == FOREGROUND_C) return mFgC;
	else return mBgC;
}

void AmPhrase::SetColor(ColorCode code, rgb_color c)
{
	if (mColorsNeedInit) InitializeColors();
	if (code == BACKGROUND_C) mBgC = c;
	else if (code == FOREGROUND_C) mFgC = c;

	AmRange		range = TimeRange();
	Invalidate(NULL, range, range);
}

BString AmPhrase::Name() const
{
	return mName;
}

void AmPhrase::SetName(const BString& name)
{
	mName = name;

	AmRange		range = TimeRange();
	Invalidate(NULL, range, range);
}

status_t AmPhrase::GetProperties(BMessage& properties) const
{
	properties.AddData("bg_c", B_RGB_COLOR_TYPE, (void*)&mBgC, sizeof(mBgC));
	properties.AddData("fg_c", B_RGB_COLOR_TYPE, (void*)&mFgC, sizeof(mFgC));
	if (mName.Length() > 0) properties.AddString("name", mName);
	return B_OK;
}

status_t AmPhrase::SetProperties(const BMessage& properties)
{
	if (mColorsNeedInit) InitializeColors();
	const rgb_color*		c;
	ssize_t					size;
	if (properties.FindData("bg_c", B_RGB_COLOR_TYPE, (const void**)&c, &size) == B_OK)
		mBgC = *c;
	if (properties.FindData("fg_c", B_RGB_COLOR_TYPE, (const void**)&c, &size) == B_OK)
		mFgC = *c;
	const char*				s;
	if (properties.FindString("name", &s) == B_OK) mName = s;
	return B_OK;
}

	/*---------------------------------------------------------
	 * NODE MANIPULATION
	 *---------------------------------------------------------*/

/* The AddNoHash() method takes care of most of the details, this one
 * mostly just makes sure my parent rehashes me if necessary.
 */
status_t AmPhrase::Add(AmEvent* event)
{
	ArpASSERT(event);
	AmRange			oldRange = TimeRange();
	status_t		answer = AddNoHash(event);
	if (mOldParent) {
		AmRange		newRange = TimeRange();
		if (oldRange != newRange) mOldParent->RangeChange(this, oldRange.start, newRange.start);
	}
	ArpASSERT( CheckAll() );
//printf("Added event "); event->Print();
	return answer;
}

status_t AmPhrase::AddAll(AmPhrase* phrase)
{
	ArpASSERT(phrase);
	AmNode*		n = phrase->HeadNode();
	while (n) {
		status_t	err = Add( n->Event() );
		if (err != B_OK) return err;
		n = n->next;
	}
	return B_OK;
}

status_t AmPhrase::Remove(AmEvent* event)
{
	ArpASSERT(event);
	AmRange			oldRange = TimeRange();
	status_t		answer = RemoveNoHash( event, event->StartTime() );
	if (mOldParent) {
		AmRange		newRange = TimeRange();
		if (oldRange != newRange) mOldParent->RangeChange(this, oldRange.start, newRange.start);
	}
	ArpASSERT( CheckAll() );
	return answer;
}

status_t AmPhrase::SetEventStartTime(AmEvent* event, AmTime newTime)
{
	ArpASSERT(event);
	if (!event) return B_ERROR;
	#ifdef _SET_DIRTY_TRACE
	printf("AmPhrase::SetEventStartTime() set dirty to true\n");
	#endif
	SetEndTimeIsDirty();

	AmTime		oldStart = StartTime();
	status_t	answer = RemoveNoHash( event, event->StartTime() );
	/* This is an odd situation that arises from the complexity of the
	 * tool model -- sometimes undo states will set times for events,
	 * but the events have been removed from their phrases.  In this
	 * case, I'll go ahead and set the time like nothing happened,
	 * but I don't actually contain the event, and so I return an error.
	 */
	if (answer != B_OK) {
		event->SetStartTime(newTime);
		return answer;
	}
	AmRange		eRange = event->TimeRange();
	event->SetStartTime(newTime);
	event->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
	Invalidate(event, eRange, event->TimeRange() );
	answer = AddNoHash( event );
	if( mOldParent ) {
		AmTime		newStart = StartTime();
		if( oldStart != newStart ) mOldParent->RangeChange( this, oldStart, newStart );
	}
	ArpASSERT( CheckAll() );
	return answer;
}

status_t AmPhrase::SetEventEndTime(AmEvent* event, AmTime newTime)
{
	ArpASSERT( event );
#ifdef _SET_DIRTY_TRACE
printf("AmPhrase::SetEndTime() 1 set dirty to true\n");
#endif
SetEndTimeIsDirty();
	return SetEndTime(event, newTime, event->EndTime());
}

status_t AmPhrase::SetEndTime(AmEvent* event, AmTime newTime, AmTime oldTime)
{
	ArpASSERT( event );
#ifdef _SET_DIRTY_TRACE
printf("AmPhrase::SetEndTime() 2 set dirty to true\n");
#endif
SetEndTimeIsDirty();
	uint32		oldEndHash = HashFromTime( oldTime );
	uint32		newEndHash = HashFromTime( newTime );
	if (oldEndHash == newEndHash) {
		event->SetEndTime(newTime);
		event->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
		Invalidate(event, AmRange(event->StartTime(), oldTime), AmRange(event->StartTime(), newTime) );
		ArpASSERT( CheckAll() );
		return B_OK;
	}
	/* This is an odd situation that arises from the complexity of the
	 * tool model -- sometimes undo states will set end times for events,
	 * but the events have been removed from their phrases.  In this
	 * case, I'll go ahead and set the time like nothing happened,
	 * but I don't actually contain the event, and so I return an error.
	 */
	event->SetEndTime(newTime);
	if (oldEndHash >= mChains.size() ) return B_ERROR;
	if (!ValidateHash( newEndHash ) ) return B_NO_MEMORY;
	event->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
	Invalidate(event, AmRange(event->StartTime(), oldTime), AmRange(event->StartTime(), newTime) );
	/* Make sure the span events are synced.
	 */
	if( newEndHash < oldEndHash ) {
		mChains[newEndHash + 1].RemoveSpanEvent( event );
	} else if( newEndHash > oldEndHash ) {
		mChains[oldEndHash + 1].AddSpanEvent( event );
	}
	ArpASSERT( CheckAll() );
	return B_OK;
}

AmEventParent* AmPhrase::Parent() const
{
	return mParent;
}

void AmPhrase::SetParent(AmEventParent* parent)
{
	mParent = parent;
}

void AmPhrase::TimeRangeDirty()
{
	SetEndTimeIsDirty();
	if (mParent) mParent->TimeRangeDirty();
}

void AmPhrase::Invalidate(	AmEvent* changedEvent,
							AmRange oldRange, AmRange newRange)
{
	if (oldRange != newRange) {
		#ifdef _SET_DIRTY_TRACE
		printf("AmPhrase::Invalidate() set dirty to true\n");
		#endif
		SetEndTimeIsDirty();
	}
	if (!mParent) return;
	
	AmRange			or = oldRange, nr = newRange;
	for (uint32 k = 0; k <mLinks.size(); k++) {
		AmTime		offset = mLinks[k]->StartTime() - StartTime();
		if (or.IsValid() ) or += AmRange(oldRange.start + offset, oldRange.end + offset);
		nr += AmRange(newRange.start + offset, newRange.end + offset);
	}
	mParent->Invalidate(changedEvent, or, nr);
}

#if 0
	virtual void	ChangeEvent(AmEvent* child,
								AmTime oldStart, AmTime newStart,
								AmTime oldEnd, AmTime newEnd);

void AmPhrase::ChangeEvent(	AmEvent* child,
							AmTime oldStart, AmTime newStart,
							AmTime oldEnd, AmTime newEnd)
{
	Invalidate( child, AmRange(oldStart, oldEnd), AmRange(newStart, newEnd) );
#if 0
if (oldStart != newStart || oldEnd != newEnd) {
	#ifdef _SET_DIRTY_TRACE
	printf("AmPhrase::ChangeEvent() set dirty to true\n");
	#endif
	SetEndTimeIsDirty();
}
	if (mParent) mParent->ChangeEvent(child, oldStart, newStart, oldEnd, newEnd);
#endif
}
#endif

status_t AmPhrase::SetList(AmNode* head)
{
	AmTime		oldStart = StartTime();

	/* Delete my current data.
	 */
	for( uint32 k = 0; k < mChains.size(); k++ )
		delete mChains[k].mSpanNode;
	AmNode*		n = HeadNode();
	delete n;
#ifdef _SET_DIRTY_TRACE
printf("AmPhrase::SetList() set dirty to true\n");
#endif
SetEndTimeIsDirty();

	/* Flush the chains, rebuild them big enough to accomodate the new list.
	 */
	mChains.resize(0);
	AmTime		endTime = 0;
	for (n = head; n; n = n->next) {
		if (n->EndTime() > endTime) endTime = n->EndTime();
		if (n->Event() ) n->Event()->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
	}
	if (!ValidateHash( HashFromTime(endTime) ) ) return B_ERROR;

	/* Take ownership of the list.
	 */
	for( n = head; n; n = n->next ) {
		uint32		hash = HashFromTime( n->StartTime() );
		if (!mChains[hash].mHeadNode) mChains[hash].mHeadNode = n;
		if (n->EndTime() > mChains[hash].mEndTime) mChains[hash + 1].AddSpanEvent( n->Event() );
		n->Event()->SetParent(this);
	}

	/* If my start time changed, notify my parent.
	 */
	if (mOldParent) {
		AmTime		newStart = StartTime();
		if (oldStart != newStart) mOldParent->RangeChange( this, oldStart, newStart );
	}
	ArpASSERT( CheckAll() );
	return B_OK;
}

void AmPhrase::DeleteEvents()
{
	AmNode*		n = HeadNode();
	if (n) n->DeleteListContents();
	delete n;
	for (uint32 k = 0; k < mChains.size(); k++) {
		if (mChains[k].mSpanNode) {
			delete mChains[k].mSpanNode;
			mChains[k].mSpanNode = NULL;
		}
		mChains[k].mHeadNode = NULL;
	}
#ifdef _SET_DIRTY_TRACE
printf("AmPhrase::DeleteEvents() set dirty to true\n");
#endif
SetEndTimeIsDirty();
}

void AmPhrase::Sync(_AmPhraseSelectionEntry* entry)
{
	ArpASSERT(entry && entry->mTopPhrase && entry->mTopPhrase->Phrase() == this);
	if (!entry) return;
	uint32			count = 0;
	for (uint32 k = 0; k < entry->mEntries.size(); k++) {
		if (entry->mEntries[k].mEvent
				&& entry->mEntries[k].mEvent->mFlags&AmEvent::NEEDS_SYNC_FLAG) {
			Sync(entry->mEntries[k].mEvent);
			count++;
		}
	}
	if (count > 0) {
		SetEndTimeIsDirty();
		AmRange		newRange = TimeRange();
		Invalidate(NULL, AmRange(), newRange);
		ArpASSERT( CheckAll() );
	}
}

void AmPhrase::Print(bool recurse, uint32 tabs) const
{
	for (uint32 j = 0; j < tabs; j++) printf("\t");
	
	if (mEndTimeIsDirty)
		printf("AmPhrase %p (%ld nodes, end time %lld)\n", this, CountNodes(), EndTime() );
	else
		printf("AmPhrase %p (%ld nodes, end time %lld - cached %lld)\n", this, CountNodes(), EndTime(), mCachedEndTime);

	for( uint32 k = 0; k < mChains.size(); k++ ) {
		for (uint32 j = 0; j < tabs; j++) printf("\t");
		printf("%ld: ", k);
		mChains[k].Print(recurse, tabs);
	}
}

void AmPhrase::PrintNoChains(bool recurse, uint32 tabs) const
{
	for (uint32 j = 0; j < tabs; j++) printf("\t");

	if (mEndTimeIsDirty)
		printf("AmPhrase %p (%ld nodes, end time %lld)\n", this, CountNodes(), EndTime() );
	else
		printf("AmPhrase %p (%ld nodes, end time %lld - cached %lld)\n", this, CountNodes(), EndTime(), mCachedEndTime);
	
	AmNode*		n = HeadNode();
	int32		k = 0;
	while (n) {
		if (n && n->Event() ) {
			for (uint32 j = 0; j < tabs + 1; j++) printf("\t");
			printf("%ld: ", k);
			n->Event()->Print();
			if (recurse && n->Event()->Type() == n->Event()->PHRASE_TYPE) {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(n->Event() );
				if (pe && pe->Phrase() ) pe->Phrase()->PrintNoChains(recurse, tabs + 2);
			}
		}
		n = n->next;
		k++;
	}
}

void  AmPhrase::do_check() const
{
	CheckAll();
}

AmTime AmPhrase::CountEndTime() const
{
	/* FIX:  I found a bug in the old way of doing it -- it was ignoring
	 * the fact that a phrase earlier in my list could have an end time
	 * beyond a phrase later in the list -- and really, doing it this way
	 * is just as bad as stepping through the list.  Need to get an end time
	 * cache in that works.
	 */
	AmTime		endTime = 0;
	if (mChains.size() < 1) return endTime;
	for (uint32 k = mChains.size() - 1; k >= 0; k--) {
		if ( !mChains[k].IsEmpty() ) {
			AmTime		end = mChains[k].EndTime();
			if (end > endTime) endTime = end;
		}
		/* FIX:  There's an inherent flaw with looping backwards through
		 * a vector:  I'm iterating over an unsigned variable, so the stopping
		 * condition won't actually be reached.  I should use an stl iterator
		 * for this.
		 */
		if (k == 0) return endTime;
	}
	return endTime;
}

void AmPhrase::InitializeColors() const
{
	mColorsNeedInit = false;
	mBgC = Prefs().Color(AM_ARRANGE_BG_C);
	mFgC.red = mFgC.green = 0;
	mFgC.blue = mFgC.alpha = 255;
}

/* Answer the chain for the supplied event.  This method will make
 * sure that all necessary chains exist (based on the start and end
 * times of the event).
 */
_AmChain* AmPhrase::ChainFor(AmEvent* event)
{
	ArpASSERT( event->StartTime() >= 0 );
	ArpASSERT( event->StartTime() <= event->EndTime() );
	if( event->StartTime() < 0 ) return 0;
	uint32		endHash = HashFromTime( event->EndTime() );
	if( !ValidateHash( endHash ) ) return 0;
	uint32		startHash = HashFromTime( event->StartTime() );
	return &( mChains[startHash] );
}

/* Answer true if the hash exists, false otherwise.  Also, if the
 * hash is beyond my current bounds, resize myself so it exists.
 */
bool AmPhrase::ValidateHash(uint32 hash)
{
	if( mChains.size() <= hash ) {
		mChains.resize( hash + ARP_CHAIN_INCREMENT );
		/* FIX: right now it's just easier to assign all the next
		 * and prev values, and it's trivial processor-wise,  but at
		 * some point really should just assign the new ones.
		 */
		_AmChain*	mPrev = 0;
		_AmChain*	curr;
		int32		chainIndex = 0;
		for( uint32 k=0; k<mChains.size(); k++ ) {
			curr = &( mChains[k] );
			curr->SetRange( chainIndex, chainIndex + mChainSize - 1 );
			curr->mPrev = mPrev;
			if( mPrev ) mPrev->mNext = curr;

			mPrev = curr;
			chainIndex += mChainSize;
		}
	}
	return true;
}

uint32 AmPhrase::HashFromTime(AmTime time) const
{
	ArpASSERT( time >= 0 );
	if( time < 0 ) return 0;
	return (uint32)floor( time / mChainSize );
}

/* It's been reported that a phrase that exists in me has had its start
 * time change.  I want to rehash it and reorder my node list.
 */
void AmPhrase::RangeChange(AmPhrase* phrase, AmTime oldStart, AmTime newStart)
{
	ArpASSERT( phrase );
	AmTime			thisOldStart = StartTime();
	/* Find and remove the node (located at the old index) containing phrase,
	 * then add it back in to rehash it.
	 */
	uint32			hash = HashFromTime( oldStart );
	if( !ValidateHash( hash ) ) return;
	AmPhraseEvent*	pe = mChains[hash].PhraseEventFor( phrase );
	if( !pe ) return;
	pe->IncRefs();
	if( RemoveNoHash( pe, oldStart ) != B_OK ) {
		pe->DecRefs();
		return;
	}
	AddNoHash( pe );
	pe->DecRefs();
	
	if( mOldParent ) {
		AmTime	thisNewStart = StartTime();
		if( thisOldStart != thisNewStart )
			mOldParent->RangeChange( this, thisOldStart, thisNewStart );
	}
	ArpASSERT( CheckAll() );
}

status_t AmPhrase::AddNoHash(AmEvent* event)
{
	ArpASSERT(event);
	/* Add the event and make sure spanning events are added.
	 */
	_AmChain*	chain = ChainFor(event);
	if (!chain) return B_NO_MEMORY;
	AmNode*		node = chain->Add(event);
	if (!node) return B_NO_MEMORY;
	event->SetParent(this);
	event->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
#ifdef _SET_DIRTY_TRACE
printf("AmPhrase::AddNoHash() set dirty to true\n");
#endif
SetEndTimeIsDirty();
	/* Set the parent if necessary.
	 */
	if (node->Event()->Type() == node->Event()->PHRASE_TYPE) {
		AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( node->Event() );
		if ( pe && pe->Phrase() ) pe->Phrase()->mOldParent = this;
	}
	return B_OK;
}

status_t AmPhrase::RemoveNoHash(AmEvent* event, AmTime time)
{
	ArpASSERT(event);
#if 0
AmPhraseEvent*	pe = 0;
if( event->Type() == event->PHRASE_TYPE ) {
	pe = dynamic_cast<AmPhraseEvent*>( event );
//uint32	oldHash = hash;
	if( pe->Phrase().IsEmpty() ) {
printf("EVENT TO REMOVE: "); event->Print();
printf("PHRASE IT'S BEING REMOVED FROM: ");
Print();
		debugger("halt");
	}
//if( oldHash != hash ) debugger("hashes are different");
}
#endif
	/* Remove the node.
	 */
	uint32		hash = HashFromTime(time);
	if ( !ValidateHash(hash) ) return B_ERROR;
	AmNode*		node = mChains[hash].Remove(event);
	if (!node) return B_ERROR;
	event->SetParent(NULL);
	event->mFlags &= ~AmEvent::NEEDS_SYNC_FLAG;
#ifdef _SET_DIRTY_TRACE
printf("AmPhrase::RemoveNoHash() set dirty to true\n");
#endif
SetEndTimeIsDirty();
	/* Unset the parent if necessary.
	 */
	if( node->Event()->Type() == node->Event()->PHRASE_TYPE ) {
		AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( node->Event() );
		if ( pe && pe->Phrase() ) pe->Phrase()->mOldParent = NULL;
	}
	/* Delete that evil node.
	 */
	delete node;
	return B_OK;
}

void AmPhrase::SetEndTimeIsDirty()
{
	mEndTimeIsDirty = true;
	if (mOldParent) mOldParent->SetEndTimeIsDirty();
}

void AmPhrase::Sync(AmEvent* event)
{
	for (uint32 k = 0; k < mChains.size(); k++) mChains[k].Sync(event);
	/* Clean up any traces of this event.
	 */
	AddNoHash(event);
}

static void print_nodes(AmNode* node)
{
	if (!node) {
		printf("NULL\n");
		return;
	}
	while (node) {
		printf("0x%p: ", node);
		node->Print();
		node = node->next;
	}
}

static void print_nodes_backwards(AmNode* node)
{
	if (!node) {
		printf("NULL\n");
		return;
	}
	while (node) {
		printf("0x%p: ", node);
		node->Print();
		node = node->prev;
	}
}

bool AmPhrase::CheckAll() const
{
#if 1
	if( HeadNode() != CheckHead() ) {
		printf("Bad phrase head.\nHeadNode() is:\n");
		print_nodes(HeadNode());
		printf("CheckHead() is:\n");
		print_nodes(CheckHead());
		debugger("AmPhrase has bad head node");
	}
	if ( !CheckNoEmptyNodes() ) debugger("AmNode has no AmEvent");
	AmNode*		node = HeadNode();
	while (node) {
		if (node->prev == node) { node->Print(); debugger("AmNode has prev that points to itself"); }
		if (node->next == node) { node->Print(); debugger("AmNode has next that points to itself"); }
		node = node->next;
	}
	if( TailNode() != CheckTail() ) {
		printf("Bad phrase tail.\nTailNode() is:\n");
		print_nodes_backwards(TailNode());
		printf("CheckTail() is:\n");
		print_nodes_backwards(CheckTail());
		debugger("AmPhrase has bad tail node");
	}

	if (!CheckEndTime() ) {
		debugger("AmPhrase has bad end time");
	}

	if( !CheckChainConnections() )		debugger("AmPhrase has bad chain connections");
	if( CheckNodeCount() != CheckChainCount() ) {
		printf("Bad node count: CheckNodeCount()=%ld, CheckChainCount()=%ld\n",
				CheckNodeCount(), CheckChainCount());
		printf("Raw nodes (CheckNodeCount()):\n");
		print_nodes(HeadNode());
		printf("Chain nodes (CheckChainCount()):\n");
		Print();
		debugger("AmPhrase has bad node count");
	}
	if( !CheckSpanEvents() )	{ Print(); debugger("AmPhrase has bad span events"); }
	AmEvent*	event;
	if( (event = CheckForDuplicates()) ) {
		printf("AMPHRASE HAS DUPLICATE EVENT.  PHRASE:\n");
		Print();
		printf("DUPLICATE:\n");
		event->Print();
		debugger("AmPhrase includes event more than once");
	}

	AmEventParent*	parent = Parent();
	while (parent) {
		AmPhrase*	p = dynamic_cast<AmPhrase*>(parent);
		if (p) p->CheckAll();
		parent = parent->Parent();
	}
#endif
	return true;
}

bool AmPhrase::CheckNoEmptyNodes() const
{
	AmNode*		node = HeadNode();
	while (node) {
		if (node->Event() == NULL) debugger("AmNode has no AmEvent");
		node = node->next;
	}

	for( uint32 k = 0; k < mChains.size(); k++ ) {
		AmNode*		node = mChains[k].mSpanNode;
		while (node) {
			if (node->Event() == NULL) debugger("AmNode has no AmEvent");
			node = node->next;
		}
	}
	return true;
}

AmNode* AmPhrase::CheckHead() const
{
	for( uint32 k = 0; k < mChains.size(); k++ ) {
		if( mChains[k].HeadNode() ) return mChains[k].HeadNode();
	}
	return 0;
}

AmNode* AmPhrase::CheckTail() const
{
	AmNode*		head = HeadNode();
	if( !head ) return 0;
	return head->TailNode();
}

bool AmPhrase::CheckEndTime() const
{
	/* This is a different way of checking the end time then the
	 * CountEndTime() method -- either way could be considered 'more'
	 * correct, since they would each reveal a different kind of
	 * problem.  Therefore, I check both.
	 */
	AmTime		countEndTime = 0;
	AmNode*		n = HeadNode();
	while (n) {
		if (n->EndTime() > countEndTime) countEndTime = n->EndTime();
		n = n->next;
	}

	if (CountEndTime() != countEndTime) {
		printf("Bad end time (%p): CountEndTime()=%lld, last node end time=%lld in\n",
				this, CountEndTime(), countEndTime);
printf("HERE ARE THE CHAINS:\n");
for (uint32 k = 0; k < mChains.size(); k++) {
	mChains[k].Print(true, 0);
}

//		printf("Bad end time (%p): EndTime()=%lld, countEndTime()=%lld in\n",
//				this, EndTime(), countEndTime);
#if 1
		printf("AmPhrase %p (%ld nodes)\n", this, CountNodes() );
			for( uint32 k = 0; k < mChains.size(); k++ ) {
			for (uint32 j = 0; j < 0; j++) printf("\t");
			printf("%ld: ", k);
			mChains[k].Print(true, 1);
		}
#endif
		debugger("AmPhrase has bad end time");
		return false;
	}

	if (!mEndTimeIsDirty) {
		if (mCachedEndTime != countEndTime) {
			printf("Bad cached end time (%p): cachedEndTime()=%lld, countEndTime()=%lld in\n",
					this, mCachedEndTime, countEndTime);
#if 1
			printf("AmPhrase %p (%ld nodes)\n", this, CountNodes() );
				for( uint32 k = 0; k < mChains.size(); k++ ) {
				for (uint32 j = 0; j < 0; j++) printf("\t");
				printf("%ld: ", k);
				mChains[k].Print(true, 1);
			}
#endif
			debugger("Cached end time is wrong");
			return false;
		}
	}

	return true;
}

bool AmPhrase::CheckChainConnections() const
{
	const _AmChain*		prev = 0;
	const _AmChain*		curr;
	for( uint32 k = 0; k < mChains.size(); k++ ) {
		curr = &( mChains[k] );
		if( curr->mPrev != prev ) return false;
		if( prev ) {
			if( prev->mNext != curr ) return false;
		}
		prev = curr;
	}
	return true;
}

uint32 AmPhrase::CheckNodeCount() const
{
	uint32		count = 0;
	AmNode*		node = HeadNode();
	while( node ) {
		count++;
		node = node->next;
	}
	return count;
}

uint32 AmPhrase::CheckChainCount() const
{
	uint32		count = 0;
	for( uint32 k = 0; k < mChains.size(); k++ ) {
		count += mChains[k].CountEvents();
	}
	return count;
}

/* Answer true if all the events that require span events
 * have them, and have them only in the places they are supposed to be.
 */
bool AmPhrase::CheckSpanEvents() const
{
	AmNode*		n = HeadNode();
	while( n ) {
		uint32	startHash = HashFromTime( n->StartTime() );
		uint32	endHash = HashFromTime( n->EndTime() );
		if( mChains.size() <= endHash ) return false;
		/* Current phrase events aren't spanned.
		 */
		if( n->Event()->Type() != n->Event()->PHRASE_TYPE ) {
			for( uint32 k = 0; k < mChains.size(); k++ ) {
				bool		included = mChains[k].CheckIncludesSpan( n->Event() );
				if( included && ( k < startHash + 1 || k > endHash ) ) {
					printf("***CHAIN %ld (%lld to %lld) should not have span event ",
								k, mChains[k].mStartTime, mChains[k].mEndTime);
					n->Event()->Print();
					debugger("span exists that shouldn't");
					return false;
				}
				if( !included && ( k >= startHash + 1 && k <= endHash ) ) {
					printf("***CHAIN %ld (%lld to %lld) should have event ",
								k, mChains[k].mStartTime, mChains[k].mEndTime);
					n->Event()->Print();
					debugger("span that should exist doesn't");
					return false;
				}
			}
		}
		n = n->next;
	}
	return true;
}

AmEvent* AmPhrase::CheckForDuplicates() const
{
	AmNode*		n = HeadNode();
	while( n ) {
		AmEvent*	event = n->Event();
		AmNode*	n2 = n->next;
		while( n2 ) {
			if( event == n2->Event() ) return event;
			n2 = n2->next;
		}
		n = n->next;
	}
	return 0;
}

bool AmPhrase::ContainsLink(AmLinkEvent* linkEvent) const
{
	for (uint32 k = 0; k < mLinks.size(); k++) {
		if (mLinks[k] == linkEvent) return true;
	}
	return false;
}

/***************************************************************************
 * AM-SIGNATURE-PHRASE
 ***************************************************************************/
AmSignaturePhrase::AmSignaturePhrase(uint32 chainSize)
		: inherited(chainSize)
{
	AmSignature*	newSig = new AmSignature(0, 1, 4, 4);
	if (newSig) Add(newSig);
}

AmSignaturePhrase::AmSignaturePhrase(const AmSignaturePhrase& o)
		: inherited(o)
{
}

static bool valid_beat_value(uint32 bv)
{
	return bv == 1 || bv == 2 || bv == 4 || bv == 8 || bv == 16 || bv == 32;
}

/* Perform a swap -- remove all the signatures from oldSignatures and
 * place them into newSignatures, while simultaneously giving them the
 * currect time values.  This is done because it is theoretically possible
 * for signatures to get our of order while they are being reassigned.
 * If they were to get out of order and you were operating on the same
 * list, the result would be corrupted data.
 */
static void straighten_out_signatures(AmPhrase& oldSignatures)
{
	AmPhrase		newSignatures;
	AmNode*			n = oldSignatures.HeadNode();
	AmNode*			nextN = NULL;
	AmSignature		currentSig;
	AmSignature*	prevSig = NULL;
	currentSig.Set(0, 1, 4, 4);
	AmTime			sigLength = currentSig.Duration();
	vector<AmSignature*>	removedSigs;
	while (n) {
		nextN = n->next;
		AmSignature*	sig = dynamic_cast<AmSignature*>( n->Event() );
		if (sig) {
			oldSignatures.Remove(sig);
			ArpASSERT( currentSig.Measure() <= sig->Measure() );
			while ( currentSig.Measure() < sig->Measure() ) {
				currentSig.Set( currentSig.StartTime() + sigLength,
								currentSig.Measure() + 1,
								currentSig.Beats(),
								currentSig.BeatValue() );
			}
			sig->Set( currentSig.StartTime(),
						sig->Measure(),
						sig->Beats(),
						sig->BeatValue() );
			currentSig.Set(*sig);
			sigLength = currentSig.Duration();
			/* Only add this sig into the list if it's different from
			 * the previous one.
			 */
			if ( !prevSig
					|| (prevSig->Beats() != sig->Beats())
					|| (prevSig->BeatValue() != sig->BeatValue()) ) {
				newSignatures.Add(sig);
				prevSig = sig;
			} else {
				removedSigs.push_back(sig);
			}
		}
		n = nextN;
	}
	n = newSignatures.HeadNode();
	while (n) {
		nextN = n->next;
		AmEvent*	event = n->Event();
		newSignatures.Remove(event);
		oldSignatures.Add(event);
		n = nextN;
	}
}

status_t AmSignaturePhrase::SetSignature(int32 measure, uint32 beats, uint32 beatValue)
{
	if (!valid_beat_value(beatValue)) return B_ERROR;
	/* First see if there's an existing measure I can make use of
	 */
	AmNode*			n = HeadNode();
	AmTime			endTime = 0;
	while (n) {
		AmSignature*	sig = dynamic_cast<AmSignature*>( n->Event() );
		if (sig) {
			endTime = sig->EndTime() + 1;
			if (measure < sig->Measure()) {
				/* The time doesn't matter, since all the times for
				 * the measures will get straightened out later on.  All
				 * that matters is that the events be in the proper order.
				 */
				AmSignature*	newSig = new AmSignature();
				if (!newSig) return B_NO_MEMORY;
				newSig->Set(sig->StartTime() - 1, measure, beats, beatValue);
				Add(newSig);
				break;
			} else if (sig->Measure() == measure) {
				/* I need to remove this because setting the beats and
				 * beat values affects the sig's end time -- if I don't
				 * remove this, it might leave span events around.
				 */
				Remove(sig);
				sig->Set(sig->StartTime(), beats, beatValue);
				Add(sig);
				break;
			}
		}
		if (!(n->next)) {
			AmSignature*	newSig = new AmSignature();
			if (!newSig) return B_NO_MEMORY;
			newSig->Set(endTime, measure, beats, beatValue);
			Add(newSig);
			break;
		}
		n = n->next;
	}
	/* Now I know I have a signature and it's been added.  Run through
	 * and straighten out all the start and end times for the signatures.
	 */
//printf("BEFORE STRAIGHTENING: "); Print();
	straighten_out_signatures(*this);
//printf("AFTER STRAIGHTENING: "); Print();
	/* Now report on a change starting with the time of the measure
	 * supplied to this method.
	 */
	n = HeadNode();
	while (n) {
		AmSignature*	sig = dynamic_cast<AmSignature*>( n->Event() );
		if (sig) {
			if (sig->Measure() == measure) {
				AmRange		range( sig->StartTime(), sig->EndTime() );
//				MergeRangeChange(range, range, sig);
				return B_OK;
			} else if (sig->Measure() > measure) {
				AmNode*	prev = n->prev;
				if (prev) {
					AmRange		range( prev->StartTime(), prev->EndTime() );
//					MergeRangeChange(range, range, prev->Event() );
					return B_OK;
				}
			}
		}
		n = n->next;
	}
//	AmSignature		fakeSig(0);
//	MergeRangeChange(AmRange(0, 0), AmRange(0, 0), &fakeSig);
	return B_OK;
}

status_t AmSignaturePhrase::GetSignature(AmTime time, AmSignature& signature) const
{
	ArpASSERT(time >= 0);
	if (time < 0) return B_ERROR;
	AmNode*			node = HeadNode();
	ArpASSERT(node);
	if (!node) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	ArpASSERT(sig && sig->StartTime() == 0);
	if (!sig) return B_ERROR;
	if (sig->StartTime() != 0) return B_ERROR;
	AmSignature*	nextSig = NULL;
	AmNode*			nextNode = node->next;
	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	AmSignature		currentSig(*sig);
	AmTime			sigLength = currentSig.Duration();

	while (currentSig.EndTime() < time) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.Measure() == nextSig->Measure() ) ) {
			currentSig.Set(*nextSig);
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	/* Technically this check isn't necessary but I'm a safe guy.
	 */
	if ( time >= currentSig.StartTime() && time <= currentSig.EndTime() ) {
		signature.Set(currentSig);
		return B_OK;
	}
//	debugger("Failed to find measure");
	return B_ERROR;
}

status_t AmSignaturePhrase::GetSignatureForMeasure(int32 measure, AmSignature& signature) const
{
	ArpASSERT(measure >= 1);
	if (measure < 1) return B_ERROR;
	AmNode*			node = HeadNode();
	ArpASSERT(node);
	if (!node) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	ArpASSERT(sig && sig->StartTime() == 0);
	if (!sig) return B_ERROR;
	if (sig->StartTime() != 0) return B_ERROR;
	AmSignature*	nextSig = NULL;
	AmNode*			nextNode = node->next;
	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	AmSignature		currentSig(*sig);
	AmTime			sigLength = currentSig.Duration();

	if (currentSig.Measure() > measure) return B_ERROR;
	if (currentSig.Measure() == measure) {
		signature.Set(currentSig);
		return B_OK;
	}

	while (currentSig.Measure() < measure) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.Measure() == nextSig->Measure() ) ) {
			currentSig.Set(*nextSig);
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	ArpASSERT(currentSig.Measure() == measure);
	signature.Set(currentSig);
	return B_OK;
}

/***************************************************************************
 * _AM-CHAIN
 ****************************************************************************/
_AmChain::_AmChain()
		: mNext(0), mPrev(0), mStartTime(0), mEndTime(0), mHeadNode(0), mSpanNode(0)
{
}

_AmChain::~_AmChain()
{
	mHeadNode = 0;
	mSpanNode = 0;
}

_AmChain& _AmChain::operator=(const _AmChain &c)
{
	mNext = c.mNext;
	mPrev = c.mPrev;
	mStartTime = c.mStartTime;
	mEndTime = c.mEndTime;
	mHeadNode = c.mHeadNode;
	mSpanNode = c.mSpanNode;
	return *this;
}

void _AmChain::SetRange(AmTime startTime, AmTime endTime)
{
	mStartTime = startTime;
	mEndTime = endTime;
}

AmTime _AmChain::EndTime() const
{
	AmTime		endTime = 0;
	AmNode*		node = mHeadNode;
	while (node && node->StartTime() <= mEndTime) {
		if (node->EndTime() > endTime) endTime = node->EndTime();
		node = node->next;
	}
	node = mSpanNode;
	while (node) {
		if (node->EndTime() > endTime) endTime = node->EndTime();
		node = node->next;
	}
	return endTime;
}

AmNode* _AmChain::HeadNode(PhraseSearchType search) const
{
	if (mHeadNode) return mHeadNode;
	if (mNext && search == FORWARDS_SEARCH) return mNext->HeadNode(search);
	if (mPrev && search == BACKWARDS_SEARCH) return mPrev->HeadNode(search);
	return 0;
}

AmNode* _AmChain::SpanNode(PhraseSearchType search) const
{
	if( mSpanNode ) return mSpanNode;
	if( mNext && search == FORWARDS_SEARCH ) return mNext->SpanNode( search );
	if( mPrev && search == BACKWARDS_SEARCH ) return mPrev->SpanNode( search );
	return 0;
}


AmNode* _AmChain::FindNode(	AmTime time,
							PhraseSearchType search) const
{
	if( search == FORWARDS_SEARCH ) return FindNodeForwards( time );
	if( search == BACKWARDS_SEARCH ) return FindNodeBackwards( time );
	return 0;
}

AmNode* _AmChain::Add(AmEvent* event)
{
	ArpASSERT( event );
	AmNode*		node = new AmNode( event );
	if( !node ) return 0;
	if( !mHeadNode ) {
		mHeadNode = node;
		ConnectNewHeadNode( node );
	} else {
		if( mHeadNode->AddNode( node ) != B_OK ) {
			delete node;
			return 0;
		}
		if( mHeadNode->prev == node ) mHeadNode = node;
	}
	if( mNext && ( event->Type() != event->PHRASE_TYPE ) )
		mNext->AddSpanEvent( event );
	return node;
}

AmNode* _AmChain::Remove(AmEvent* event)
{
	/* Find the node to remove
	 */
	AmNode*		node = mHeadNode;
	AmNode*		nextHead = NextHead();
	AmNode*		eventNode = NULL;
	while (node && node != nextHead) {
		if (node->Event() == event) {
			eventNode = node;
			break;
		}
		node = node->next;
	}
	if (!eventNode) return NULL;
	/* Remove it
	 */
	AmNode*		nextNode = mHeadNode->next;
	eventNode->RemoveNode();
	if (eventNode == mHeadNode) {
		if (nextNode && nextNode->StartTime() <= mEndTime) mHeadNode = nextNode;
		else mHeadNode = NULL;
	}
	if (mNext) mNext->RemoveSpanEvent(event);
	return eventNode;
}

AmPhraseEvent* _AmChain::PhraseEventFor(AmPhrase* phrase) const
{
	AmNode*		node = mHeadNode;
	AmNode*		nextHead = NextHead();
	while( node && node != nextHead ) {
		if( node->Event()->Type() == node->Event()->PHRASE_TYPE ) {
			AmPhraseEvent*	pe;
			if( (pe = dynamic_cast<AmPhraseEvent*>( node->Event() ) ) != 0 ) {
				if(pe->Phrase() == phrase) return pe;
			}
		}
		node = node->next;
	}
	return 0;
}

bool _AmChain::IsEmpty() const
{
	return !mHeadNode && !mSpanNode;
}

uint32 _AmChain::CountEvents() const
{
	uint32		count = 0;
	AmNode*		node;
	AmNode*		nextHead = NextHead();
	for( node = mHeadNode; node && node != nextHead; node = node->next )
		count++;
	return count;
}

uint32 _AmChain::CountSpans() const
{
	uint32		count = 0;
	AmNode*		n = mSpanNode;
	while ( n ) {
		count++;
		n = n->next;
	}
	return count;
}

void _AmChain::Sync(AmEvent* event)
{
	AmNode*		span = mSpanNode;
	while (span) {
		if (span->Event() == event) {
			AmNode*		spanHead = NULL;
			if (span == mSpanNode) spanHead = mSpanNode->next;
			span->RemoveNode();
			if (spanHead) mSpanNode = spanHead;
			break;
		}
		span = span->next;
	}
	Remove(event);
}

void _AmChain::Print(bool recurse, uint32 tabs) const
{
	printf( "%lld to %lld (%ld spans)\n", mStartTime, mEndTime, CountSpans() );
	AmNode*		n = mSpanNode;
	while (n) {
		for (uint32 k = 0; k < tabs; k++) printf("\t");
		printf("\t(span) ");
		n->Print();
		n = n->next;
	}
	n = mHeadNode;
	AmNode*		nextHead = NextHead();
	while (n && n != nextHead) {
		for (uint32 k = 0; k < tabs; k++) printf("\t");
		printf("\t");
		if (recurse && n->Event() && n->Event()->Type() == AmEvent::PHRASE_TYPE) {
			AmPhraseEvent*		pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe && pe->Phrase() ) {
				pe->Phrase()->Print(recurse, tabs + 1);
			} else n->Print();
		} else n->Print();
		n = n->next;
	}
	fflush(stdout);
}

AmNode* _AmChain::NextHead() const
{
	_AmChain*	nextChain = mNext;
	while( nextChain ) {
		if( nextChain->mHeadNode ) return nextChain->mHeadNode;
		nextChain = nextChain->mNext;
	}
	return 0;
}

status_t _AmChain::AddSpanEvent(AmEvent* event)
{
	ArpASSERT( event );
	if( event->EndTime() < mStartTime ) return B_OK;
	AmNode*		node = new AmNode( event );
	if( !node ) return B_NO_MEMORY;
	if( mSpanNode ) {
		mSpanNode->prev = node;
		node->next = mSpanNode;
	}
	mSpanNode = node;
	if( !mNext ) return B_OK;
	return mNext->AddSpanEvent( event );
}

void _AmChain::RemoveSpanEvent(AmEvent* event)
{
	ArpASSERT( event );
	if( mNext ) mNext->RemoveSpanEvent( event );
	/* Find the node to remove
	 */
	AmNode*		node = mSpanNode;
	AmNode*		eventNode = 0;
	while( node ) {
		if( node->Event() == event ) {
			eventNode = node;
			break;
		}
		node = node->next;
	}
	if( !eventNode ) return;
	/* Remove it
	 */
	AmNode*		next = mSpanNode->next;
	eventNode->RemoveNode();
	if( node == mSpanNode ) {
		if( next ) mSpanNode = next;
		else mSpanNode = 0;
	}
}

void _AmChain::ConnectNewHeadNode(AmNode* node)
{
	ArpASSERT( node );
	_AmChain*	chain = mPrev;
	while( chain ) {
		if( chain->mHeadNode ) {
			chain->mHeadNode->AddNode( node );
			return;
		}
		chain = chain->mPrev;
	}
	chain = mNext;
	while( chain ) {
		if( chain->mHeadNode ) {
			chain->mHeadNode->AddNode( node );
			return;
		}
		chain = chain->mNext;
	}
}

AmNode* _AmChain::FindNodeBackwards(AmTime time) const
{
	AmNode*			node = PrevAvailableHeadNode();
	if( !node ) return 0;
	/* I have been supplied the first head node on or before
	 * the requested time.  There are two paths in front of
	 * me:  One is that the node I want is at or after the
	 * node I have.  In this case, I seek forward:
	 */
	if( node->StartTime() <= time ) {
		while( node ) {
			if( node->StartTime() == time ) return node;
			if( !node->next || node->next->StartTime() > time ) return node;
			node = node->next;
		}
	}
	/* The other is that, while I am in the block requested,
	 * everything in this block is actually higher than the
	 * time requested.  In this case, I seek backwards:
	 */
	while( node ) {
		if( node->StartTime() <= time ) return node;
		node = node->prev;
	}
	return 0;
}

AmNode* _AmChain::FindNodeForwards(AmTime time) const
{
	AmNode*		node = NextAvailableHeadNode();
	while( node ) {
		if( node->StartTime() >= time ) return node;
		node = node->next;
	}
	return 0;
}

AmNode* _AmChain::PrevAvailableHeadNode() const
{
	if( mHeadNode ) return mHeadNode;
	if( !mPrev ) return 0;
	return mPrev->PrevAvailableHeadNode();
}

AmNode* _AmChain::NextAvailableHeadNode() const
{
	if( mHeadNode ) return mHeadNode;
	if( !mNext ) return 0;
	return mNext->NextAvailableHeadNode();
}

bool _AmChain::CheckIncludesSpan(AmEvent* event) const
{
	ArpASSERT( event );
	AmNode*		n = mSpanNode;
	while( n ) {
		if( n->Event() == event ) return true;
		n = n->next;
	}
	return false;
}
