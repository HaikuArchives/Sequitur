/* ArpPandemic.cpp
 */
#include "ArpPandemic.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <be/interface/MenuField.h>
#include <be/interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpRangeControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "AmPublic/AmSongObserver.h"

ArpMOD();
static AmStaticResources gRes;

static const char* MIN_VALUE_STR		= "min_value";
static const char* MAX_VALUE_STR		= "max_value";
static const char* DENSITY_STR			= "density";

/*****************************************************************************
 * ARP-PANDEMIC-FILTER
 *****************************************************************************/
ArpPandemicFilter::ArpPandemicFilter(	ArpPandemicFilterAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mPrevControlValue(64),
		  mControlNumber(10), mMinValue(0), mMaxValue(127),
		  mMultiplier(1), mQuantize(1), mEighths(2), mDensity(42)
{
	if (config) PutConfiguration(config);
}

ArpPandemicFilter::~ArpPandemicFilter()
{
}

AmEvent* ArpPandemicFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	if (event->Type() != event->NOTEON_TYPE) return event;

	AmTime		duration = mMultiplier * ((mQuantize*2)/mEighths);
	if (duration <= 1)  {
		AmControlChange*	cc = new AmControlChange( mControlNumber, RandomControlValue(), event->StartTime() );
		if (!cc) return event;
		event->AppendEvent(cc);
		return event;
	}
	
	AmTime		density = mDensity * 10;
	int32		valueSteps = (duration / density) + 1, count = 1;
	uint8		controlValue = RandomControlValue();
	float		valueMult = ( (float)(controlValue - mPrevControlValue) / (float)valueSteps );
	AmEvent*	tail = event;
	for (AmTime k = 0; k < duration; k += density) {
		uint8	newControlValue = (uint8) (mPrevControlValue + (count * valueMult));
		AmControlChange*	cc = new AmControlChange(mControlNumber, newControlValue, event->StartTime() + k);
		if (cc) {
			cc->SetNextFilter(mHolder->FirstConnection() );
			tail->AppendEvent(cc);
			tail = cc;
		}
		count++;
	}
	AmControlChange*	cc = new AmControlChange( mControlNumber, controlValue, event->StartTime() + duration );
	if (cc) {
		cc->SetNextFilter(mHolder->FirstConnection() );
		tail->AppendEvent(cc);
	}
	mPrevControlValue = controlValue;
	return event;
}

status_t ArpPandemicFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ( (err = values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumber)) != B_OK) return err;
	if ( (err = values->AddInt32(MIN_VALUE_STR, mMinValue)) != B_OK) return err;
	if ( (err = values->AddInt32(MAX_VALUE_STR, mMaxValue)) != B_OK) return err;
	if ( (err = values->AddInt32(AM_MULTIPLIER_CONTROL_KEY_STR, mMultiplier)) != B_OK ) return err;
	if ( (err = add_time(*values, AM_QUANTIZE_CONTROL_KEY_STR, mQuantize)) != B_OK ) return err;
	if ( (err = values->AddInt32(AM_EIGHTHS_CONTROL_KEY_STR, mEighths)) != B_OK ) return err;
	if ( (err = values->AddInt32(DENSITY_STR, mDensity)) != B_OK) return err;

	return B_OK;
}

status_t ArpPandemicFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	AmTime		t;
	int32		i;
	if (values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, &i) == B_OK) mControlNumber = (uint8)i;
	if (values->FindInt32(MIN_VALUE_STR, &i) == B_OK) mMinValue = (uint8)i;
	if (values->FindInt32(MAX_VALUE_STR, &i) == B_OK) mMaxValue = (uint8)i;
	if (mMinValue > mMaxValue) {
		uint8	swap = mMinValue;
		mMinValue = mMaxValue;
		mMaxValue = swap;
	}

	if (values->FindInt32(AM_MULTIPLIER_CONTROL_KEY_STR, &i) == B_OK) mMultiplier = i;
	if (find_time(*values, AM_QUANTIZE_CONTROL_KEY_STR, &t) == B_OK) mQuantize = t;
	if (values->FindInt32(AM_EIGHTHS_CONTROL_KEY_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mEighths = i;
	}
	if (values->FindInt32(DENSITY_STR, &i) == B_OK) mDensity = i;

	return B_OK;
}

class ArpPandemicSettings : public AmFilterConfigLayout
{
public:
	ArpPandemicSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		try {
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
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new AmControlChangeListPanel("Control Changes", this, initSettings))
					->SetParams(ArpMessage()
						.SetBool(ArpScrollArea::ScrollHorizontalP,false)
						.SetBool(ArpScrollArea::ScrollVerticalP,true)
						.SetBool(ArpScrollArea::InsetCornerP,false)
						.SetInt32(ArpScrollArea::BorderStyleP,B_FANCY_BORDER)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										MIN_VALUE_STR, "Min value:",
										mImpl.AttachControl(MIN_VALUE_STR),
										0, 127))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										MAX_VALUE_STR, "Max value:",
										mImpl.AttachControl(MAX_VALUE_STR),
										0, 127))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new AmDurationControl("duration", "Duration:", this, initSettings))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										DENSITY_STR, "Density:",
										mImpl.AttachControl(DENSITY_STR),
										1, 400))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
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

status_t ArpPandemicFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpPandemicSettings(mHolder, config));
	return B_OK;
}

uint8 ArpPandemicFilter::RandomControlValue() const
{
	float		r = floor( ( drand48() * (mMaxValue - mMinValue) ) + mMinValue );
	if( r < mMinValue ) r = mMinValue;
	else if( r > mMaxValue ) r = mMaxValue;
	return (uint8)r;
}

/*****************************************************************************
 * ARP-PANDEMIC-FILTER-ADDON
 *****************************************************************************/
void ArpPandemicFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I generate random control changes with each note I receive. By default,
	I generate a single random pan value between 0 and 127. The range can be constrained
	by setting the min value and max value properties. If the duration is greater than one,
	a series of control changes will be generated, that sweep from the previously generated
	value to the new random value. The density control sets how many controllers are generated 
	during the sweep -- lower values generate more controllers.</p>";
}

void ArpPandemicFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpPandemicFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpPandemicFilterAddOn::NewInstance(	AmFilterHolderI* holder,
													const BMessage* config)
{
	return new ArpPandemicFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpPandemicFilterAddOn(cookie);
	return NULL;
}
