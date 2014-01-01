/* AmPhraseEvent.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include <stdlib.h>
#include <be/interface/InterfaceDefs.h>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmEventInspectors.h"
#include "AmKernel/AmPhraseEvent.h"

#include <InterfaceDefs.h>

#define NOISY 0

/***************************************************************
 * AM-PHRASE-EVENT
 ***************************************************************/
#if 0
void* AmPhraseEvent::operator new(size_t size)
{
	ArpASSERT( size == sizeof(AmPhraseEvent) );
	return GetEvent( PHRASE_TYPE, size );
}

void AmPhraseEvent::operator delete(void* ptr, size_t size)
{
	ArpASSERT( size == sizeof(AmPhraseEvent) );
	SaveEvent( PHRASE_TYPE, ptr );
}

AmEvent* AmPhraseEvent::Copy() const
{
	return new AmPhraseEvent(*this);
}
#endif

AmTime AmPhraseEvent::TimeOffset() const
{
	return 0;
}

AmRange AmPhraseEvent::EventRange(const AmEvent* event) const
{
	return event->TimeRange();
}

status_t AmPhraseEvent::SetEventStartTime(AmEvent* event, AmTime newTime)
{
	if (!mPhrase) return B_ERROR;
	return mPhrase->SetEventStartTime(event, newTime);
}

status_t AmPhraseEvent::SetEventEndTime(AmEvent* event, AmTime newTime)
{
	if (!mPhrase) return B_ERROR;
	return mPhrase->SetEventEndTime(event, newTime);
}

AmEventParent* AmPhraseEvent::Parent() const
{
	return mParent;
}

void AmPhraseEvent::SetParent(AmEventParent* parent)
{
	mParent = parent;
}

void AmPhraseEvent::Invalidate(	AmEvent* changedEvent,
								AmRange oldRange, AmRange newRange)
{
	if (mParent) mParent->Invalidate(changedEvent, oldRange, newRange);
}

void AmPhraseEvent::TimeRangeDirty()
{
	if (mParent) mParent->TimeRangeDirty();
}

bool AmPhraseEvent::IsEmpty() const
{
	if (!mPhrase) return true;
	return mPhrase->IsEmpty();
}

AmPhrase* AmPhraseEvent::Phrase()
{
	return mPhrase;
}

const AmPhrase* AmPhraseEvent::Phrase() const
{
	return mPhrase;
}

status_t AmPhraseEvent::GetAsMessage(BMessage& msg) const
{
	status_t	err = AmEvent::GetAsMessage(msg);
	if (err != B_OK) return err;
	add_time(msg, "start_time", StartTime() );
	add_time(msg, "end_time",   EndTime() );
	return B_OK;
}

void AmPhraseEvent::Print() const
{
	if (!mPhrase) {
		printf("AmPhraseEvent at %lld with no phrase\n", mTime);
		return;
	}
	printf("AmPhraseEvent at %lld phrase start: %lld end: %lld -- (%ld events) %p\n",
			mTime, mPhrase->StartTime(), mPhrase->EndTime(), mPhrase->CountNodes(), this );
	mPhrase->PrintNoChains(true, 1);
}

#if 0
AmPhraseEvent& AmPhraseEvent::operator=(const AmPhraseEvent& o)
{
	AmEvent::operator=(o);
	if (mPhrase && o.Phrase() ) {
		AmNode*		head = o.Phrase()->HeadNode();
		if (head) head = head->Copy();
		mPhrase->SetList(head);
	}
	return *this;
}
#endif

#if 0
AmTime AmPhraseEvent::Time() const
{
	return mPhrase->StartTime();
}

AmTime AmPhraseEvent::EndTime() const
{
	return mPhrase->EndTime();
}
#endif

AmPhraseEvent::AmPhraseEvent()
		: inherited(0), mPhrase(NULL)
{
}

AmPhraseEvent::AmPhraseEvent(AmTime startTime)
		: inherited(startTime), mPhrase(NULL)
{
}

AmPhraseEvent::AmPhraseEvent(const AmPhraseEvent& o)
		: inherited(o), mPhrase(NULL)
{
}

AmPhraseEvent::AmPhraseEvent(const BMessage& flatEvent)
		: AmEvent(flatEvent)
{
}

