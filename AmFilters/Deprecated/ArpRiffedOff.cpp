#include "ArpRiffedOff.h"

#include <cstdio>
#include <cstdlib>
#include <experimental/ResourceSet.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpLayout/ArpViewWrapper.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmFilterConfigLayout.h"

static const AmTime		QUANT_PRIMES		= 3*5*7;
static const char*		MEASURE_LENGTH_STR	= "measure_length";

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

/* These belong with the subdivider filter, but since I've hacked multiple
 * filters together, they're up here for now.
 */
static void get_sd_step_string(uint32 number, BString& out)
{
	out << number << "_sd_stp";
}

static void get_sd_freq_string(uint32 number, BString& out)
{
	out << number << "_sd_frq";
}

/* These belong with the vaccine filter, but since I've hacked multiple
 * filters together, they're up here for now.
 */
static void get_v_prox_string(const char* key, BString& out)
{
	out << key << "_v_prox";
}

static void get_v_freq_string(const char* key, BString& out)
{
	out << key << "_v_freq";
}

static void get_v_amt_string(const char* key, BString& out)
{
	out << key << "_v_amt";
}

static const char* VACCINE1_KEY		= "1";
static const char* VACCINE2_KEY		= "2";

/*****************************************************************************
 * ARP-RIFFED-OFF-FILTER
 *****************************************************************************/
ArpRiffedOffFilter::ArpRiffedOffFilter(	ArpRiffedOffFilterAddOn* addon,
									AmFilterHolderI* holder,
									const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon),
	  mHolder(holder),
	  /* FIX:  For now, just create a pattern based on sixteenths.  Need to
	   * expand this to allow users to set the note duration.
	   */
	  mQuantizeTime((AmTime)(PPQN * 0.25)), mModifier(2), mFullTime((mQuantizeTime*2*QUANT_PRIMES)/mModifier),
//	  mMeasureCount(1), mSeed(0)
	  mMeasureCount(1), mSeed( (bigtime_t)(system_time() * 0.33) ),
	  mVaccine1(holder, VACCINE1_KEY), mVaccine2(holder, VACCINE2_KEY, 0, 50, -200),
	  mSubdivider(holder)
{
	if (config) PutConfiguration(config);
	srand( (int32)(system_time()/100) );
	mSeed = rand();
}

ArpRiffedOffFilter::~ArpRiffedOffFilter()
{
}

static inline AmTime quantize(AmTime inTime, AmTime fullTime)
{
	const int64 t = ((int64)inTime)*QUANT_PRIMES;
	return (t-(t%fullTime)) / QUANT_PRIMES;
}

static inline AmTime next_time(const AmEvent* event, AmTime fullTime, AmTime quantizeTime)
{
	AmTime	nextTime = quantize(event->EndTime() + 1, fullTime);
	while ( event->EndTime() >= nextTime) nextTime += quantizeTime;
	return nextTime;
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

AmEvent* ArpRiffedOffFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event) return event;
	ArpVALIDATE(mAddOn && mHolder && params && params->cur_signature, return event);
	const AmSignature*	curSig = params->cur_signature;
	AmSignature			sig;
	if ( !get_signature(event->StartTime(), curSig, sig) ) return event;
	mCounter = 0;
	bigtime_t	curSeed = mSeed;
	AmTime		duration = event->Duration();
	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	head = dynamic_cast<AmNoteOn*>(event);
		if (!head) return event;
		
		AmTime		nextTime = event->StartTime();

		AmNoteOn*	curEvent = head;
		AmNoteOn*	e = curEvent;
		while (mCounter < mMeasureCount) {
			e->SetStartTime(nextTime);
			e->SetDuration(duration);
			int32		vel = curSeed % 128;
			if (vel < 0) vel = 0; else if (vel > 127) vel = 127;
			e->SetVelocity(vel);
			
			curSeed += (bigtime_t)(curSeed * 0.13);
			e = dynamic_cast<AmNoteOn*>( mSubdivider.HandleEvent(e, params) );
			while (e) {
				AmEvent*	nextE = e->NextEvent();
				AmNoteOn* e2 = dynamic_cast<AmNoteOn*>( mVaccine1.HandleEvent(e, params) );
				if (e2) e2 = dynamic_cast<AmNoteOn*>( mVaccine2.HandleEvent(e2, params) );
				if (e2) {
					if (curEvent != e2) {
						e2->RemoveEvent();
						curEvent->AppendEvent(e2);
						curEvent = e2;
					}
				}
				e = dynamic_cast<AmNoteOn*>( nextE );
			}
			nextTime = next_time(curEvent, mFullTime, mQuantizeTime);
			if ( nextTime > sig.EndTime() ) GetSignature(nextTime, curSig, sig);

			e = dynamic_cast<AmNoteOn*>( curEvent->Copy() );
			if (!e) return head;
			e->SetNextEvent(NULL);
			e->SetPrevEvent(NULL);
		}
		/* Remove any events with a velocity of 0.
		 */
		if (!head) return NULL;
		
		while (head && (head->Velocity() == 0) && (head->NextEvent()) ) {
			AmNoteOn*	killed = head;
			head = dynamic_cast<AmNoteOn*>( head->NextEvent() );
			killed->RemoveEvent();
			killed->Delete();
		}
		AmNoteOn*	next = dynamic_cast<AmNoteOn*>( head->NextEvent() );
		while (next) {
			AmNoteOn*	killed = next;
			next = dynamic_cast<AmNoteOn*>( next->NextEvent() );
			if (killed->Velocity() == 0) {
				killed->RemoveEvent();
				killed->Delete();
			}
		}
		return head;
