/* MPlanet.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 04.25.00		hackborn
 * Created this file.
 */

#ifndef MDUST_MPLANET_H
#define MDUST_MPLANET_H

#include "ArpMidi2/ArpMidiEvents.h"
#include "MDust/MParticle.h"
#include "MDust/MSequence.h"

/***************************************************************************
 * M-PLANET
 * This type of particle is responsible for generating a signal -- it could
 * be thought of as an oscillator in a traditional synth.
 ***************************************************************************/
class MPlanet : public MParticle
{
public:
	MPlanet(	float startX,
				float startY,
				float startZ,
				float mass,
				uchar channel = 0);
	virtual ~MPlanet();

	virtual bool IsPlanet() const		{ return true; }

	/* Answer my sequence for any stars to do their processing.
	 * The sequence is a very temporary thing, any valid for
	 * a single tick of the MIDI clock.
	 */
	MSequence* Sequence();

	virtual void Generate(ArpMidiT tick);
	virtual void Process(ArpMidiT tick, MSpaceI& space);
	virtual void Perform(MSpaceI& space);

	virtual void Draw(	BGLView* onView,
						int32 xLimit,
						int32 yLimit,
						int32 zLimit);

protected:	
	uchar			mChannel;
	MSequence*		mSequence;
	
	void FreeMemory(void);
};

#endif
