#include "ArpVaccine.h"

#include <stdio.h>
#include <stdlib.h>
#include <be/experimental/ResourceSet.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmFilterConfigLayout.h"

ArpMOD();

static BResourceSet gResources;
static int32 gInitResources = 0;
BResourceSet& Resources()
{
	if (atomic_or(&gInitResources, 1) == 0) {
		gResources.AddResources((void*)Resources);
		atomic_or(&gInitResources, 2);
	} else {
		while ((gInitResources&2) == 0) snooze(20000);
	}
	return gResources;
}

static const char*		PROXIMITY_STR		= "proximity";
static const char*		FREQUENCY_STR		= "frequency";
static const char*		AMOUNT_STR			= "amount";

static const int32		MIN_AMOUNT			= -400;
static const int32		MAX_AMOUNT			= 400;

/*************************************************************************
 * _PROXIMITY-FORMAT
 * A class that formats the proximity to beat field.
 *************************************************************************/
class _ProximityFormat : public ArpIntFormatterI
{
public:
	_ProximityFormat()	{ ; }

	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number <= 0) out << "Furthest";
		else if (number >= 100) out << "Closest";
		else out << number;
	}
};

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

/*************************************************************************
 * _FREQUENCY-FORMAT
 * A class that formats 0 to off and everything else to a percent.
 *************************************************************************/
class _FrequencyFormat : public ArpIntFormatterI
{
public:
	_FrequencyFormat()	{ ; }

	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number == 0) out << "Never";
		else if (number == 100) out << "Always";
		else out << number << '%';
	}
};

/*****************************************************************************
 * ARP-VACCINE-FILTER
 *****************************************************************************/
ArpVaccineFilter::ArpVaccineFilter(	ArpVaccineFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* settings)
		: AmFilterI(addon), mAddOn(addon), mHolder(holder), mProximity(100), mFrequency(50), mAmount(200)
{
	if (settings) PutConfiguration(settings);
}

ArpVaccineFilter::~ArpVaccineFilter()
{
}

static bool get_signature(AmTime time, const AmSignature* sig, AmSignature& answer)
{
	if (!sig) return false;
	if (sig->StartTime() > time) return false;
	answer.Set(*sig);
	AmTime		sigLength = answer.Duration();
	while (answer.EndTime() < time)
		answer.Set( answer.StartTime() + sigLength, answer.Measure() + 1,
					answer.Beats(), answer.BeatValue() );
	return true;	
}

static float proximity_to_beat(AmTime time, AmSignature sig)
{
	if ( time < sig.StartTime() || time > sig.EndTime() ) {
		printf("ArpVaccine::proximity_to_beat ERROR:  time not in sig\n");
		return 0.0;
	}
	AmTime		ticksPerBeat = sig.TicksPerBeat();
	AmTime		t = time - sig.StartTime();
	AmRange		range(0, ticksPerBeat - 1);
	uint32		beat = 1;
	while (t > range.end) {
		beat++;
		range.start += ticksPerBeat;
		range.end += ticksPerBeat;
	}
	AmTime		delta = range.start;
	t -= delta;
	range.start -= delta;
	range.end -= delta;
	float	prox = (float)t / (float)range.end;
	if (prox < 0) prox = 0.0;
	if (prox > 1) prox = 1.0;
	return fabs(1.0 - prox);	
}

AmEvent* ArpVaccineFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event) return event;
	if (mFrequency == 0 || mAmount == 0) return event;
	if (event->Type() != event->NOTEON_TYPE && event->Type() != event->NOTEOFF_TYPE) return event;
	if ( !ShouldVaccinate(mFrequency) ) return event;
	ArpVALIDATE(params && params->cur_signature, return event);
	const AmSignature*	curSig = params->cur_signature;
	AmSignature			sig;
	if ( !get_signature(event->StartTime(), curSig, sig) ) return event;

	int32				prox = (int32)(proximity_to_beat(event->StartTime(), sig) * 100);
	
	float	prox2 = (float)abs(mProximity - prox) / 100;
