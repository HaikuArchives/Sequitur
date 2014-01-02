/* ArpUncertainChorus.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "ArpUncertainChorus.h"

ArpMOD();
static AmStaticResources gRes;

static const char*	FREQUENCY_STR		= "Frequency";

/*****************************************************************************
 * ARP-UNCERTAIN-CHORUS-SETTINGS
 *****************************************************************************/
class ArpUncertainChorusSettings : public AmFilterConfigLayout
{
public:
	ArpUncertainChorusSettings(	AmFilterHolderI* target,
								const BMessage& initSettings);
		
protected:
	typedef AmFilterConfigLayout inherited;
};

/*****************************************************************************
 * ARP-UNCERTAIN-CHORUS-FILTER
 *****************************************************************************/
ArpUncertainChorusFilter::ArpUncertainChorusFilter(ArpUncertainChorusAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder), mFrequency(50), mFlags(MIRROR_Y)
{
	mSeed = int32(system_time()/100);
	if (config) PutConfiguration(config);
}

ArpUncertainChorusFilter::~ArpUncertainChorusFilter()
{
}

static bool should_chorus(int32 frequency)
{
	if (frequency <= 0) return false;
	if (frequency >= 100) return true;
	if (rand() % 100 < frequency) return true;
	else return false;
}

AmEvent* ArpUncertainChorusFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (event && mHolder) event->SetNextFilter(mHolder->FirstConnection() );
	if (!event || !should_chorus(mFrequency) ) return event;

	int32		newNote = rand() % 128;
	if (newNote < 0) newNote = 0;
	else if (newNote > 127) newNote = 127;

	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	e = dynamic_cast<AmNoteOn*>(event);
		if (!e) return event;
		if (e->Note() != newNote) {
			AmNoteOn*	e2 = dynamic_cast<AmNoteOn*>(e->Copy() );
			if (e2) {
				e2->SetNote(uint8(newNote) );
				e->AppendEvent(e2);
			}
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff*	e = dynamic_cast<AmNoteOff*>(event);
		if (!e) return event;
		if (e->Note() != newNote) {
			AmNoteOff*	e2 = dynamic_cast<AmNoteOff*>(e->Copy() );
			if (e2) {
				e2->SetNote(uint8(newNote) );
				e->AppendEvent(e2);
			}
		}
	}
	
	return event;
}

AmEvent* ArpUncertainChorusFilter::HandleToolEvent(	AmEvent* event, const am_filter_params* params,
													const am_tool_filter_params* toolParams)
{
	if (event && mHolder) event->SetNextFilter(mHolder->FirstConnection() );
	if (!event || !toolParams) return event;
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	e = dynamic_cast<AmNoteOn*>(event);
		if (!e) return event;
		BuildTable(e->Note(), toolParams);
		for (uint32 k = 0; k < NOTE_SIZE; k++) {
			if (mNotes[k] ) {
				AmNoteOn*	e2 = dynamic_cast<AmNoteOn*>(e->Copy() );
				if (e2) {
					e2->SetNote(k);
					e->AppendEvent(e2);
				}
			}
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
		AmNoteOff*	e = dynamic_cast<AmNoteOff*>(event);
		if (!e) return event;
		BuildTable(e->Note(), toolParams);
		for (uint32 k = 0; k < NOTE_SIZE; k++) {
			if (mNotes[k] ) {
				AmNoteOff*	e2 = dynamic_cast<AmNoteOff*>(e->Copy() );
				if (e2) {
					e2->SetNote(k);
					e->AppendEvent(e2);
				}
			}
		}
	}
	
	return event;
}

status_t ArpUncertainChorusFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err=values->AddInt32(FREQUENCY_STR, mFrequency)) != B_OK) return err;

	return B_OK;
}

status_t ArpUncertainChorusFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if (values->FindInt32(FREQUENCY_STR, &i) == B_OK) mFrequency = i;

	return B_OK;
}

status_t ArpUncertainChorusFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpUncertainChorusSettings(mHolder, config));
	return B_OK;
}

void ArpUncertainChorusFilter::Start(uint32 context)
{
	if (context&TOOL_CONTEXT) mSeed = int32(system_time()/100);
}

bool ArpUncertainChorusFilter::BuildTable(uint8 note, const am_tool_filter_params* toolParams)
{
	for (uint32 k = 0; k < NOTE_SIZE; k++) mNotes[k] = false;
	int32		change = toolParams->cur_y_value - toolParams->orig_y_value;

	srand(mSeed);
	if (change < 0) {
		for (int32 k = - 1; k >= change; k--) {
			if (note + k < 0) break;
			mNotes[note + k] = should_chorus(mFrequency);
			if (mFlags&MIRROR_Y) {
				if (note - k < int32(NOTE_SIZE)) mNotes[note - k] = should_chorus(mFrequency);
			}
		}
	} else if (change > 0) {
		for (int32 k = 1; k <= change; k++) {
			if (note + k >= int32(NOTE_SIZE)) break;
			mNotes[note + k] = should_chorus(mFrequency);
			if (mFlags&MIRROR_Y) {
				if (note - k >= 0) mNotes[note - k] = should_chorus(mFrequency);
			}
		}
	} else return false;

	return true;
}

/*****************************************************************************
 * ARP-UNCERTAIN-CHORUS-ADD-ON
 *****************************************************************************/
ArpUncertainChorusAddOn::ArpUncertainChorusAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpUncertainChorusAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I create a random chorus for each note I receives.  If I am in an
	input or output pipeline, I create a random note based on my Frequency.  If
	I am in a tool pipeline, I follow the mouse, and create random notes between
	where the mouse was pressed and its current Y position.</p>";
}

void ArpUncertainChorusAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpUncertainChorusAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpUncertainChorusAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * ARP-UNCERTAIN-CHORUS-SETTINGS
 *****************************************************************************/
ArpUncertainChorusSettings::ArpUncertainChorusSettings(	AmFilterHolderI* target,
														const BMessage& initSettings)
		: inherited(target, initSettings)
{
	float	labelW = -1, intW = -1;
	const BFont*	font = be_plain_font;
	if (font) {
		labelW = font->StringWidth("Frequency:") + 5;
		intW = font->StringWidth("Always") + 5;
	}
	try {
		ArpKnobPanel*	freqPanel = NULL;

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
			->AddLayoutChild((new ArpViewWrapper(freqPanel = new ArpKnobPanel(FREQUENCY_STR, "Frequency:",
												mImpl.AttachControl(FREQUENCY_STR),
												0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);
		if (freqPanel) {
			ArpIntControl*	intCtrl = freqPanel->IntControl();
			if (intCtrl) intCtrl->SetFormatter(arp_new_frequency_formatter() );
		}
	} catch(...) {
		throw;
	}
		
	Implementation().RefreshControls(mSettings);
}

