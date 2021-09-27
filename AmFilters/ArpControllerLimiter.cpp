#include <cstdio>
#include <cstdlib>
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
#include "ArpControllerLimiter.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		MOTION1_STR			= "motion1";
static const char*		MOTION2_STR			= "motion2";

/**********************************************************************
 * _MC-FILTER-SETTINGS
 **********************************************************************/
class _McFilterSettings : public AmFilterConfigLayout
{
public:
	_McFilterSettings(	AmFilterHolderI* target,
						const BMessage& initSettings);

	virtual void MessageReceived(BMessage* msg);

protected:
	typedef AmFilterConfigLayout inherited;
	AmMotionEditor*		mEditor1;
	AmMotionEditor*		mEditor2;
	
	void AddViews(	ArpBaseLayout* toLayout, ArpConfigureImpl& impl,
					float labelW, float intW,
					const BMessage& initSettings);
	void RefreshControls(const BMessage& settings);
};

/*****************************************************************************
 * ARP-MOTION-CONTROL-FILTER
 *****************************************************************************/
ArpControllerLimiterFilter::ArpControllerLimiterFilter(	ArpControllerLimiterAddOn* addon,
												AmFilterHolderI* holder,
												const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder),
		  mMotion1(0), mMotion2(0), mControlNumber(10)
{
	mMotion1 = NewInitMotion("Pulse 4");
	mMotion2 = NewInitMotion("Pulse 4");
	if (settings) PutConfiguration(settings);
}

ArpControllerLimiterFilter::~ArpControllerLimiterFilter()
{
	delete mMotion1;
	delete mMotion2;
}

AmEvent* ArpControllerLimiterFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
//printf("1 params %p\n", params);
	if (!event || event->Type() != event->CONTROLCHANGE_TYPE || !params) return event;
	if (!mMotion1 || !mMotion2) return event;
//printf("2\n");

	AmControlChange*		e = dynamic_cast<AmControlChange*>(event);
	if (!e) return event;
//printf("3 %d (%d)\n", e->ControlNumber(), mControlNumber);
	if (mControlNumber != e->ControlNumber()) return event;
//printf("4\n");
	float			y1, y2;
	if (am_get_motion_y(event->StartTime(), mMotion1, params, &y1) != B_OK) return event;
	if (am_get_motion_y(event->StartTime(), mMotion2, params, &y2) != B_OK) return event;
//printf("Motion1 %f, Motion2 %f\n", y1, y2);
	
	int32			low = int32( ((y1 + 1) * 0.5) * 127),
					high = int32( ((y2 + 1) * 0.5) * 127);
	if (low < 0) low = 0;
	else if (low > 127) low = 127;
	if (high < 0) high = 0;
	else if (high > 127) high = 127;
	if (low > high) {
		int32		t = low;
		low = high;
		high = t;
	}

	int32			cv = e->ControlValue();
	if (cv < low) e->SetControlValue(uint8(low));
	else if (cv > high) e->SetControlValue(uint8(high));
	return event;
}

status_t ArpControllerLimiterFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err = values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumber)) != B_OK) return err;
	if (mMotion1) {
		BMessage	msg;
		if ((err = mMotion1->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION1_STR, &msg)) != B_OK) return err;
	}
	if (mMotion2) {
		BMessage	msg;
		if ((err = mMotion2->WriteTo(msg)) != B_OK) return err;
		if ((err = values->AddMessage(MOTION2_STR, &msg)) != B_OK) return err;
	}

	return B_OK;
}

status_t ArpControllerLimiterFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	int32		i;
	if (values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, &i) == B_OK) mControlNumber = (uint8)i;

	BMessage	msg;
	if (values->FindMessage(MOTION1_STR, &msg) == B_OK) {
		delete mMotion1;
		mMotion1 = AmMotionI::NewMotion(msg);
		msg.MakeEmpty();
	}
	if (values->FindMessage(MOTION2_STR, &msg) == B_OK) {
		delete mMotion2;
		mMotion2 = AmMotionI::NewMotion(msg);
	}

	return B_OK;
}

