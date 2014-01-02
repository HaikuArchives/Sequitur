/* ArpOscillator.cpp
 */
#include <stdio.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpOscillator.h"

#include "AmKernel/AmSineSweep.h"

ArpMOD();
static AmStaticResources gRes;

/*****************************************************************************
 * ARP-TIME-STRETCH-FILTER
 *****************************************************************************/
ArpOscillatorFilter::ArpOscillatorFilter(	ArpOscillatorAddOn* addon,
											AmFilterHolderI* holder,
											const BMessage* config)
		: AmFilterI(addon),
		  mAddOn(addon), mHolder(holder), mSweep(0), mResolution(100),
		  mCurStep(0), mLast(-1)
{
	SetFlag(OSCILLATOR_FLAG, true);

	mSweep = new AmSineSweep(1, 0.75);
	if (config) PutConfiguration(config);
}

ArpOscillatorFilter::~ArpOscillatorFilter()
{
	delete mSweep;
}

status_t ArpOscillatorFilter::PutConfiguration(const BMessage* values)
{
	status_t result = AmFilterI::PutConfiguration(values);
	// Want to make sure that oscillator mode is always turned on.
	SetFlag(OSCILLATOR_FLAG, true);
	return result;
}

AmEvent* ArpOscillatorFilter::OscPulse(	AmTime start, AmTime end,
										const am_filter_params* params)
{
	if (!mSweep) return 0;
//	printf("Pulse\n");
	AmEvent*		e = 0;
	if (mCurStep >= 1) mCurStep = 0;
	float		v = mSweep->At(mCurStep);
	int32		cur = int32(v * 128);
	if (cur < 0) cur = 0;
	else if (cur > 127) cur = 127;
	if (mLast != cur) {
		e = new AmControlChange(7, uint8(cur), start);
		mLast = cur;
	}

	mCurStep += 0.1;	
	return e;
}

#if 0
	virtual AmEvent*	StartSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params = NULL);
	virtual AmEvent*	FinishSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params = NULL);

AmEvent* ArpOscillatorFilter::StartSection(	AmTime firstTime, AmTime lastTime,
											const am_filter_params* params)
{
	return 0;
}

AmEvent* ArpOscillatorFilter::FinishSection(AmTime firstTime, AmTime lastTime,
											const am_filter_params* params)
{
	if (!mSweep) return 0;
	if (!params || !params->cur_signature) return 0;
//	printf("StartSection %lld - %lld\n", firstTime, lastTime);
//	params->cur_signature->Print();

	AmEvent*		headE = 0;
	AmEvent*		curE = 0;
	int16			last = -1;
	AmTime			startT = params->cur_signature->StartTime(), endT = params->cur_signature->EndTime(),
					duration = params->cur_signature->Duration();
	
	while (!(firstTime >= startT && firstTime <= endT)) {
//		printf("\t%lld - %lld\n", startT, endT);
		startT += duration;
		endT += duration;
	}
//	printf("\t%lld - %lld (RIGHT)\n", startT, endT);
	
	for (AmTime time = firstTime; time < lastTime; time += mResolution) {
		if (time > endT) {
			startT += duration;
			endT += duration;
		}
		float		v = mSweep->At(float(time - startT) / float(endT - startT));
		int16		cur = int16(v * 127);
		if (cur < 0) cur = 0;
		else if (cur > 127) cur = 127;
		if (last != cur) {
			AmControlChange*	cc = new AmControlChange(7, uint8(cur), time);
			if (!headE) headE = cc;
			if (curE) curE->AppendEvent(cc);
			curE = cc;
			last = cur;
		}
	}
//	if (headE) headE->PrintChain();
	return headE;
}
#endif

AmEvent* ArpOscillatorFilter::HandleEvent(	AmEvent* event,
											const am_filter_params* params)
{
	if (!event) return event;
	if (event->Type() != event->CONTROLCHANGE_TYPE) return event;
	AmControlChange*		cc = (AmControlChange*)event;
	if (cc->ControlNumber() == 7) return event;
	mSweep->HandleEvent(event);
	return event;
}

/*****************************************************************************
 * ARP-OSCILLATOR-ADD-ON
 *****************************************************************************/
void ArpOscillatorAddOn::LongDescription(BString& name, BString& str) const
{
	AmFilterAddOn::LongDescription(name, str);
	str << "<P>This filter time stretches all selected events.  Tool only.&nbsp;";
}

void ArpOscillatorAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = 1;
	*minor = 0;
}

BBitmap* ArpOscillatorAddOn::Image(BPoint requestedSize) const
{
	const BBitmap* bm = gRes.Resources().FindBitmap(B_MESSAGE_TYPE, "Class Icon");
	if (bm) return new BBitmap(bm);
	return NULL;
}

AmFilterI* ArpOscillatorAddOn::NewInstance(	AmFilterHolderI* holder,
											const BMessage* config)
{
	return new ArpOscillatorFilter(this, holder, config);
}

extern "C" _EXPORT AmFilterAddOn* make_nth_filter(int32 n, image_id /*you*/,
												  const void* cookie, uint32 /*flags*/, ...)
{
	if (n == 0) return new ArpOscillatorAddOn(cookie);
	return NULL;
}
