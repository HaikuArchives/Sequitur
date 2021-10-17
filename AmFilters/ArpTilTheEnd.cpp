#include "ArpTilTheEnd.h"

#include <cstdio>
#include <cstdlib>
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const char*	MODE_STR				= "mode";
static const char*	GRID_STR				= "grid";

/*****************************************************************************
 *	_ECHO-FILTER-SETTINGS
 *****************************************************************************/
class _EchoFilterSettings : public AmFilterConfigLayout
{
public:
	_EchoFilterSettings(AmFilterHolderI* target,
						const BMessage& initSettings);

	virtual void		MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	BRadioButton*		mFillRadio;
	BRadioButton*		mCopyRadio;
	BCheckBox*			mGridCheck;

	void RefreshControls(const BMessage& settings);
};

/*****************************************************************************
 *	ARP-ECHO-FILTER
 *****************************************************************************/
ArpTilTheEndFilter::ArpTilTheEndFilter(ArpTilTheEndAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder),
	  mMode(FILL_MEASURES_MODE), mDepth(3),
	  mGridChoice(MY_GRID), mMultiplier(1), mQuantize(PPQN), mEighths(2)
{
	if (config) PutConfiguration(config);
}

ArpTilTheEndFilter::~ArpTilTheEndFilter()
{
}

AmEvent* ArpTilTheEndFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != 0 && mHolder != 0, return event);

	AmTime			grid = mMultiplier * ((mQuantize*2)/mEighths);

	if (mMode == FILL_MEASURES_MODE)
		PerformFillMeasuresMode(event, params, grid);
	else if (mMode == COPY_MEASURES_MODE)
		PerformCopyMeasuresMode(event, params);

	if (event) event->SetNextFilter(mHolder->ConnectionAt(0) );

	return event;
}

AmEvent* ArpTilTheEndFilter::HandleToolEvent(	AmEvent* event,
												const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn && mHolder && toolParams, return event);

	AmTime			grid;
	if (mGridChoice == TOOL_GRID) grid = toolParams->GridTime();
	else grid = mMultiplier * ((mQuantize*2)/mEighths);

	if (mMode == FILL_MEASURES_MODE)
		PerformFillMeasuresMode(event, params, grid);
	else if (mMode == COPY_MEASURES_MODE)
		PerformCopyMeasuresMode(event, params);

	if (event) event->SetNextFilter(mHolder->ConnectionAt(0) );

	return event;
}

status_t ArpTilTheEndFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err = values->AddInt32(MODE_STR, mMode)) != B_OK) return err;
	if ((err = values->AddInt32(GRID_STR, mGridChoice)) != B_OK) return err;
	if ((err = values->AddInt32("depth", mDepth)) != B_OK) return err;
	if ((err = values->AddInt32(AM_MULTIPLIER_CONTROL_KEY_STR, mMultiplier)) != B_OK ) return err;
	if ((err = add_time(*values, AM_QUANTIZE_CONTROL_KEY_STR, mQuantize)) != B_OK ) return err;
	if ((err = values->AddInt32(AM_EIGHTHS_CONTROL_KEY_STR, mEighths)) != B_OK ) return err;

	return B_OK;
}

status_t ArpTilTheEndFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32 i;
	if (values->FindInt32(MODE_STR, &i) == B_OK) mMode = i;
	if (values->FindInt32("depth", &i) == B_OK) mDepth = i;
	if (values->FindInt32(GRID_STR, &i) == B_OK) {
		if (i < MY_GRID) i = MY_GRID;
		if (i > TOOL_GRID) i = TOOL_GRID;
		mGridChoice = i;
	}
	AmTime t;	
	if (values->FindInt32(AM_MULTIPLIER_CONTROL_KEY_STR, &i) == B_OK) mMultiplier = i;
	if (find_time(*values, AM_QUANTIZE_CONTROL_KEY_STR, &t) == B_OK) mQuantize = t;
	if (values->FindInt32(AM_EIGHTHS_CONTROL_KEY_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mEighths = i;
	}

	return B_OK;
}

status_t ArpTilTheEndFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _EchoFilterSettings(mHolder, config));
	return B_OK;
}

void ArpTilTheEndFilter::PerformFillMeasuresMode(	AmEvent* event,
													const am_filter_params* params,
													AmTime grid)
{
	if (!params || !params->cur_signature) return;
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(1);
	if (!nextHolder) nextHolder = mHolder->ConnectionAt(0);	

	AmSignature			sig;
	AmEvent*			curEvent = event;
	AmTime				curTime = curEvent->StartTime();
	if (am_get_measure(curTime, params->cur_signature, sig) != B_OK) return;
	curTime += grid;
	for (int32 k = 0; k < mDepth; k++) {
		while (curTime <= sig.EndTime() ) {
			AmEvent*	nextEvent = curEvent->Copy();
			if (!nextEvent) return;
			nextEvent->SetStartTime(curTime);
			nextEvent->SetNextFilter(nextHolder);
			curEvent->AppendEvent(nextEvent);
			curEvent = nextEvent;
			curTime += grid;
		}
		if (am_get_measure(curTime, params->cur_signature, sig) != B_OK) return;
	}
}