void AmPhraseEvent::RealDelete()
{
//	debugger("Delete ze phrase\n");
	delete this;
}

// #pragma mark -

/***************************************************************
 * AM-ROOT-PHRASE-EVENT
 ***************************************************************/
AmRootPhraseEvent::AmRootPhraseEvent()
{
	mPhrase = new AmPhrase();
	if (mPhrase) mPhrase->SetParent(this);
}

AmRootPhraseEvent::AmRootPhraseEvent(const AmPhraseEvent& o)
		: inherited(o)
{
	if ( o.Phrase() ) mPhrase = new AmPhrase(*(o.Phrase()));
	else mPhrase = new AmPhrase();
	if (mPhrase) mPhrase->SetParent(this);
}

AmRootPhraseEvent::AmRootPhraseEvent(const AmPhrase& o)
{
	mPhrase = new AmPhrase(o);
	if (mPhrase) mPhrase->SetParent(this);
}

AmRootPhraseEvent::AmRootPhraseEvent(const BMessage& flatEvent)
		: inherited(flatEvent)
{
	mPhrase = new AmPhrase();
	if (mPhrase) {
		mPhrase->SetParent(this);
		BMessage	msg;
		for (int32 k = 0; flatEvent.FindMessage("event", k, &msg) == B_OK; k++) {
			AmEvent*	e = am_get_as_event(msg);
			if (e) mPhrase->Add(e);
			msg.MakeEmpty();
		}
	}
}

AmRootPhraseEvent::~AmRootPhraseEvent()
{
	if (mPhrase) mPhrase->DeleteEvents();
	delete mPhrase;
	mPhrase = NULL;
}

AmTime AmRootPhraseEvent::StartTime() const
{
	return mPhrase->StartTime();
}

AmTime AmRootPhraseEvent::EndTime() const
{
	return mPhrase->EndTime();
}

void AmRootPhraseEvent::SetStartTime(AmTime newTime)
{
	ASSERT(newTime >= 0);
	if (mPhrase) mPhrase->SetStartTime(newTime);
}

status_t AmRootPhraseEvent::Add(AmEvent* event)
{
	if (!mPhrase) return B_ERROR;
	return mPhrase->Add(event);
}

status_t AmRootPhraseEvent::Remove(AmEvent* event)
{
	if (!mPhrase) return B_ERROR;
	return mPhrase->Remove(event);
}

AmEvent* AmRootPhraseEvent::Copy() const
{
	return new AmRootPhraseEvent(*this);
}

status_t AmRootPhraseEvent::GetAsMessage(BMessage& msg) const
{
	if (AmEvent::GetAsMessage(msg) != B_OK) return B_ERROR;
	if (!mPhrase) return B_OK;
	AmNode*		n = mPhrase->HeadNode();
	while (n) {
		BMessage	eventMsg('evnt');
		if (n->Event()->GetAsMessage(eventMsg) == B_OK)
			msg.AddMessage("event", &eventMsg);
		n = n->next;
	}
	return B_OK;
}

static bool equal_phrases(AmPhrase* phrase1, AmPhrase* phrase2)
{
	if (!phrase1 && !phrase2) return true;
	else if (!phrase1 || !phrase2) return false;
	AmNode*		n1 = phrase1->HeadNode();
	AmNode*		n2 = phrase2->HeadNode();
	while (n1 && n2) {
		ArpASSERT(n1->Event() && n2->Event());
		if (n1->Event()->Equals(n2->Event()) == false) return false;
		n1 = n1->next;
		n2 = n2->next;
	}
	if (n1 || n2) return false;
	return true;
}

bool AmRootPhraseEvent::Equals(AmEvent* event) const
{
	ArpASSERT(event);
	if ( Type() != event->Type() ) return false;
	if ( Subtype() != event->Subtype() ) return false;
	AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(event);
	if (!pe) return false;
	return equal_phrases(mPhrase, pe->Phrase());
}

void AmRootPhraseEvent::Print() const
{
	if (!mPhrase) {
		printf("AmRootPhraseEvent at %lld with no phrase\n", mTime);
		return;
	}
	printf("AmRootPhraseEvent at %lld phrase start: %lld end: %lld -- (%ld events) %p\n",
			mTime, mPhrase->StartTime(), mPhrase->EndTime(), mPhrase->CountNodes(), this );
	mPhrase->PrintNoChains(true, 1);
}