status_t ArpControllerLimiterFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _McFilterSettings(mHolder, config));
	return B_OK;
}

AmMotionI* ArpControllerLimiterFilter::NewInitMotion(const BString& key) const
{
	/* Answer the requested motion.
	 */
	AmMotionI*		m = AmGlobals().NewMotion(key);
	if (m) return m;
	/* If it's not there, answer the first installed motion.
	 */
	BString			lbl, k;
	if (AmGlobals().GetMotionInfo(0, lbl, k) == B_OK) {
		m = AmGlobals().NewMotion(k);
		if (m) return m;
	}
	/* If it's not there, answer a blank motion.
	 */
	BMessage		config;
	return AmMotionI::NewMotion(config);
}

// #pragma mark -

/*****************************************************************************
 * ARP-MOTION-CONTROL-ADD-ON
 *****************************************************************************/
ArpControllerLimiterAddOn::ArpControllerLimiterAddOn(const void* cookie)
		: AmFilterAddOn(cookie)
{
}

void ArpControllerLimiterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I transform each note I receives into a series of control changes \n"
		"based on my current motion.</P>\n"
	"<h4>Amount</h4>\n"
	"	Amount determines the level of the controls generated. \n"
	"	<UL>\n"
	"		<LI><I>From velocity</I> uses each note's velocity to determine the amount. \n"
	"				For example, if a note has a velocity of 64, then the final generated \n"
	"				control changes will range from 0 to 64.  If a note has a velocity of \n"
	"				100, then the resulting control changes will range from 0 to 100.</LI> \n"
	"		<LI><I>From value</I> uses the value of the knob (immediately below the From \n"
	"				value radio button) to determine the final control change values.  If \n"
	"				the knob is at 100, then motion values of 100 translate to the maximum \n"
	"				possible control change value - 127.  If the knob has a negative value, \n"
	"				then the motion is inverted.</LI>\n"
	"		<LI><I>Follow mouse</I> uses the mouse's Y position to determine the level.</LI>\n"
	"	</UL>\n"
	""
	"<h4>Motion</h4>\n"
	"	The Motion menu button presents a list of all available motions.  Selecting one \n"
	"	will copy that motion into the filter, making it the current motion.  Below this \n"
	"	button is a motion editor, which operates the same as the Edit Motion window. \n"
	"\n"
	"<h4>Control Number</h4>\n"
	"	This list box allows you to select the type of control change that will be created.\n";
}

void ArpControllerLimiterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpControllerLimiterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpControllerLimiterAddOn(cookie);
	return NULL;
}

// #pragma mark -

/**********************************************************************
 * _MC-FILTER-SETTINGS
 **********************************************************************/
_McFilterSettings::_McFilterSettings(	AmFilterHolderI* target,
										const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings),
		  mEditor1(0), mEditor2(0)
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

void _McFilterSettings::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
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

	vBar->AddLayoutChild(mEditor1 = new AmMotionEditor("motion1_editor", this, initSettings, MOTION1_STR))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,1)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest));
	vBar->AddLayoutChild(mEditor2 = new AmMotionEditor("motion2_editor", this, initSettings, MOTION2_STR))
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
#if 0
	int32		i;
	if (settings.FindInt32(MODE_STR, &i) == B_OK) {
		if (i == ArpControllerLimiterFilter::VELOCITY_AMOUNT) {
			if (mVelocityRadio) mVelocityRadio->SetValue(B_CONTROL_ON);
		} else if (i == ArpControllerLimiterFilter::SELECTED_AMOUNT) {
			if (mValueRadio) mValueRadio->SetValue(B_CONTROL_ON);
		}
	}

	if (settings.FindInt32(AMOUNT_STR, &i) == B_OK) {
		if (mAmountKnob && mAmountKnob->KnobControl() )
			mAmountKnob->KnobControl()->SetValue(i);
	}
#endif
}
