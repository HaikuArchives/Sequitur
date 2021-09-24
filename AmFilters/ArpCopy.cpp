#include "ArpCopy.h"

#include <cstdio>
#include <cstdlib>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ViewStubs.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

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
 * ARP-COPY-FILTER
 *****************************************************************************/
ArpCopyFilter::ArpCopyFilter(ArpCopyFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder)
{
	if (config) PutConfiguration(config);
}

ArpCopyFilter::~ArpCopyFilter()
{
}

AmEvent* ArpCopyFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	ArpVALIDATE(event && mHolder, return event);

	uint32		count = mHolder->CountConnections();
	bool		usedFirst = false;
	for (uint32 k = 0; k < count; k++) {
		AmFilterHolderI*	h = mHolder->ConnectionAt(k);
		if (h) {
			if (!usedFirst) {
				usedFirst = true;
				event->SetNextFilter(h);
			} else {
				AmEvent*	e = event->Copy();
				if (e) {
					e->SetNextFilter(h);
					event->AppendEvent(e);
				}
			}
		}
	}

	return event;
}

status_t ArpCopyFilter::Configure(ArpVectorI<BView*>& panels)
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
void ArpCopyFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I copy all events I receive to all of my outputs.</P>";
}

void ArpCopyFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpCopyFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpCopyFilterAddOn(cookie);
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
		);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}
