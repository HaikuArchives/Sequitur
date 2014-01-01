/* ArpMerge.cpp
 */
#include <stdio.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpMerge.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-MERGE-FILTER
 *****************************************************************************/
ArpMergeFilter::ArpMergeFilter(	ArpMergeFilterAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpMergeFilter::~ArpMergeFilter()
{
}

AmEvent* ArpMergeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	ArpVALIDATE(event != NULL && mHolder != NULL, return event);
	event->SetNextFilter(mHolder->FirstConnection() );
	return event;
}

/*****************************************************************************
 * ARP-MERGE-FILTER-ADDON
 *****************************************************************************/
void ArpMergeFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I am a simple passthrough.  I merge together input from multiple
	sources.</P>";
}

void ArpMergeFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpMergeFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpMergeFilterAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpMergeFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpMergeFilterAddOn(cookie);
	return NULL;
}