//	printf("proximity is %ld, that puts prox2 at %f\n", prox, prox2);
	if (prox2 < 0) prox2 = 0;
	else if (prox2 > 1) prox2 = 1;
	prox2 = 1 - prox2;

	/* If there are tool params, set my amount based on the param data.
	 */
	int32				amount = mAmount;
	if (params->flags&AMFF_TOOL_PARAMS) {
		if (mAmount >= 0) amount = (int32)(params->view_orig_y_pixel - params->view_cur_y_pixel);
		else amount = (int32)(params->view_cur_y_pixel - params->view_orig_y_pixel);
		if (amount < MIN_AMOUNT) amount = MIN_AMOUNT;
		else if (amount > MAX_AMOUNT) amount = MAX_AMOUNT;
	}
	
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	noe = dynamic_cast<AmNoteOn*>(event);
		if (noe) {
			int32	vel = noe->Velocity();
			float	scale = ((float)amount / 100) * prox2;
			vel = vel + (int32)(vel * scale);
			if (vel < 0) vel = 0;
			else if (vel > 127) vel = 127;
			noe->SetVelocity(vel);
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
	}

	return event;
}

static void add_vaccine_box(ArpBaseLayout* toLayout, ArpConfigureImpl& impl, float labelW, float intW)
{
	ArpBaseLayout*	vBar = (new ArpRunningBar("SubVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	toLayout->AddLayoutChild(vBar);

	ArpKnobPanel*	kp = 0;
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(PROXIMITY_STR, "Proximity to beat:", impl.AttachControl(PROXIMITY_STR), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _ProximityFormat() );
	}

	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(FREQUENCY_STR, "Frequency:", impl.AttachControl(FREQUENCY_STR), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _FrequencyFormat() );
	}

	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel(AMOUNT_STR, "Amount:", impl.AttachControl(AMOUNT_STR), MIN_AMOUNT, MAX_AMOUNT, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW)))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if (kp) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if (intCtrl) intCtrl->SetFormatter( new _OffPercentFormat() );
	}
}

class ArpVaccineFilterSettings : public AmFilterConfigLayout
{
public:
	ArpVaccineFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		float	labelW = -1, intW = -1;
		const BFont*	font = be_plain_font;
		if( font ) {
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
							.SetFloat(ArpRunningBar::WeightC,3)
							.SetInt32(ArpRunningBar::FillC,ArpEastWest)
						)
					);
			ArpBaseLayout*	colHBar = (new ArpRunningBar("ColHBar"))
											->SetParams(ArpMessage()
												.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
												.SetFloat(ArpRunningBar::IntraSpaceP, .5)
											);
			topVBar->AddLayoutChild( colHBar );
			add_vaccine_box(colHBar, mImpl, labelW, intW);
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
	}

protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpVaccineFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	if (values->AddInt32(PROXIMITY_STR, mProximity) != B_OK) return B_ERROR;
	if (values->AddInt32(FREQUENCY_STR, mFrequency) != B_OK) return B_ERROR;
	if (values->AddInt32(AMOUNT_STR, mAmount) != B_OK) return B_ERROR;

	return B_OK;
}

status_t ArpVaccineFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	int32		i;
	if (values->FindInt32(PROXIMITY_STR, &i) == B_OK) mProximity = i;
	if (values->FindInt32(FREQUENCY_STR, &i) == B_OK) mFrequency = i;
	if (values->FindInt32(AMOUNT_STR, &i) == B_OK) mAmount = i;

	return B_OK;
}

status_t ArpVaccineFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpVaccineFilterSettings(mHolder, config));
	return B_OK;
}

bool ArpVaccineFilter::ShouldVaccinate(int32 frequency) const
{
	if (frequency <= 0) return false;
	if (frequency >= 100) return true;

	srand( (int32)(system_time()/100) );
	int32 percent = rand() % 100;

	if (percent < frequency) return true;
	else return false;
}

/*****************************************************************************
 * ARP-VACCINE-FILTER-ADD-ON
 *****************************************************************************/
void ArpVaccineFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpVaccineFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpVaccineFilterAddOn(cookie);
	return NULL;
}
