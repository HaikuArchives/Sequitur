/* MRhythmStar.cpp
 */
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "MDust/MPlanet.h"
#include "MDust/MRhythmStar.h"
#include "MDustPublic/MSpaceI.h"

/***************************************************************************
 * M-RHYTHM-STAR
 ***************************************************************************/
MRhythmStar::MRhythmStar(	float startX,
							float startY,
							float startZ,
							float mass)
		: MStar(startX, startY, startZ, mass)
{
}

MRhythmStar::~MRhythmStar()
{
}

void MRhythmStar::Process(ArpMidiT tick, MSpaceI& space)
{
	MParticleI*				particle;
	for (uint32 k=0; (particle = space.ParticleAt(k)) != 0; k++) {
		if ( particle->IsPlanet() ) {
			MPlanet*		planet = (MPlanet*)particle;
			MSequence*		sequence = planet->Sequence();
			if ( (sequence != 0)
					&& (sequence->IsEmpty() == false) ) {
				MSequence	duplicates;
				float		dist = DistanceTo(planet);
				CreateDuplicates(sequence, &duplicates, dist);
				sequence->Merge(&duplicates);
			}
		}
	}
}

void MRhythmStar::CreateDuplicates(	MSequence* originals,
									MSequence* duplicates,
									float distance)
{
	ArpMidiT		dupInterval = 25;
	uint32			num = 0;
	if ( (distance > 0.75) || (distance < -0.75) ) num = 0;
	else if ( (distance > 0.50) || (distance < -0.50) ) num = 1;
	else if ( (distance > 0.25) || (distance < -0.25) ) num = 2;
	else num = 3;
	if (num == 0) return;
		
	ArpMidiNode*	node = (ArpMidiNode*)originals->HeadNode();
	while (node != 0) {
		if (node->Event()->Type() == node->Event()->NOTEON_EVENT) {
			ArpMidiNoteOn*	event = (ArpMidiNoteOn*)node->Event();
			for (uint32 k=0; k<num; k++) {
				ArpMidiNoteOn*	on;
				ArpMidiNoteOff*	off;
				ArpMidiT		time = event->Time() + ( (num + 1) * dupInterval);
				if ( (off = new ArpMidiNoteOff(event->Note(), 0, time + event->Duration() )) != 0 ) {
					if ( (on = new ArpMidiNoteOn(event->Note(), event->Velocity(), time)) != 0 ) {
						on->SetDuration( event->Duration() );
						duplicates->Add(on);
					}
					duplicates->Add(off);
				}

			}
		}
		node = (ArpMidiNode*)node->next;
	}
}