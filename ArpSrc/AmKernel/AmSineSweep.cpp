#include <cmath>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmSineSweep.h"

static const int16		SIN_KEY	= 'si';

#if 0
/***************************************************************************
 * GL-SIN-CRV-ADD-ON
 ***************************************************************************/
GlSin1dAddOn::GlSin1dAddOn()
		: inherited("arp", "Sin1d", "1D", "Sin", 1, 0,
					GlNodeIo::NewIo(GL_CURVE_IO, 1),
					GlNodeIo::NewIo(GL_CURVE_IO, 1)),
		  mCycleType(0), mPhaseType(0)
{
	ArpASSERT(GlCurve::RegisterKey(SIN_KEY));
	mCycleType		= AddParamType(new GlFloatParamType('c_', "Cycle", 0.0, 100.0, 2.0, 0.1));
	mPhaseType		= AddParamType(new GlFloatParamType('p_', "Phase", 0.0, 1.0, 0.0, 0.1));
mOtherNames.push_back("Sin1d01");
}

GlNode* GlSin1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlSin1d(this, config);
}

// #pragma mark -
#endif

/***************************************************************************
 * AM-SINE-SWEEP
 ***************************************************************************/
AmSineSweep::AmSineSweep(float c, float p)
		: inherited(SIN_KEY), mCycle(M_PI * 2 * c), mPhase(p)
{
}

AmSineSweep::AmSineSweep(const AmSineSweep& o)
		: inherited(o), mCycle(o.mCycle), mPhase(o.mPhase)
{
}

AmSweep* AmSineSweep::Clone() const
{
	return new AmSineSweep(*this);
}

void AmSineSweep::HandleEvent(AmEvent* event)
{
	if (event->Type() != event->CONTROLCHANGE_TYPE) return;
	AmControlChange*		cc = (AmControlChange*)event;
//	if (cc->ControlNumber() != 48) return;
	mCycle = M_PI * 2 * (1 + ((cc->ControlValue() / 127.0) * 4));
//printf("CC %d %d made period %f\n", cc->ControlNumber(), cc->ControlValue(), mCycle);
}

float AmSineSweep::Process(float v) const
{
	ArpASSERT(v >= 0 && v <= 1);
	float		s = sin( (v * mCycle) + (mPhase * M_PI * 2) );
	if (s < -1) s = -1;
	if (s > 1) s = 1;
	return (s + 1) * 0.5;
}
