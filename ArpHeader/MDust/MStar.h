/* MStar.h
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

#ifndef MDUST_MSTAR_H
#define MDUST_MSTAR_H

#include "ArpMidi2/ArpMidiEvents.h"
#include "MDust/MParticle.h"

/***************************************************************************
 * M-STAR
 * This type of particle is responsible for processing whatever data that
 * planets generate.
 ***************************************************************************/
class MStar : public MParticle
{
public:
	MStar(	float startX,
			float startY,
			float startZ,
				float mass);
	virtual ~MStar();

	virtual bool IsStar() const			{ return true; }

	void SetControlNumber(uint8 controlNumber);

	virtual void Generate(ArpMidiT tick);
	virtual void Process(ArpMidiT tick, MSpaceI& space);
	virtual void Perform(MSpaceI& space);

	virtual void Draw(	BGLView* onView,
						int32 xLimit,
						int32 yLimit,
						int32 zLimit);

private:
	uint8					mControlNumber;

	void FreeMemory(void);
};

#endif
