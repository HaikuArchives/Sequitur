/* ArpQuantize.cpp
 */
#include "ArpQuantize.h"

#include <stdio.h>
#include <stdlib.h>
#include <interface/CheckBox.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/RadioButton.h>
#include <interface/StringView.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "ArpViews/ArpRangeControl.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfig.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "AmPublic/AmSongObserver.h"

ArpMOD();
static AmStaticResources gRes;

static const uint32		ABSOLUTE_MSG		= 'iabs';
static const uint32		SCALE_MSG			= 'iscl';
static const uint32 	GRID_MSG			= 'iGrd';

static const AmTime		QUANT_PRIMES		= 3*5*7;

static const char*		QUANTIZE_STR		= "quantize";
static const char*		MODIFIER_STR		= "modifier";
static const char*		GRID_STR			= "grid";
static const char*		DO_START_TIME_STR	= "do_start_time";
static const char*		DO_END_TIME_STR		= "do_duration";

static const uint32		CHOOSE_SHIFT_MSG	= 'icsf';
static const char*		SHIFT_STR			= "shift";

static const char*		ATTRACTION_STR		= "attr";

/*****************************************************************************
 * ARP-QUANTIZE-FILTER
 *****************************************************************************/
ArpQuantizeFilter::ArpQuantizeFilter(	ArpQuantizeFilterAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mMultiplier(1), mQuantizeTime(PPQN), mDivider(2), mFullTime(mMultiplier*((mQuantizeTime*2*QUANT_PRIMES)/mDivider)),
		  mGridChoice(MY_GRID), mDoStartTime(true), mDoEndTime(true),
		  mShift(LEFT_SHIFT), mAttraction(100)
{
	if (config) PutConfiguration(config);
}

ArpQuantizeFilter::~ArpQuantizeFilter()
{
}

AmEvent* ArpQuantizeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	DoQuantize(event, mFullTime, mMultiplier*((mQuantizeTime*2)/mDivider));

	return event;
}

AmEvent* ArpQuantizeFilter::HandleToolEvent(AmEvent* event,
											const am_filter_params* params,
											const am_tool_filter_params* toolParams)
{
	if (!event || !toolParams) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	AmTime		fullTime = mFullTime;
	int64		smallest = mMultiplier * ((mQuantizeTime * 2) / mDivider);
	if (mGridChoice == TOOL_GRID) {
		fullTime = toolParams->grid_multiplier * ((toolParams->grid_value * 2 * QUANT_PRIMES) / toolParams->grid_divider);
		smallest = toolParams->grid_multiplier * ((toolParams->grid_value * 2) / toolParams->grid_divider);
	}

	DoQuantize(event, fullTime, smallest);

	return event;
}

/*************************************************************************
 * _PERCENT-FORMAT
 * A class that formats floats as percentages.
 *************************************************************************/
class _PercentFormat : public ArpIntFormatterI
{
public:
	_PercentFormat()	{ ; }
	virtual void FormatInt(int32 number, BString& out) const
	{
		out << number << '%';
	}
};

static const char* gShiftNames[] = {
	"Shift Left",
	"Shift Right",
	"Shift Closest",
	0
};

static void add_menu_item(const char* label, uint32 msgWhat, const char* msgKey, int32 msgIndex, int32 initialIndex, BMenu* toMenu, bool enabled = true)
{
	if (!toMenu) return;
	BMessage*	msg = new BMessage( msgWhat );
	if( !msg ) return;
	if( msg->AddInt32( msgKey, msgIndex ) != B_OK ) {
		delete msg;
		return;
	}
	BMenuItem*	item = new BMenuItem( label, msg );
	if( !item ) {
		delete msg;
		return;
	}
	if( msgIndex == initialIndex ) item->SetMarked( true );
	item->SetEnabled( enabled );
	toMenu->AddItem(item);
}

