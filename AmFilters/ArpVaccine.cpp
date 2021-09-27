#include "ArpVaccine.h"

#include <cstdio>
#include <cstdlib>
#include <interface/CheckBox.h>
#include <interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmMotionI.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		FOLLOW_Y_STR			= "Follow Y";
static const char*		FOLLOW_X_STR			= "Follow X";
static const char*		INVERT_X_STR			= "Invert X";
static const char*		FREQUENCY_STR			= "frequency";
static const char*		AMOUNT_STR				= "amount";
static const char*		MOTION_FROM_TRACK_STR	= "Motion From Track";
static const char*		MOTION_STR				= "motion";

static const int32		MIN_AMOUNT				= -400;
static const int32		MAX_AMOUNT				= 400;


static const char*		CHANGE_NOTE_STR			= "Change Note";
enum {
	NOTE_VELOCITY			= 0x0001,
	NOTE_PITCH				= 0x0002,

	NOTE_DEFAULTS	=
		NOTE_VELOCITY,
};

/**********************************************************************
 * _VACCINE-FILTER-SETTINGS and _TOOL-SETTINGS
 **********************************************************************/
class _VaccineFilterSettings : public AmFilterConfigLayout
{
public:
	_VaccineFilterSettings(	AmFilterHolderI* target,
							const BMessage& initSettings);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

protected:
	typedef AmFilterConfigLayout inherited;
	BCheckBox*			mFollowXBox;
	BCheckBox*			mInvertXBox;
	ArpMenuField*		mRhythmField;
	BCheckBox*			mTrackMotionBox;
	AmMotionEditor*		mEditor;
	
	void AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
					float labelW, float intW,
					const BMessage& initSettings);
};

/*****************************************************************************
 * ARP-VACCINE-FILTER
 *****************************************************************************/
ArpVaccineFilter::ArpVaccineFilter(	ArpVaccineAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder), mMotion(NULL),
		  mChangeFlags(FOLLOW_Y_FLAG), mNoteFlags(NOTE_DEFAULTS),
		  mFrequency(100), mAmount(100)
{
	mSeed = int32(system_time()/100);
	InitMotion();
	if (settings) PutConfiguration(settings);
}

ArpVaccineFilter::~ArpVaccineFilter()
{
	delete mMotion;
}

AmEvent* ArpVaccineFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

AmEvent* ArpVaccineFilter::HandleBatchEvents(	AmEvent* event,
												const am_filter_params* params,
												const AmEvent* /*lookahead*/)
{
	if (!event || !mHolder || !params || !params->cur_signature) return event;
	AmMotionChange*		motionEvent = NULL;
	bool				deleteMotionEvent = false;
	if (mChangeFlags&MOTION_FROM_TRACK_FLAG) {
		motionEvent = params->MotionChange(mHolder->TrackId() );
	} else {
		if (mMotion) {
			motionEvent = new AmMotionChange(mMotion, 1, 0);
			deleteMotionEvent = true;
		}
	}

	if (!motionEvent) return event;
	VaccinateBatchEvents(event, params, motionEvent, NULL);
	
	if (deleteMotionEvent) motionEvent->Delete();
	return event;
}

AmEvent* ArpVaccineFilter::HandleBatchToolEvents(	AmEvent* event,
													const am_filter_params* params,
													const am_tool_filter_params* toolParams,
													const AmEvent* /*lookahead*/)
{
	if (!event || !mHolder) return event;
	event->SetNextFilter(mHolder->FirstConnection() );
	if (!params || !params->cur_signature || !toolParams) return event;
	AmMotionChange*		motionEvent = NULL;
	bool				deleteMotionEvent = false;
	if (mChangeFlags&MOTION_FROM_TRACK_FLAG) {
		motionEvent = params->MotionChange(toolParams->track_context);
	} else {
		if (mMotion) {
			motionEvent = new AmMotionChange(mMotion, 1, 0);
			deleteMotionEvent = true;
		}
	}

	if (!motionEvent) return event;
	VaccinateBatchEvents(event, params, motionEvent, toolParams);
	
	if (deleteMotionEvent) motionEvent->Delete();
	return event;
}

