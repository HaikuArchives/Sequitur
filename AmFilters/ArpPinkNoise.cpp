/* PinkNoiseFilter.cpp
 */
#include "ArpPinkNoise.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef AMKERNEL_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

static AmStaticResources gRes;

PinkNoiseFilter::PinkNoiseFilter(	PinkNoiseFilterAddOn* addon,
							AmFilterHolderI* holder,
							const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
}

PinkNoiseFilter::~PinkNoiseFilter()
{
}


class VossRand
{
private:
	int firstVal;
	
public:
	VossRand(int note)
	{
		srand48((unsigned)time(NULL));
		this->firstVal = note;
	}	
		
	double GetValue(double s, double k)
	{
		double x, r;
		r = drand48();
		r = int(r*100);
		x = s*k + (sqrt(1 - pow(k,2.0))) * r;
		if (int(x) == 0 || int(x) > 128 ) {
			return this->GetValue(firstVal,0.6);
		} else return x;
	}
}; 

AmEvent* PinkNoiseFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	event->SetNextFilter(mHolder->FirstConnection() );
	double nextVal, cons = 0.6;  // it would be nice to one day make 'cons' user defined.
	
	if( event->Type() == event->NOTEON_TYPE || event->Type() == event->NOTEOFF_TYPE ) {
	
		AmNoteOn* note = dynamic_cast<AmNoteOn*>( event );
		if( !note ) return event;

		AmNoteOn* nextNote = dynamic_cast<AmNoteOn*>( note->Copy() );
		AmNoteOn* prevNote = note;
		
		VossRand pn(note->Note());
	
		for (int k=1; k<=20; k++) {
			if (nextNote->Duration() <= 0) {
				nextNote->Delete();
				return event;
			}
			nextVal = pn.GetValue(prevNote->Note(),cons); 
			nextNote->SetStartTime(prevNote->EndTime() + 1);
			nextNote->SetNote(int(nextVal));
			prevNote->AppendEvent(nextNote);
			prevNote = nextNote;
			nextNote = dynamic_cast<AmNoteOn*> ( nextNote->Copy() );
		}
	}
	return event;
}


status_t PinkNoiseFilter::GetConfiguration(BMessage* values) const
{
	return AmFilterI::GetConfiguration(values);
}

status_t PinkNoiseFilter::PutConfiguration(const BMessage* values)
{
	return AmFilterI::PutConfiguration(values);
}

/*****************************************************************************
 * PinkNoise-FILTER-ADDON
 *****************************************************************************/
 
void PinkNoiseFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>Successive notes constrained to Voss-McCartney 1/f fractal algorithm.</p>";
}

void PinkNoiseFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* PinkNoiseFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new PinkNoiseFilterAddOn(cookie);
	return NULL;
}
