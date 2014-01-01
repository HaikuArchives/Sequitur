/* MPlanet.cpp
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "MDust/MPlanet.h"
#include "MDustPublic/MSpaceI.h"

static const float	NOTE_RANGE	= 127;
static const float	VEL_RANGE	= 127;

static bool NOTE = false;

/***************************************************************************
 * M-PLANET
 ***************************************************************************/
MPlanet::MPlanet(	float startX,
					float startY,
					float startZ,
					float mass,
					uchar channel)
		: MParticle(startX, startY, startZ, mass),
		mChannel(channel), mSequence(0)
{
}

MPlanet::~MPlanet()
{
	FreeMemory();
	delete mSequence;
}

MSequence* MPlanet::Sequence()
{
	if (mSequence == 0) mSequence = new MSequence();
	return mSequence;
}

void MPlanet::Generate(ArpMidiT tick)
{
#if 0
	if (NOTE) return;
	NOTE = true;
	if (Sequence() != 0) {
		ArpMidiNoteOn*	noteOn;
		ArpMidiNoteOff*	noteOff;
		if ( (noteOff = new ArpMidiNoteOff(64, 0, 60000)) != 0 ) {
			if ( (noteOn = new ArpMidiNoteOn(64, 127, 0)) != 0 ) {
				noteOn->SetDuration(60000);
				mSequence->Add(noteOn);
			}
			mSequence->Add(noteOff);
		}
	}
#endif
#if 1
	ArpMidiT	on = 30, duration = 60;
	if ( (tick % on) == 0) {
		uint8	note = (uint8)((mPos.x + 1) * (NOTE_RANGE / 2));
		uint8	vel  = (uint8)(((mPos.y + 1) * (32)) + 64);

		if (Sequence() != 0) {
			ArpMidiNoteOn*	noteOn;
			ArpMidiNoteOff*	noteOff;
			if ( (noteOff = new ArpMidiNoteOff(note, 0, tick + duration)) != 0 ) {
				if ( (noteOn = new ArpMidiNoteOn(note, vel, tick)) != 0 ) {
					noteOn->SetDuration(duration);
					mSequence->Add(noteOn);
				}
				mSequence->Add(noteOff);
			}
		}
	}
#endif	
#if 0
	ArpMidiT	on = 100, duration = 50;
	if(mChannel == 1) {
		on = 50;
		duration = 20;
	}
	if ( (tick % on) == 0) {
		uint8	note = (uint8)((mCurrPos.x + 1) * (NOTE_RANGE / 2));
		uint8	vel  = (uint8)((mCurrPos.y + 1) * (VEL_RANGE / 2));
if (mChannel == 0) vel = 0;
		if (Sequence() != 0) {
			ArpMidiNoteOn*	noteOn;
			ArpMidiNoteOff*	noteOff;
			if ( (noteOff = new ArpMidiNoteOff(note, 0, tick + duration)) != 0 ) {
				if ( (noteOn = new ArpMidiNoteOn(note, vel, tick)) != 0 ) {
					noteOn->SetDuration(duration);
					mSequence->Add(noteOn);
				}
				mSequence->Add(noteOff);
			}
		}
	}
#endif
}

void MPlanet::Process(ArpMidiT tick, MSpaceI& space)
{
}

void MPlanet::Perform(MSpaceI& space)
{
	if (mSequence != 0) {
		space.Perform(mSequence, mChannel);
		delete mSequence;
		mSequence = 0;
	}
}

void MPlanet::Draw(	BGLView* onView,
					int32 xLimit,
					int32 yLimit,
					int32 zLimit)
{
	if (mQuadric == 0) return;
	
	glPushMatrix();
	glTranslatef(mPos.x, mPos.y, mPos.z);
	glColor4f(0.8, 0, 0.5, 1);
	gluSphere(mQuadric, mSize - 0.1, 20, 20);
	glPopMatrix();
}

void MPlanet::FreeMemory(void)
{
}
