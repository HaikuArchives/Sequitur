#include "ArpMonophobic.h"

#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpDebug.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-MONOPHOBIC-FILTER
 *****************************************************************************/
ArpMonophobicFilter::ArpMonophobicFilter(	ArpMonophobicAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder), mAmount(1)
{
	if (settings) PutConfiguration(settings);
}

ArpMonophobicFilter::~ArpMonophobicFilter()
{
}

AmEvent* ArpMonophobicFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return event;
}

AmEvent* ArpMonophobicFilter::HandleToolEvent(	AmEvent* event, const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	if (!event || !toolParams || !mHolder) return event;

	event->SetNextFilter(mHolder->FirstConnection() );
	AmRange			range(event->TimeRange() );
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	e = dynamic_cast<AmNoteOn*>(event);
		if (e) e->SetNote(NewNote(e->Note(), range, toolParams) );
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff*	e = dynamic_cast<AmNoteOff*>(event);
		if (e) e->SetNote(NewNote(e->Note(), range, toolParams) );
	}
	
	return event;
}

status_t ArpMonophobicFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	return B_OK;
}

status_t ArpMonophobicFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	return B_OK;
}

status_t ArpMonophobicFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	return GetConfiguration(&config);
}

uint8 ArpMonophobicFilter::NewNote(	uint8 oldNote, AmRange noteRange,
									const am_tool_filter_params* toolParams)
{
	float		offset = (toolParams->cur_y_value - toolParams->orig_y_value)
							* am_x_amount(toolParams, noteRange.start + ((noteRange.end - noteRange.start) / 2));
	int32		newNote = int32(oldNote + offset);
	if (newNote < 0) newNote = 0;
	else if (newNote > 127) newNote = 127;
	return uint8(newNote);
}

/*****************************************************************************
 * ARP-MONOPHOBIC-ADD-ON
 *****************************************************************************/
void ArpMonophobicAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I am only available in a tool pipeline.  I cause all selected"
	"note events to follow the mouse as it is dragged up and down.</P>";
}

void ArpMonophobicAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpMonophobicAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(	int32 n, image_id /*you*/,
													const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpMonophobicAddOn(cookie);
	return NULL;
}