class ArpQuantizeFilterSettings : public AmFilterConfigLayout
{
public:
	ArpQuantizeFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings),
		  mGridCheck(NULL), mDurCtrl(NULL), mShiftMenu(0), mShiftPopUpMenu(0)
	{
		int32			initialShift = 0;
		initSettings.FindInt32(SHIFT_STR, &initialShift);

		mShiftPopUpMenu = new BPopUpMenu("shift");
		for (int32 i = 0; gShiftNames[i] != 0; i++)
			add_menu_item(gShiftNames[i], CHOOSE_SHIFT_MSG, SHIFT_STR, i, initialShift, mShiftPopUpMenu);

		float			labelW = -1, intW = -1;
		const BFont*	font = be_plain_font;
		if (font) {
			labelW = font->StringWidth("Attraction:");
			intW = font->StringWidth("100%") + 5;
		}

		ArpKnobPanel*	kp = 0;

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

				->AddLayoutChild((new ArpBox("DurationBox", "Quantize"))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
					->AddLayoutChild((new ArpRunningBar("DurationVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->AddLayoutChild((new ArpViewWrapper(mGridCheck =
							new BCheckBox(BRect(0,0,10,10), "grid", "Tools use grid",
									new BMessage(GRID_MSG),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
								.SetBool(ArpRunningBar::AlignLabelsC,false)
							)
						)
						->AddLayoutChild((mDurCtrl = new AmDurationControl("quantize_to", "", this, initSettings,
																AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
																QUANTIZE_STR, MODIFIER_STR))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,3)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
								.SetBool(ArpRunningBar::AlignLabelsC,true)
							)
						)
					)
				)

				->AddLayoutChild((mShiftMenu =
								  new ArpMenuField("shift_menu", "Shift: ",
													mShiftPopUpMenu))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)
				->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(ATTRACTION_STR, "Attraction:", mImpl.AttachControl(ATTRACTION_STR), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					)
				)

				->AddLayoutChild((new ArpViewWrapper(
						new BCheckBox(BRect(0,0,10,10),
										DO_START_TIME_STR,
										"Quantize start time",
										mImpl.AttachCheckBox(DO_START_TIME_STR),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpViewWrapper(
						new BCheckBox(BRect(0,0,10,10),
										DO_END_TIME_STR,
										"Quantize end time",
										mImpl.AttachCheckBox(DO_END_TIME_STR),
										B_FOLLOW_NONE,
										B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE) ))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
			);
		} catch(...) {
			throw;
		}

		if (kp) {
			ArpIntControl*	intCtrl = kp->IntControl();
			if (intCtrl) intCtrl->SetFormatter(new _PercentFormat() );
		}

		Implementation().RefreshControls(mSettings);
		RefreshControls(initSettings);
	}

	void AttachedToWindow()
	{
		inherited::AttachedToWindow();
		if (mShiftPopUpMenu) mShiftPopUpMenu->SetTargetForItems(this);
	}

	virtual	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
			case GRID_MSG: {
				BMessage		upd;
				if (upd.AddInt32(GRID_STR, mGridCheck->Value()
								? ArpQuantizeFilter::TOOL_GRID : ArpQuantizeFilter::MY_GRID) == B_OK)
					Implementation().SendConfiguration(&upd);
			} break;
			case CHOOSE_SHIFT_MSG:
				{
					if (mShiftPopUpMenu) {
						BMenuItem*	item = mShiftPopUpMenu->FindMarked();
						BMessage*	msg = item ? item->Message() : 0;
						if (msg) {
							Implementation().SendConfiguration(msg);
							mSettings.Update(*msg);
						}
					}
				}
				break;
			case ARP_PUT_CONFIGURATION_MSG:
				{
					BMessage	settings;
					if (msg->FindMessage("settings", &settings) == B_OK)
						if (mDurCtrl) mDurCtrl->Refresh(settings);
				}
				// Note: no break on purpose
			default:
				inherited::MessageReceived( msg );
		}
	}

protected:
	typedef AmFilterConfigLayout inherited;
	BCheckBox*				mGridCheck;
	AmDurationControl*		mDurCtrl;
	ArpMenuField*			mShiftMenu;
	BPopUpMenu*				mShiftPopUpMenu;

	void RefreshControls(const BMessage& settings)
	{
		int32		i;
		if (settings.FindInt32(GRID_STR, &i) == B_OK) {
			mGridCheck->SetValue(i == ArpQuantizeFilter::MY_GRID ? B_CONTROL_OFF : B_CONTROL_ON);
		}
		BMenuItem*	item;
		if (mShiftPopUpMenu && settings.FindInt32(SHIFT_STR, &i) == B_OK) {
			for (int32 k = 0; (item = mShiftPopUpMenu->ItemAt(k)); k++) {
				if (k == i) item->SetMarked(true);
				else item->SetMarked(false);
			}
		}
	}

};

status_t ArpQuantizeFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ( (err = values->AddInt32(AM_MULTIPLIER_CONTROL_KEY_STR, mMultiplier)) != B_OK ) return err;
	if ( (err = add_time(*values, QUANTIZE_STR, mQuantizeTime)) != B_OK ) return err;
	if ( (err = values->AddInt32(MODIFIER_STR, mDivider)) != B_OK ) return err;
	if ( (err = values->AddInt32(GRID_STR, mGridChoice)) != B_OK) return err;
	if ( (err = values->AddInt32(SHIFT_STR, mShift)) != B_OK) return err;
	if ( (err = values->AddInt32(ATTRACTION_STR, mAttraction)) != B_OK) return err;
	if ( (err = values->AddBool(DO_START_TIME_STR, mDoStartTime)) != B_OK ) return err;
	if ( (err = values->AddBool(DO_END_TIME_STR, mDoEndTime)) != B_OK ) return err;
	return B_OK;
}

