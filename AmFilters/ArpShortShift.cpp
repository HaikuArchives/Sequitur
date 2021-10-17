#include "ArpShortShift.h"

#include <cstdio>
#include <cstdlib>
#include <interface/CheckBox.h>
#include <interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		START_QUANT_STR			= "start_quant";
static const char*		START_DIV_STR			= "start_div";
static const char*		START_MULT_STR			= "start_mult";
static const char*		START_TOOL_USE_GRID_STR	= "start_tool_use_grid";
static const char*		START_NEG_STR			= "start_neg";
static const char*		END_QUANT_STR			= "end_quant";
static const char*		END_DIV_STR				= "end_div";
static const char*		END_MULT_STR			= "end_mult";
static const char*		END_TOOL_USE_GRID_STR	= "end_tool_use_grid";
static const char*		END_NEG_STR				= "end_neg";

/*****************************************************************************
 * _RHYTHMICC-FILTER-SETTINGS
 *****************************************************************************/
class _ShortShiftSettings : public AmFilterConfigLayout
{
public:
	_ShortShiftSettings(AmFilterHolderI* target,
						const BMessage& initSettings);

protected:
	typedef AmFilterConfigLayout inherited;
};

/*****************************************************************************
 * ARP-RHYTHMICC-FILTER
 *****************************************************************************/
ArpShortShiftFilter::ArpShortShiftFilter(	ArpShortShiftAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder),
		  mStartMult(1), mStartQuant(PPQN), mStartDiv(2), mStartToolUseGrid(false), mStartNeg(false),
		  mEndMult(1), mEndQuant(PPQN), mEndDiv(2), mEndToolUseGrid(false), mEndNeg(true)
{
	if (settings) PutConfiguration(settings);
}

ArpShortShiftFilter::~ArpShortShiftFilter()
{
}

AmEvent* ArpShortShiftFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event || !mHolder) return event;

	Shift(event, mStartMult, mStartQuant, mStartDiv, mEndMult, mEndQuant, mEndDiv);

	return event;
}

AmEvent* ArpShortShiftFilter::HandleToolEvent(	AmEvent* event,
												const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	if (!event || !mHolder || !toolParams) return event;

	int32		sMult = mStartMult, sDiv = mStartDiv,
				eMult = mEndMult, eDiv = mEndDiv;
	AmTime		sQuant = mStartQuant, eQuant = mEndQuant;

	if (mStartToolUseGrid) {
		sMult = toolParams->grid_multiplier;
		sQuant = toolParams->grid_value;
		sDiv = toolParams->grid_divider;
	}
	if (mEndToolUseGrid) {
		eMult = toolParams->grid_multiplier;
		eQuant = toolParams->grid_value;
		eDiv = toolParams->grid_divider;
	}

	Shift(event, sMult, sQuant, sDiv, eMult, eQuant, eDiv);

	return event;
}

void ArpShortShiftFilter::Shift(AmEvent* event,
								int32 sMult, AmTime sQuant, int32 sDiv,
								int32 eMult, AmTime eQuant, int32 eDiv)
{
	ArpASSERT(event);
	
	event->SetNextFilter(mHolder->FirstConnection() );

	AmTime		endTime = event->EndTime();
	
	AmTime		shift = (((sQuant*2)/sDiv)*sMult);
	if (mStartNeg) shift = -shift;
	event->SetStartTime(event->StartTime() + shift);

	shift = (((eQuant*2)/eDiv)*eMult);
	if (mEndNeg) shift = -shift;
	event->SetEndTime(endTime + shift);
}

