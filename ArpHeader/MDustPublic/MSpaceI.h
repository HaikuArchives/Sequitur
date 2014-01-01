/* MSpaceI.h
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
 * 04.14.00		hackborn
 * Created this file.
 */

#ifndef MDUSTPUBLIC_MSPACEI_H
#define MDUSTPUBLIC_MSPACEI_H

#include "ArpMidiPublic/ArpMidiEventsI.h"
#include "MDustPublic/MParticleI.h"
#include "MDust/MSequence.h"

/***************************************************************************
 * M-SPACE-I
 * This defines one space, which is a collection of particles.
 ***************************************************************************/
class MSpaceI
{
public:
	virtual ~MSpaceI()		{ }

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
	virtual MParticleI* ParticleAt(uint32 index) = 0;
	
	virtual void Increment(bool print) = 0;
	virtual void Perform(const ArpMidiEventI* event, uchar channel) = 0;
	/* Merge all data in sequence into myself.  When this is
	 * finished, I am the owner of all the events in the sequence,
	 * but not the actual sequence, which needs to be deleted
	 * by the caller.
	 */
	virtual void Perform(MSequence* sequence, uchar channel) = 0;
	/* Draw my particles on the supplied view.
	 */
	virtual void Draw(BGLView* onView) = 0;
};

#endif
