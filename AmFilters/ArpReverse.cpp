/* ArpReverse.cpp
 */
#include <stdio.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpReverse.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-REVERSE-FILTER
 *****************************************************************************/
ArpReverseFilter::ArpReverseFilter(	ArpReverseFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpReverseFilter::~ArpReverseFilter()
{
}

status_t ArpReverseFilter::PutConfiguration(const BMessage* values)
{
	status_t result = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);
	return result;
}

AmEvent* ArpReverseFilter::HandleEvent(	AmEvent* event,
										const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

AmEvent* ArpReverseFilter::HandleBatchEvents(	AmEvent* event,
												const am_filter_params* params,
												const AmEvent* /*lookahead*/)
{
	if (!event) return event;
	AmEvent*	result = 0;
	AmEvent*	e = event;
	AmEvent*	next = 0;

	/* Get the range to reverse.
	 */
	while (e) {
		next = e;
		e = e->NextEvent();
	}
	AmTime		start = event->StartTime(), end = next->StartTime();
	/* Do the reversal.
	 */
	e = event;
	while (e) {
		next = e->RemoveEvent();
		e->SetStartTime(end - (e->StartTime() - start));
		if (!result) result = e;
		else {
			result->SetPrevEvent(e);
			e->SetNextEvent(result);
			result = e;
		}
		e = next;
	}

#if 0
	printf("Final chain:\n");
	if (result) result->HeadEvent()->PrintChain();
	else printf("NONE!\n");
#endif	
		
	if (!result) return result;
	return result->HeadEvent();
}

/*****************************************************************************
 * ARP-REVERSE-FILTER-ADDON
 *****************************************************************************/
void ArpReverseFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>This filter time-reverses all events.&nbsp;";
}

void ArpReverseFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpReverseFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpReverseFilterAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpReverseFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpReverseFilterAddOn(cookie);
	return NULL;
}