void ArpTilTheEndFilter::PerformCopyMeasuresMode(AmEvent* event, const am_filter_params* params)
{
	if (!params || !params->cur_signature) return;
	AmFilterHolderI*	nextHolder = mHolder->ConnectionAt(1);
	if (!nextHolder) nextHolder = mHolder->ConnectionAt(0);	

	AmSignature			sig;
	AmEvent*			curEvent = event;
	if (am_get_measure(curEvent->StartTime(), params->cur_signature, sig) != B_OK) return;
	AmTime				delta = curEvent->StartTime() - sig.StartTime();
	for (int32 k = 0; k < mDepth; k++) {
		if (am_get_measure(sig.EndTime() + 1, params->cur_signature, sig) != B_OK) return;
		if (sig.StartTime() + delta <= sig.EndTime() ) {
			AmEvent*	nextEvent = curEvent->Copy();
			if (!nextEvent) return;
			nextEvent->SetStartTime(sig.StartTime() + delta);
			nextEvent->SetNextFilter(nextHolder);
			curEvent->AppendEvent(nextEvent);
			curEvent = nextEvent;
		}
	}
}

// #pragma mark -

/*****************************************************************************
 *	ARP-ECHO-FILTER-ADD-ON
 *****************************************************************************/
void ArpTilTheEndAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>I am a form of echo that uses measure information for my stop condition. \n"
	"When I am in fill mode, I will fill depth number of measures with the current \n"
	"event.  When I am in copy mode, I place a single copy of the event in depth \n"
	"number of measures.</p>\n"
	"\n"
	"<P>If I have a single connection, all events go to that connection.  However, \n"
	"if I have two connections the original event will go out the first connection, \n"
	"and all new events will go out the second.</p>\n"
	"\n"
	"<p>The Grid is only active in Fill mode.  It determines the grid on which \n"
	"each note is aligned.</p>\n";
}

void ArpTilTheEndAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpTilTheEndAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpTilTheEndAddOn(cookie);
	return NULL;
}

// #pragma mark -

/*****************************************************************************
 *	_ECHO-FILTER-SETTINGS
 *****************************************************************************/
static const uint32		FILL_MEASURE_MSG	= 'iFil';
static const uint32		COPY_MEASURE_MSG	= 'iCpy';
static const uint32 	GRID_MSG			= 'iGrd';

_EchoFilterSettings::_EchoFilterSettings(	AmFilterHolderI* target,
											const BMessage& initSettings)
		: inherited(target, initSettings),
		  mFillRadio(NULL), mCopyRadio(NULL), mGridCheck(NULL)
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
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
			->AddLayoutChild((new ArpRunningBar("ModeHBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)

				->AddLayoutChild((new ArpRunningBar("RadioVBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
					->AddLayoutChild((new ArpViewWrapper(mFillRadio = 
						new BRadioButton(BRect(0,0,10,10), "fill_radio", "Fill",
								new BMessage(FILL_MEASURE_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((new ArpViewWrapper(mCopyRadio = 
						new BRadioButton(BRect(0,0,10,10), "copy_radio", "Copy",
								new BMessage(COPY_MEASURE_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
				)
			)
			->AddLayoutChild((new ArpTextControl(
									"depth", "Depth:","",
									mImpl.AttachTextControl("depth")))
				->SetParams(ArpMessage()
					.SetString(ArpTextControl::MinTextStringP, "888")
					.SetString(ArpTextControl::PrefTextStringP, "88888888")
				)
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)


			->AddLayoutChild((new ArpBox("GridBox", "Grid"))
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
						new BCheckBox(BRect(0,0,10,10), "grid", "Tools use grid duration",
								new BMessage(GRID_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
							.SetBool(ArpRunningBar::AlignLabelsC,false)
						)
					)
					->AddLayoutChild((new AmDurationControl("duration", "", this, initSettings))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							.SetBool(ArpRunningBar::AlignLabelsC,false)
						)
					)
				)
			)
#if 0


			->AddLayoutChild((new AmDurationControl("duration", "Duration:", this, initSettings))
				->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,3)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
					.SetBool(ArpRunningBar::AlignLabelsC,true)
				)
			)
#endif
		);
	} catch(...) {
		throw;
	}
	
	Implementation().RefreshControls(mSettings);
	RefreshControls(initSettings);
}

void _EchoFilterSettings::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case FILL_MEASURE_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpTilTheEndFilter::FILL_MEASURES_MODE) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case COPY_MEASURE_MSG: {
			BMessage		upd;
			if (upd.AddInt32(MODE_STR, ArpTilTheEndFilter::COPY_MEASURES_MODE) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;
		case GRID_MSG: {
			BMessage		upd;
			if (upd.AddInt32(GRID_STR, mGridCheck->Value()
							? ArpTilTheEndFilter::TOOL_GRID : ArpTilTheEndFilter::MY_GRID) == B_OK)
				Implementation().SendConfiguration(&upd);
		} break;

		default:
			inherited::MessageReceived(msg);
	}
}

void _EchoFilterSettings::RefreshControls(const BMessage& settings)
{
	int32		i;
	if (settings.FindInt32(MODE_STR, &i) == B_OK) {
		if (i == ArpTilTheEndFilter::FILL_MEASURES_MODE) {
			if (mFillRadio) mFillRadio->SetValue(B_CONTROL_ON);
		} else if (i == ArpTilTheEndFilter::COPY_MEASURES_MODE) {
			if (mCopyRadio) mCopyRadio->SetValue(B_CONTROL_ON);
		}
	}
	if (settings.FindInt32(GRID_STR, &i) == B_OK) {
		mGridCheck->SetValue(i == ArpTilTheEndFilter::MY_GRID ? B_CONTROL_OFF : B_CONTROL_ON);
	}
}
