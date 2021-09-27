#include "ArpUnquantize.h"

#ifndef AMPUBLIC_AMFILTERCONFIG_H
#include "AmPublic/AmFilterConfigLayout.h"
#endif
#include "AmPublic/AmControls.h"

#include <CheckBox.h>

#include <ArpLayout/ArpViewWrapper.h>
#include <ArpKernel/ArpDebug.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>

ArpMOD();
static AmStaticResources gRes;

static const char* AMOUNT_STR		 = "amount";
static const char* DRIFTING_STR		 = "drifting";
static const char* STEP_STR			 = "step";
static const char* RECOVERY_STR		 = "recovery";

static const char*		AMT_QUANT_STR	= "amt quant";
static const char*		AMT_EIGHTHS_STR	= "amt 8ths";
static const char*		AMT_MULT_STR	= "amt mult";
static const char*		STP_QUANT_STR	= "stp quant";
static const char*		STP_EIGHTHS_STR	= "stp 8ths";
static const char*		STP_MULT_STR	= "stp mult";
static const char*		RCV_QUANT_STR	= "rcv quant";
static const char*		RCV_EIGHTHS_STR	= "rcv 8ths";
static const char*		RCV_MULT_STR	= "rcv mult";

ArpUnquantizeFilter::ArpUnquantizeFilter(ArpUnquantizeFilterAddOn* addon,
							 AmFilterHolderI* holder,
							 const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  mDrifting(true),
	  mAmtMult(1),		mAmtQuant(PPQN/8),			mAmtEighths(2),
	  mStepMult(1),		mStepQuant(PPQN/32),		mStepEighths(2),
	  mRecoveryMult(4),	mRecoveryQuant(PPQN*4),		mRecoveryEighths(2),
	  mLastTime(0), mDrift(0)
{
	srand48((int32)system_time());
	if (config) PutConfiguration(config);
}

ArpUnquantizeFilter::~ArpUnquantizeFilter()
{
}

AmEvent* ArpUnquantizeFilter::HandleEvent(AmEvent* event, const am_filter_params* /*params*/)
{
	if (!event) return event;
	if (mHolder) event->SetNextFilter(mHolder->FirstConnection() );

	AmTime		amount = mAmtMult * ((mAmtQuant*2)/mAmtEighths);
	AmTime		step = mStepMult * ((mStepQuant*2)/mStepEighths);
	AmTime		recovery = mRecoveryMult * ((mRecoveryQuant*2)/mRecoveryEighths);

	if (!mDrifting) {
		// Simple unquantize: just move each event my a random amount,
		// ranging from -(amount/2) to +(amount/2).
		event->SetStartTime(event->StartTime() - (amount/2) + (AmTime)(drand48()*amount));
	} else {
		// Drifting unquantize: move each not by a drift factor, and
		// randomly shift the drift at every event.  Move the drift
		// closer to zero as the distance between events gets longer.
		AmTime newTime = event->StartTime();
		if (newTime < mLastTime) {
			mDrift = 0;
		} else {
			double recover = (double)(recovery-(newTime-mLastTime))/(double)recovery;
			if (recover < 0) recover = 0;
			mDrift = (AmTime)(mDrift*recover);
		}
		mDrift += (AmTime)(drand48()*step) - (step/2);
		if (mDrift < -(amount/2)) mDrift = -(amount/2);
		else if (mDrift > (amount/2)) mDrift = (amount/2);
		mLastTime = newTime;
		event->SetStartTime(newTime + mDrift);
	}
	return event;
}

class ArpUnquantizeFilterSettings : public AmFilterConfigLayout
{
public:
	ArpUnquantizeFilterSettings(AmFilterHolderI* target,
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
				->AddLayoutChild((new AmDurationControl("amount", "Amount:", this, initSettings,
														AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
														AMT_QUANT_STR, AMT_EIGHTHS_STR, AMT_MULT_STR))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)

				->AddLayoutChild((new ArpViewWrapper(
					new BCheckBox(BRect(0,0,10,10), DRIFTING_STR, "Drifting",
							mImpl.AttachCheckBox(DRIFTING_STR),
							B_FOLLOW_NONE,
							B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE)))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,1)
						.SetInt32(ArpRunningBar::FillC,ArpWest)
					)
				)

				->AddLayoutChild((new AmDurationControl("step", "Step:", this, initSettings,
														AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
														STP_QUANT_STR, STP_EIGHTHS_STR, STP_MULT_STR))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
				->AddLayoutChild((new AmDurationControl("recovery", "Recovery:", this, initSettings,
														AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
														RCV_QUANT_STR, RCV_EIGHTHS_STR, RCV_MULT_STR))
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						.SetBool(ArpRunningBar::AlignLabelsC,true)
					)
				)
			);
		} catch(...) {
			throw;
		}
		
		Implementation().RefreshControls(mSettings);
		UpdateControls();
	}
	
	virtual void MessageReceived(BMessage *message)
	{
		inherited::MessageReceived(message);
		if (message && message->what == ARP_PUT_CONFIGURATION_MSG) {
			UpdateControls();
		}
	}
	
	void UpdateControls()
	{
		BCheckBox* cb = dynamic_cast<BCheckBox*>(FindView(DRIFTING_STR));
		if (cb) {
			BTextControl* tc;
			tc = dynamic_cast<BTextControl*>(FindView(STEP_STR));
			if (tc) tc->SetEnabled(cb->Value());
			tc = dynamic_cast<BTextControl*>(FindView(RECOVERY_STR));
			if (tc) tc->SetEnabled(cb->Value());
		}
	}
	
protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpUnquantizeFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	
	if ((err=values->AddBool(DRIFTING_STR, mDrifting)) != B_OK) return err;

	if ( (err = values->AddInt32(AMT_MULT_STR, mAmtMult)) != B_OK ) return err;
	if ( (err = add_time(*values, AMT_QUANT_STR, mAmtQuant)) != B_OK ) return err;
	if ( (err = values->AddInt32(AMT_EIGHTHS_STR, mAmtEighths)) != B_OK ) return err;

	if ( (err = values->AddInt32(STP_MULT_STR, mStepMult)) != B_OK ) return err;
	if ( (err = add_time(*values, STP_QUANT_STR, mStepQuant)) != B_OK ) return err;
	if ( (err = values->AddInt32(STP_EIGHTHS_STR, mStepEighths)) != B_OK ) return err;

	if ( (err = values->AddInt32(RCV_MULT_STR, mRecoveryMult)) != B_OK ) return err;
	if ( (err = add_time(*values, RCV_QUANT_STR, mRecoveryQuant)) != B_OK ) return err;
	if ( (err = values->AddInt32(RCV_EIGHTHS_STR, mRecoveryEighths)) != B_OK ) return err;

	return B_OK;
}

status_t ArpUnquantizeFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;
	
	bool b;
	if (values->FindBool(DRIFTING_STR, &b) == B_OK) mDrifting = b;

	AmTime t;
	/* Backwards compatibility
	 */
	if (find_time(*values, AMOUNT_STR, &t) == B_OK) {
		AmDurationControl::SplitTicks(t, &mAmtMult, &mAmtQuant, &mAmtEighths);
	}
	if (find_time(*values, STEP_STR, &t) == B_OK) {
		AmDurationControl::SplitTicks(t, &mStepMult, &mStepQuant, &mStepEighths);
	}
	if (find_time(*values, RECOVERY_STR, &t) == B_OK) {
		AmDurationControl::SplitTicks(t, &mRecoveryMult, &mRecoveryQuant, &mRecoveryEighths);
	}
	/* End backwards compatibility
	 */
	int32	i;
	if (values->FindInt32(AMT_MULT_STR, &i) == B_OK) mAmtMult = i;
	if (find_time(*values, AMT_QUANT_STR, &t) == B_OK) mAmtQuant = t;
	if (values->FindInt32(AMT_EIGHTHS_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mAmtEighths = i;
	}
	if (values->FindInt32(STP_MULT_STR, &i) == B_OK) mStepMult = i;
	if (find_time(*values, STP_QUANT_STR, &t) == B_OK) mStepQuant = t;
	if (values->FindInt32(STP_EIGHTHS_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mStepEighths = i;
	}
	if (values->FindInt32(RCV_MULT_STR, &i) == B_OK) mRecoveryMult = i;
	if (find_time(*values, RCV_QUANT_STR, &t) == B_OK) mRecoveryQuant = t;
	if (values->FindInt32(RCV_EIGHTHS_STR, &i) == B_OK) {
		if (i <= 2) i = 2;
		else if (i <= 4) i = 3;
		else if (i <= 6) i = 5;
		else i = 7;
		mRecoveryEighths = i;
	}

	return B_OK;
}

status_t ArpUnquantizeFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpUnquantizeFilterSettings(mHolder, config));
	return B_OK;
}

/* ----------------------------------------------------------------
   ArpUnquantizeFilterAddOn Class
   ---------------------------------------------------------------- */
void ArpUnquantizeFilterAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<p>Typically, MIDI performances have computer-perfect timing. \n"
	"Unquantizing is a process that slightly alters the time of MIDI events in \n"
	"order to create a sloppy, more human feel.  The Amount parameter of this \n"
	"filter determines the maximum number of ticks that a single event can be \n"
	"offset, by randomly generating a value between zero and the Amount and applying \n"
	"that to each event.</p>\n"
"\n"
	"<P>Randomly quantizing a performance makes it less mechanical, but generally \n"
	"not any more human, since human players tend to have patterns in their \n"
	"imperfections.  If the Drifting parameter is on, this filter uses the Recovery \n"
	"value and Step value together to simulate a human player drifting on and off \n"
	"the beat.  The Recovery is the maximum number of ticks it takes the performer \n"
	"to return to the correct time, the Steps are the number of ticks in each step \n"
	"on the return.  This filter is used heavily in the file <i>Examples/DianneHackborn \n"
	"- Bells.mid</i></p>\n";
}

void ArpUnquantizeFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpUnquantizeFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpUnquantizeFilterAddOn(cookie);
	return NULL;
}