status_t ArpShortShiftFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err = values->AddInt32(START_MULT_STR, mStartMult)) != B_OK) return err;
	if ((err = add_time(*values, START_QUANT_STR, mStartQuant)) != B_OK) return err;
	if ((err = values->AddInt32(START_DIV_STR, mStartDiv)) != B_OK) return err;
	if ((err = values->AddBool(START_TOOL_USE_GRID_STR, mStartToolUseGrid)) != B_OK) return err;
	if ((err = values->AddBool(START_NEG_STR, mStartNeg)) != B_OK) return err;

	if ((err = values->AddInt32(END_MULT_STR, mEndMult)) != B_OK) return err;
	if ((err = add_time(*values, END_QUANT_STR, mEndQuant)) != B_OK) return err;
	if ((err = values->AddInt32(END_DIV_STR, mEndDiv)) != B_OK) return err;
	if ((err = values->AddBool(END_TOOL_USE_GRID_STR, mEndToolUseGrid)) != B_OK) return err;
	if ((err = values->AddBool(END_NEG_STR, mEndNeg)) != B_OK) return err;

	return B_OK;
}

status_t ArpShortShiftFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32		i;
	AmTime		t;
	bool		b;
	if (values->FindInt32(START_MULT_STR, &i) == B_OK) mStartMult = i;
	if (find_time(*values, START_QUANT_STR, &t) == B_OK) mStartQuant = t;
	if (values->FindInt32(START_DIV_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mStartDiv = i;
	}
	if (values->FindBool(START_TOOL_USE_GRID_STR, &b) == B_OK) mStartToolUseGrid = b;
	if (values->FindBool(START_NEG_STR, &b) == B_OK) mStartNeg = b;

	if (values->FindInt32(END_MULT_STR, &i) == B_OK) mEndMult = i;
	if (find_time(*values, END_QUANT_STR, &t) == B_OK) mEndQuant = t;
	if (values->FindInt32(END_DIV_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mEndDiv = i;
	}
	if (values->FindBool(END_TOOL_USE_GRID_STR, &b) == B_OK) mEndToolUseGrid = b;
	if (values->FindBool(END_NEG_STR, &b) == B_OK) mEndNeg = b;

	return B_OK;
}

status_t ArpShortShiftFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _ShortShiftSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-SHORT-SHIFT-FILTER-ADD-ON
 *****************************************************************************/
void ArpShortShiftAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I offset the start and end times of each event I receive. \n"
	"The Start shift parameter increases the start time by the given value.  If \n"
	"the Negative box is checked, the start time decreases.  If the Use tool grid \n"
	"box is checked, then the Shift: time specified is ignored, and the current \n"
	"grid is used.</p>\n"
	"\n"
	"<p>The End parameters work the same, but affect the end time of each \n"
	"event.  To retain the same note duration, the End shift and End negative \n"
	"values should be identical to the Start shift and Start negative values.</p>\n";
}

void ArpShortShiftAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpShortShiftAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(	int32 n, image_id /*you*/,
													const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpShortShiftAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * _SHORT-SHIFT-SETTINGS
 *****************************************************************************/
_ShortShiftSettings::_ShortShiftSettings(	AmFilterHolderI* target,
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
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpBox("StartBox", "Start"))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					.SetBool(ArpRunningBar::AlignLabelsC,false)
				)
				->AddLayoutChild((new ArpRunningBar("StartVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new AmDurationControl("quantize_to", "Shift:", this, initSettings,
											AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
											START_QUANT_STR, START_DIV_STR, START_MULT_STR))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							.SetBool(ArpRunningBar::AlignLabelsC,true)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														START_TOOL_USE_GRID_STR, "Tools use grid",
														mImpl.AttachCheckBox(START_TOOL_USE_GRID_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														START_NEG_STR, "Negative",
														mImpl.AttachCheckBox(START_NEG_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
				)
			)
			->AddLayoutChild((new ArpBox("EndBox", "End"))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					.SetBool(ArpRunningBar::AlignLabelsC,false)
				)
				->AddLayoutChild((new ArpRunningBar("EndVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new AmDurationControl("quantize_to", "Shift:", this, initSettings,
											AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
											END_QUANT_STR, END_DIV_STR, END_MULT_STR))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							.SetBool(ArpRunningBar::AlignLabelsC,true)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														END_TOOL_USE_GRID_STR, "Tools use grid",
														mImpl.AttachCheckBox(END_TOOL_USE_GRID_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														END_NEG_STR, "Negative",
														mImpl.AttachCheckBox(END_NEG_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
				)
			)
		);

	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
}