AmRootPhraseEvent& AmRootPhraseEvent::operator=(const AmRootPhraseEvent& o)
{
	inherited::operator=(o);
	if (mPhrase && o.Phrase() ) {
		AmNode*		head = o.Phrase()->HeadNode();
		if (head) head = head->Copy();
		mPhrase->SetList(head);
	}
	return *this;
}

// #pragma mark -

/***************************************************************
 * AM-BANK-CHANGE
 ***************************************************************/
AmBankChange::AmBankChange(uint32 bankNumber, const char* name)
		: mBankNumber(bankNumber), mName(name)
{
	mPhrase = new AmPhrase();
	if (mPhrase) mPhrase->SetParent(this);
}

AmBankChange::AmBankChange(const AmBankChange& o)
		: inherited(o)
{
	if ( o.Phrase() ) mPhrase = new AmPhrase(*(o.Phrase()));
	else mPhrase = new AmPhrase();
	if (mPhrase) mPhrase->SetParent(this);
	mBankNumber = o.BankNumber();
	mName = o.Name();
}

AmBankChange::AmBankChange(const BMessage& flatEvent)
		: inherited(flatEvent)
{
}

uint32 AmBankChange::BankNumber() const
{
	return mBankNumber;
}

void AmBankChange::SetBankNumber(uint32 bankNumber)
{
	mBankNumber = bankNumber;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

const char* AmBankChange::Name() const
{
	return mName.String();
}

void AmBankChange::SetName(const char* name)
{
	mName = name;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

void AmBankChange::Set(uint32 bankNumber, const char* name)
{
	mBankNumber = bankNumber;
	mName = name;
	AmRange		range = TimeRange();
	Invalidate(this, range, range);
}

static void copy_into(AmEvent* source, AmEvent* target)
{
	ArpASSERT(source && target);
	if (!source || !target) return;
	if (source->Type() == source->CONTROLCHANGE_TYPE
			&& target->Type() == target->CONTROLCHANGE_TYPE) {
		AmControlChange*	sourceE = dynamic_cast<AmControlChange*>(source);
		AmControlChange*	targetE = dynamic_cast<AmControlChange*>(target);
		if (sourceE && targetE) {
			targetE->SetControlNumber(sourceE->ControlNumber() );
			targetE->SetControlValue(sourceE->ControlValue() );
		}
	}
}

status_t AmBankChange::SetTo(ArpCRef<AmBankI> bank)
{
	if (!Phrase() ) return B_ERROR;
	
	AmEvent*			newSelection = bank->NewBankSelection();
	if (!newSelection) return B_NO_MEMORY;
	AmBankChange*		newBe = dynamic_cast<AmBankChange*>(newSelection);
	if (!newBe) {
		newSelection->Delete();
		return B_ERROR;
	}

	/* Not sure if there is a better way to do this.  Right now,
	 * I assume that all bank changes in a device have the same
	 * 'makeup' -- i.e., control changes are in the same position.
	 * If that's ever false, then this won't work (although every
	 * other sequencer makes the assumption, so it seems pretty
	 * safe, plus I guess I could enforce it in the UI, too --  in
	 * fact, enforcing it might be a nice user convenience).
	 */
	AmNode*			sourceN = newBe->Phrase()->HeadNode();
	AmNode*			targetN = Phrase()->HeadNode();
	while (sourceN && targetN) {
		/* Preserve program changes.
		 */
		if (sourceN->Event()->Type() != sourceN->Event()->PROGRAMCHANGE_TYPE) {
			copy_into(sourceN->Event(), targetN->Event());
		}
		sourceN = sourceN->next;
		targetN = targetN->next;
	}
	Set(newBe->BankNumber(), newBe->Name() );

	newSelection->Delete();
	return B_OK;
}

AmProgramChange* AmBankChange::ProgramChange() const
{
	if (!Phrase() ) return NULL;
	AmNode*			node = Phrase()->HeadNode();
	while (node) {
		if (!node->next && node->Event() && node->Event()->Type() == AmEvent::PROGRAMCHANGE_TYPE)
			return dynamic_cast<AmProgramChange*>(node->Event() );
		node = node->next;
	}
	return NULL;
}

bool AmBankChange::Matches(const AmBankChange* be, bool exactly) const
{
	ArpASSERT(be);
	if (!mPhrase || !be->Phrase() ) return false;
	return Matches(be->Phrase()->HeadNode(), be->Phrase()->TailNode(), exactly);
}

static bool events_match(const AmEvent* e1, const AmEvent* e2, bool exactly)
{
	if (e1->Type() != e2->Type() ) return false;
	if (e1->Type() == e1->CONTROLCHANGE_TYPE) {
		const AmControlChange*	cc1 = dynamic_cast<const AmControlChange*>(e1);
		const AmControlChange*	cc2 = dynamic_cast<const AmControlChange*>(e2);
		if (!cc1 || !cc2) return false;
		if (cc1->ControlNumber() != cc2->ControlNumber() ) return false;
		if (exactly && cc1->ControlValue() != cc2->ControlValue() ) return false;
	} else if (exactly && e1->Type() == e1->PROGRAMCHANGE_TYPE) {
		const AmProgramChange*	pc1 = dynamic_cast<const AmProgramChange*>(e1);
		const AmProgramChange*	pc2 = dynamic_cast<const AmProgramChange*>(e2);
		if (!pc1 || !pc2) return false;
		if (pc1->ProgramNumber() != pc2->ProgramNumber() ) return false;
	}
	return true;
}

bool AmBankChange::Matches(const AmNode* head, const AmNode* tail, bool exactly) const
{
	const AmNode*	h1 = mPhrase->HeadNode();
	const AmNode*	h2 = head;
	while (h1 || h2) {
		if (!h1 || !h2) return false;
		if (!h1->Event() || !h2->Event() ) return false;
		/* The final program change in the bank event is actual data, and plays
		 * no role in actually selecting the bank.  So I never want to do an
		 * exact match in that case.
		 */
		if (!h1->next && h1->Event()->Type() == AmEvent::PROGRAMCHANGE_TYPE)
			exactly = false;

		if (!events_match(h1->Event(), h2->Event(), exactly)) return false;

		h1 = h1->next;
		if (h2 == tail) h2 = NULL;
		else h2 = h2->next;
	}
	return true;
}

AmEvent* AmBankChange::Copy() const
{
	return new AmBankChange(*this);
}

BView* AmBankChange::NewView(ViewType type, BRect frame) const
{
	if (type == INSPECTOR_VIEW) return new AmBankChangeInspector(frame);
	else return NULL;
}

void AmBankChange::Print() const
{
	if (!mPhrase) {
		printf("AmBankChange %s at %lld with no phrase\n", mName.String(), mTime);
		return;
	}
	printf("AmBankChange %s at %lld phrase start: %lld end: %lld -- (%ld events) %p\n",
			mName.String(), mTime, mPhrase->StartTime(),
			mPhrase->EndTime(), mPhrase->CountNodes(), this);
	mPhrase->PrintNoChains(true, 1);
}
// #pragma mark -

/***************************************************************
 * AM-ROOT-PHRASE-EVENT
 ***************************************************************/
AmInnerPhraseEvent::AmInnerPhraseEvent()
{
	mPhrase = new AmPhrase();
	if (mPhrase) mPhrase->SetParent(this);
}

AmInnerPhraseEvent::AmInnerPhraseEvent(const AmInnerPhraseEvent& o)
		: inherited(o)
{
	if ( o.Phrase() ) mPhrase = new AmPhrase(*(o.Phrase()));
	else mPhrase = new AmPhrase();
	if (mPhrase) mPhrase->SetParent(this);
}

AmInnerPhraseEvent::AmInnerPhraseEvent(const AmPhrase& o)
		: inherited(o)
{
}

AmInnerPhraseEvent::AmInnerPhraseEvent(const BMessage& flatEvent)
		: inherited(flatEvent)
{
}

AmEvent* AmInnerPhraseEvent::Copy() const
{
	return new AmInnerPhraseEvent(*this);
}

void AmInnerPhraseEvent::Print() const
{
	if (!mPhrase) {
		printf("AmInnerPhraseEvent at %lld with no phrase\n", mTime);
		return;
	}
	printf("AmInnerPhraseEvent at %lld phrase start: %lld end: %lld -- (%ld events) %p\n",
			mTime, mPhrase->StartTime(), mPhrase->EndTime(), mPhrase->CountNodes(), this );
	mPhrase->PrintNoChains(true, 1);
}

AmInnerPhraseEvent& AmInnerPhraseEvent::operator=(const AmInnerPhraseEvent& o)
{
	inherited::operator=(o);
	return *this;
}

// #pragma mark -

/***************************************************************
 * AM-LINK-EVENT
 ***************************************************************/
AmLinkEvent::AmLinkEvent()
{
}

AmLinkEvent::AmLinkEvent(AmTime startTime, AmPhrase* phrase)
		: inherited(startTime)
{
	mPhrase = phrase;
//	if (mPhrase) mPhrase->Link(this);
}

AmLinkEvent::~AmLinkEvent()
{
//	if (mPhrase) mPhrase->Unlink(this);
}

AmTime AmLinkEvent::TimeOffset() const
{
	if (!mPhrase) return 0;
	return mPhrase->StartTime() - StartTime();
}

AmTime AmLinkEvent::EndTime() const
{
/* Fix -- move things to duration - based.
 */
	if (!mPhrase) return StartTime();
	AmRange		range( mPhrase->TimeRange() );
	if ( !range.IsValid() ) return StartTime();
	return StartTime() + (range.end - range.start);
}

AmRange AmLinkEvent::EventRange(const AmEvent* event) const
{
	AmRange		eventRange = event->TimeRange();
	if (!mPhrase) return eventRange;
	AmTime		offset = StartTime() - mPhrase->StartTime();
	eventRange.start += offset;
	eventRange.end += offset;
	return eventRange;
}

status_t AmLinkEvent::SetEventStartTime(AmEvent* event, AmTime newTime)
{
	if (!mPhrase) return B_ERROR;
	AmTime		offset = mPhrase->StartTime() - StartTime();
	return mPhrase->SetEventStartTime(event, newTime + offset);
}

status_t AmLinkEvent::SetEventEndTime(AmEvent* event, AmTime newTime)
{
	if (!mPhrase) return B_ERROR;
	AmTime		offset = mPhrase->StartTime() - StartTime();
	return mPhrase->SetEventEndTime(event, newTime + offset);
}

status_t AmLinkEvent::Add(AmEvent* event)
{
debugger("Not yet");
	if (!mPhrase) return B_ERROR;
	return mPhrase->Add(event);
}

status_t AmLinkEvent::Remove(AmEvent* event)
{
debugger("Not yet");
	if (!mPhrase) return B_ERROR;
	return mPhrase->Remove(event);
}

AmEvent* AmLinkEvent::Copy() const
{
	return new AmLinkEvent(*this);
}

bool AmLinkEvent::Equals(AmEvent* event) const
{
	ArpASSERT(event);
	if ( Type() != event->Type() ) return false;
	if ( Subtype() != event->Subtype() ) return false;
	AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>(event);
	if (!pe) return false;
	return mPhrase == pe->Phrase();
}

void AmLinkEvent::Print() const
{
	if (!mPhrase) {
		printf("AmLinkEvent at %lld with no phrase\n", mTime);
		return;
	}
	printf("AmLinkEvent at %lld phrase start: %lld end: %lld -- (%ld events) %p\n",
			mTime, mPhrase->StartTime(), mPhrase->EndTime(), mPhrase->CountNodes(), this );
}

void AmLinkEvent::AddedToPhrase()
{
	if (mPhrase) mPhrase->Link(this);
}

void AmLinkEvent::RemovedFromPhrase()
{
	if (mPhrase) mPhrase->Unlink(this);
}

AmLinkEvent& AmLinkEvent::operator=(const AmLinkEvent& o)
{
	inherited::operator=(o);
	if (mPhrase) mPhrase->Unlink(this);
	mPhrase = const_cast<AmPhrase*>( o.Phrase() );
	if (mPhrase) mPhrase->Link(this);
	return *this;
}
