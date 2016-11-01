#include "ArpMotionSplitter.h"

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

static const char*		FOLLOW_X_STR			= "Follow X";
static const char*		FOLLOW_Y_STR			= "Follow Y";
static const char*		FREQUENCY_STR			= "frequency";
static const char*		AMOUNT_STR				= "amount";
static const char*		MOTION_FROM_TRACK_STR	= "Motion From Track";
static const char*		MOTION_STR				= "motion";
static const char*		SPLIT_POINT_STR			= "Split Point";

static const int32		MIN_AMOUNT				= -400;
static const int32		MAX_AMOUNT				= 400;
static const int32		MIN_SPLIT_POINT			= -100;
static const int32		MAX_SPLIT_POINT			= 100;

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
	ArpMenuField*		mRhythmField;
	AmMotionEditor*		mEditor;
	
	void AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
					float labelW, float intW,
					const BMessage& initSettings);
};

/*****************************************************************************
 * ARP-MOTION-SPLITTER-FILTER
 *****************************************************************************/
ArpMotionSplitterFilter::ArpMotionSplitterFilter(	ArpMotionSplitterAddOn* addon,
													AmFilterHolderI* holder,
													const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder), mMotion(NULL),
		  mChangeFlags(FOLLOW_Y_FLAG), mFrequency(100), mAmount(100),
		  mSplitPoint(0)
{
	InitMotion();
	if (settings) PutConfiguration(settings);
}

ArpMotionSplitterFilter::~ArpMotionSplitterFilter()
{
	delete mMotion;
}

AmEvent* ArpMotionSplitterFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

AmEvent* ArpMotionSplitterFilter::HandleBatchEvents(AmEvent* event,
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

AmEvent* ArpMotionSplitterFilter::HandleBatchToolEvents(AmEvent* event,
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

void ArpMotionSplitterFilter::VaccinateBatchEvents(	AmEvent* event, const am_filter_params* params,
													AmMotionChange* curMotion,
													const am_tool_filter_params* toolParams)
{
	ArpASSERT(params && params->cur_signature);
	if (!curMotion) return;
	AmSignature		measure(*(params->cur_signature) );
	int32			motionMeasureOffset = 0;
	while (event) {
		float		y;
		event->SetNextFilter(mHolder->FirstConnection() );
		if ( ShouldVaccinate()
				&& am_motion_hit_at_time(	event->StartTime(), &curMotion, &motionMeasureOffset,
											measure, &y) == B_OK) {
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
						amount = amount * am_x_amount(toolParams, event->StartTime() );
					}
				}
			}
			if (amount >= float(mSplitPoint) / 100) event->SetNextFilter(mHolder->ConnectionAt(0) );
			else event->SetNextFilter(mHolder->ConnectionAt(1) );
		}
		event = event->NextEvent();
	}

	measure.RemoveEvent();
}

status_t ArpMotionSplitterFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if (values->AddBool(FOLLOW_X_STR, mChangeFlags&FOLLOW_X_FLAG) != B_OK) return B_ERROR;
	if (values->AddBool(FOLLOW_Y_STR, mChangeFlags&FOLLOW_Y_FLAG) != B_OK) return B_ERROR;
	if (values->AddBool(MOTION_FROM_TRACK_STR, mChangeFlags&MOTION_FROM_TRACK_FLAG) != B_OK) return B_ERROR;
	if (values->AddInt32(FREQUENCY_STR, mFrequency) != B_OK) return B_ERROR;
	if (values->AddInt32(AMOUNT_STR, mAmount) != B_OK) return B_ERROR;
	if (values->AddInt32(SPLIT_POINT_STR, mSplitPoint) != B_OK) return B_ERROR;
	if (mMotion) {
		BMessage	msg;
		if ((err = mMotion->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION_STR, &msg)) != B_OK) return err;
	}

	return B_OK;
}

