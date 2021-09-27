#include "ArpUnstack.h"

#include <cstdio>
#include <cstdlib>
#include <interface/CheckBox.h>
#include <interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char*		THRESHOLD_STR			= "threshold";
static const char*		FREQUENCY_STR			= "frequency";
static const char*		SHIFT_ON_STR			= "shift_on";
static const char*		SHIFT_QUANT_STR			= "shift_quant";
static const char*		SHIFT_DIV_STR			= "shift_div";
static const char*		SHIFT_MULT_STR			= "shift_mult";
static const char*		SHIFT_TOOL_USE_GRID_STR	= "shift_tool_use_grid";
static const char*		SHIFT_NEG_STR			= "shift_neg";

/*****************************************************************************
 * _UNSTACK-SETTINGS
 *****************************************************************************/
class _UnstackSettings : public AmFilterConfigLayout
{
public:
	_UnstackSettings(AmFilterHolderI* target,
						const BMessage& initSettings);

protected:
	typedef AmFilterConfigLayout inherited;
};

/*****************************************************************************
 * ARP-RHYTHMICC-FILTER
 *****************************************************************************/
ArpUnstackFilter::ArpUnstackFilter(	ArpUnstackAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder),
		  mThreshold(1), mFrequency(50), mShiftOn(false), mShiftMult(1),
		  mShiftQuant(PPQN), mShiftDiv(2), mShiftToolUseGrid(false), mShiftNeg(false)
{
	if (settings) PutConfiguration(settings);
}

ArpUnstackFilter::~ArpUnstackFilter()
{
}

AmEvent* ArpUnstackFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

AmEvent* ArpUnstackFilter::HandleBatchEvents(	AmEvent* event,
												const am_filter_params* params,
												const AmEvent* /*lookahead*/)
{
	if (!event || !mHolder) return event;

	return Unstack(event, mShiftMult, mShiftQuant, mShiftDiv);
}

AmEvent* ArpUnstackFilter::HandleBatchToolEvents(	AmEvent* event,
													const am_filter_params* params,
													const am_tool_filter_params* toolParams,
													const AmEvent* /*lookahead*/)
{
	if (!event || !mHolder || !toolParams) return event;

	int32		shiftMult = mShiftMult, shiftDiv = mShiftDiv;
	AmTime		shiftQuant = mShiftQuant;

	if (mShiftToolUseGrid) {
		shiftMult = toolParams->grid_multiplier;
		shiftQuant = toolParams->grid_value;
		shiftDiv = toolParams->grid_divider;
	}

	return Unstack(event, shiftMult, shiftQuant, shiftDiv);
}

status_t ArpUnstackFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if ((err = values->AddInt32(THRESHOLD_STR, mThreshold)) != B_OK) return err;
	if ((err = values->AddInt32(FREQUENCY_STR, mFrequency)) != B_OK) return err;
	if ((err = values->AddBool(SHIFT_ON_STR, mShiftOn)) != B_OK) return err;
	if ((err = values->AddInt32(SHIFT_MULT_STR, mShiftMult)) != B_OK) return err;
	if ((err = add_time(*values, SHIFT_QUANT_STR, mShiftQuant)) != B_OK) return err;
	if ((err = values->AddInt32(SHIFT_DIV_STR, mShiftDiv)) != B_OK) return err;
	if ((err = values->AddBool(SHIFT_TOOL_USE_GRID_STR, mShiftToolUseGrid)) != B_OK) return err;
	if ((err = values->AddBool(SHIFT_NEG_STR, mShiftNeg)) != B_OK) return err;

	return B_OK;
}

status_t ArpUnstackFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);
	if (err != B_OK) return err;

	int32		i;
	AmTime		t;
	bool		b;
	if (values->FindInt32(THRESHOLD_STR, &i) == B_OK) {
		mThreshold = i;
		if (mThreshold < 1) mThreshold = 1;
	}
	if (values->FindInt32(FREQUENCY_STR, &i) == B_OK) mFrequency = i;
	if (values->FindBool(SHIFT_ON_STR, &b) == B_OK) mShiftOn = b;
	if (values->FindInt32(SHIFT_MULT_STR, &i) == B_OK) mShiftMult = i;
	if (find_time(*values, SHIFT_QUANT_STR, &t) == B_OK) mShiftQuant = t;
	if (values->FindInt32(SHIFT_DIV_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mShiftDiv = i;
	}
	if (values->FindBool(SHIFT_TOOL_USE_GRID_STR, &b) == B_OK) mShiftToolUseGrid = b;
	if (values->FindBool(SHIFT_NEG_STR, &b) == B_OK) mShiftNeg = b;

	return B_OK;
}

status_t ArpUnstackFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _UnstackSettings(mHolder, config));
	return B_OK;
}