#if 0		
	} else if( event->Type() == event->NOTEOFF_TYPE ) {
		AmNoteOff*	note = dynamic_cast<AmNoteOff*>(event);
		if( !note ) return event;

		for( uint32 k = 0; k < MAX_CHORUS; k++ ) {
			if( mOctaves[k] != 0 || mSteps[k] != 0 || mVelocities[k] != 100 ) {
				uint8 pitch = note->Note();
				int32 delta = (mOctaves[k]*12) + (mSteps[k]);
				if (pitch+delta > 0x7f) pitch = 0x7f;
				else if(pitch+delta < 0) pitch = 0;
				else pitch += delta;

				AmNoteOff*	e = dynamic_cast<AmNoteOff*>( note->Copy() );
				if( e ) {
					e->SetNote( pitch );
					float	newVel = e->Velocity() * (float)(mVelocities[k]/100);
					if( newVel < 0 ) newVel = 0;
					else if( newVel > 127 ) newVel = 127;
					e->SetVelocity( newVel );
					head->AppendEvent(e);
				}
			}
		}
#endif
	}

	return event;
}

void ArpRiffedOffFilter::GetSignature(AmTime time, const AmSignature* sig, AmSignature& answer)
{
	/* Fix:  Track any changes in the signature
	 */
	AmTime		sigLength = answer.Duration();
	while (answer.EndTime() < time) {
		mCounter++;
		answer.Set( answer.StartTime() + sigLength, answer.Measure() + 1,
					answer.Beats(), answer.BeatValue() );
	}
}

