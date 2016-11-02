#include "ArpRhythmiCC.h"

#include <stdio.h>
#include <stdlib.h>
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

static const char*		PROXIMITY_STR		= "proximity";
static const char*		FREQUENCY_STR		= "frequency";
static const char*		AMOUNT_STR			= "amount";
//static const char*		FLAGS_STR			= "flags";
static const char*		FOLLOW_X_STR		= "Follow X";
static const char*		MOTION_STR			= "motion";

static const int32		MIN_AMOUNT			= -400;
static const int32		MAX_AMOUNT			= 400;

/*****************************************************************************
 * _RHYTHMICC-FILTER-SETTINGS
 *****************************************************************************/
class _RhythmiCcFilterSettings : public AmFilterConfigLayout
{
public:
	_RhythmiCcFilterSettings(	AmFilterHolderI* target,
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
 * ARP-RHYTHMICC-FILTER
 *****************************************************************************/
ArpRhythmiCcFilter::ArpRhythmiCcFilter(	ArpRhythmiCcFilterAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder), mMotion(NULL),
		  mControlNumber(7), mFrequency(50), mAmount(1), mFlags(FOLLOW_X)
{
	InitMotion();
	if (settings) PutConfiguration(settings);
}

ArpRhythmiCcFilter::~ArpRhythmiCcFilter()
{
	delete mMotion;
}

AmEvent* ArpRhythmiCcFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return event;
}

AmEvent* ArpRhythmiCcFilter::HandleToolEvent(	AmEvent* event, const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	if (!event || !params || !toolParams || !mMotion || !mHolder) return event;
	AmRange			range(toolParams->start_time, toolParams->end_time);
	if (!range.IsValid()) return event;
#if 0
printf("Create CC's for event at %lld", event->StartTime());
if (params->flags&AMFF_FIRST_EVENT) printf(" (first event)");
if (params->flags&AMFF_LAST_EVENT) printf(" (last event)");
printf("\n");
#endif
	if (toolParams->flags&AMFF_FIRST_EVENT) {
//		printf("%d - FIRST EVENT\n", mControlNumber);
		mLastRange.MakeInvalid();
	}
	if (toolParams->flags&AMFF_LAST_EVENT) {
//		printf("\t%d - LAST EVENT\n", mControlNumber);
		AmRange		newRange(range);
//printf("%d - Last range is %lld to %lld, new range is %lld to %lld\n", mControlNumber, mLastRange.start,
//mLastRange.end, newRange.start, newRange.end);
		if (mLastRange.IsValid() ) {
			if (mLastRange.Contains(newRange)) newRange.MakeInvalid();
			else if (mLastRange.Overlaps(newRange)) newRange -= mLastRange; 
		}
		if (newRange.IsValid()) {
//printf("\t%d - generate events\n", mControlNumber);
			mLastRange = newRange;
			float		amount = mAmount;
			float		pixels = (float)(toolParams->orig_y_pixel - toolParams->cur_y_pixel) / 100;
			if (pixels > 1) pixels = 1;
			else if (pixels < -1) pixels = -1;
			amount = pixels;
			return GenerateRhythm(newRange, event, amount, params, toolParams);
		}
	}

	return event;
}

status_t ArpRhythmiCcFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if (values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumber) != B_OK) return B_ERROR;
	if (values->AddInt32(FREQUENCY_STR, mFrequency) != B_OK) return B_ERROR;
	if ((err = values->AddBool(FOLLOW_X_STR, mFlags&FOLLOW_X)) != B_OK) return err;
//	if (values->AddInt32(FLAGS_STR, mFlags) != B_OK) return B_ERROR;
	if (mMotion) {
		BMessage	msg;
		if ((err = mMotion->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION_STR, &msg)) != B_OK) return err;
	}
	return B_OK;
}

status_t ArpRhythmiCcFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	BMessage	msg;
	if (values->FindMessage(MOTION_STR, &msg) == B_OK) {
		delete mMotion;
		mMotion = AmMotionI::NewMotion(msg);
	}

	int32		i;
	if (values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, &i) == B_OK) mControlNumber = (uint8)i;
	if (values->FindInt32(FREQUENCY_STR, &i) == B_OK) mFrequency = i;
	bool		b;
	if (values->FindBool(FOLLOW_X_STR, &b) == B_OK) {
		if (b) mFlags |= FOLLOW_X;
		else mFlags &= ~FOLLOW_X;
	}
//	if (values->FindInt32(FLAGS_STR, &i) == B_OK) mFlags = i;

	return B_OK;
}

status_t ArpRhythmiCcFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _RhythmiCcFilterSettings(mHolder, config));
	return B_OK;
}

