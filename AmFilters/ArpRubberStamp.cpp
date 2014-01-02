#include "ArpRubberStamp.h"

#include <stdio.h>
#include <stdlib.h>
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

//static const char*		AMOUNT_STR			= "amount";
static const char*		FOLLOW_X_STR		= "Follow X";
static const char*		USE_TOOL_BOUNDS_STR	= "Use Tool Bounds";
static const char*		MOTION_STR			= "motion";

static const int32		MIN_AMOUNT			= -400;
static const int32		MAX_AMOUNT			= 400;

static const char*		QUANTIZE_STR		= "quantize";
static const char*		MODIFIER_STR		= "modifier";

/*****************************************************************************
 * _RHYTHMICC-FILTER-SETTINGS
 *****************************************************************************/
class _RubberStampSettings : public AmFilterConfigLayout
{
public:
	_RubberStampSettings(	AmFilterHolderI* target,
								const BMessage& initSettings);

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* msg);

protected:
	typedef AmFilterConfigLayout inherited;
	ArpMenuField*		mRhythmField;

	void AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl, float labelW, float intW,
					const BMessage& initSettings);
};

/*************************************************************************
 * _PROXIMITY-FORMAT
 * A class that formats the proximity to beat field.
 *************************************************************************/
class _ProximityFormat : public ArpIntFormatterI
{
public:
	_ProximityFormat()	{ ; }

	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number <= 0) out << "Furthest";
		else if (number >= 100) out << "Closest";
		else out << number;
	}
};

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

/*************************************************************************
 * _FREQUENCY-FORMAT
 * A class that formats 0 to off and everything else to a percent.
 *************************************************************************/
class _FrequencyFormat : public ArpIntFormatterI
{
public:
	_FrequencyFormat()	{ ; }

	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number == 0) out << "Never";
		else if (number == 100) out << "Always";
		else out << number << '%';
	}
};

/*****************************************************************************
 * ARP-RUBBER-STAMP-FILTER
 *****************************************************************************/
ArpRubberStampFilter::ArpRubberStampFilter(	ArpRubberStampAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder), mMotion(NULL),
		  mControlNumber(7), mMultiplier(1), mQuantizeTime(PPQN), mDivider(2),
		  mAmount(1), mFlags(FOLLOW_X)
{
	InitMotion();
	if (settings) PutConfiguration(settings);
}

ArpRubberStampFilter::~ArpRubberStampFilter()
{
	delete mMotion;
}

AmEvent* ArpRubberStampFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return event;
}

AmEvent* ArpRubberStampFilter::HandleToolEvent(	AmEvent* event, const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	if (!event || !params || !toolParams || !mMotion || !mHolder) return event;

	if (event->Type() == event->NOTEON_TYPE) {
		AmRange			range(event->TimeRange() );
		if (range.IsValid() ) {
			float		amount = mAmount;
			float		pixels = (float)(toolParams->orig_y_pixel - toolParams->cur_y_pixel) / 100;
			if (pixels > 1) pixels = 1;
			else if (pixels < -1) pixels = -1;
			amount = pixels;
			event->Delete();
			return GenerateMotion(range, amount, params, toolParams);
		}
	}

	return event;
}

status_t ArpRubberStampFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if (values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumber) != B_OK) return B_ERROR;
	if( (err = values->AddInt32(AM_MULTIPLIER_CONTROL_KEY_STR, mMultiplier)) != B_OK ) return err;
	if( (err = add_time(*values, QUANTIZE_STR, mQuantizeTime)) != B_OK ) return err;
	if( (err = values->AddInt32(MODIFIER_STR, mDivider)) != B_OK ) return err;
	if ((err = values->AddBool(FOLLOW_X_STR, mFlags&FOLLOW_X)) != B_OK) return err;
	if ((err = values->AddBool(USE_TOOL_BOUNDS_STR, mFlags&USE_TOOL_BOUNDS)) != B_OK) return err;
	if (mMotion) {
		BMessage	msg;
		if ((err = mMotion->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION_STR, &msg)) != B_OK) return err;
	}
	return B_OK;
}

status_t ArpRubberStampFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	BMessage	msg;
	if (values->FindMessage(MOTION_STR, &msg) == B_OK) {
		delete mMotion;
		mMotion = AmMotionI::NewMotion(msg);
	}

	int32		i;
	AmTime		t;
	if (values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, &i) == B_OK) mControlNumber = (uint8)i;
	if (values->FindInt32(AM_MULTIPLIER_CONTROL_KEY_STR, &i) == B_OK) mMultiplier = i;
	if (find_time(*values, QUANTIZE_STR, &t) == B_OK) mQuantizeTime = t;
	if (values->FindInt32(MODIFIER_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mDivider = i;
	}
	bool		b;
	if (values->FindBool(FOLLOW_X_STR, &b) == B_OK) {
		if (b) mFlags |= FOLLOW_X;
		else mFlags &= ~FOLLOW_X;
	}
	if (values->FindBool(USE_TOOL_BOUNDS_STR, &b) == B_OK) {
		if (b) mFlags |= USE_TOOL_BOUNDS;
		else mFlags &= ~USE_TOOL_BOUNDS;
	}

	return B_OK;
}

