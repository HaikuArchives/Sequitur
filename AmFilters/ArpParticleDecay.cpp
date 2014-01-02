/* ParticleDecayFilter.cpp
 */
#include "ArpParticleDecay.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#ifndef AMKERNEL_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

static AmStaticResources gRes;

ParticleDecayFilter::ParticleDecayFilter(	ParticleDecayFilterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
}

ParticleDecayFilter::~ParticleDecayFilter()
{
}


class VossRand
{

public:
	VossRand(void)
	{
		srand48((unsigned)time(NULL));
	}	
		
	double GetValue(int32 s, double k)
	{
		double x, r;
		r = drand48();
		x = s*k + (sqrt(1 - pow(k,2.0))) * r;
		return x;
	}
	
}; 

AmEvent* ParticleDecayFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	event->SetNextFilter(mHolder->FirstConnection() );
	double  nextVal, cons=0.65;  // it would be nice to one day make 'cons' user defined.
	int counter;
	
	if (event->Type() == event->NOTEON_TYPE || event->Type() == event->NOTEOFF_TYPE) {
	
		AmNoteOn* note = dynamic_cast<AmNoteOn*>( event );
		if( !note ) return event;

		AmNoteOn* nextNote = dynamic_cast<AmNoteOn*>( note->Copy() );
		
		if (nextNote->Duration() <= 0) {
			nextNote->Delete();
			return event;
		}

		AmNoteOn* prevNote = note;
	//	AmNoteOn* firstNote = note;
	
		VossRand pn;
	
		for ( counter = 1; counter <= 10; counter++ ) { //also this user definable
			nextVal = pn.GetValue(prevNote->Note(), cons); 
			nextNote->SetStartTime(prevNote->EndTime() + 1);
			nextNote->SetNote(int(nextVal));
			prevNote->AppendEvent(nextNote);
			prevNote = nextNote;
			nextNote = dynamic_cast<AmNoteOn*> ( nextNote->Copy() );
		}
	}
		return event;
}

status_t ParticleDecayFilter::GetConfiguration(BMessage* values) const
{
	return AmFilterI::GetConfiguration(values);
}

status_t ParticleDecayFilter::PutConfiguration(const BMessage* values)
{
	return AmFilterI::PutConfiguration(values);
}

/*****************************************************************************
 * ParticleDecay-FILTER-ADDON
 *****************************************************************************/
void ParticleDecayFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>Successive notes asymptotically approach zero. Simulates radioactive beta decay.</p>";
}

void ParticleDecayFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ParticleDecayFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ParticleDecayFilterAddOn(cookie);
	return NULL;
}
