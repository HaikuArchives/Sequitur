#include "ArpDianne.h"

#ifndef AMKERNEL_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static AmStaticResources gRes;

/* ----------------------------------------------------------------
   ArpExampleFilter Class
   ---------------------------------------------------------------- */

ArpExampleFilter::ArpExampleFilter(ArpExampleFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpExampleFilter::~ArpExampleFilter()
{
}

AmEvent* ArpExampleFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	
	event->SetNextFilter(mHolder->FirstConnection() );
	AmEvent* head = event;
	
	if( event->Type() == event->NOTEON_TYPE ) {
		AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
		if( !note ) return event;
		
		if( event->StartTime() >= PPQN ) {
			AmEvent* prevEvent = event->Copy();
			prevEvent->SetStartTime(prevEvent->StartTime() - PPQN);
			AmNoteOn* prevNote = dynamic_cast<AmNoteOn*>(prevEvent);
			if( !prevNote ) {
				prevEvent->Delete();
			} else {
				prevNote->SetVelocity(prevNote->Velocity()/2);
				event->InsertEvent(prevEvent);
				head = prevEvent;
			}
		}
		
		AmEvent* nextEvent = event->Copy();
		nextEvent->SetStartTime(nextEvent->StartTime() + PPQN);
		AmNoteOn* nextNote = dynamic_cast<AmNoteOn*>(nextEvent);
		if( !nextNote ) {
			nextEvent->Delete();
		} else {
			nextNote->SetVelocity(nextNote->Velocity()/2);
			event->AppendEvent(nextEvent);
		}
	}
	
	return head;
}

status_t ArpExampleFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	return B_OK;
}

status_t ArpExampleFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	return B_OK;
}

status_t ArpExampleFilter::Configure(ArpVectorI<BView*>& /*panels*/)
{
	return B_OK;
}
	
/* ----------------------------------------------------------------
   ArpExampleFilterAddOn Class
   ---------------------------------------------------------------- */
void ArpExampleFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>In addition to the Dan filter, I am a template for writing
	your own C++ filters.  The source code can be found in the
	<i>DevKit/ExampleFilters/</i> folder.</P>
	
	<P>In previous versions of Sequitur I was the Example filter.  For every
	note I receive, I create a softer note one beat before and one beat after
	the input note.</P>";
}

void ArpExampleFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpExampleFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpExampleFilterAddOn(cookie);
	return NULL;
}
