/* ArpVelociTease.cpp
 */
#include "ArpVelociTease.h"

#include <cstdio>
#include <cstdlib>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/RadioButton.h>
#include <interface/StringView.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "ArpViews/ArpRangeControl.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "AmPublic/AmFilterConfig.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const uint32		ABSOLUTE_MSG		= 'iabs';
static const uint32		SCALE_MSG			= 'iscl';
static const uint32		DELTA_MSG			= 'idlt';
static const uint32		RANDOM_MSG			= 'irnd';

static const char*		MODE_STR			= "mode";
static const char*		ABSOLUTE_STR		= "absolute";
static const char*		SCALE_STR			= "scale";
static const char*		DELTA_STR			= "delta";

static const float		MIN_SCALE			= 1;
static const float		MAX_SCALE			= 400;
static const int32		MIN_DELTA			= -128;
static const int32		MAX_DELTA			= 128;

/*****************************************************************************
 *	_VELOCITY-FILTER-SETTINGS
 *****************************************************************************/
class _VelocityFilterSettings : public AmFilterConfigLayout
{
public:
	_VelocityFilterSettings(AmFilterHolderI* target,
							const BMessage& initSettings);

	virtual	void MessageReceived(BMessage *msg);

private:
	typedef AmFilterConfigLayout inherited;
	BRadioButton*		mAbsRadio;
	BRadioButton*		mSclRadio;
	BRadioButton*		mDeltaRadio;
	BRadioButton*		mRndRadio;
	ArpIntControl*		mAbsInt;
	ArpIntControl*		mSclInt;
	ArpIntControl*		mDeltaInt;
	
	void	RefreshControls(const BMessage& settings);
	void	TurnOn(BRadioButton* onRadio, ArpIntControl* onInt);
};

/*****************************************************************************
 * ARP-VELOCITY-FILTER
 *****************************************************************************/
ArpVelociTeaseFilter::ArpVelociTeaseFilter(	ArpVelociTeaseAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mMode(ABSOLUTE_MODE), mAbsolute(106), mScale(100), mDelta(0)
{
	mSeed = system_time();
	if (config) PutConfiguration(config);
}

ArpVelociTeaseFilter::~ArpVelociTeaseFilter()
{
}

AmEvent* ArpVelociTeaseFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return HandleBatchEvents(event, params);
}

AmEvent* ArpVelociTeaseFilter::HandleBatchEvents(AmEvent* event, const am_filter_params* /*params*/,
												const AmEvent* /*lookahead*/)
{
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	srand(mSeed);

	AmEvent*		cur = event;
	while (cur) {
		AmNoteOn* note = NULL;
		if (cur->Type() == cur->NOTEON_TYPE
				&& (note = dynamic_cast<AmNoteOn*>(cur)) != NULL) {
			int32		newVelocity = mAbsolute;
			if (mMode == ABSOLUTE_MODE) newVelocity = mAbsolute;
			else if (mMode == SCALE_MODE) newVelocity = int32(note->Velocity() * (mScale / 100) );
			else if (mMode == DELTA_MODE) newVelocity = int32(note->Velocity() + mDelta);
			else if (mMode == RANDOM_MODE) newVelocity = int32(rand() % 128);
			
			if (newVelocity < 0) newVelocity = 0;
			else if (newVelocity > 127) newVelocity = 127;
			note->SetVelocity(newVelocity);
		}
		cur->SetNextFilter(mHolder->FirstConnection() );		
		cur = cur->NextEvent();
	}	
	return event;
}

status_t ArpVelociTeaseFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if (values->AddInt32(MODE_STR, mMode) != B_OK) return B_ERROR;
	if (values->AddInt32(ABSOLUTE_STR, mAbsolute) != B_OK) return B_ERROR;
	if (values->AddFloat(SCALE_STR, mScale) != B_OK) return B_ERROR;
	if (values->AddInt32(DELTA_STR, mDelta) != B_OK) return B_ERROR;
	return B_OK;
}

