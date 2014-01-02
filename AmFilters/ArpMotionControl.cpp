/* ArpMotionControl.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <InterfaceKit.h>
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
#include "ArpMotionControl.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		MOTION_STR			= "motion";
static const char*		MODE_STR			= "Mode";
static const char*		TOOL_AMOUNT_STR		= "Tool Amount";
static const char*		AMOUNT_STR			= "Amount";

static const int32		MIN_AMOUNT			= -400;
static const int32		MAX_AMOUNT			= 400;

/**********************************************************************
 * _MC-FILTER-SETTINGS
 **********************************************************************/
class _McFilterSettings : public AmFilterConfigLayout
{
public:
	_McFilterSettings(	AmFilterHolderI* target,
						const BMessage& initSettings);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);

protected:
	typedef AmFilterConfigLayout inherited;
	BRadioButton*		mVelocityRadio;
	BRadioButton*		mValueRadio;
	ArpKnobPanel*		mAmountKnob;
	ArpMenuField*		mMotionField;
	AmMotionEditor*		mEditor;
	
	void AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
					float labelW, float intW,
					const BMessage& initSettings);
	void RefreshControls(const BMessage& settings);
};

/*****************************************************************************
 * ARP-MOTION-CONTROL-FILTER
 *****************************************************************************/
ArpMotionControlFilter::ArpMotionControlFilter(	ArpMotionControlAddOn* addon,
												AmFilterHolderI* holder,
												const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder),
		  mMotion(NULL), mMode(VELOCITY_AMOUNT), mFlags(0), mAmount(100),
		  mControlNumber(10), mGrid(PPQN / 32)
{
	InitMotion();
	if (settings) PutConfiguration(settings);
}

ArpMotionControlFilter::~ArpMotionControlFilter()
{
	delete mMotion;
}

AmEvent* ArpMotionControlFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event || !event->Type() == event->NOTEON_TYPE || !params) return event;
	AmNoteOn*		e = dynamic_cast<AmNoteOn*>(event);
	if (!e) return event;

	float			mod = mAmount / 100;
	if (mMode == VELOCITY_AMOUNT) mod = float(e->Velocity()) / 127;

	AmEvent*		ans = MotionControl(e->StartTime(),	mod, params);
	e->Delete();
	return ans;
}

AmEvent* ArpMotionControlFilter::HandleToolEvent(	AmEvent* event,
													const am_filter_params* params,
													const am_tool_filter_params* toolParams)
{
	if (!event || !event->Type() == event->NOTEON_TYPE || !params) return event;
	AmNoteOn*		e = dynamic_cast<AmNoteOn*>(event);
	if (!e) return event;

	float			mod = mAmount / 100;
	if (mMode == VELOCITY_AMOUNT) mod = float(e->Velocity()) / 127;
	if (mFlags&TOOL_AMOUNT_FLAG && toolParams) {
		float	pixels = (float)(toolParams->orig_y_pixel - toolParams->cur_y_pixel) / 100;
		mod  = mod * pixels;
	}

	AmEvent*		ans = MotionControl(e->StartTime(),	mod, params);
	e->Delete();
	return ans;
}

status_t ArpMotionControlFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err = values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumber)) != B_OK) return err;
	if ((err = values->AddInt32(MODE_STR, mMode)) != B_OK) return err;
	if (values->AddBool(TOOL_AMOUNT_STR, mFlags&TOOL_AMOUNT_FLAG) != B_OK) return B_ERROR;
	if (values->AddInt32(AMOUNT_STR, mAmount) != B_OK) return B_ERROR;
	if (mMotion) {
		BMessage	msg;
		if ((err = mMotion->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION_STR, &msg)) != B_OK) return err;
	}

	return B_OK;
}

status_t ArpMotionControlFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	int32		i;
	bool		b;
	if (values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, &i) == B_OK) mControlNumber = (uint8)i;
	if (values->FindInt32(MODE_STR, &i) == B_OK) {
		if (i < VELOCITY_AMOUNT) i = VELOCITY_AMOUNT;
		if (i > SELECTED_AMOUNT) i = SELECTED_AMOUNT;
		mMode = i;
	}
	if (values->FindBool(TOOL_AMOUNT_STR, &b) == B_OK) {
		if (b) mFlags |= TOOL_AMOUNT_FLAG;
		else mFlags &= ~TOOL_AMOUNT_FLAG;
	}
	if (values->FindInt32(AMOUNT_STR, &i) == B_OK) mAmount = i;

	BMessage	msg;
	if (values->FindMessage(MOTION_STR, &msg) == B_OK) {
		delete mMotion;
		mMotion = AmMotionI::NewMotion(msg);
	}

	return B_OK;
}

