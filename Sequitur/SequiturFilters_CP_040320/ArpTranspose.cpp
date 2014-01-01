#include "ArpTranspose.h"

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

/*************************************************************************
 * _TRANSPOSE-MAP
 * A class that turns values of 0 into 'Off' and leaves all others alone.
 *************************************************************************/
class _TransposeMap : public ArpIntToStringMapI
{
public:
	_TransposeMap()	{ ; }
	virtual status_t IdForName(const char *name, int32 *answer) const;
	virtual status_t NameForId(int32 id, char **answer) const;
};


ArpTransposeFilter::ArpTransposeFilter(ArpTransposeFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  mOctaves(1), mSteps(0)
{
	if (config) PutConfiguration(config);
}

ArpTransposeFilter::~ArpTransposeFilter()
{
}

AmEvent* ArpTransposeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);

	event->SetNextFilter(mHolder->FirstConnection() );
		
	if( event->Type() == event->NOTEON_TYPE ) {
		AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
		if( !note ) return event;
		
		uint8 pitch = note->Note();
		int32 delta = mOctaves*12+mSteps;
		if (pitch+delta > 0x7f) pitch = 0x7f;
		else if(pitch+delta < 0) pitch = 0;
		else pitch += delta;
		note->SetNote(pitch);
	} else if( event->Type() == event->NOTEOFF_TYPE ) {
		AmNoteOff* note = dynamic_cast<AmNoteOff*>(event);
		if( !note ) return event;
		
		uint8 pitch = note->Note();
		int32 delta = mOctaves*12+mSteps;
		if (pitch+delta > 0x7f) pitch = 0x7f;
		else if(pitch+delta < 0) pitch = 0;
		else pitch += delta;
		note->SetNote(pitch);
	}
	
	return event;
}

class ArpTransposeFilterSettings : public AmFilterConfigLayout
{
public:
	ArpTransposeFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		float	labelW = -1;
		const BFont*	font = be_plain_font;
		if (font) labelW = font->StringWidth("Octave:");
		try {
			ArpKnobPanel*	octavePanel = 0;
			ArpKnobPanel*	stepPanel = 0;

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
				->AddLayoutChild((new ArpViewWrapper(octavePanel = new ArpKnobPanel( "octaves", "Octave:",
													mImpl.AttachControl("octaves"),
													-10, 10, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpViewWrapper(stepPanel = new ArpKnobPanel( "steps", "Step:",
													mImpl.AttachControl("steps"),
													-12, 12, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
			);
			if (octavePanel) {
				ArpIntControl*	intCtrl = octavePanel->IntControl();
				if (intCtrl) intCtrl->SetStringMap(new _TransposeMap() );
			}
			if (stepPanel) {
				ArpIntControl*	intCtrl = stepPanel->IntControl();
				if (intCtrl) intCtrl->SetStringMap(new _TransposeMap() );
			}
		} catch(...) {
			throw;
		}
		
		Implementation().RefreshControls(mSettings);
	}
		
protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpTransposeFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err=values->AddInt32("octaves", mOctaves)) != B_OK) return err;
	if ((err=values->AddInt32("steps", mSteps)) != B_OK) return err;
	return B_OK;
}

status_t ArpTransposeFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if (values->FindInt32("octaves", &i) == B_OK) mOctaves = i;
	if (values->FindInt32("steps", &i) == B_OK) mSteps = i;
	return B_OK;
}

status_t ArpTransposeFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpTransposeFilterSettings(mHolder, config));
	return B_OK;
}

/* ----------------------------------------------------------------
   ArpTransposeFilterAddOn Class
   ---------------------------------------------------------------- */
void ArpTransposeFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I shift notes up or down in pitch. You can control the shift in octaves or steps.</p>";
}

void ArpTransposeFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpTransposeFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpTransposeFilterAddOn(cookie);
	return NULL;
}

/*************************************************************************
 * _TRANSPOSE-MAP
 * A class that turns values of 0 into 'Off' and leaves all others alone.
 *************************************************************************/
status_t _TransposeMap::IdForName(const char *name, int32 *answer) const
{
	if( strcmp(name, "Off") == 0 ) {
		*answer = 0;
		return B_OK;
	}
	*answer = -1;
	return B_ERROR;
}

status_t _TransposeMap::NameForId(int32 id, char **answer) const
{
	if( id == 0 ) {
		*answer = "Off";
		return B_OK;
	}
	return B_ERROR;
}