status_t ArpVaccineFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if (values->AddBool(FOLLOW_Y_STR, mChangeFlags&FOLLOW_Y_FLAG) != B_OK) return B_ERROR;
	if (values->AddBool(FOLLOW_X_STR, mChangeFlags&FOLLOW_X_FLAG) != B_OK) return B_ERROR;
	if (values->AddBool(INVERT_X_STR, mChangeFlags&INVERT_X_FLAG) != B_OK) return B_ERROR;
	if (values->AddBool(MOTION_FROM_TRACK_STR, mChangeFlags&MOTION_FROM_TRACK_FLAG) != B_OK) return B_ERROR;
	if (values->AddInt32(FREQUENCY_STR, mFrequency) != B_OK) return B_ERROR;
	if (values->AddInt32(AMOUNT_STR, mAmount) != B_OK) return B_ERROR;
	if (mMotion) {
		BMessage	msg;
		if ((err = mMotion->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION_STR, &msg)) != B_OK) return err;
	}

	if ( (err = values->AddInt32(CHANGE_NOTE_STR, mNoteFlags)) != B_OK) return err;

	return B_OK;
}

status_t ArpVaccineFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);

	if (err != B_OK) return err;
	bool		b;
	if (values->FindBool(FOLLOW_Y_STR, &b) == B_OK) {
		if (b) mChangeFlags |= FOLLOW_Y_FLAG;
		else mChangeFlags &= ~FOLLOW_Y_FLAG;
	}
	if (values->FindBool(FOLLOW_X_STR, &b) == B_OK) {
		if (b) mChangeFlags |= FOLLOW_X_FLAG;
		else mChangeFlags &= ~FOLLOW_X_FLAG;
	}
	if (values->FindBool(INVERT_X_STR, &b) == B_OK) {
		if (b) mChangeFlags |= INVERT_X_FLAG;
		else mChangeFlags &= ~INVERT_X_FLAG;
	}
	if (values->FindBool(MOTION_FROM_TRACK_STR, &b) == B_OK) {
		if (b) mChangeFlags |= MOTION_FROM_TRACK_FLAG;
		else mChangeFlags &= ~MOTION_FROM_TRACK_FLAG;
	}

	BMessage	msg;
	if (values->FindMessage(MOTION_STR, &msg) == B_OK) {
		delete mMotion;
		mMotion = AmMotionI::NewMotion(msg);
	}

	int32		i;
	if (values->FindInt32(FREQUENCY_STR, &i) == B_OK) mFrequency = i;
	if (values->FindInt32(AMOUNT_STR, &i) == B_OK) mAmount = i;
	if (values->FindInt32(CHANGE_NOTE_STR, &i) == B_OK) mNoteFlags = i;

	return B_OK;
}

status_t ArpVaccineFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _VaccineFilterSettings(mHolder, config));
	return B_OK;
}

void ArpVaccineFilter::Start(uint32 context)
{
	if (context&TOOL_CONTEXT) mSeed = int32(system_time()/100);
}

void ArpVaccineFilter::VaccinateBatchEvents(AmEvent* event, const am_filter_params* params,
											AmMotionChange* curMotion,
											const am_tool_filter_params* toolParams)
{
	ArpASSERT(params && params->cur_signature);
	if (!curMotion) return;
	AmSignature*	measure = dynamic_cast<AmSignature*>(params->cur_signature->CopyChain());
	if (!measure) return;
	int32			motionMeasureOffset = 0;

	while (event) { 
		float		y;
		if (CanVaccinate(event->Type()) && ShouldVaccinate()
				&& am_motion_hit_at_time(	event->StartTime(), &curMotion, &motionMeasureOffset,
											*measure, &y) == B_OK) {
//			printf("Time %lld hit is %f\n", event->StartTime(), y);
			float		amount = (mAmount / 100) * y;
			float		yAmount = 1;
			if (toolParams) {
				if (mChangeFlags&FOLLOW_Y_FLAG) {
					float	pixels = (float)(toolParams->orig_y_pixel - toolParams->cur_y_pixel) / 100;
					amount = pixels;
					yAmount = y;
				}
				if (mChangeFlags&FOLLOW_X_FLAG) {
					AmTime	s = toolParams->start_time, e = toolParams->end_time, c = toolParams->cur_time;
					if (s >= 0 && e >= 0 && c >= 0) {
						float		xamt = am_x_amount(toolParams, event->StartTime() );
						if (mChangeFlags&INVERT_X_FLAG) xamt = fabs(1 -xamt);
						amount = amount * xamt;
					}
				}
			}

			if (event->Type() == event->CONTROLCHANGE_TYPE)
				VaccinateControlChange(event, amount, yAmount);
			else if (event->Type() == event->PITCHBEND_TYPE)
				VaccinatePitchBend(event, amount, yAmount);
			else if (event->Type() == event->NOTEON_TYPE)
				VaccinateNoteOn(event, amount, yAmount);
			else if (event->Type() == event->NOTEOFF_TYPE)
				VaccinateNoteOff(event, amount, yAmount);
			else if (event->Type() == event->TEMPOCHANGE_TYPE)
				VaccinateTempoChange(event, amount, yAmount);
			else if (event->Type() == event->CHANNELPRESSURE_TYPE)
				VaccinateChannelPressure(event, amount, yAmount);
		}
		event->SetNextFilter(mHolder->FirstConnection() );
		event = event->NextEvent();
	}

	measure->Delete();
}

