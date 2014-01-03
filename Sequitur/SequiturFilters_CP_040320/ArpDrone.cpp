/* ArpDrone.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ViewStubs.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "ArpDrone.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		DRONE_NOTE_STR		= "Drone Note";

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
ArpDroneFilter::ArpDroneFilter(ArpDroneAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder), mSplitPoint(64)
{
	if (config) PutConfiguration(config);
}

ArpDroneFilter::~ArpDroneFilter()
{
}

AmEvent* ArpDroneFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	ArpVALIDATE(event && mHolder, return event);
	
	if(mHolder->CountConnections() < 2)
		return event;
	
	//int32 note = -1;
	if(event->Type() == event->NOTEON_TYPE) {
		AmNoteOn* e1 = dynamic_cast<AmNoteOn*>(event);
		if(e1) {
			AmNoteOn* e2 = dynamic_cast<AmNoteOn*>(e1->Copy());
			if(e2) {
				e2->SetNote(mSplitPoint);
				e2->SetNextFilter(mHolder->ConnectionAt(1));
				e1->AppendEvent(e2);
				//note = e1->Note();
			}
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff* e1 = dynamic_cast<AmNoteOff*>(event);
		if(e1) {
			AmNoteOff* e2 = dynamic_cast<AmNoteOff*>(e1->Copy());
			if(e2) {
				e2->SetNote(mSplitPoint);
				e2->SetNextFilter(mHolder->ConnectionAt(1));
				e1->AppendEvent(e2);
				//note = e->Note();
			}
		}
	}
	/*
	if(note < 0)
		return event;
	
	if(note >= mSplitPoint)
		event->SetNextFilter(mHolder->ConnectionAt(0));
	else
		event->SetNextFilter(mHolder->ConnectionAt(1));
		*/
	return event;
}

status_t ArpDroneFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if(err != B_OK)
		return err;
	
	if((err=values->AddInt32(DRONE_NOTE_STR, mSplitPoint)) != B_OK)
		return err;
	
	return B_OK;
}

status_t ArpDroneFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if(err != B_OK)
		return err;
	
	int32 i;
	if(values->FindInt32(DRONE_NOTE_STR, &i) == B_OK)
		mSplitPoint = i;
	
	return B_OK;
}

status_t ArpDroneFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if(err != B_OK)
		return err;
	panels.push_back(new _CopyFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-COPY-FILTER-ADD-ON
 *****************************************************************************/
ArpDroneAddOn::ArpDroneAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpDroneAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>Send copies of all note events to another pipeline, forcing all notes
to the same value.</P>
<P>Example use: place Drone in a bass track, send its output to a drum track, 
select a note which plays a bass drum in Drones GUI - the bass drum with be
automatically snychronized with the bass.</P>";
}

void ArpDroneAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpDroneAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if(bm)
		return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if(n == 0)
		return new ArpDroneAddOn(cookie);
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
			->AddLayoutChild((new AmKeyControl(
									DRONE_NOTE_STR, "Drone Note:",
									mImpl.AttachControl(DRONE_NOTE_STR)))
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
