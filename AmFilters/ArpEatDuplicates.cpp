/* ArpEatDuplicates.cpp
 */
#include <cstdio>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpEatDuplicates.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-EAT-DUPLICATES-FILTER
 *****************************************************************************/
ArpEatDuplicatesFilter::ArpEatDuplicatesFilter(	ArpEatDuplicatesFilterAddOn* addon,
												AmFilterHolderI* holder,
												const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder), mLock("ArpEatDuplicates")
{
	Init(true);
	if (config) PutConfiguration(config);
}

ArpEatDuplicatesFilter::~ArpEatDuplicatesFilter()
{
}

status_t ArpEatDuplicatesFilter::PutConfiguration(const BMessage* values)
{
	status_t result = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);
	return result;
}

AmEvent* ArpEatDuplicatesFilter::StartSection(	AmTime firstTime, AmTime lastTime,
												const am_filter_params* params)
{
	Init();
	return NULL;
}

void ArpEatDuplicatesFilter::Start(uint32 context)
{
	Init();
}

AmEvent* ArpEatDuplicatesFilter::HandleEvent(	AmEvent* event,
												const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

AmEvent* ArpEatDuplicatesFilter::HandleBatchEvents(	AmEvent* event,
													const am_filter_params* params,
													const AmEvent* /*lookahead*/)
{
	BAutolock _l(mLock);
	AmEvent*	result = NULL;
	AmEvent*	pos = NULL;
	AmEvent*	next;
	AmEvent*	gen;

	while (event) {
		next = event->RemoveEvent();
		gen = NULL;
		if (mHolder) event->SetNextFilter(mHolder->FirstConnection() );
		
		if (event->Type() == event->NOTEON_TYPE) {
			AmNoteOn*			e = dynamic_cast<AmNoteOn*>(event);
			if (e) {
				const AmRange		time = e->TimeRange();
				const int32			note = e->Note();
				AmNoteOn*			last = mNotes[note];
				if (last) {
					const AmRange lastTime = last->TimeRange();
					if (lastTime.start != time.start) {
						// This note's start time is different than the last, so
						// generate the last one.
						if (lastTime.start < time.start && lastTime.end > time.end) {
							last->SetEndTime(time.end);
						}
						gen = last;
						mNotes[note] = e;
					} else if (last->Velocity() < e->Velocity()) {
						// The new note is louder than the last; replace the last.
						if (time.end < lastTime.end) event->SetEndTime(lastTime.end);
						last->Delete();
						mNotes[note] = e;
					} else {
						// The new note is softer than the last; eat it.
						if (time.end > lastTime.end) last->SetEndTime(time.end);
						event->Delete();
					}
				} else {
					// Remember this note.
					mNotes[note] = e;
				}
			}
		} else if (event->Type() == event->CONTROLCHANGE_TYPE) {
			AmControlChange*	e = dynamic_cast<AmControlChange*>(event);
			if (e) {
				const AmTime		time = e->StartTime();
				const int32			num = e->ControlNumber();
				const int32			value = e->ControlValue();
				AmControlChange*	last = mControls[num];
				if (last) {
					if (last->ControlValue() == value) {
						// Same as last control change; eat it.
						event->Delete();
					} else if (last->StartTime() == time) {
						// Control change at same time; replace previous time.
						last->Delete();
						mControls[num] = e;
					} else {
						// A change; generate the previous control change.
						gen = last;
						mControls[num] = e;
					}
				} else {
					// Remember this control change.
					mControls[num] = e;
				}
			}
		} else if (event->Type() == event->CHANNELPRESSURE_TYPE) {
			AmChannelPressure*	e = dynamic_cast<AmChannelPressure*>(event);
			if (e) {
				if (mChannelPressure == e->Pressure()) {
					event->Delete();
				} else {
					mChannelPressure = e->Pressure();
					gen = event;
				}
			}
		} else if (event->Type() == event->PITCHBEND_TYPE) {
			AmPitchBend*		e = dynamic_cast<AmPitchBend*>(event);
			if (e) {
				if (mPitch == e->Value()) {
					event->Delete();
				} else {
					mPitch = e->Value();
					gen = event;
				}
			}
		}
		
		if (gen) {
			pos = pos->MergeEvent(gen);
			if (!result) result = pos;
		}
		
		event = next;
	}
	
	// Collect up pending events.
	uint32 k;
	for (k = 0; k < NOTE_SIZE; k++) {
		if (mNotes[k]) {
			pos = pos->MergeEvent(mNotes[k]);
			if (!result) result = pos;
			mNotes[k] = NULL;
		}
	}
	for (k = 0; k < CONTROL_SIZE; k++) {
		if (mControls[k]) {
			pos = pos->MergeEvent(mControls[k]);
			if (!result) result = pos;
			mControls[k] = NULL;
		}
	}
	
	//printf("Final chain:\n");
	//if (result) result->HeadEvent()->PrintChain();
	//else printf("NONE!\n");
	
	if (!result) return result;
	return result->HeadEvent();
}

void ArpEatDuplicatesFilter::Init(bool first)
{
	BAutolock _l(mLock);
	uint32 k;
	for (k = 0; k < NOTE_SIZE; k++) {
		if (!first && mNotes[k]) mNotes[k]->Delete();
		mNotes[k] = NULL;
		mNoteTimes[k].MakeInvalid();
	}
	for (k = 0; k < CONTROL_SIZE; k++) {
		if (!first && mControls[k]) mControls[k]->Delete();
		mControls[k] = NULL;
		mControlValues[k] = -1;
	}
	mPitch = -1;
	mChannelPressure = -1;
}

/*****************************************************************************
 * ARP-EAT-DUPLICATES-FILTER-ADDON
 *****************************************************************************/
void ArpEatDuplicatesFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>This filter deletes the following events:</P>\n"
		"<UL>\n"
		"	<LI><I>Channel pressure events</I>.  Any channel pressure events with the same \n"
		"						value as the previous channel pressure are deleted.</LI>\n"
		"	<LI><I>Control change events</I>.  Any control change events with the same value \n"
		"						as the previous control change are deleted.</LI>\n"
		"	<LI><I>Note events</I>.  Any overlapping notes are deleted.  The note with \n"
		"						the highest velocity is retained.</LI>\n"
		"	<LI><I>Pitch bend events</I>.  Any pitch bend events with the same value as the \n"
		"						previous pitch bend are deleted.</LI>\n"
		"</UL>\n"
		"This filter has no controls.&nbsp;\n";
}

void ArpEatDuplicatesFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpEatDuplicatesFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpEatDuplicatesFilterAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpEatDuplicatesFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpEatDuplicatesFilterAddOn(cookie);
	return NULL;
}