status_t ArpMotionSplitterFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);

	if (err != B_OK) return err;
	bool		b;
	if (values->FindBool(FOLLOW_X_STR, &b) == B_OK) {
		if (b) mChangeFlags |= FOLLOW_X_FLAG;
		else mChangeFlags &= ~FOLLOW_X_FLAG;
	}
	if (values->FindBool(FOLLOW_Y_STR, &b) == B_OK) {
		if (b) mChangeFlags |= FOLLOW_Y_FLAG;
		else mChangeFlags &= ~FOLLOW_Y_FLAG;
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
	if (values->FindInt32(SPLIT_POINT_STR, &i) == B_OK) {
		mSplitPoint = i;
		if (mSplitPoint < MIN_SPLIT_POINT) mSplitPoint = MIN_SPLIT_POINT;
		else if (mSplitPoint > MAX_SPLIT_POINT) mSplitPoint = MAX_SPLIT_POINT;
	}
	
	return B_OK;
}

status_t ArpMotionSplitterFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _VaccineFilterSettings(mHolder, config));
	return B_OK;
}

bool ArpMotionSplitterFilter::ShouldVaccinate() const
{
	if (mFrequency <= 0) return false;
	else if (mFrequency >= 100) return true;

	srand(system_time() );
	int32 percent = rand() % 100;

	if (percent < mFrequency) return true;
	else return false;
}

void ArpMotionSplitterFilter::InitMotion()
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
 * ARP-MOTION-SPLITTER-ADD-ON
 *****************************************************************************/
void ArpMotionSplitterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I send all events either down my pipeline or to my connection, if any.
		The Split point controls where to send events:  All events on or above the split
		point continue down my pipeline, all events below it are sent to my connection.
		The split point corresponds to the possible values of the motion, ranging from -100
		to 100.
		
		<h4>Amount</h4>
			<P>This control modifies the motion curve, acting as an offset to the split point.
			When the Amount: value is at 100%, the split point directly corresponds to
			the current motion.  As the amount increases, the split point effectively
			decreases.</P>
			
			<P>If Tools follow Y is on, then when this filter is used from a tool, the amount
			will be set according to the distance the mouse has traveled up or down from the
			point at which it was pressed.  Tools follow X adds an addition dimension, with
			the amount dropping off towards either end of the selected events.</P>
			
		<h4>Frequency</h4>
			This control determines how often the filter is applied.  With a Frequency
			of 0%, this filter is bypassed.  With a Frequency of 100% this filter
			operates on every event it receives.  When between those values, this filter
			will operate on each event based on the selected Frequency.
			
		<h4>Motion Selection</h4>
			The Use Motion button presents a list of all the installed motions and lets
			you select one, copying the selection into the filter and making it the active
			motion.  If Use motion from track is on, then the filter's current motion is
			ignored, and whatever motion is active in the track for the events being processed
			is used.";
}

void ArpMotionSplitterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpMotionSplitterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpMotionSplitterAddOn(cookie);
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
		  mRhythmField(NULL), mEditor(NULL)
{
	float	labelW = -1, intW = -1;
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

	vBar->AddLayoutChild((new ArpIntControl(
								SPLIT_POINT_STR, "Split point:",
								mImpl.AttachControl(SPLIT_POINT_STR),
								-100, 100))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)
			)
		);

	ArpBaseLayout*	box = NULL;
	ArpKnobPanel*	kp = NULL;
	vBar->AddLayoutChild((box = new ArpBox("AmountBox", "Amount"))
		->SetConstraints(ArpMessage()
			.SetFloat(ArpRunningBar::WeightC,0)
			.SetInt32(ArpRunningBar::FillC,ArpFillAll)
			.SetBool(ArpRunningBar::AlignLabelsC,false)
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
												FOLLOW_X_STR, "Tools follow X",
												mImpl.AttachCheckBox(FOLLOW_X_STR))))										
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
			->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
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
