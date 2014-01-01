/* ArpKeyboardSplitter.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <be/InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ViewStubs.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "ArpVelocitySplitter.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		SPLIT_POINT_STR		= "Split Point";

/*****************************************************************************
 *	_COPY-FILTER-SETTINGS
 *****************************************************************************/
class _CopyFilterSettings : public AmFilterConfigLayout
{
public:
	_CopyFilterSettings(AmFilterHolderI* target,
						const BMessage& initSettings);

private:
	typedef AmFilterConfigLayout inherited;
};

/*****************************************************************************
 * ARP-KEYBOARD-SPLITTER-FILTER
 *****************************************************************************/
ArpVelocitySplitterFilter::ArpVelocitySplitterFilter(ArpVelocitySplitterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder), mSplitPoint(64)
{
	if (config) PutConfiguration(config);
}

ArpVelocitySplitterFilter::~ArpVelocitySplitterFilter()
{
}

AmEvent* ArpVelocitySplitterFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	ArpVALIDATE(event && mHolder, return event);
	int32				vel = -1;
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*		e = dynamic_cast<AmNoteOn*>(event);
		if (e) vel = e->Velocity();
	}
	if (vel < 0) return event;

	if (vel >= mSplitPoint) event->SetNextFilter(mHolder->ConnectionAt(0) );
	else event->SetNextFilter(mHolder->ConnectionAt(1) );
	return event;
}

status_t ArpVelocitySplitterFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err=values->AddInt32(SPLIT_POINT_STR, mSplitPoint)) != B_OK) return err;

	return B_OK;
}

status_t ArpVelocitySplitterFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if (values->FindInt32(SPLIT_POINT_STR, &i) == B_OK) mSplitPoint = i;

	return B_OK;
}

status_t ArpVelocitySplitterFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _CopyFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-COPY-FILTER-ADD-ON
 *****************************************************************************/
ArpVelocitySplitterAddOn::ArpVelocitySplitterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpVelocitySplitterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>All notes above the split point continue down my pipeline.  Notes
below the split point are sent to my connection, if any.</p>";
}

void ArpVelocitySplitterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpVelocitySplitterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpVelocitySplitterAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * _COPY-FILTER-SETTINGS
 *****************************************************************************/
_CopyFilterSettings::_CopyFilterSettings(	AmFilterHolderI* target,
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
			->AddLayoutChild((new ArpIntControl(
									SPLIT_POINT_STR, "Split point:",
									mImpl.AttachControl(SPLIT_POINT_STR),
									0, 127))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}
