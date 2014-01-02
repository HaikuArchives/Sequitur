#include "ArpTrackStamp.h"

#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 *	_TRACK-STAMP-FILTER-SETTINGS
 *****************************************************************************/
class _TrackStampFilterSettings : public AmFilterConfigLayout
{
public:
	_TrackStampFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings);

private:
	typedef AmFilterConfigLayout inherited;
};

/*****************************************************************************
 *	ARP-TRACK-STAMP-FILTER
 *****************************************************************************/
ArpTrackStampFilter::ArpTrackStampFilter(	ArpTrackStampAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* config)
	: AmFilterI(addon), mAddOn(addon), mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpTrackStampFilter::~ArpTrackStampFilter()
{
}

AmEvent* ArpTrackStampFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);
	event->SetTrackId(mHolder->TrackId());
	return event;
}

status_t ArpTrackStampFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _TrackStampFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-TRACK-STAMP-ADD-ON
 *****************************************************************************/
void ArpTrackStampAddOn::LongDescription(BString& name, BString& str) const
{
	inherited::LongDescription(name, str);
	str << "<p>I stamp each event with the track I reside in.</p>";
}

void ArpTrackStampAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpTrackStampAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpTrackStampAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * _TRACK-STAMP-FILTER-SETTINGS
 *****************************************************************************/
_TrackStampFilterSettings::_TrackStampFilterSettings(	AmFilterHolderI* target,
														const BMessage& initSettings)
		: inherited(target, initSettings)
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
					.SetFloat(ArpRunningBar::WeightC,3)
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
