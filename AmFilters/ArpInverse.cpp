/* ArpInverse.cpp
 */
#include <stdio.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpInverse.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-INVERSE-FILTER
 *****************************************************************************/
ArpInverseFilter::ArpInverseFilter(	ArpInverseFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpInverseFilter::~ArpInverseFilter()
{
}

status_t ArpInverseFilter::PutConfiguration(const BMessage* values)
{
	status_t result = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);
	return result;
}

AmEvent* ArpInverseFilter::HandleEvent(	AmEvent* event,
										const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

static void add_range(int32 val, int32* start, int32* end)
{
	if (*start < 0 || val < *start) *start = val;
	if (*end < 0 || val > *end) *end = val;
}

AmEvent* ArpInverseFilter::HandleBatchEvents(	AmEvent* event,
												const am_filter_params* params,
												const AmEvent* /*lookahead*/)
{
	if (!event) return event;
	AmEvent*	e = event;

	int32		start = -1, end = -1;
	/* Get the range to invert.
	 */
	while (e) {
		if (e->Type() == e->NOTEON_TYPE) {
			AmNoteOn*	no = dynamic_cast<AmNoteOn*>(e);
			if (no) add_range(no->Note(), &start, &end);
		} else if (e->Type() == e->NOTEOFF_TYPE) {
			AmNoteOff*	no = dynamic_cast<AmNoteOff*>(e);
			if (no) add_range(no->Note(), &start, &end);
		}
		e = e->NextEvent();
	}
	if (start < 0 || end < 0) return event;
	/* Do the invert.
	 */
	e = event;
	while (e) {
		if (e->Type() == e->NOTEON_TYPE) {
			AmNoteOn*	no = dynamic_cast<AmNoteOn*>(e);
			if (no) {
				no->SetNote(uint8(end - (no->Note() - start)));
			}
		} else if (e->Type() == e->NOTEOFF_TYPE) {
			AmNoteOff*	no = dynamic_cast<AmNoteOff*>(e);
			if (no) {
				no->SetNote(uint8(end - (no->Note() - start)));
			}
		}
		e = e->NextEvent();
	}
	
	return event;
}

/*****************************************************************************
 * ARP-INVERSE-FILTER-ADDON
 *****************************************************************************/
void ArpInverseFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>This filter inverts the pitches of all notes.&nbsp;";
}

void ArpInverseFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpInverseFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpInverseFilterAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpInverseFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpInverseFilterAddOn(cookie);
	return NULL;
}