status_t ArpVelociTeaseFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	// Want to make sure that batch mode is always turned on.
	SetFlag(BATCH_FLAG, true);

	if (err != B_OK) return err;
	int32	i;
	if (values->FindInt32(MODE_STR, &i) == B_OK) mMode = i;
	if (values->FindInt32(ABSOLUTE_STR, &i) == B_OK) mAbsolute = i;
	float	f;
	if (values->FindFloat(SCALE_STR, &f) == B_OK) {
		if (f < MIN_SCALE) mScale = MIN_SCALE;
		else if (f > MAX_SCALE) mScale = MAX_SCALE;
		else mScale = f;
	}
	if (values->FindInt32(DELTA_STR, &i) == B_OK) {
		mDelta = i;
		if (mDelta < MIN_DELTA) mDelta = MIN_DELTA;
		else if (mDelta > MAX_DELTA) mDelta = MAX_DELTA;
	}
	return B_OK;
}

status_t ArpVelociTeaseFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new _VelocityFilterSettings(mHolder, config));
	return B_OK;
}

void ArpVelociTeaseFilter::Start(uint32 context)
{
	if (context&TOOL_CONTEXT) mSeed = system_time();
}

/*****************************************************************************
 * ARP-VELOCITY-FILTER-ADDON
 *****************************************************************************/
void ArpVelociTeaseAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I alter the velocity of all incoming notes.  The change I make depends on"
	"my current mode.</P>"
	"<UL>"
	"	<LI><I>Set to</I> will set all velocities to the specified value.</LI>"
	"	<LI><I>Scaled</I> will scale all velocities.  For example, a scale of 50 will"
	"			halve the velocities; 200 will double them.  A value of 100 will have"
	"			no effect.</LI>"
	"	<LI><I>Change by</I> will change the velocities by the given delta.  For example,"
	"			if the delta is 10, then a note with velocity 100 will be changed to 110."
	"			If the delta is -10, then the same note will be 90.  A value of 0 will"
	"			have no effect.</LI>"
	"	<LI><I>Random</I> will assign a random velocity to each note.</LI>"
	"</UL>&nbsp;";
}

void ArpVelociTeaseAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpVelociTeaseAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpVelociTeaseAddOn::NewInstance(AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpVelociTeaseFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpVelociTeaseAddOn(cookie);
	return NULL;
}

/*****************************************************************************
 *	_VELOCITY-FILTER-SETTINGS
 *****************************************************************************/