AmEvent* ArpUnstackFilter::Unstack(	AmEvent* event, int32 shiftMult, AmTime shiftQuant,
									int32 shiftDiv)
{
	AmTime		shift = (((shiftQuant * 2)/ shiftDiv) * shiftMult);
	if (mShiftNeg) shift = -shift;
	srand(system_time() );

	AmEvent*	ans = NULL;
	AmEvent*	thinner = NULL;
	AmTime		time = -1;
	uint32		thinCount = 0;
	while (event) {
		AmEvent*		next = event->RemoveEvent();

		if (event->StartTime() != time && thinner) {
			ans = ThinAndMerge(thinner, ans, thinCount, shift);
			thinner = NULL;
			thinCount = 0;
		}

		if (event->Type() != event->NOTEON_TYPE) {
			if (!ans) ans = event;
			else ans = ans->MergeEvent(event);
		} else {
			if (!thinner) thinner = event;
			else thinner = thinner->MergeEvent(event);
			thinCount++;
		}

		time = event->StartTime();
		event = next;
	}

	if (thinner) ans = ThinAndMerge(thinner, ans, thinCount, shift);
	if (!ans) return NULL;
	return ans->HeadEvent();
}

AmEvent* ArpUnstackFilter::ThinAndMerge(AmEvent* thin, AmEvent* ans, uint32 thinCount, AmTime shift)
{
	if (!thin) return ans;
	thin = thin->HeadEvent();

	if (int32(thinCount) > mThreshold) {
		if (mFrequency > 0) {
			AmEvent*	e = thin;
			thin = NULL;
			while (e) {
				AmEvent*	next = e->RemoveEvent();
				if ( (rand() % 100) <= mFrequency) {
					e->Delete();
				} else {
					if (!thin) thin = e;
					else thin = thin->MergeEvent(e);
				}
				e = next;
			}
		}

		if (thin) thin = thin->HeadEvent();
		
		if (mShiftOn) {
			AmEvent*	e = thin;
			uint32		c = 0;
			while (e) {
				e->SetStartTime(e->StartTime() + (shift * c));
				e = e->NextEvent();
				c++;
			}
		}
	}
	
	AmEvent*		newAns = ans;
	while (thin) {
		AmEvent*	next = thin->RemoveEvent();
		if (!newAns) newAns = thin;
		else newAns = newAns->MergeEvent(thin);
		thin = next;
	}
	return newAns;
}

/*****************************************************************************
 * ARP-UNSTACK-ADD-ON
 *****************************************************************************/
void ArpUnstackAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I thin out stacks of notes, where a stack is a series of notes \n"
	"at the same start time.  The Frequency control determines how many notes \n"
	"from a stack I remove.  If this value is at Never, I do not remove any notes. \n"
	"If it is at Always, I remove every note in every stack.  The Threshold control \n"
	"determines how many notes must start at the same time before I consider them \n"
	"a stack.  The default threshold is 1, meaning that any group of two or more \n"
	"notes occurring at the same time is a stack.</p>\n"
	"\n"
	"<p>The Shift parameters are an optional time shift I can apply to the notes, \n"
	"which is another way to thin them.  These parameters will only work if the \n"
	"On checkbox is checked.</p>\n"
	"\n"
	"<p>The Start shift parameter increases the start time of each note in a \n"
	"stack by the given value, resulting in a staircase effect for groups of notes. \n"
	"If the Negative box is checked, the start time decreases.  If the Use tool \n"
	"grid box is checked, then the Shift: time specified is ignored, and the current \n"
	"grid is used.</p>\n";
}

void ArpUnstackAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpUnstackAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(	int32 n, image_id /*you*/,
													const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpUnstackAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 * _UNSTACK-SETTINGS
 *****************************************************************************/
_UnstackSettings::_UnstackSettings(	AmFilterHolderI* target,
									const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
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
			->AddLayoutChild((new ArpIntControl(
									THRESHOLD_STR, "Threshold:",
									mImpl.AttachControl(THRESHOLD_STR),
									1, 999))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)
			->AddLayoutChild((new ArpBox("ShiftBox", "Shift"))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					.SetBool(ArpRunningBar::AlignLabelsC,false)
				)
				->AddLayoutChild((new ArpRunningBar("ShiftVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														SHIFT_ON_STR, "On",
														mImpl.AttachCheckBox(SHIFT_ON_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new AmDurationControl("quantize_to", "Shift:", this, initSettings,
											AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
											SHIFT_QUANT_STR, SHIFT_DIV_STR, SHIFT_MULT_STR))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							.SetBool(ArpRunningBar::AlignLabelsC,true)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														SHIFT_TOOL_USE_GRID_STR, "Tools use grid",
														mImpl.AttachCheckBox(SHIFT_TOOL_USE_GRID_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(new BCheckBox( BRect(0,0,0,0),
														SHIFT_NEG_STR, "Negative",
														mImpl.AttachCheckBox(SHIFT_NEG_STR))))										
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					)
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