status_t ArpQuantizeFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	AmTime		t;
	int32		i;
	bool		b;
	if( values->FindInt32(AM_MULTIPLIER_CONTROL_KEY_STR, &i) == B_OK) mMultiplier = i;
	if( find_time(*values, QUANTIZE_STR, &t) == B_OK) mQuantizeTime = t;
	if( values->FindInt32(MODIFIER_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mDivider = i;
	}
	if (values->FindInt32(GRID_STR, &i) == B_OK) {
		if (i < MY_GRID) i = MY_GRID;
		if (i > TOOL_GRID) i = TOOL_GRID;
		mGridChoice = i;
	}
	if (values->FindInt32(SHIFT_STR, &i) == B_OK) mShift = i;
	if (values->FindInt32(ATTRACTION_STR, &i) == B_OK) mAttraction = i;
	if (values->FindBool(DO_START_TIME_STR, &b) == B_OK) mDoStartTime = b;
	if (values->FindBool(DO_END_TIME_STR, &b) == B_OK) mDoEndTime = b;

	mFullTime = mMultiplier*((mQuantizeTime*2*QUANT_PRIMES)/mDivider);
	return B_OK;
}

status_t ArpQuantizeFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpQuantizeFilterSettings(mHolder, config));
	return B_OK;
}

#if 0
void ArpQuantizeFilter::DoQuantize(AmEvent* event, AmTime fullTime, const int64 smallest)
{
	ArpASSERT(event && mHolder);
	if (mDoStartTime) {
		AmTime			startTime = quantize(event->StartTime(), fullTime);
		if (mShift == RIGHT_SHIFT) startTime = startTime + smallest;
		else if (mShift == CLOSEST_SHIFT) {
			if (event->StartTime() - startTime > (startTime + smallest) - event->StartTime())
				startTime = startTime + smallest;
		}
		event->SetStartTime(startTime);
	}
	if (mDoEndTime) {
		AmTime			endTime;
		if (event->Duration() <= smallest) endTime = event->StartTime() + smallest - 1;
		else endTime = quantize(event->EndTime(), fullTime) - 1;
		if (endTime <= event->StartTime() ) endTime = event->StartTime() + 1;
		event->SetEndTime(endTime);
	}
	event->SetNextFilter(mHolder->FirstConnection() );
}
#endif

static inline AmTime quantize(AmTime inTime, AmTime fullTime)
{
	const int64 t = ((int64)inTime)*QUANT_PRIMES;
	return (t-(t%fullTime)) / QUANT_PRIMES;
}

static inline AmTime quantize_shift(AmTime inTime, AmTime fullTime, int64 smallest, int32 shift)
{
	const int64	t = ((int64)inTime)*QUANT_PRIMES;
	AmTime		leftTime = (t-(t%fullTime)) / QUANT_PRIMES;
	if (leftTime == inTime) return 0;
	if (shift == ArpQuantizeFilter::RIGHT_SHIFT) return smallest - (inTime - leftTime);
	else if (shift == ArpQuantizeFilter::CLOSEST_SHIFT) {
		if (inTime - leftTime >= (leftTime + smallest) - inTime)
			return smallest - (inTime - leftTime);
	}
	return leftTime - inTime;
}

void ArpQuantizeFilter::DoQuantize(AmEvent* event, AmTime fullTime, const int64 smallest)
{
	ArpASSERT(event && mHolder);
	if (mDoStartTime) {
		AmTime			offset = quantize_shift(event->StartTime(), fullTime, smallest, mShift);
		offset = AmTime(double(offset) * (double(mAttraction) / 100));
		event->SetStartTime(event->StartTime() + offset);
	}
	if (mDoEndTime) {
		AmTime			endTime;
		if (event->Duration() <= smallest) endTime = event->StartTime() + smallest - 1;
		else endTime = quantize(event->EndTime(), fullTime) - 1;
		if (endTime <= event->StartTime() ) endTime = event->StartTime() + 1;
		event->SetEndTime(endTime);
	}
	event->SetNextFilter(mHolder->FirstConnection() );
}

// #pragma mark -

/*****************************************************************************
 * ARP-QUANTIZE-FILTER-ADDON
 *****************************************************************************/
void ArpQuantizeFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I snap event times to a grid.  If the <B>Tools use grid</B> option is
	active, then I will use the grid from the current track window (this option is
	only valid if I am being run from a tool).  Otherwise, I snap events to the
	grid specified by my duration control.</p>

	<P>If <B>Quantize start time</B> is checked, then I align the start of all events
	to the grid.  If <B>Quantize end time</B> is active, then I align the end of
	all events to the grid.</p>";
}

void ArpQuantizeFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 3;
}

BBitmap* ArpQuantizeFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpQuantizeFilterAddOn::NewInstance(	AmFilterHolderI* holder,
												const BMessage* config)
{
	return new ArpQuantizeFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpQuantizeFilterAddOn(cookie);
	return NULL;
}
