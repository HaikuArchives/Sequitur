#include "ArpAccentRandomizer.h"

#ifndef AMPUBLIC_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "ArpLayout/ArpViewWrapper.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

ArpMOD();
static AmStaticResources gRes;


ArpAccentRandomizerFilter::ArpAccentRandomizerFilter(ArpAccentRandomizerFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  mRandomizeAmount(5)
{
	mSeed = int32(system_time()/100);
	if(config)
		PutConfiguration(config);
}

ArpAccentRandomizerFilter::~ArpAccentRandomizerFilter()
{
}

AmEvent* ArpAccentRandomizerFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if(!event)
		return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);
	
	event->SetNextFilter(mHolder->FirstConnection() );
	
	if( (event->Type() == event->NOTEON_TYPE) && (0 < mRandomizeAmount) ) {
		AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
		if(!note)
			return event;
		int32 delta = rand() % mRandomizeAmount;
		if(rand() % 2) // flip sign with 50% chance
			delta *= -1;
		int32 vel = note->Velocity();
		vel += delta;
		if(vel > 127)
			vel = 127;
		if(vel < 0)
			vel = 0;
		note->SetVelocity(vel);
	}
	return event;
}

class ArpAccentRandomizerFilterSettings : public AmFilterConfigLayout
{
public:
	ArpAccentRandomizerFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		float	labelW = -1;
		const BFont*	font = be_plain_font;
		if (font) labelW = font->StringWidth("Randomize Amount:");
		try {
			ArpKnobPanel* randomizePanel = NULL;

			AddLayoutChild((new ArpRunningBar("TopVBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
				->AddLayoutChild((new ArpTextControl(
										SZ_FILTER_LABEL, "Label:","",
										mImpl.AttachTextControl(SZ_FILTER_LABEL)))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "8")
						.SetString(ArpTextControl::PrefTextStringP, "8888888888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpViewWrapper(randomizePanel = new ArpKnobPanel("randomize_amount", "Randomize Amount:",
									mImpl.AttachControl("randomize_amount"),
									0, 25, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC, 3)
						.SetInt32(ArpRunningBar::FillC, ArpEastWest)
					)
				)
				
			);
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
	}
		
protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpAccentRandomizerFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if(B_OK != (err=values->AddInt32("randomize_amount", mRandomizeAmount))) return err;
	return B_OK;
}

status_t ArpAccentRandomizerFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if(B_OK == values->FindInt32("randomize_amount", &i)) mRandomizeAmount = i;
	return B_OK;
}

status_t ArpAccentRandomizerFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpAccentRandomizerFilterSettings(mHolder, config));
	return B_OK;
}

void ArpAccentRandomizerFilter::Start(uint32 context)
{
	if (context&TOOL_CONTEXT) {
		mSeed = int32(system_time()/100);
		srand(mSeed);
	}
}

/* ----------------------------------------------------------------
   ArpAccentRandomizerFilterAddOn Class
   ---------------------------------------------------------------- */
void ArpAccentRandomizerFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>Randomize the velocity of notes by the amount selected in the parameter window.</p>";
}

void ArpAccentRandomizerFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpAccentRandomizerFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpAccentRandomizerFilterAddOn(cookie);
	return NULL;
}