void ArpVaccineFilter::VaccinateControlChange(AmEvent* event, float amount, float yAmount)
{
	AmControlChange*	e = dynamic_cast<AmControlChange*>(event);
	if (!e) return;
	int32	newVal = (int32)(e->ControlValue() + (amount * yAmount * 127));
	if (newVal > 127) newVal = 127;
	else if (newVal < 0) newVal = 0;
	e->SetControlValue(newVal);
}

void ArpVaccineFilter::VaccinatePitchBend(AmEvent* event, float amount, float yAmount)
{
	AmPitchBend*		e = dynamic_cast<AmPitchBend*>(event);
	if (!e) return;
	int32	newVal = (int32)(e->Value() + (amount * yAmount * 8191));
	if (newVal > AM_PITCH_MAX) newVal = AM_PITCH_MAX;
	else if (newVal < AM_PITCH_MIN) newVal = AM_PITCH_MIN;
	e->SetValue(newVal);
}

void ArpVaccineFilter::VaccinateNoteOn(AmEvent* event, float amount, float yAmount)
{
	AmNoteOn*	e = dynamic_cast<AmNoteOn*>(event);
	if (!e) return;

	if (mNoteFlags&NOTE_VELOCITY) {
		int32	newVel = (int32)(e->Velocity() + (amount * yAmount * 127));
		if (newVel > 127) newVel = 127;
		else if (newVel < 0) newVel = 0;
		e->SetVelocity(newVel);
	}
	if (mNoteFlags&NOTE_PITCH) {
		int32		delta = int32(yAmount * amount * 127);
		int32		newNote = (int32)(e->Note() + delta);
		if (newNote > 127) newNote = 127;
		else if (newNote < 0) newNote = 0;
		e->SetNote(newNote);
	}
}

void ArpVaccineFilter::VaccinateNoteOff(AmEvent* event, float amount, float yAmount)
{
	AmNoteOff*	e = dynamic_cast<AmNoteOff*>(event);
	if (!e) return;
	if (mChangeFlags&NOTE_PITCH) {
		int32		delta = int32(yAmount * amount * 127);
		int32		newNote = (int32)(e->Note() + delta);
		if (newNote > 127) newNote = 127;
		else if (newNote < 0) newNote = 0;
		e->SetNote(newNote);
	}
}

void ArpVaccineFilter::VaccinateTempoChange(AmEvent* event, float amount, float yAmount)
{
	AmTempoChange*	e = dynamic_cast<AmTempoChange*>(event);
	if (!e) return;
	int32	newVal = (int32)(e->Tempo() + AM_TEMPO_MIN + (amount * yAmount * (AM_TEMPO_MAX - AM_TEMPO_MIN)));
	if (newVal > AM_TEMPO_MAX) newVal = AM_TEMPO_MAX;
	else if (newVal < AM_TEMPO_MIN) newVal = AM_TEMPO_MIN;
	e->SetTempo(newVal);
}

