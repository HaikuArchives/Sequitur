/* MRhythmDust.cpp
 */
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "MDust/MPlanet.h"
#include "MDust/MRhythmDust.h"
#include "MDustPublic/MSpaceI.h"

static uint32		TICK_COUNTER	= 0;
static const uint32	INTERVAL		= 15;

/***************************************************************************
 * M-RHYTHM-DUST
 ***************************************************************************/
MRhythmDust::MRhythmDust(	float startX,
							float startY,
							float startZ,
							float mass)
		: MStar(startX, startY, startZ, mass),
		mCounter(0), mIndex(0)
{
}

MRhythmDust::~MRhythmDust()
{
}

void MRhythmDust::Process(ArpMidiT tick, MSpaceI& space)
{
#if 0
	TICK_COUNTER++;
	if (TICK_COUNTER < INTERVAL) return;
	TICK_COUNTER = 0;

	uint8		value = 127;
	if (mIndex == 1 ) value = 117;
	else if (mIndex == 2 ) value = 107;
	else if (mIndex == 3 ) value = 97;
	else if (mIndex == 4 ) value = 87;
	else if (mIndex == 5 ) value = 77;
	else if (mIndex == 6 ) value = 67;
	else if (mIndex == 7 ) value = 57;
	else if (mIndex == 8 ) value = 47;
	else if (mIndex == 9 ) value = 37;
	else if (mIndex == 10) value = 27;
	else if (mIndex == 11) value = 17;
	else if (mIndex == 12) value = 17;
	mIndex++;
	if (mIndex > 12) mIndex = 0;
	
	MParticleI*				particle;
	for (uint32 k=0; (particle = space.ParticleAt(k)) != 0; k++) {
		if ( particle->IsPlanet() ) {
			MPlanet*		planet = (MPlanet*)particle;
			MSequence*		sequence = planet->Sequence();
			if (sequence != 0) {
				ArpMidiControlChange*	cc = new ArpMidiControlChange(7, value, tick);
				if (cc != 0) sequence->Add(cc);
			}
		}
	}
#endif
}