status_t ArpMotionControlFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _McFilterSettings(mHolder, config));
	return B_OK;
}

AmEvent* ArpMotionControlFilter::MotionControl(	AmTime time, float amount,
												const am_filter_params* params)
{
	if (!mMotion || !params || !params->cur_signature) return NULL;
	AmSignature		sig;
	if (am_get_measure(time, params->cur_signature, sig) != B_OK) return NULL;
	if (mMotion->EditingMode() == ENVELOPE_MODE)
		return EnvelopeMotionControl(time, amount, sig);
	
	AmEvent*	answer = NULL;
	uint32		count = mMotion->CountHits();
	for (uint32 k = 0; k < count; k++) {
		BPoint		pt;
		if (mMotion->GetHit(k, &pt) == B_OK) {
			AmTime	newTime = AmTime(time + (sig.Duration() * pt.x));
			int32	newValue = int32(pt.y * amount * 127);
			if (newValue > 127) newValue = 127;
			else if (newValue < 0) newValue = 0;

			AmControlChange*	cc = new AmControlChange(mControlNumber, newValue, newTime);
			if (cc) {
				cc->SetNextFilter(mHolder->FirstConnection() );
				if (answer) answer->AppendEvent(cc);
				answer = cc;
			}
		}
	}
	if (!answer) return NULL;
	return answer->HeadEvent();
}

static int32 cc_value(AmRange range, int32 v1, int32 v2, AmTime t)
{
	int32		minV = min(v1, v2), maxV = max(v1, v2);
	int32		newValue = int32(float((t - range.start) * (maxV - minV)) / float(range.end - range.start));
	if (v1 < v2) newValue += v1;
	else if (v2 < v1) newValue = v1 - newValue;
	else newValue = v1;
	if (newValue > 127) newValue = 127;
	else if (newValue < 0) newValue = 0;
	return newValue;
}

AmEvent* ArpMotionControlFilter::EnvelopeMotionControl(	AmTime time, float amount,
														AmSignature& sig)
{
	AmEvent*	answer = NULL;
	uint32		count = mMotion->CountHits();
	if (count < 1) return NULL;
	BPoint		prevPt;
	if (mMotion->GetHit(0, &prevPt) != B_OK) return NULL;
	
	for (uint32 k = 1; k < count; k++) {
		BPoint		pt;
		if (mMotion->GetHit(k, &pt) == B_OK) {
			AmRange		range(time + (sig.Duration() * prevPt.x), time + (sig.Duration() * pt.x) ); 
			int32		prevValue = int32(prevPt.y * amount * 127);
			if (prevValue > 127) prevValue = 127;
			else if (prevValue < 0) prevValue = 0;
			int32		nextValue = int32(pt.y * amount * 127);
			if (nextValue > 127) nextValue = 127;
			else if (nextValue < 0) nextValue = 0;
			int32		lastValue = -1;
			for (AmTime t = range.start; t < range.end; t += mGrid) {
				int32	newValue = cc_value(range, prevValue, nextValue, t);
				if (lastValue != newValue) {
					AmControlChange*	cc = new AmControlChange(mControlNumber, newValue, t);
					if (cc) {
						cc->SetNextFilter(mHolder->FirstConnection() );
						if (answer) answer->AppendEvent(cc);
						answer = cc;
					}
				}
				lastValue = newValue;
			}
		}
		prevPt = pt;
	}
	if (!answer) return NULL;
	return answer->HeadEvent();
}

void ArpMotionControlFilter::InitMotion()
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

// #pragma mark -

/*****************************************************************************
 * ARP-MOTION-CONTROL-ADD-ON
 *****************************************************************************/
ArpMotionControlAddOn::ArpMotionControlAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpMotionControlAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I transform each note I receives into a series of control changes
		based on my current motion.</P>
	<h4>Amount</h4>
		Amount determines the level of the controls generated.
		<UL>
			<LI><I>From velocity</I> uses each note's velocity to determine the amount.
					For example, if a note has a velocity of 64, then the final generated
					control changes will range from 0 to 64.  If a note has a velocity of
					100, then the resulting control changes will range from 0 to 100.</LI>
			<LI><I>From value</I> uses the value of the knob (immediately below the From
					value radio button) to determine the final control change values.  If
					the knob is at 100, then motion values of 100 translate to the maximum
					possible control change value - 127.  If the knob has a negative value,
					then the motion is inverted.</LI>
			<LI><I>Follow mouse</I> uses the mouse's Y position to determine the level.</LI>
		</UL>
	
	<h4>Motion</h4>
		The Motion menu button presents a list of all available motions.  Selecting one
		will copy that motion into the filter, making it the current motion.  Below this
		button is a motion editor, which operates the same as the Edit Motion window.

	<h4>Control Number</h4>
		This list box allows you to select the type of control change that will be created.";
}

void ArpMotionControlAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpMotionControlAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpMotionControlAddOn(cookie);
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
 * _MC-FILTER-SETTINGS
 **********************************************************************/
static const uint32 	VELOCITY_MSG	= 'iVel';
static const uint32 	VALUE_MSG		= 'iVal';

static const uint32		MOTION_MSG		= 'iRtm';
static const char*		MOTION_KEY_STR	= "motion_key";

static ArpMenuField* new_motion_menu_field()
{
	BMenu*		menu = new BMenu("Motion");
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

_McFilterSettings::_McFilterSettings(	AmFilterHolderI* target,
										const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings),
		  mVelocityRadio(NULL), mValueRadio(NULL),
		  mAmountKnob(NULL), mMotionField(NULL), mEditor(NULL)
{
	float			labelW = -1, intW = -1;
	const BFont*	font = be_plain_font;
	if (font) {
		labelW = font->StringWidth( "Proximity to beat:");
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
	RefreshControls(initSettings);
}

void _McFilterSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mMotionField) mMotionField->Menu()->SetTargetForItems(this);
}

void _McFilterSettings::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case VELOCITY_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpMotionControlFilter::VELOCITY_AMOUNT) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case VALUE_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpMotionControlFilter::SELECTED_AMOUNT) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
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
		default:
			inherited::MessageReceived(msg);
	}
}

void _McFilterSettings::AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
									float labelW, float intW,
									const BMessage& initSettings)
{
	ArpBaseLayout*	vBar = (new ArpRunningBar("SubVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	toLayout->AddLayoutChild(vBar);

	vBar->AddLayoutChild((new ArpBox("AmountBox", "Amount"))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
				.SetInt32(ArpRunningBar::FillC,ArpFillAll)
				.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
		->AddLayoutChild((new ArpRunningBar("AmountVBar"))
			->SetParams(ArpMessage()
				.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
				.SetFloat(ArpRunningBar::IntraSpaceP, .5)
			)
			->AddLayoutChild((new ArpViewWrapper(mVelocityRadio = new BRadioButton( BRect(0,0,10,10),
														"velocity_radio", "From velocity",
														new BMessage(VELOCITY_MSG), 
														B_FOLLOW_NONE,
														B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(mValueRadio = new BRadioButton( BRect(0,0,10,10),
														"value_radio", "From value",
														new BMessage(VALUE_MSG), 
														B_FOLLOW_NONE,
														B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(mAmountKnob = new ArpKnobPanel(AMOUNT_STR, NULL, impl.AttachControl(AMOUNT_STR), MIN_AMOUNT, MAX_AMOUNT, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, 10, intW)))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														TOOL_AMOUNT_STR, "Follow mouse (tool only)",
														mImpl.AttachCheckBox(TOOL_AMOUNT_STR))))										
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
		)
	);

	if (mAmountKnob) {
		ArpIntControl*	intCtrl = mAmountKnob->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _OffPercentFormat() );
	}

	mMotionField = new_motion_menu_field();
	if (mMotionField) {
		vBar->AddLayoutChild(mMotionField
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,0)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	}

	vBar->AddLayoutChild(mEditor = new AmMotionEditor("motion_editor", this, initSettings, MOTION_STR))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest));

	vBar->AddLayoutChild((new AmControlChangeListPanel("Control Changes", this, initSettings))
		->SetParams(ArpMessage()
			.SetBool(ArpScrollArea::ScrollHorizontalP,false)
			.SetBool(ArpScrollArea::ScrollVerticalP,true)
			.SetBool(ArpScrollArea::InsetCornerP,false)
			.SetInt32(ArpScrollArea::BorderStyleP,B_FANCY_BORDER)
		)
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,1)
			.SetInt32(ArpRunningBar::FillC,ArpWest)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
		)
	);

}

void _McFilterSettings::RefreshControls(const BMessage& settings)
{
	int32		i;
	if (settings.FindInt32(MODE_STR, &i) == B_OK) {
		if (i == ArpMotionControlFilter::VELOCITY_AMOUNT) {
			if (mVelocityRadio) mVelocityRadio->SetValue(B_CONTROL_ON);
		} else if (i == ArpMotionControlFilter::SELECTED_AMOUNT) {
			if (mValueRadio) mValueRadio->SetValue(B_CONTROL_ON);
		}
	}

	if (settings.FindInt32(AMOUNT_STR, &i) == B_OK) {
		if (mAmountKnob && mAmountKnob->KnobControl() )
			mAmountKnob->KnobControl()->SetValue(i);
	}
}