status_t ArpRubberStampFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _RubberStampSettings(mHolder, config));
	return B_OK;
}

void ArpRubberStampFilter::InitMotion()
{
	/* Initialize myself to the first installed motion.
	 */
	delete mMotion;
	mMotion = NULL;
	BString		label, key;
	if (AmGlobals().GetMotionInfo(0, label, key) != B_OK) return;
	mMotion = AmGlobals().NewMotion(key);
}

AmEvent* ArpRubberStampFilter::GenerateMotion(	AmRange range, float amount,
												const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	ArpVALIDATE(mMotion && params && params->cur_signature, return NULL);
	AmTime			curTime = range.start;
	AmTime			grid = (((mQuantizeTime*2)/mDivider)*mMultiplier);
	AmEvent*		e = NULL;
	int32			lastValue = -1;
	while (curTime <= range.end) {
		BPoint		hit;
		AmRange		hitRange;
		if (am_get_motion_hits(curTime, mMotion, params, &hit, NULL, NULL, &hitRange) == B_OK) {
			/* Generate all hits in the range.
			 */
			while (curTime <= range.end && curTime <= hitRange.end) {
				/* Get the correct value.
				 */
				int32				value = 64 + (int32)(64 * hit.y);
				float				newAmount = amount;
				if (mFlags&FOLLOW_X && toolParams && toolParams->cur_time >= 0
						&& toolParams->start_time >= 0 && toolParams->end_time >= 0) {
					AmRange			r(range);
					if (mFlags&USE_TOOL_BOUNDS) {
						r.start = toolParams->start_time;
						r.end = toolParams->end_time;
					}
					newAmount = newAmount * am_x_amount(toolParams->cur_time, curTime, r.start, r.end);
				}
				value = int32(value * newAmount);
				if (value > 127) value = 127;
				else if (value < 0) value = 0;
				if (lastValue < 0 || lastValue != value) {
					AmControlChange*	cc = new AmControlChange(mControlNumber, (uint8)value, curTime);
					if (cc) {
						cc->SetNextFilter(mHolder->FirstConnection() );
						if (!e) e = cc;
						else e = e->MergeEvent(cc);
					}
					lastValue = value;
				}
				curTime += grid;
			}
		}
	}

	return e;
}

/*****************************************************************************
 * ARP-RUBBER-STAMP-ADD-ON
 *****************************************************************************/
void ArpRubberStampAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I transform events into control changes based on my motion.
	<I>This is a prerelease version of this filter.</I></p>";
}

void ArpRubberStampAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 0;
	*minor = 1;
}

BBitmap* ArpRubberStampAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(	int32 n, image_id /*you*/,
													const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpRubberStampAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * _RHYTHMICC-FILTER-SETTINGS
 *****************************************************************************/
static const uint32		MOTION_MSG		= 'iRtm';
static const char*		MOTION_KEY_STR	= "motion_key";

static ArpMenuField* new_rhythm_menu_field()
{
	BMenu*		menu = new BMenu("motion_menu");
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

//	menu->SetLabelFromMarked(true);
//	menu->SetRadioMode(true);	
	ArpMenuField*	field = new ArpMenuField("rhythm_field", "Motion:", menu);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

_RubberStampSettings::_RubberStampSettings(	AmFilterHolderI* target,
													const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings),
		  mRhythmField(NULL)
{
	float	labelW = -1, intW = -1;
	const BFont*	font = be_plain_font;
	if( font ) {
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
						.SetFloat(ArpRunningBar::WeightC,3)
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
}

void _RubberStampSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mRhythmField) mRhythmField->Menu()->SetTargetForItems(this);
}

void _RubberStampSettings::MessageReceived(BMessage* msg)
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
					}
					delete m;
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void _RubberStampSettings::AddViews(ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
									float labelW, float intW,
									const BMessage& initSettings)
{
	ArpBaseLayout*	vBar = (new ArpRunningBar("SubVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	toLayout->AddLayoutChild(vBar);

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

	vBar->AddLayoutChild((new AmDurationControl("quantize_to", "Grid:", this, initSettings,
											AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
											QUANTIZE_STR, MODIFIER_STR))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,3)
			.SetInt32(ArpRunningBar::FillC,ArpEastWest)
			.SetBool(ArpRunningBar::AlignLabelsC,true)
		)
	);

	vBar->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														USE_TOOL_BOUNDS_STR, "Use tool bounds",
														mImpl.AttachCheckBox(USE_TOOL_BOUNDS_STR))))										
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,3)
			.SetInt32(ArpRunningBar::FillC,ArpEastWest)
		)
	);

#if 0
	ArpKnobPanel*	kp = 0;
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(AMOUNT_STR, "Amount:", impl.AttachControl(AMOUNT_STR), MIN_AMOUNT, MAX_AMOUNT, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _OffPercentFormat() );
	}
#endif
	mRhythmField = new_rhythm_menu_field();
	if (mRhythmField) {
		vBar->AddLayoutChild(mRhythmField
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	}
}
