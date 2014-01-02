/* ArpLegato.cpp
 */
#include <stdio.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLegato.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-LEGATO-FILTER
 *****************************************************************************/
ArpLegatoFilter::ArpLegatoFilter(	ArpLegatoAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder), mFlags(0)
{
	Init();
	if (config) PutConfiguration(config);
}

ArpLegatoFilter::~ArpLegatoFilter()
{
}

AmEvent* ArpLegatoFilter::HandleEvent(	AmEvent* event,
										const am_filter_params* params)
{
	ArpVALIDATE(event != NULL, return event);

	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*			e = dynamic_cast<AmNoteOn*>(event);
		if (e) {
			AmRange			r = e->TimeRange();
			AmRange			r2 = mNotes[e->Note()];
			if (r2.IsValid() && r.Overlaps(r2) ) {
				event->Delete();
				return NULL;
			}
			mNotes[e->Note()] = r;
		}
	}
	
	return event;
}

void ArpLegatoFilter::Start(uint32 context)
{
	Init();
}

void ArpLegatoFilter::Init()
{
	for (uint32 k = 0; k < NOTE_SIZE; k++) mNotes[k].MakeInvalid();
	mLast.MakeInvalid();
}

/*****************************************************************************
 * ARP-LEGATO-ADD-ON
 *****************************************************************************/
void ArpLegatoAddOn::LongDescription(BString& str) const
{
	AmFilterAddOn::LongDescription(BString& name, str);
	str << "This filter allows only one note to play at a time.&nbsp;";
}

void ArpLegatoAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpLegatoAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpLegatoAddOn::NewInstance(	AmFilterHolderI* holder,
										const BMessage* config)
{
	return new ArpLegatoFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpLegatoAddOn(cookie);
	return NULL;
}
