/* Dissolve.cpp
 */
#include "ArpDissolve.h"

#ifndef AMKERNEL_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>

static AmStaticResources gRes;

DissolveFilter::DissolveFilter(	DissolveFilterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
}

DissolveFilter::~DissolveFilter()
{
}


AmEvent* DissolveFilter::HandleEvent(AmEvent* event, const am_filter_params* )
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	event->SetNextFilter(mHolder->FirstConnection() );
	if (event->Type() == event->NOTEON_TYPE || event->Type() == event->NOTEOFF_TYPE) {
	
		AmNoteOn* note;
		note = dynamic_cast<AmNoteOn*>( event ); /* casting event to AmNoteOn */
		if( !note ) return event;
	
		AmTime stoppingCondition = note->Duration() / 14;
		if (stoppingCondition <= 0) return event;
		
		AmNoteOn* nextNote = dynamic_cast<AmNoteOn*>( note->Copy() );
		AmNoteOn* prevNote = note;
	
		while ( nextNote != NULL && nextNote->Duration() >= stoppingCondition ) {
			if (nextNote->Duration() <= 0) {
				nextNote->Delete();
				return event;
			}
//			if ( prevNote->Duration() < 4 ) return event;	
			nextNote->SetDuration(prevNote->Duration () * 0.75 );
			nextNote->SetStartTime(prevNote->EndTime() + 1);
			nextNote->SetVelocity(prevNote->Velocity() * 0.80);
			nextNote->SetNote(prevNote->Note() - 1);
			prevNote->AppendEvent(nextNote);
			prevNote = nextNote;
			nextNote = dynamic_cast<AmNoteOn*> ( nextNote->Copy() );
		}
	}
	return event;
}

status_t DissolveFilter::GetConfiguration(BMessage* values) const
{
	return AmFilterI::GetConfiguration(values);
}

status_t DissolveFilter::PutConfiguration(const BMessage* values)
{
	return AmFilterI::PutConfiguration(values);
}

/*****************************************************************************
 * ARP-DAN-FILTER-ADDON
 *****************************************************************************/
void DissolveFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>1 a: to become dissipated or decomposed.  1 b: to fade away.</P>";
}


void DissolveFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* DissolveFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new DissolveFilterAddOn(cookie);
	return NULL;
}
