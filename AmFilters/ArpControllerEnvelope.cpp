/* ArpControllerEnvelope.cpp
 */
#include "ArpControllerEnvelope.h"

#include <cstdio>
#include <cstdlib>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmFilterConfigLayout.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmEvents.h"

ArpMOD();

static AmStaticResources gRes;

static const char*		MULT_ATK_STR	=	"mult_atk";
static const char*		QUANT_ATK_STR	=	"quant_atk";
static const char*		EIGHTHS_ATK_STR	=	"8ths_atk";
static const char*		MULT_DCY_STR	=	"mult_dcy";
static const char*		QUANT_DCY_STR	=	"quant_dcy";
static const char*		EIGHTHS_DCY_STR	=	"8ths_dcy";
static const char*		INITIAL_VAL_STR	=	"initial_val";
static const char*		DENSITY_STR		=	"density";

/*****************************************************************************
 * ARP-CONTROLLER-ENVELOPE-FILTER
 *****************************************************************************/
ArpControllerEnvelopeFilter::ArpControllerEnvelopeFilter(	ArpControllerEnvelopeFilterAddOn* addon,
															AmFilterHolderI* holder,
															const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder),
		  mControlNumber(7), mInitialValue(0), mDensity(21),
		  mMultAtk(1), mQuantizeAtk(PPQN / 8), mEighthsAtk(2),
		  mMultDcy(1), mQuantizeDcy(PPQN / 4), mEighthsDcy(2)
{
	if (config) PutConfiguration( config );
}

ArpControllerEnvelopeFilter::~ArpControllerEnvelopeFilter()
{
}

AmEvent* ArpControllerEnvelopeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn != NULL && mHolder != NULL, return event);
	if (event->Type() != event->NOTEON_TYPE) return event;
	AmNoteOn*			note = dynamic_cast<AmNoteOn*>(event);
	if (!note) return event;
	uchar				velocity = note->Velocity();
	AmTime				evTime = note->StartTime();
	AmTime				attack = mMultAtk*((mQuantizeAtk*2)/mEighthsAtk);
	AmTime				sustain = note->Duration();
	AmTime				decay = mMultDcy*((mQuantizeDcy*2)/mEighthsDcy);
	event->Delete();
	
	AmTime				headTime = evTime - attack;
	if (headTime < 0) headTime = 0;
	AmControlChange*	ccHead = new AmControlChange(mControlNumber, mInitialValue, headTime);
	AmControlChange*	ccCurr = ccHead;
	AmControlChange*	ccTmp;
	AmTime				density = mDensity * 10;
	AmTime				k;
	int32				range = abs(velocity - mInitialValue);
	if (!ccHead) return 0;
	ccHead->SetNextFilter(mHolder);
	/* Attack
	 */
	for (k = attack - density; k > 0; k -= density) {
		int32		vel = (int32)(  (range * k) / attack );
		if (mInitialValue < velocity) vel = velocity - vel;
		else if (mInitialValue > velocity) vel = velocity + vel;
		else vel = mInitialValue;
		if (vel < 0) vel = 0;
		else if (vel > 127) vel = 127;

		if ( (evTime - k) >= 0) {
			if( (ccTmp = new AmControlChange(mControlNumber, (uint8)vel, evTime - k)) ) {
				ccTmp->SetNextFilter(mHolder->FirstConnection() );
				ccCurr->AppendEvent(ccTmp);
				ccTmp->SetNextFilter(mHolder);
				ccCurr = ccTmp;
			}
		}
	}
	/* Sustain
	 */
	if ( (ccTmp = new AmControlChange(mControlNumber, velocity, evTime)) ) {
		ccTmp->SetNextFilter(mHolder->FirstConnection() );
		ccCurr->AppendEvent(ccTmp);
		ccTmp->SetNextFilter(mHolder);
		ccCurr = ccTmp;
	}
	/* Decay
	 */
	for (k = 0; k < decay; k += density) {
		int32	vel = (int32)( (range * k) / decay );
		if (mInitialValue < velocity) vel = velocity - vel;
		else if (mInitialValue > velocity) vel = velocity + vel;
		else vel = mInitialValue;
		if (vel < 0) vel = 0;
		else if (vel > 127) vel = 127;

		if( (ccTmp = new AmControlChange(mControlNumber, (uint8)vel, evTime + sustain + k )) ) {
			ccTmp->SetNextFilter(mHolder->FirstConnection() );
			ccCurr->AppendEvent(ccTmp);
			ccTmp->SetNextFilter(mHolder);
			ccCurr = ccTmp;
		}
	}
	/* Final initial value
	 */
	if( (ccTmp = new AmControlChange(mControlNumber, mInitialValue, evTime + sustain + decay )) ) {
		ccTmp->SetNextFilter(mHolder->FirstConnection() );
		ccCurr->AppendEvent(ccTmp);
		ccTmp->SetNextFilter(mHolder);
		ccCurr = ccTmp;
	}
	return ccHead;
}

BView* ArpControllerEnvelopeFilter::NewEditView(BPoint requestedSize) const
{
	return NULL;
}

status_t ArpControllerEnvelopeFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if (values->AddInt32(AM_CONTROL_CHANGE_KEY_STR, mControlNumber) != B_OK) return B_ERROR;
	if( (err = values->AddInt32(MULT_ATK_STR, mMultAtk)) != B_OK ) return err;
	if( (err = add_time(*values, QUANT_ATK_STR, mQuantizeAtk)) != B_OK ) return err;
	if( (err = values->AddInt32(EIGHTHS_ATK_STR, mEighthsAtk)) != B_OK ) return err;
	if( (err = values->AddInt32(MULT_DCY_STR, mMultDcy)) != B_OK ) return err;
	if( (err = add_time(*values, QUANT_DCY_STR, mQuantizeDcy)) != B_OK ) return err;
	if( (err = values->AddInt32(EIGHTHS_DCY_STR, mEighthsDcy)) != B_OK ) return err;
	if (values->AddInt32(INITIAL_VAL_STR, mInitialValue) != B_OK) return B_ERROR;
	if (values->AddInt32(DENSITY_STR, mDensity) != B_OK) return B_ERROR;
	
	return B_OK;
}

status_t ArpControllerEnvelopeFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	int32	i;
	AmTime	t;
	if (values->FindInt32(AM_CONTROL_CHANGE_KEY_STR, &i) == B_OK) mControlNumber = (uint8)i;

	if( values->FindInt32(MULT_ATK_STR, &i) == B_OK) mMultAtk = i;
	if( find_time(*values, QUANT_ATK_STR, &t) == B_OK) mQuantizeAtk = t;
	if( values->FindInt32(EIGHTHS_ATK_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mEighthsAtk = i;
	}

	if( values->FindInt32(MULT_DCY_STR, &i) == B_OK) mMultDcy = i;
	if( find_time(*values, QUANT_DCY_STR, &t) == B_OK) mQuantizeDcy = t;
	if( values->FindInt32(EIGHTHS_DCY_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mEighthsDcy = i;
	}

	if (values->FindInt32(INITIAL_VAL_STR, &i) == B_OK) mInitialValue = (uint8)i;
	if (values->FindInt32(DENSITY_STR, &i) == B_OK) mDensity = i;
	
	return B_OK;
}

class ArpControllerEnvelopeSettings : public AmFilterConfigLayout
{
public:
	ArpControllerEnvelopeSettings(AmFilterHolderI* target,
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
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,false)
					)
				)
				->AddLayoutChild((new AmControlChangeListPanel("Control Changes", this, initSettings))
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
				)
				->AddLayoutChild((new AmDurationControl("attack", "Attack:", this, initSettings,
														AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
														QUANT_ATK_STR, EIGHTHS_ATK_STR, MULT_ATK_STR))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new AmDurationControl("decay", "Decay:", this, initSettings,
														AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
														QUANT_DCY_STR, EIGHTHS_DCY_STR, MULT_DCY_STR))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										INITIAL_VAL_STR, "Initial value:",
										mImpl.AttachControl(INITIAL_VAL_STR),
										0, 127))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new ArpIntControl(
										DENSITY_STR, "Density:",
										mImpl.AttachControl(DENSITY_STR),
										1, 400))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,0)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
			);
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
	}

protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpControllerEnvelopeFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpControllerEnvelopeSettings(mHolder, config));
	return B_OK;
}

/*****************************************************************************
 * ARP-CONTROLLER-ENVELOPE-FILTER-ADDON
 *****************************************************************************/
void ArpControllerEnvelopeFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P><I>This filter is now obsolete.  Use Motion Control, instead.</I></P>"
	"<P>I convert each note I receive into a series of control changes based on the note "
"velocity -- effectively, I transform a note into a controller sweep. One such application would"
"be to write a single drum part, and 'carve' that part out of a long, evolving sound. Here is how"
"you would achieve that effect:</P>"

"<P>Create a song with two tracks, both of which have the same MIDI port and channel chosen as their"
"output filter. On the second track, select a sound that will play for any note duration (for example,"
"a flute would work -- a percussion sound like a snare drum would not work, because the sound stops"
"playing shortly after the note is triggered), then place a note down at measure two and continue it"
"for two measures. Keep in mind that you want this track to have a very thick sound. Since you'll be"
"carving pieces out of it, there shouldn't be any quiet time. Layer the voice or even start a new one"
"halfway through the original if necessary.</P>"

"<P>On the first track, place a Controller Envelope filter as the only filter in the output pipeline"
"(other than the output filter). Now open the first track and place several short notes (start with"
"16ths) during the two measures that you placed the note on track two. When you play the song, you"
"should only hear track two during the notes you placed in track one.</P>"

"<P>This filter generates control changes of your choice (the default is volume, control change 7),"
"starting with an initial value (by default zero), and continuing for the attack duration until the"
"start of the note is reached. The final control change of the attack duration will be the same value"
"as the note velocity -- that is, the controllers will sweep down if the initial value is higher than"
"the note velocity, and sweep up if the initial value is lower than the note velocity. The decay"
"duration sweeps the controllers back to the initial value. You can select how many controllers are"
"generated by setting the density value -- lower values will generate more controllers. "

"<P>While this filter might not seem obvious from the description, you can spend a few moments using"
"it in the edit pipeline to understand how it works.</P>";
}

void ArpControllerEnvelopeFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 1;
}

BBitmap* ArpControllerEnvelopeFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpControllerEnvelopeFilterAddOn::NewInstance(AmFilterHolderI* holder,
														const BMessage* config)
{
	return new ArpControllerEnvelopeFilter( this, holder, config );
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpControllerEnvelopeFilterAddOn(cookie);
	return NULL;
}
