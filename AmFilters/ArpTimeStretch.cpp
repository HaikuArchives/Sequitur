/* ArpTimeStretch.cpp
 */
#include <stdio.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpTimeStretch.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-TIME-STRETCH-FILTER
 *****************************************************************************/
ArpTimeStretchFilter::ArpTimeStretchFilter(	ArpTimeStretchFilterAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpTimeStretchFilter::~ArpTimeStretchFilter()
{
}

status_t ArpTimeStretchFilter::PutConfiguration(const BMessage* values)
{
	status_t result = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);
	return result;
}

AmEvent* ArpTimeStretchFilter::HandleEvent(	AmEvent* event,
											const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

static double get_multiplier(AmTime range, AmTime cur, float below1Step, float above1Step)
{
	int32		count = 1;
	double		step = count / below1Step;
	while (step < 1) {
		AmTime	t = AmTime(range * step);
		if (cur <= t) return step;
		count++;
		step = count / below1Step;
	}
	if (cur <= range) return 1.0;
	count = 1;
	while (true) {
		step = pow(above1Step, count);
		AmTime	t = AmTime(range * step);
		if (cur <= t) return step;
		count++;
	}
	return 0.0;
}

AmEvent* ArpTimeStretchFilter::HandleBatchToolEvents(	AmEvent* event,
														const am_filter_params* params,
														const am_tool_filter_params* toolParams,
														const AmEvent* /*lookahead*/)
{
	if (!event || !toolParams) return event;

	/* Find value to multiply by.
	 */
	float		below1Step = 2, above1Step = 2;
	AmTime		start = toolParams->start_time;
	AmTime		range = toolParams->end_time - start, cur = toolParams->cur_time - start;
	if (cur < 0) return event;
	double		multiplier = get_multiplier(range, cur, below1Step, above1Step);

	/* Multiply.
	 */
	AmEvent*	e = event;
	while (e) {
		AmRange		r = e->TimeRange();
		e->SetStartTime(start + AmTime(multiplier * (r.start - start)));
		e->SetEndTime(start + AmTime(multiplier * (r.end - start)));
		e = e->NextEvent();
	}

	return event;
}

/*****************************************************************************
 * ARP-TIME-STRETCH-FILTER-ADDON
 *****************************************************************************/
void ArpTimeStretchFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>This filter time stretches all selected events.  Tool only.&nbsp;";
}

void ArpTimeStretchFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpTimeStretchFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpTimeStretchFilterAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpTimeStretchFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpTimeStretchFilterAddOn(cookie);
	return NULL;
}
