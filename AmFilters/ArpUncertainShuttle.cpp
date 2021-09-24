/* ArpUncertainShuttle.cpp
 */
#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpDebug.h"
#include "ArpUncertainShuttle.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-FILTER
 *****************************************************************************/
ArpUncertainShuttleFilter::ArpUncertainShuttleFilter(ArpUncertainShuttleAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder)
{
	mSeed = int32(system_time()/100);
	if (config) PutConfiguration(config);
}

ArpUncertainShuttleFilter::~ArpUncertainShuttleFilter()
{
}

AmEvent* ArpUncertainShuttleFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	ArpVALIDATE(event && mHolder, return event);

	uint32				count = mHolder->CountConnections();
	AmFilterHolderI*	h = mHolder->ConnectionAt(rand() % count);
	if (!h) h = mHolder->FirstConnection();
	event->SetNextFilter(h);
	return event;
}

status_t ArpUncertainShuttleFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	return B_OK;
}

status_t ArpUncertainShuttleFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	return B_OK;
}

status_t ArpUncertainShuttleFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	return B_OK;
}

void ArpUncertainShuttleFilter::Start(uint32 context)
{
	if (context&TOOL_CONTEXT) {
		mSeed = int32(system_time()/100);
		srand(mSeed);
	}
}

/*****************************************************************************
 * ARP-UNCERTAIN-SHUTTLE-ADD-ON
 *****************************************************************************/
ArpUncertainShuttleAddOn::ArpUncertainShuttleAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpUncertainShuttleAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>For every event I receive, I randomly choose one of my"
	"connections and send the event to it.  I can have any number of"
	"connections.</p>";
}

void ArpUncertainShuttleAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpUncertainShuttleAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpUncertainShuttleAddOn(cookie);
	return NULL;
}