_VelocityFilterSettings::_VelocityFilterSettings(	AmFilterHolderI* target,
													const BMessage& initSettings)
		: inherited(target, initSettings),
		  mAbsRadio(NULL), mSclRadio(NULL), mDeltaRadio(NULL), mRndRadio(NULL),
		  mAbsInt(NULL), mSclInt(NULL), mDeltaInt(NULL)
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
					.SetFloat(ArpRunningBar::WeightC,0)
					.SetInt32(ArpRunningBar::FillC,ArpEastWest)
				)
			)

			->AddLayoutChild((new ArpRunningBar("ContentVBar"))
				->SetParams(ArpMessage()
					.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
					.SetFloat(ArpRunningBar::IntraSpaceP, .5)
				)
					->SetConstraints(ArpMessage()
					.SetFloat(ArpRunningBar::WeightC,1)
					.SetInt32(ArpRunningBar::FillC,ArpFillAll)
				)

				->AddLayoutChild((new ArpRunningBar("HBar1"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
						->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)

					->AddLayoutChild((new ArpViewWrapper(mAbsRadio = 
						new BRadioButton(BRect(0,0,10,10), "absolute_radio", "Set to",
									new BMessage(ABSOLUTE_MSG),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((mAbsInt = new ArpIntControl(
											ABSOLUTE_STR, "Value:",
											mImpl.AttachControl(ABSOLUTE_STR),
											0, 127))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEast)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("HBar2"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
						->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)

					->AddLayoutChild((new ArpViewWrapper(mSclRadio = 
						new BRadioButton(BRect(0,0,10,10), "scale_radio", "Scaled",
								new BMessage(SCALE_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((mSclInt = new ArpIntControl(
											SCALE_STR, "Scale:",
											mImpl.AttachControl(SCALE_STR),
											MIN_SCALE, MAX_SCALE))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEast)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("HBar3"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
						->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpViewWrapper(mDeltaRadio = 
						new BRadioButton(BRect(0,0,10,10), "delta_radio", "Change by",
								new BMessage(DELTA_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
					->AddLayoutChild((mDeltaInt = new ArpIntControl(
											DELTA_STR, "Delta:",
											mImpl.AttachControl(DELTA_STR),
											MIN_DELTA, MAX_DELTA))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEast)
						)
					)
				)
				->AddLayoutChild((new ArpRunningBar("HBar4"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)
						->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpFillAll)
					)
					->AddLayoutChild((new ArpViewWrapper(mRndRadio = 
						new BRadioButton(BRect(0,0,10,10), "rnd_radio", "Random",
								new BMessage(RANDOM_MSG),
								B_FOLLOW_NONE,
								B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpWest)
						)
					)
				)
			)
		);
	} catch(...) {
		throw;
	}
	Implementation().RefreshControls(mSettings);
	RefreshControls(initSettings);
}

void _VelocityFilterSettings::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case ABSOLUTE_MSG:
			{
				BMessage		upd;
				if (upd.AddInt32("mode", ArpVelociTeaseFilter::ABSOLUTE_MODE) == B_OK)
					Implementation().SendConfiguration(&upd);
				TurnOn(mAbsRadio, mAbsInt);
			}
			break;
		case SCALE_MSG:
			{
				BMessage		upd;
				if (upd.AddInt32("mode", ArpVelociTeaseFilter::SCALE_MODE) == B_OK)
					Implementation().SendConfiguration(&upd);
				TurnOn(mSclRadio, mSclInt);
			}
			break;
		case DELTA_MSG:
			{
				BMessage		upd;
				if (upd.AddInt32("mode", ArpVelociTeaseFilter::DELTA_MODE) == B_OK)
					Implementation().SendConfiguration(&upd);
				TurnOn(mDeltaRadio, mDeltaInt);
			}
			break;
		case RANDOM_MSG:
			{
				BMessage		upd;
				if (upd.AddInt32("mode", ArpVelociTeaseFilter::RANDOM_MODE) == B_OK)
					Implementation().SendConfiguration(&upd);
				TurnOn(mRndRadio, NULL);
			}
			break;
		case ARP_PUT_CONFIGURATION_MSG:
			{
				BMessage	settings;
				if (msg->FindMessage("settings", &settings) == B_OK)
					RefreshControls(settings);
			}
			// Note: no break on purpose
		default:
			inherited::MessageReceived( msg );
	}
}

void _VelocityFilterSettings::RefreshControls(const BMessage& settings)
{
	int32		mode;
	if (settings.FindInt32("mode", &mode) == B_OK) {
		if (mode == ArpVelociTeaseFilter::ABSOLUTE_MODE)
			TurnOn(mAbsRadio, mAbsInt);
		else if (mode == ArpVelociTeaseFilter::SCALE_MODE)
			TurnOn(mSclRadio, mSclInt);
		else if (mode == ArpVelociTeaseFilter::DELTA_MODE)
			TurnOn(mDeltaRadio, mDeltaInt);
		else if (mode == ArpVelociTeaseFilter::RANDOM_MODE)
			TurnOn(mRndRadio, NULL);
	}
}

void _VelocityFilterSettings::TurnOn(BRadioButton* onRadio, ArpIntControl* onInt)
{
	ArpASSERT(onRadio);
	if (mAbsRadio) {
		if (mAbsRadio == onRadio) mAbsRadio->SetValue(B_CONTROL_ON);
		else mAbsRadio->SetValue(B_CONTROL_OFF);
	}
	if (mSclRadio) {
		if (mSclRadio == onRadio) mSclRadio->SetValue(B_CONTROL_ON);
		else mSclRadio->SetValue(B_CONTROL_OFF);
	}
	if (mDeltaRadio) {
		if (mDeltaRadio == onRadio) mDeltaRadio->SetValue(B_CONTROL_ON);
		else mDeltaRadio->SetValue(B_CONTROL_OFF);
	}
	if (mRndRadio) {
		if (mRndRadio == onRadio) mRndRadio->SetValue(B_CONTROL_ON);
		else mRndRadio->SetValue(B_CONTROL_OFF);
	}
		
	if (mAbsInt) {
		if (mAbsInt == onInt) mAbsInt->SetEnabled(true);
		else mAbsInt->SetEnabled(false);
	}
	if (mSclInt) {
		if (mSclInt == onInt) mSclInt->SetEnabled(true);
		else mSclInt->SetEnabled(false);
	}
	if (mDeltaInt) {
		if (mDeltaInt == onInt) mDeltaInt->SetEnabled(true);
		else mDeltaInt->SetEnabled(false);
	}
}