status_t ArpRhythmiCcFilter::GetProperties(BMessage* properties) const
{
	status_t	err = inherited::GetProperties(properties);
	if (err != B_OK) return err;
	if ((err = properties->AddBool(FOLLOW_X_STR, mFlags&FOLLOW_X)) != B_OK) return err;
	return B_OK;
}

void ArpRhythmiCcFilter::InitMotion()
{
	/* Initialize myself to the first installed motion.
	 */
	delete mMotion;
	mMotion = NULL;
	BString		label, key;
	if (AmGlobals().GetMotionInfo(0, label, key) != B_OK) return;
	mMotion = AmGlobals().NewMotion(key);
}

AmEvent* ArpRhythmiCcFilter::GenerateRhythm(AmRange range, AmEvent* include,
											float amount, const am_filter_params* params,
											const am_tool_filter_params* toolParams)
{
	ArpASSERT(mMotion && params && toolParams);
	if (!mMotion || !params || !params->cur_signature) return include;
	uint32				measureCount = mMotion->CountMeasures();
	if (measureCount == 0) return include;
	AmEvent*			e = include;
	AmTime				curTime = range.start;
	while (curTime <= range.end) {
		AmSignature		measure;
		if (am_get_measure(curTime, params->cur_signature, measure) != B_OK)
			return e;
		int32			m = 0;
		if (measureCount > 1) m = (measure.Measure() - 1) % measureCount;
		BPoint			hit;
		for (uint32 k = 0; mMotion->GetHit(k, &hit) == B_OK; k++) {
			float		floorhit = floor(hit.x);
			if (m < int32(floorhit)) break;
			/* Generate events for just the given measure of the motion.
			 */
			if (m == int32(floorhit)) {
				float	shiftedHit = hit.x;
				if (m > 0) shiftedHit = hit.x - floorhit;
				AmTime	hitTime = AmTime(measure.StartTime() + (shiftedHit * measure.Duration()));
				if (hitTime > range.end) return e;
				/* Generate the event.
				 */
				int32				value = 64 + (int32)(64 * hit.y);
				float				newAmount = amount;
				if (mFlags&FOLLOW_X && toolParams && toolParams->cur_time >= 0
						&& toolParams->start_time >= 0 && toolParams->end_time >= 0) {
					newAmount = newAmount * am_x_amount(toolParams, hitTime);
				}
				value = int32(value * newAmount);
				if (value > 127) value = 127;
				else if (value < 0) value = 0;
				AmControlChange*	cc = new AmControlChange(mControlNumber, (uint8)value, hitTime);
				if (cc) {
					cc->SetNextFilter(mHolder->FirstConnection() );
					e = e->MergeEvent(cc);
				}
			}
		}
		/* The iteration over the hit got all hits in this measure,
		 * so I skip to the next measure for processing.
		 */
		curTime = measure.EndTime() + 1;
	}
	return e;
}

/*****************************************************************************
 * ARP-RHYTHMICC-FILTER-ADD-ON
 *****************************************************************************/
void ArpRhythmiCcFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpRhythmiCcFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(	int32 n, image_id /*you*/,
													const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpRhythmiCcFilterAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * _RHYTHMICC-FILTER-SETTINGS
 *****************************************************************************/
static const uint32		MOTION_MSG		= 'iRtm';
static const char*		MOTION_KEY_STR	= "motion_key";

static ArpMenuField* new_rhythm_menu_field()
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

//	menu->SetLabelFromMarked(true);
//	menu->SetRadioMode(true);	
	ArpMenuField*	field = new ArpMenuField("rhythm_field", "Motion:", menu);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

_RhythmiCcFilterSettings::_RhythmiCcFilterSettings(	AmFilterHolderI* target,
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

void _RhythmiCcFilterSettings::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (mRhythmField) mRhythmField->Menu()->SetTargetForItems(this);
}

void _RhythmiCcFilterSettings::MessageReceived(BMessage* msg)
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

void _RhythmiCcFilterSettings::AddViews(ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
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

	ArpKnobPanel*	kp = 0;
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(PROXIMITY_STR, "Proximity to beat:", impl.AttachControl(PROXIMITY_STR), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _ProximityFormat() );
	}

	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(FREQUENCY_STR, "Frequency:", impl.AttachControl(FREQUENCY_STR), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _FrequencyFormat() );
	}

	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(AMOUNT_STR, "Amount:", impl.AttachControl(AMOUNT_STR), MIN_AMOUNT, MAX_AMOUNT, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _OffPercentFormat() );
	}
	mRhythmField = new_rhythm_menu_field();
	if (mRhythmField) {
		vBar->AddLayoutChild(mRhythmField
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	}
}
