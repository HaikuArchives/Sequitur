/* ArpVelocityMap.cpp
 */
#include "ArpVelocityMap.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char*	FROM_MIN		= "From Min";
static const char*	FROM_MAX		= "From Max";
static const char*	TO_MIN			= "To Min";
static const char*	TO_MAX			= "To Max";

/*****************************************************************************
 * ARP-VELOCITY-MAP-FILTER
 *****************************************************************************/
ArpVelocityMapFilter::ArpVelocityMapFilter(	ArpVelocityMapAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mFromRange(0, 127), mToRange(0, 127)
{
	if (config) PutConfiguration(config);
}

ArpVelocityMapFilter::~ArpVelocityMapFilter()
{
}

static bool in_range(AmNoteOn* event, AmRange range)
{
	uint8		min = range.start, max = range.end;
	if (range.end < range.start) {
		min = range.end;
		max = range.start;
	}
	return event->Velocity() >= min && event->Velocity() <= max;
}

AmEvent* ArpVelocityMapFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	event->SetNextFilter(mHolder->ConnectionAt(0) );
	if (event->Type() != event->NOTEON_TYPE) return event;
	AmNoteOn*		e = dynamic_cast<AmNoteOn*>(event);
	if (!e || !in_range(e, mFromRange) ) return event;

	int32		fromStart = mFromRange.start, fromEnd = mFromRange.end;
	int32		toStart = mToRange.start, toEnd = mToRange.end;
	/* If both the from and start are reversed, but them back to normal.
	 * Otherwise leave them skewed, to allow users to invert the range.
	 */
	if (fromEnd < fromStart && toEnd < toStart) {
		fromStart = mFromRange.end;
		fromEnd = mFromRange.start;
		toStart = mToRange.end;
		toEnd = mToRange.start;
	}
	float		scale = fabs(float(fromEnd - fromStart) / float(toEnd - toStart));
	if (mToRange.end < mToRange.start) scale = -scale;
	int32		newVel = int32(toStart + ((e->Velocity() - fromStart) * scale));
	
	if (toStart < toEnd) {
		if (newVel < toStart) newVel = toStart;
		else if (newVel > toEnd) newVel = toEnd;
	} else {
		if (newVel < toEnd) newVel = toEnd;
		else if (newVel > toStart) newVel = toStart;
	}
	e->SetVelocity(newVel);
	
	return event;
}

status_t ArpVelocityMapFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ( (err = values->AddInt32(FROM_MIN, int32(mFromRange.start) )) != B_OK) return err;
	if ( (err = values->AddInt32(FROM_MAX, int32(mFromRange.end) )) != B_OK) return err;
	if ( (err = values->AddInt32(TO_MIN, int32(mToRange.start) )) != B_OK) return err;
	if ( (err = values->AddInt32(TO_MAX, int32(mToRange.end) )) != B_OK) return err;

	return B_OK;
}

status_t ArpVelocityMapFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32		i;
	if (values->FindInt32(FROM_MIN, &i) == B_OK) mFromRange.start = i;
	if (values->FindInt32(FROM_MAX, &i) == B_OK) mFromRange.end = i;
	if (values->FindInt32(TO_MIN, &i) == B_OK) mToRange.start = i;
	if (values->FindInt32(TO_MAX, &i) == B_OK) mToRange.end = i;

	return B_OK;
}

class ArpVelocityMapSettings : public AmFilterConfigLayout
{
public:
	ArpVelocityMapSettings(AmFilterHolderI* target,
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
				->AddLayoutChild((new ArpIntControl(
										FROM_MIN, "From start:",
										mImpl.AttachControl(FROM_MIN),
										0, 127))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										FROM_MAX, "From end:",
										mImpl.AttachControl(FROM_MAX),
										0, 127))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										TO_MIN, "To start:",
										mImpl.AttachControl(TO_MIN),
										0, 127))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										TO_MAX, "To end:",
										mImpl.AttachControl(TO_MAX),
										0, 127))
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

status_t ArpVelocityMapFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpVelocityMapSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-VELOCITY-MAP-ADDON
 *****************************************************************************/
void ArpVelocityMapAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I take a range of velocities and transform it into a
	new range.  Any velocities out of range are ignored (so use multiple
	maps to get multiple bands of transformation).  I can be used to compress
	(by setting the To values to a wider range than the From values) or expand
	(by doing the opposite).</p>";
}

void ArpVelocityMapAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpVelocityMapAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpVelocityMapAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpVelocityMapFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpVelocityMapAddOn(cookie);
	return NULL;
}
