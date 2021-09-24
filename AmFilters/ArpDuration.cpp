/* ArpDuration.cpp
 */
#include "ArpDuration.h"

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
#include "AmKernel/AmEventControls.h"
#include "AmPublic/AmFilterConfig.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();
static AmStaticResources gRes;

static const uint32		ABSOLUTE_MSG		= 'iabs';
static const uint32		SCALE_MSG			= 'iscl';
static const uint32		GRID_MSG			= 'igrd';

static const char*		MODE_STR			= "mode";
static const char*		ABSOLUTE_STR		= "absolute";
static const char*		SCALE_STR			= "scale";

/*************************************************************************
 * _DURATION-CONTROL
 * A control that displays a quick-access menu and text control for editing
 * some duration filter properties.
 *************************************************************************/
/* These are the hardcoded duration constants, all based on 4/4 time.
 */
static const float	WHOLE						= PPQN * 4;
static const float	WHOLE_HALF					= PPQN * 3;
static const float	HALF						= PPQN * 2;
static const float	HALF_QUARTER				= PPQN * 1.5;
static const float	QUARTER						= PPQN;
static const float	QUARTER_EIGHTH				= PPQN * 0.75;
static const float	EIGHTH						= PPQN * 0.5;
static const float	EIGTH_SIXTEENTH				= PPQN * 0.375;
static const float	SIXTEENTH					= PPQN * 0.25;
static const float	SIXTEENTH_THIRTYSECOND		= PPQN * 0.1875;
static const float	THIRTYSECOND				= PPQN * 0.125;
static const float	THIRTYSECOND_SIXTYFOURTH	= PPQN * 0.09375;
static const float	SIXTYFOURTH					= PPQN * 0.0625;

/*****************************************************************************
 * ARP-DURATION-FILTER
 * This filter reassigns the duration of all note events that come through.
 *****************************************************************************/
ArpDurationFilter::ArpDurationFilter(	ArpDurationFilterAddOn* addon,
										AmFilterHolderI* holder,
										const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mMode(SCALE_MODE), mAbsolute(PPQN), mScale(100)
{
	if( config ) PutConfiguration( config );
}

ArpDurationFilter::~ArpDurationFilter()
{
}

AmEvent* ArpDurationFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	
	event->SetNextFilter(mHolder->FirstConnection() );
	if (mMode != GRID_MODE && event->Type() == event->NOTEON_TYPE ) {
		AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
		if (!note) return event;

		AmTime		newTime = mAbsolute;
		if (mMode == ABSOLUTE_MODE) newTime = mAbsolute;
		else if (mMode == SCALE_MODE) newTime = (AmTime)( note->Duration() * (mScale / 100) );

		/* I subtract one because otherwise absolute times run into each other.
		 */
		newTime--;
		if (newTime < 1) newTime = 1;
		note->SetDuration( newTime );
	} 
	
	return event;
}

AmEvent* ArpDurationFilter::HandleToolEvent(AmEvent* event,
											const am_filter_params* params,
											const am_tool_filter_params* toolParams)
{
	if (!event || !toolParams) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);

	event->SetNextFilter(mHolder->FirstConnection() );
	if (mMode == GRID_MODE) {
		if (event->Type() == event->NOTEON_TYPE ) {
			AmNoteOn* note = dynamic_cast<AmNoteOn*>(event);
			if (!note) return event;

			AmTime		newTime = toolParams->GridTime();
			if (newTime < 1) newTime = 1;
			note->SetDuration(newTime);
		}
	} else return HandleEvent(event);
	
	return event;
}

