/* ArpSelective.cpp
 */
#include "ArpSelective.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char*	TYPES_STR		= "Types";

enum {
	NOTE_TYPE				= 0x0001,
	CHANNELPRESSURE_TYPE	= 0x0002,
	CONTROLCHANGE_TYPE		= 0x0004,
	KEYPRESSURE_TYPE		= 0x0008,
	PITCHBEND_TYPE			= 0x0010,
	PROGRAMCHANGE_TYPE		= 0x0020,
	SYSTEMCOMMON_TYPE		= 0x0040,
	SYSTEMEXCLUSIVE_TYPE	= 0x0080,
	SYSTEMREALTIME_TYPE		= 0x0100,
	TEMPOCHANGE_TYPE		= 0x0200,

	DEFAULT_TYPES	=
		NOTE_TYPE|CHANNELPRESSURE_TYPE|CONTROLCHANGE_TYPE|
		KEYPRESSURE_TYPE|PITCHBEND_TYPE|PROGRAMCHANGE_TYPE|
		SYSTEMCOMMON_TYPE|SYSTEMEXCLUSIVE_TYPE|SYSTEMREALTIME_TYPE|
		TEMPOCHANGE_TYPE,
};

/*****************************************************************************
 * ARP-WIPE-OUT-FILTER
 *****************************************************************************/
ArpSelectiveFilter::ArpSelectiveFilter(	ArpSelectiveAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder), mTypeMask(DEFAULT_TYPES)
{
	for (uint8 k = 0; k < 128; k++) mControlNumbers.push_back(k);
	if (config) PutConfiguration(config);
}

ArpSelectiveFilter::~ArpSelectiveFilter()
{
}

static inline AmEvent* _handle(AmEvent* event, AmFilterHolderI* next)
{
	ArpASSERT(event);
	if (!next) {
		event->Delete();
		return 0;
	}
	event->SetNextFilter(next);
	return event;
}

AmEvent* ArpSelectiveFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	event->SetNextFilter(mHolder->ConnectionAt(0) );

	switch (event->Type() ) {
		case AmEvent::NOTEON_TYPE:
		case AmEvent::NOTEOFF_TYPE: {
			if (!(mTypeMask&NOTE_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::CHANNELPRESSURE_TYPE: {
			if (!(mTypeMask&CHANNELPRESSURE_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::CONTROLCHANGE_TYPE: {
			AmControlChange*	cc = dynamic_cast<AmControlChange*>(event);
			if (cc && !IncludesControlNumber(cc->ControlNumber() ) ) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::KEYPRESSURE_TYPE: {
			if (!(mTypeMask&KEYPRESSURE_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::PITCHBEND_TYPE: {
			if (!(mTypeMask&PITCHBEND_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::PROGRAMCHANGE_TYPE: {
			if (!(mTypeMask&PROGRAMCHANGE_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::SYSTEMCOMMON_TYPE: {
			if (!(mTypeMask&SYSTEMCOMMON_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::SYSTEMEXCLUSIVE_TYPE: {
			if (!(mTypeMask&SYSTEMEXCLUSIVE_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		case AmEvent::SYSTEMREALTIME_TYPE: {
			if (!(mTypeMask&SYSTEMREALTIME_TYPE)) event = _handle(event, mHolder->ConnectionAt(1));
		} break;
		default:
			break;
	}

	return event;
}

status_t ArpSelectiveFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ( (err = values->AddInt32(TYPES_STR, mTypeMask)) != B_OK) return err;

	for (uint32 k = 0; k < mControlNumbers.size(); k++) {
		if ((err = values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumbers[k])) != B_OK) return err;
	}

	return B_OK;
}

status_t ArpSelectiveFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32		i;
	if (values->FindInt32(TYPES_STR, &i) == B_OK) mTypeMask = i;
	for (int32 k = 0; values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, k, &i) == B_OK; k++) {
		if (k == 0) mControlNumbers.resize(0);
		mControlNumbers.push_back(uint8(i) );
	}

	return B_OK;
}

bool ArpSelectiveFilter::IncludesControlNumber(uint8 number) const
{
	for (uint32 k = 0; k < mControlNumbers.size(); k++) {
		if (mControlNumbers[k] == number) return true;
	}
	return false;
}

class ArpWipeOutSettings : public AmFilterConfigLayout
{
public:
	ArpWipeOutSettings(AmFilterHolderI* target,
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
				->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpWest)
					)
					->AddLayoutChild((new ArpRunningBar("BoxVGroup1"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, 0)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "notes", "Notes",
									mImpl.AttachCheckBox(TYPES_STR, NOTE_TYPE, "notes"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "cpressure", "Channel Pressure",
									mImpl.AttachCheckBox(TYPES_STR, CHANNELPRESSURE_TYPE, "cpressure"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "kpressure", "Key Pressure",
									mImpl.AttachCheckBox(TYPES_STR, KEYPRESSURE_TYPE, "kpressure"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "pbend", "Pitch Bend",
									mImpl.AttachCheckBox(TYPES_STR, PITCHBEND_TYPE, "pbend"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
					->AddLayoutChild((new ArpRunningBar("BoxVGroup2"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, 0)
						)
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,0)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "pchange", "Program Change",
									mImpl.AttachCheckBox(TYPES_STR, PROGRAMCHANGE_TYPE, "pchange"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "scommon", "System Common",
									mImpl.AttachCheckBox(TYPES_STR, SYSTEMCOMMON_TYPE, "scommon"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "sexclusive", "System Exclusive",
									mImpl.AttachCheckBox(TYPES_STR, SYSTEMEXCLUSIVE_TYPE, "sexclusive"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
						->AddLayoutChild((new ArpViewWrapper(
							new BCheckBox(BRect(0,0,10,10), "srealtime", "System Realtime",
									mImpl.AttachCheckBox(TYPES_STR, SYSTEMREALTIME_TYPE, "srealtime"),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,0)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
				)
				->AddLayoutChild((new AmControlChangeListPanel("Control Changes", this, initSettings,
																AM_CONTROL_CHANGE_KEY_STR, B_MULTIPLE_SELECTION_LIST))
					->SetParams(ArpMessage()
						.SetBool(ArpScrollArea::ScrollHorizontalP,false)
						.SetBool(ArpScrollArea::ScrollVerticalP,true)
						.SetBool(ArpScrollArea::InsetCornerP,false)
						.SetInt32(ArpScrollArea::BorderStyleP,B_FANCY_BORDER)
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpWest)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
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

status_t ArpSelectiveFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpWipeOutSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-WIPE-OUT-FILTER-ADDON
 *****************************************************************************/
void ArpSelectiveAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I allow through only the selected event types, diverting all other
	events to my second connection (if any).  By default, all event types are selected,
	allowing everything through.</P>
	
	<P>Most events have a checkbox for simple allow / don't allow behaviour.
	For control change events, you can choose the specific controller number
	to allow.</P>";
}

void ArpSelectiveAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpSelectiveAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpSelectiveAddOn::NewInstance(	AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpSelectiveFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpSelectiveAddOn(cookie);
	return NULL;
}
