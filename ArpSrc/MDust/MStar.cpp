/* MStar.cpp
 */
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "MDust/MPlanet.h"
#include "MDust/MStar.h"
#include "MDustPublic/MSpaceI.h"

#define CC_STEPS	(127)

/***************************************************************************
 * M-STAR
 ***************************************************************************/
MStar::MStar(	float startX,
				float startY,
				float startZ,
				float mass)
		: MParticle(startX, startY, startZ, mass),
		mControlNumber(0)
{
}

MStar::~MStar()
{
	FreeMemory();
}

void MStar::SetControlNumber(uint8 controlNumber)
{
	mControlNumber = controlNumber;
}

void MStar::Generate(ArpMidiT tick)
{
}

void MStar::Process(ArpMidiT tick, MSpaceI& space)
{
	if (mControlNumber == 0) return;
	if ( (tick % 2) == 0) {
		MParticleI*		particle;

		for (uint32 k=0; (particle = space.ParticleAt(k)) != 0; k++) {
			if ( particle->IsPlanet() ) {
				MPlanet*	planet = (MPlanet*)particle;
				float	dist = DistanceTo(planet);
				uint8	value = (uint8)(CC_STEPS - (dist * (CC_STEPS / 2)));
				ArpMidiControlChange*	cc = new ArpMidiControlChange(mControlNumber, value, tick);
				if ( (cc != 0) && (planet->Sequence() != 0) )
					planet->Sequence()->Add(cc);
			}
		}
	}
}

void MStar::Perform(MSpaceI& space)
{
}

void MStar::Draw(	BGLView* onView,
					int32 xLimit,
					int32 yLimit,
					int32 zLimit)
{
	if (mQuadric == 0) return;
	
	glPushMatrix();
	glTranslatef(mPos.x, mPos.y, mPos.z);
	glColor4f(1, 1, 1, 1);
	gluSphere(mQuadric, mSize - 0.1, 20, 20);
	glColor4f(1, 0, 1, 0.5);
	gluSphere(mQuadric, mSize, 20, 20);
	glPopMatrix();
}

void MStar::FreeMemory(void)
{
}