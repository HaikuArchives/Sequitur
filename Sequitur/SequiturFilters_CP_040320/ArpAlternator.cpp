/* ArpAlternate.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpAlternator.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-FILTER
 *****************************************************************************/
ArpAlternateFilter::ArpAlternateFilter(ArpAlternateAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder)
{
	mSeed = int32(system_time()/100);
	if (config) PutConfiguration(config);
}

ArpAlternateFilter::~ArpAlternateFilter()
{
}

AmEvent* ArpAlternateFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	ArpVALIDATE(event && mHolder, return event);
	
	if( event->Type() == event->NOTEON_TYPE ) {
		if(NULL != event->NextEvent())
			printf("Alternator: found next event\n");
		else
			printf("Alternator: no next event\n");
		
		uint32 count = mHolder->CountConnections();
		//uint32 dest;
		lastOut++;
		if(lastOut >= count)
			lastOut = 0;
		//AmFilterHolderI* h = mHolder->ConnectionAt(rand() % count);
		AmFilterHolderI* h = mHolder->ConnectionAt(lastOut);
		if(!h)
			h = mHolder->FirstConnection();
		event->SetNextFilter(h);
	}
	else { // copy all other events to all connections
		uint32 count = mHolder->CountConnections();
		bool usedFirst = false;
		for(uint32 i=0; i<count; i++) {
			AmFilterHolderI* h = mHolder->ConnectionAt(i);
			if(h) {
				if(!usedFirst) {
					usedFirst = true;
					event->SetNextFilter(h);
				}
				else {
					AmEvent* e = event->Copy();
					if(e) {
						e->SetNextFilter(h);
						event->AppendEvent(e);
					}
				}
			}
		}
	}
	return event;
}

status_t ArpAlternateFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	return B_OK;
}

status_t ArpAlternateFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	return B_OK;
}

status_t ArpAlternateFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	return B_OK;
}

void ArpAlternateFilter::Start(uint32 context)
{
	if (context&TOOL_CONTEXT) {
		mSeed = int32(system_time()/100);
		srand(mSeed);
	}
	lastOut = 0;
}

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-ADD-ON
 *****************************************************************************/
ArpAlternateAddOn::ArpAlternateAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpAlternateAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>Sends note events alternatingly to all connections.</p>All other event types are copied to all connections.</p>";
}

void ArpAlternateAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 0;
	*minor = 2;
}

BBitmap* ArpAlternateAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpAlternateAddOn(cookie);
	return NULL;
}