class ArpRiffedOffFilterSettings : public AmFilterConfigLayout
{
public:
	ArpRiffedOffFilterSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		float	labelW = -1, intW = -1;
		const BFont*	font = be_plain_font;
		if( font ) {
			labelW = font->StringWidth( "Octaves:");
			intW = font->StringWidth("200%") + 5;
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
			topVBar->AddLayoutChild((new ArpTextControl(
										MEASURE_LENGTH_STR, "Measure length:","",
										mImpl.AttachTextControl(MEASURE_LENGTH_STR)))
					->SetParams(ArpMessage()
						.SetString(ArpTextControl::MinTextStringP, "888")
						.SetString(ArpTextControl::PrefTextStringP, "88888888")
					)
					->SetConstraints(ArpMessage()
						.SetFloat(ArpRunningBar::WeightC,3)
						.SetInt32(ArpRunningBar::FillC,ArpEastWest)
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

status_t ArpRiffedOffFilter::GetConfiguration(BMessage* values) const
{
	status_t err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;
	if ((err=values->AddInt32(MEASURE_LENGTH_STR, mMeasureCount)) != B_OK) return err;

	return B_OK;
}

/* MOTHERFUCK this is ugly.  Because I'm cobbling one filter out of several
 * others without having a proper 'multifilter filter' I need to infer which
 * config update messages go to which of my component filters.
 */
static bool is_subdivider_message(const BMessage* values)
{
	for (uint32 k = 0; k < ArpSubdividerFilter::MAX_STEPS; k++) {
		BString		stepStr;
		get_sd_step_string(k, stepStr);
		if (values->HasInt32( stepStr.String() ) ) return true;
		BString		freqStr;
		get_sd_freq_string(k, freqStr);
		if (values->HasInt32( freqStr.String() ) ) return true;
	}
	return false;
}

static bool is_vaccine_message(const BMessage* values, const char* key)
{
	BString		proxStr;
	get_v_prox_string(key, proxStr);
	if (values->HasInt32( proxStr.String() ) ) return true;
	BString		freqStr;
	get_v_freq_string(key, freqStr);
	if (values->HasInt32( freqStr.String() ) ) return true;
	BString		amtStr;
	get_v_amt_string(key, amtStr);
	if (values->HasInt32( amtStr.String() ) ) return true;
	return false;
}

status_t ArpRiffedOffFilter::PutConfiguration(const BMessage* values)
{
	status_t err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	if ( is_subdivider_message(values) ) return mSubdivider.PutConfiguration(values);
	if ( is_vaccine_message(values, VACCINE1_KEY) ) return mVaccine1.PutConfiguration(values);
	if ( is_vaccine_message(values, VACCINE2_KEY) ) return mVaccine2.PutConfiguration(values);

	int32	i;
	if (values->FindInt32(MEASURE_LENGTH_STR, &i) == B_OK) mMeasureCount = i;

	return B_OK;
}

status_t ArpRiffedOffFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpRiffedOffFilterSettings(mHolder, config));
	err = mVaccine1.Configure(panels);
	if (err != B_OK) return err;
	err = mVaccine2.Configure(panels);
	if (err != B_OK) return err;
	return mSubdivider.Configure(panels);
}

/*****************************************************************************
 * ARP-RIFFED-OFF-FILTER-ADD-ON
 *****************************************************************************/
void ArpRiffedOffFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpRiffedOffFilterAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = Resources().FindBitmap("Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpRiffedOffFilterAddOn(cookie);
	return NULL;
}

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
ArpVaccineFilter::ArpVaccineFilter(	AmFilterHolderI* holder, const char* key,
									int32 prox, int32 freq, int32 amt)
		: mHolder(holder), mKey(key), mProximity(prox), mFrequency(freq), mAmount(amt)
{
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

	if (event->Type() == event->NOTEON_TYPE) {
		AmNoteOn*	noe = dynamic_cast<AmNoteOn*>(event);
		if (noe) {
			int32	vel = noe->Velocity();
			float	scale = ((float)mAmount / 100) * prox2;
			vel = vel + (int32)(vel * scale);
			if (vel < 0) vel = 0;
			else if (vel > 127) vel = 127;
//printf("\tturned vel %d into %ld with amount %f\n", noe->Velocity(), vel, scale);
			noe->SetVelocity(vel);
		}
	} else if (event->Type() == event->NOTEOFF_TYPE) {
	}

	return event;
}

static void add_vaccine_box(ArpBaseLayout* toLayout, const char* key, ArpConfigureImpl& impl, float labelW, float intW)
{
	ArpBaseLayout*	vBar = (new ArpRunningBar("SubVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	toLayout->AddLayoutChild(vBar);

	BString			proxStr;
	get_v_prox_string(key, proxStr);
	ArpKnobPanel*	kp = 0;
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( proxStr.String(), "Proximity to beat:", impl.AttachControl( proxStr.String() ), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetFormatter( new _ProximityFormat() );
	}

	BString			freqStr;
	get_v_freq_string(key, freqStr);
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( freqStr.String(), "Frequency:", impl.AttachControl( freqStr.String() ), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetFormatter( new _FrequencyFormat() );
	}

	BString			amtStr;
	get_v_amt_string(key, amtStr);
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( amtStr.String(), "Amount:", impl.AttachControl( amtStr.String() ), -400, 400, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetFormatter( new _OffPercentFormat() );
	}
}

class ArpVaccineFilterSettings : public AmFilterConfigLayout
{
public:
	ArpVaccineFilterSettings(	AmFilterHolderI* target,
								const BMessage& initSettings,
								const char* key)
		: AmFilterConfigLayout(target, initSettings)
	{
		BString		n("Vaccine ");
		n << key;
		SetName( n.String() );
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
#if 0
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
#endif
			ArpBaseLayout*	colHBar = (new ArpRunningBar("ColHBar"))
											->SetParams(ArpMessage()
												.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
												.SetFloat(ArpRunningBar::IntraSpaceP, .5)
											);
			topVBar->AddLayoutChild( colHBar );
			add_vaccine_box(colHBar, key, mImpl, labelW, intW);
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
//	status_t err = AmFilterI::GetConfiguration(values);
//	if (err != B_OK) return err;
	status_t	err;

	BString		proxStr;
	get_v_prox_string(mKey, proxStr);
	if ((err=values->AddInt32(proxStr.String(), mProximity)) != B_OK) return err;

	BString		freqStr;
	get_v_freq_string(mKey, freqStr);
	if ((err=values->AddInt32(freqStr.String(), mFrequency)) != B_OK) return err;

	BString		amtStr;
	get_v_amt_string(mKey, amtStr);
	if ((err=values->AddInt32(amtStr.String(), mAmount)) != B_OK) return err;

	return B_OK;
}

status_t ArpVaccineFilter::PutConfiguration(const BMessage* values)
{
//	status_t err = AmFilterI::PutConfiguration(values);
//	if (err != B_OK) return err;

	int32 		i;
	BString		proxStr;
	get_v_prox_string(mKey, proxStr);
	if (values->FindInt32(proxStr.String(), &i) == B_OK) mProximity = i;

	BString		freqStr;
	get_v_freq_string(mKey, freqStr);
	if (values->FindInt32(freqStr.String(), &i) == B_OK) mFrequency = i;

	BString		amtStr;
	get_v_amt_string(mKey, amtStr);
	if (values->FindInt32(amtStr.String(), &i) == B_OK) mAmount = i;	

	return B_OK;
}

status_t ArpVaccineFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpVaccineFilterSettings(mHolder, config, mKey));
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







/*************************************************************************
 * _STEP-MAP
 *************************************************************************/
static char* gStepMap[] = {	"* 16",	"* 8",	"* 4",	"* 2",
							"Off",
							"/ 2",	"/ 4",	"/ 8",	"/ 16" };
static const int32	MIN_SUBDIVISION = -4;
static const int32	MAX_SUBDIVISION = 4;
			
class _StepMap : public ArpIntToStringMapI
{
public:
	_StepMap()	{ ; }

	virtual status_t IdForName(const char *name, int32 *answer) const
	{	*answer = 0;

		for (int32 k = 0; k < 9; k++) {
			if (strcmp(gStepMap[k], name) == 0) {
				*answer = k - MAX_SUBDIVISION;
				return B_OK;
			}
		}
		return B_ERROR;
	}

	virtual status_t NameForId(int32 id, char **answer) const
	{
		if ( (id < MIN_SUBDIVISION) || (id > MAX_SUBDIVISION) ) return B_ERROR;
		*answer = gStepMap[id + MAX_SUBDIVISION];
		return B_OK;
	}
};

/*****************************************************************************
 * ARP-SUBDIVIDER-FILTER
 *****************************************************************************/
static float gSubdivideTable[] = {	PPQN * 4, PPQN * 2, PPQN, PPQN * 0.5,
									PPQN * 0.25, PPQN * 0.125, PPQN * 0.0625, 0 };	

ArpSubdividerFilter::ArpSubdividerFilter(AmFilterHolderI* holder)
		: mHolder(holder)
{
	InitializeSteps();
}

static int32 subdivide_index(AmTime duration)
{
	float	sub;
	int32	index = -1;
	for(int32 k = 0; (sub = gSubdivideTable[k]) != 0; k++) {
		if (duration <= sub) index = k;
	}
	return index;
}

static AmTime subdivide_to(AmTime duration, int32 step)
{
	int32	index = subdivide_index(duration);
	if (index < 0) return 0;
	int32	newIndex = index + step;
	if (newIndex < 0) return 0;
	
	float	sub;
	for(int32 k = 0; (sub = gSubdivideTable[k]) != 0; k++)
		if (k == newIndex) return (AmTime)sub;
	return 0;
}

AmEvent* ArpSubdividerFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	if (!event) return event;
	if (event->Type() != event->NOTEON_TYPE && event->Type() != event->NOTEOFF_TYPE) return event;

	AmEvent*	e = event;
	for (uint32 k = 0; k < MAX_STEPS; k++) {
		e = Subdivide(e, mStep[k], mFrequency[k]);
	}
	return e;
}

static void add_subdivider_box(ArpBaseLayout* toLayout, uint32 num, ArpConfigureImpl& impl, float labelW, float intW)
{
	BString			boxLabel("Subdivision ");
	boxLabel << num + 1;
	ArpBaseLayout*	box = (new ArpBox("SubdividerBox", boxLabel.String() ))
						->SetConstraints(ArpMessage()
							.SetFloat(ArpRunningBar::WeightC,1)
							.SetInt32(ArpRunningBar::FillC,ArpNorthSouth));
	toLayout->AddLayoutChild( box );

	ArpBaseLayout*	vBar = (new ArpRunningBar("SubVBar"))
										->SetParams(ArpMessage()
										.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
										.SetFloat(ArpRunningBar::IntraSpaceP, .5)
									);
	box->AddLayoutChild( vBar );

	BString			stepStr;
	get_sd_step_string( num, stepStr );
	ArpKnobPanel*	kp = 0;
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( stepStr.String(), "Step:", impl.AttachControl( stepStr.String() ), MIN_SUBDIVISION, MAX_SUBDIVISION, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetStringMap( new _StepMap() );
	}

	BString			frqStr;
	get_sd_freq_string( num, frqStr );
	vBar->AddLayoutChild((new ArpViewWrapper(kp = new ArpKnobPanel( frqStr.String(), "Frequency:", impl.AttachControl( frqStr.String() ), 0, 100, true, B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW, intW )))
			->SetConstraints(ArpMessage()
				.SetFloat(ArpRunningBar::WeightC,3)
				.SetInt32(ArpRunningBar::FillC,ArpEastWest)));
	if( kp ) {
		ArpIntControl*	intCtrl = kp->IntControl();
		if( intCtrl ) intCtrl->SetFormatter( new _FrequencyFormat() );
	}
}

class ArpSubdividerFilterSettings : public AmFilterConfigLayout
{
public:
	ArpSubdividerFilterSettings(AmFilterHolderI* target,
						  const BMessage& initSettings)
		: AmFilterConfigLayout(target, initSettings)
	{
		SetName("Subdivider");
		float	labelW = -1, intW = -1;
		const BFont*	font = be_plain_font;
		if( font ) {
			labelW = font->StringWidth( "Frequency:");
			intW = font->StringWidth("Always") + 5;
		}

		try {
			ArpBaseLayout*	topVBar = (new ArpRunningBar("TopVBar"))
											->SetParams(ArpMessage()
												.SetInt32(ArpRunningBar::OrientationP, B_VERTICAL)
												.SetFloat(ArpRunningBar::IntraSpaceP, .5)
											);
			AddLayoutChild( topVBar );
#if 0
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
#endif
			ArpBaseLayout*	colHBar = (new ArpRunningBar("ColHBar"))
											->SetParams(ArpMessage()
												.SetInt32(ArpRunningBar::OrientationP, B_HORIZONTAL)
												.SetFloat(ArpRunningBar::IntraSpaceP, .5)
											);
			topVBar->AddLayoutChild( colHBar );
			for( uint32 k = 0; k < ArpSubdividerFilter::MAX_STEPS; k++ )
				add_subdivider_box( colHBar, k, mImpl, labelW, intW );
		} catch(...) {
			throw;
		}
		Implementation().RefreshControls(mSettings);
	}

protected:
	typedef AmFilterConfigLayout inherited;
};

status_t ArpSubdividerFilter::GetConfiguration(BMessage* values) const
{
//	status_t err = AmFilterI::GetConfiguration(values);
//	if (err != B_OK) return err;
	status_t	err;
	for( uint32 k = 0; k < MAX_STEPS; k++ ) {
		BString		stepStr;
		get_sd_step_string(k, stepStr);
		if ((err=values->AddInt32(stepStr.String(), mStep[k])) != B_OK) return err;
		BString		freqStr;
		get_sd_freq_string(k, freqStr);
		if ((err=values->AddInt32(freqStr.String(), mFrequency[k])) != B_OK) return err;
	}

	return B_OK;
}

status_t ArpSubdividerFilter::PutConfiguration(const BMessage* values)
{
//	status_t err = AmFilterI::PutConfiguration(values);
//	if (err != B_OK) return err;

	int32 		i;
	for( uint32 k = 0; k < MAX_STEPS; k++ ) {
		BString		stepStr;
		get_sd_step_string(k, stepStr);
		if (values->FindInt32(stepStr.String(), &i) == B_OK) mStep[k] = i;
		BString		freqStr;
		get_sd_freq_string(k, freqStr);
		if (values->FindInt32(freqStr.String(), &i) == B_OK) mFrequency[k] = i;
	}

	return B_OK;
}

status_t ArpSubdividerFilter::Configure(ArpVectorI<BView*>& panels)
{
	BMessage config;
	status_t err = GetConfiguration(&config);
	if (err != B_OK) return err;
	panels.push_back(new ArpSubdividerFilterSettings(mHolder, config));
	return B_OK;
}

AmEvent* ArpSubdividerFilter::Subdivide(AmEvent* event, int32 step, int32 frequency)
{
	if (step == 0) return event;
		
	AmTime		oldSub = subdivide_to( event->Duration(), 0 );
	AmTime		newSub = subdivide_to( event->Duration(), step );
	if (oldSub <= 0 || newSub <= 0) return event;
	AmTime		newDuration = (AmTime)(((float)event->Duration() * (float)newSub) / (float)oldSub);
	if (newDuration < 1) return event;

	if (step < 0) {
		AmEvent*	e = event;
		while (e) {
			if ( ShouldSubdivide(frequency) ) e = SubdivideLarger(e, newDuration);
			else e = e->NextEvent();
		}
	} else if (step > 0) {
		AmEvent*	e = event;
		while (e) {
			if (ShouldSubdivide(frequency) ) e = SubdivideSmaller(e, newSub, newDuration);
			else e = e->NextEvent();
		}
	}
	
	return event;
}

AmEvent* ArpSubdividerFilter::SubdivideLarger(AmEvent* event, AmTime newDuration)
{
	event->SetDuration(newDuration);
	AmEvent*	next = event->NextEvent();
	AmEvent*	next2;
	while ( next && next->StartTime() <= event->EndTime() ) {
		next2 = next->NextEvent();
		next->RemoveEvent();
		next->Delete();
		next = next2;
	}
	return next;
}

AmEvent* ArpSubdividerFilter::SubdivideSmaller(AmEvent* event, AmTime newSub, AmTime newDuration)
{
	AmTime		oldEnd = event->EndTime();
	event->SetDuration(newDuration);
	AmEvent*	curEvent = event;
	AmTime		curStart = event->StartTime() + newSub;
	while ( (curStart + newDuration) <= oldEnd ) {
		AmEvent*	copy = curEvent->Copy();
		if (copy) {
			copy->SetStartTime(curStart);
			copy->SetDuration(newDuration);
			curEvent->MergeEvent(copy);
			curEvent = copy;
		}
		curStart += newSub;
	}
	ArpASSERT(curEvent);
	if (!curEvent) return NULL;
	return curEvent->NextEvent();
}

bool ArpSubdividerFilter::ShouldSubdivide(int32 frequency) const
{
	if (frequency <= 0) return false;
	if (frequency >= 100) return true;

	srand( (int32)(system_time()/100) );
	int32 percent = rand() % 100;

	if (percent < frequency) return true;
	else return false;
}

void ArpSubdividerFilter::InitializeSteps()
{
	for (int32 k = 0; k < MAX_STEPS; k++) {
		mStep[k] = 0;
		mFrequency[k] = 50;
	}
//	mStep[0] = 1;
}