class ArpDurationFilterSettings : public AmFilterConfigLayout
{
public:
	ArpDurationFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings),
		mAbsRadio(0), mSclRadio(0), mGridRadio(NULL), mAbsText(0), mSclText(0)
	{
#if 0
		AmSignature		sig;
		sig.Set(0, 1, 4, 4);
		AmTimeView*		timeView;
#endif
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

				->AddLayoutChild((new ArpRunningBar("ContentHBar"))
					->SetParams(ArpMessage()
						.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
						.SetFloat(ArpRunningBar::IntraSpaceP, .5)
					)

					->AddLayoutChild((new ArpRunningBar("RadioVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->AddLayoutChild((new ArpViewWrapper(mAbsRadio = 
							new BRadioButton(BRect(0,0,10,10), "absolute_radio", "Absolute",
									new BMessage(ABSOLUTE_MSG),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
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
						->AddLayoutChild((new ArpViewWrapper(mGridRadio = 
							new BRadioButton(BRect(0,0,10,10), "grid_radio", "From grid (tool only)",
									new BMessage(GRID_MSG),
									B_FOLLOW_NONE,
									B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,1)
								.SetInt32(ArpRunningBar::FillC,ArpWest)
							)
						)
					)
				
					->AddLayoutChild((new ArpRunningBar("TextVBar"))
						->SetParams(ArpMessage()
							.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
							.SetFloat(ArpRunningBar::IntraSpaceP, .5)
						)
						->AddLayoutChild((mAbsText = new ArpTextControl(
												ABSOLUTE_STR, "Absolute:", "",
												mImpl.AttachTextControl(ABSOLUTE_STR)))
							->SetParams(ArpMessage()
								.SetString(ArpTextControl::MinTextStringP, "888")
								.SetString(ArpTextControl::PrefTextStringP, "88888888")
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,3)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
						->AddLayoutChild((mSclText = new ArpTextControl(
												SCALE_STR, "Scale:", "",
												mImpl.AttachTextControl(SCALE_STR)))
							->SetParams(ArpMessage()
								.SetString(ArpTextControl::MinTextStringP, "888")
								.SetString(ArpTextControl::PrefTextStringP, "88888888")
							)
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,3)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
#if 0
						->AddLayoutChild((new ArpViewWrapper(timeView = new AmTimeView(sig)))
							->SetConstraints(ArpMessage()
								.SetFloat(ArpRunningBar::WeightC,3)
								.SetInt32(ArpRunningBar::FillC,ArpEastWest)
							)
						)
#endif
					)
				)
			);
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
		RefreshControls( initSettings );
	}

	virtual	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {			
			case ABSOLUTE_MSG:
				{
					BMessage		upd;
					if( upd.AddInt32( "mode", ArpDurationFilter::ABSOLUTE_MODE ) == B_OK )
						Implementation().SendConfiguration(&upd);
					if( mAbsText ) mAbsText->SetEnabled( true );
					if( mSclText ) mSclText->SetEnabled( false );
				}
				break;
			case SCALE_MSG:
				{
					BMessage		upd;
					if( upd.AddInt32( "mode", ArpDurationFilter::SCALE_MODE ) == B_OK )
						Implementation().SendConfiguration(&upd);
					if( mAbsText ) mAbsText->SetEnabled( false );
					if( mSclText ) mSclText->SetEnabled( true );
				}
				break;
			case GRID_MSG:
				{
					BMessage		upd;
					if( upd.AddInt32( "mode", ArpDurationFilter::GRID_MODE ) == B_OK )
						Implementation().SendConfiguration(&upd);
					if (mAbsText) mAbsText->SetEnabled(false);
					if (mSclText) mSclText->SetEnabled(false);
				}
				break;
			case ARP_PUT_CONFIGURATION_MSG:
				{
					BMessage	settings;
					if( msg->FindMessage( "settings", &settings ) == B_OK )
						RefreshControls( settings );
				}
				// Note: no break on purpose
			default:
				inherited::MessageReceived( msg );
		}
	}

protected:
	typedef AmFilterConfigLayout inherited;

private:
	BRadioButton*		mAbsRadio;
	BRadioButton*		mSclRadio;
	BRadioButton*		mGridRadio;
	ArpTextControl*		mAbsText;
	ArpTextControl*		mSclText;
	
	void RefreshControls(const BMessage& settings)
	{
		int32		mode;
		if (settings.FindInt32( "mode", &mode ) == B_OK ) {
			if (mode == ArpDurationFilter::ABSOLUTE_MODE ) {
				if (mAbsRadio) mAbsRadio->SetValue( B_CONTROL_ON );
				if (mAbsText) mAbsText->SetEnabled( true );
				if (mSclText) mSclText->SetEnabled( false );
			}
			if (mode == ArpDurationFilter::SCALE_MODE) {
				if( mSclRadio ) mSclRadio->SetValue( B_CONTROL_ON );
				if( mAbsText ) mAbsText->SetEnabled( false );
				if( mSclText ) mSclText->SetEnabled( true );
			}
			if (mode == ArpDurationFilter::GRID_MODE) {
				if (mGridRadio) mGridRadio->SetValue( B_CONTROL_ON );
				if (mAbsText) mAbsText->SetEnabled(false);
				if (mSclText) mSclText->SetEnabled(false);
			}
		}
	}
};

status_t ArpDurationFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if (values->AddInt32(MODE_STR, mMode) != B_OK ) return B_ERROR;
	if (add_time(*values, ABSOLUTE_STR, mAbsolute) != B_OK ) return B_ERROR;
	if (values->AddFloat(SCALE_STR, mScale) != B_OK ) return B_ERROR;
	return B_OK;
}

status_t ArpDurationFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32		i;
	AmTime		t;
	float		f;
	if (values->FindInt32(MODE_STR, &i) == B_OK) mMode = i;
	if (find_time(*values, ABSOLUTE_STR, &t) == B_OK) mAbsolute = t;
	if (values->FindFloat(SCALE_STR, &f) == B_OK) mScale = f;
	return B_OK;
}

status_t ArpDurationFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpDurationFilterSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-DURATION-FILTER-ADDON
 *****************************************************************************/
void ArpDurationFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>I set or modify the duration of all incoming notes.</P>"
	"<UL>"
	"	<LI><I>Absolute</I> mode sets the duration of all notes to the supplied MIDI ticks.</LI>"
	"	<LI><I>Scaled</I> mode modifies the duration of all notes by the supplied"
	"			scale.  At 100%, there will be no change.  At 50%, all notes will"
	"			be halved in length, at 200% they will be doubled.</LI>"
	"	<LI><I>From grid</I> sets the duration to the current grid value in the track"
	"			window.  This mode is only valid when the Duration filter is being"
	"			operated from inside a tool.</LI>"
	"</UL>";
}

void ArpDurationFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpDurationFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpDurationFilterAddOn::NewInstance(	AmFilterHolderI* holder,
												const BMessage* config)
{
	return new ArpDurationFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpDurationFilterAddOn(cookie);
	return NULL;
}