void ArpVaccineFilter::VaccinateChannelPressure(AmEvent* event, float amount, float yAmount)
{
	AmChannelPressure*	e = dynamic_cast<AmChannelPressure*>(event);
	if (!e) return;
	int32	newVal = (int32)(e->Pressure() + (amount * yAmount * 127));
	if (newVal > 127) newVal = 127;
	else if (newVal < 0) newVal = 0;
	e->SetPressure(newVal);
}

bool ArpVaccineFilter::CanVaccinate(AmEvent::EventType et) const
{
	if (et == AmEvent::CONTROLCHANGE_TYPE || et == AmEvent::PITCHBEND_TYPE
			|| et == AmEvent::NOTEON_TYPE || et == AmEvent::NOTEOFF_TYPE
			|| et == AmEvent::TEMPOCHANGE_TYPE || et == AmEvent::CHANNELPRESSURE_TYPE)
		return true;
	return false;
}

bool ArpVaccineFilter::ShouldVaccinate() const
{
	if (mFrequency <= 0) return false;
	if (mFrequency >= 100) return true;

	srand( (int32)(system_time()/100) );
	int32 percent = rand() % 100;

	if (percent < mFrequency) return true;
	else return false;
}

void ArpVaccineFilter::InitMotion()
{
	delete mMotion;
	mMotion = NULL;
	/* See if the standard Pulse 4 motion is installed.  If so,
	 * initialize to that.
	 */
	mMotion = AmGlobals().NewMotion("Pulse 4");
	if (mMotion) return;
	/* Otherwise, initialize myself to the first installed motion.
	 */
	BString		label, key;
	if (AmGlobals().GetMotionInfo(0, label, key) != B_OK) return;
	mMotion = AmGlobals().NewMotion(key);
	if (mMotion) return;
	/* Finally, if there are none, just a blank motion.
	 */
	BMessage	config;
	mMotion = AmMotionI::NewMotion(config);
}

/*****************************************************************************
 * ARP-VACCINE-ADD-ON
 *****************************************************************************/
void ArpVaccineAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I alter the value of various event properties based on my motion. \n"
		"For each event I receive, I check where it falls on the motion, then alter \n"
		"it accordingly.  The property that is altered depends on the event:</p>\n"
		"<ul><li><i>Control changes</i>: control value</li>\n"
		"	<li><i>Pitch bends</i>: pitch value</li>\n"
		"	<li><i>Tempo changes</i>: tempo value</li>\n"
		"	<li><i>Channel pressure</i>: pressure value</li>\n"
		"	<li><i>Notes</i>: the velocity and/or pitch, depending on which box is checked</li>\n"
		"</ul>\n"
		"	\n"
		"<h4>Amount</h4>\n"
		"	<p>Amount determines how much the current motion value affects the value. \n"
		"	When the Amount: value is at 100%, then the final value will exactly \n"
		"	match the current motion value.  For example, if a control change being processed \n"
		"	occurs at a motion hit that is at 50%, then the CC's value will be increased by \n"
		"	a total of 50% of the value range, which is 64.</p> \n"
		"	\n"
		"	<p>Tools follow Y is only meaningful when this filter is being used from \n"
		"	a tool.  It causes the Amount value to be set by how far the mouse has \n"
		"	traveled from the original point.  If the mouse travels below the original \n"
		"	point, the motion becomes inverted.</p>\n"
		"	\n"
		"	<p>Tools follow X causes the Amount to taper off towards either end of \n"
		"	the selected events.</p>\n"
		"	\n"
		"	<p>Invert X is only valid if Tools follow X is on.  It inverts the tapering \n"
		"	effect, essentially turning it from a cone to a bowl.</p>\n"
"\n"
		"<h4>Frequency</h4>\n"
		"	<p>This parameter determines how frequently events will be processed.  When \n"
		"	set to 0% (Never) this filter is effectively bypassed.  When set to 100% \n"
		"	(Always) every event is processed.</p>\n"
	"\n"
		"<h4>Motion Selection</h4>\n"
		"	<p>The Use Motion button presents a list of all the motions currently \n"
		"	installed in the system.  Selecting one will copy it into the motion editor \n"
		"	at the bottom of the window.</p>\n"
		"	\n"
		"	<p>If Use motion from track is on, then my motion is ignored.  Instead, \n"
		"	I use whatever the current track motion is active for each event being \n"
		"	processed.  If no track motion is set, then nothing happens.</p>\n";
}

void ArpVaccineAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpVaccineAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpVaccineAddOn(cookie);
	return NULL;
}

// #pragma mark -

/*************************************************************************
 * _OFF-PERCENT-FORMAT
 * A class that formats 0 to off and everything else to a percent.
 *************************************************************************/
class _OffPercentFormat : public ArpIntFormatterI
{
public:
	_OffPercentFormat()	{ ; }

	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number == 0) out << "Off";
		else out << number << '%';
	}
};

/**********************************************************************
 * _VACCINE-FILTER-SETTINGS
 **********************************************************************/
static const uint32		MOTION_MSG		= 'iRtm';
static const char*		MOTION_KEY_STR	= "motion_key";

static ArpMenuField* new_rhythm_menu_field()
{
	BMenu*		menu = new BMenu("Use motion");
	if (!menu) return NULL;
	BString		label, key;
	for (uint32 k = 0; AmGlobals().GetMotionInfo(k, label, key) == B_OK; k++) {
		BMessage*		msg = new BMessage(MOTION_MSG);
		if (msg) {
			msg->AddString(MOTION_KEY_STR, key);
			BMenuItem*	item = new BMenuItem(label.String(), msg);
			if (!item) delete msg;
			else menu->AddItem(item);
		}
	}

	ArpMenuField*	field = new ArpMenuField("motion_field", NULL, menu);
	if (!field) {
		delete menu;
		return NULL;
	}
	field->SetDivider(0);
	return field;
}

_VaccineFilterSettings::_VaccineFilterSettings(	AmFilterHolderI* target,
												const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings),
		  mFollowXBox(NULL), mInvertXBox(NULL), mRhythmField(NULL),
		  mTrackMotionBox(NULL), mEditor(NULL)
{
	float	labelW = -1, intW = -1;
	const BFont*	font = be_plain_font;
	if (font) {
		labelW = font->StringWidth("Proximity to beat:");
		intW = font->StringWidth("Furthest") + 5;
	}

	try {
		ArpBaseLayout*	topVBar = (new ArpRunningBar("TopVBar"))
										->SetParams(ArpMessage()
											.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
											.SetFloat(ArpRunningBar::IntraSpaceP, .5)
										);
		AddLayoutChild( topVBar );
		topVBar->AddLayoutChild((new ArpTextControl(
									SZ_FILTER_LABEL, "Label:","",
									mImpl.AttachTextControl(SZ_FILTER_LABEL)))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "8")
						.SetString(ArpTextControl::PrefTextStringP, "8888888888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				);
		ArpBaseLayout*	colHBar = (new ArpRunningBar("ColHBar"))
										->SetParams(ArpMessage()
											.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
											.SetFloat(ArpRunningBar::IntraSpaceP, .5)
										);
		topVBar->AddLayoutChild( colHBar );
		AddViews(colHBar, mImpl, labelW, intW, initSettings);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);

	if (mTrackMotionBox && mRhythmField) {
		if (mTrackMotionBox->Value() == B_CONTROL_ON) mRhythmField->SetEnabled(false);
	}
	if (mFollowXBox && mInvertXBox) {
		if (mFollowXBox->Value() != B_CONTROL_ON) mInvertXBox->SetEnabled(false);
	}

}

void _VaccineFilterSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mRhythmField) mRhythmField->Menu()->SetTargetForItems(this);
}

void _VaccineFilterSettings::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MOTION_MSG: {
			const char*		key;
			if (msg->FindString(MOTION_KEY_STR, &key) == B_OK) {
				AmMotionI*	m = AmGlobals().NewMotion(key);
				if (m) {
					BMessage		rMsg, containerMsg;
					if (m->WriteTo(rMsg) == B_OK && containerMsg.AddMessage(MOTION_STR, &rMsg) == B_OK) {
						Implementation().SendConfiguration(&containerMsg);
						mSettings.Update(containerMsg);
						if (mEditor) mEditor->Refresh(containerMsg);
					}
					delete m;
				}
			}
		} break;
		case ArpConfigureImpl::CONFIG_REPORT_MSG: {
			const char*		param;
			if (msg->FindString("arp:param", &param) == B_OK) {
				if (mRhythmField && strcmp(param, MOTION_FROM_TRACK_STR) == 0) {
					if (mTrackMotionBox && mTrackMotionBox->Value() == B_CONTROL_ON)
						mRhythmField->SetEnabled(false);
					else mRhythmField->SetEnabled(true);
				} else if (mInvertXBox && strcmp(param, FOLLOW_X_STR) == 0) {
					if (mFollowXBox && mFollowXBox->Value() == B_CONTROL_ON)
						mInvertXBox->SetEnabled(true);
					else mInvertXBox->SetEnabled(false);
				}
			}
		} // Note: no break on purpose				
		default:
			inherited::MessageReceived(msg);
	}
}

void _VaccineFilterSettings::AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
										float labelW, float intW,
										const BMessage& initSettings)
{
	ArpBaseLayout*	vBar = (new ArpRunningBar("SubVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	toLayout->AddLayoutChild(vBar);

	ArpBaseLayout*	box = NULL;
	ArpKnobPanel*	kp = NULL;

	vBar->AddLayoutChild((new ArpRunningBar("BoxHGroup"))
		->SetParams(ArpMessage()
			.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
			.SetFloat(ArpRunningBar::IntraSpaceP, .5)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetInt32(ArpRunningBar::FillC,ArpWest)
		)
		->AddLayoutChild((box = new ArpBox("AmountBox", "Amount"))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
				.SetInt32(ArpRunningBar::FillC,ArpFillAll)
				.SetBool(ArpRunningBar::AlignLabelsC,false)
			)
		)
		->AddLayoutChild((new ArpRunningBar("NoteVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, 0)
			)
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
				.SetInt32(ArpRunningBar::FillC,ArpWest)
			)
			->AddLayoutChild((new ArpViewWrapper(
				new BCheckBox(BRect(0,0,10,10), "velocity", "Change note velocity",
						mImpl.AttachCheckBox(CHANGE_NOTE_STR, NOTE_VELOCITY, "pitch"),
						B_FOLLOW_NONE,
						B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(
				new BCheckBox(BRect(0,0,10,10), "pitch", "Change note pitch",
						mImpl.AttachCheckBox(CHANGE_NOTE_STR, NOTE_PITCH, "pitch"),
						B_FOLLOW_NONE,
						B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpWest)
				)
			)
		)
	);
	if (box) {
		box->AddLayoutChild((new ArpRunningBar("AmountVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(AMOUNT_STR, "Amount:", impl.AttachControl(AMOUNT_STR), MIN_AMOUNT, MAX_AMOUNT, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
												FOLLOW_Y_STR, "Tools follow Y",
												mImpl.AttachCheckBox(FOLLOW_Y_STR))))										
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(mFollowXBox = new BCheckBox( BRect(0,0,0,0),
												FOLLOW_X_STR, "Tools follow X",
												mImpl.AttachCheckBox(FOLLOW_X_STR))))										
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(mInvertXBox = new BCheckBox( BRect(0,0,0,0),
												INVERT_X_STR, "Invert X",
												mImpl.AttachCheckBox(INVERT_X_STR))))										
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);
	}

	/* This should be for the Amount control, no one interfere.
	 */
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter(new _OffPercentFormat() );
	}

	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(FREQUENCY_STR, "Frequency:", impl.AttachControl(FREQUENCY_STR), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter(arp_new_frequency_formatter() );
	}

	vBar->AddLayoutChild((box = new ArpBox("MotionBox", "Motion selection"))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
	);
	mRhythmField = new_rhythm_menu_field();
	if (box && mRhythmField) {
		box->AddLayoutChild((new ArpRunningBar("DurationVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild(mRhythmField
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(mTrackMotionBox = new BCheckBox( BRect(0,0,0,0),
											MOTION_FROM_TRACK_STR, "Use motion from track",
										mImpl.AttachCheckBox(MOTION_FROM_TRACK_STR))))										
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		);
	} 
		
	vBar->AddLayoutChild(mEditor = new AmMotionEditor("motion_editor", this, initSettings, MOTION_STR))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest));
}
