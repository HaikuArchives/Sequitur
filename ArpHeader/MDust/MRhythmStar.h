/* MRhythmStar.h
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
 * 04.28.00		hackborn
 * Created this file.
 */

#ifndef MDUST_MRHYTHMSTAR_H
#define MDUST_MRHYTHMSTAR_H

#include "MDust/MStar.h"

/***************************************************************************
 * M-RHYTHM-STAR
 * This star alters whatever note on events it finds in planets:  The closer
 * the star, the more it duplicates the events.
 ***************************************************************************/
class MRhythmStar : public MStar
{
public:
	MRhythmStar(float startX,
				float startY,
				float startZ,
				float mass);
	virtual ~MRhythmStar();

	virtual void Process(ArpMidiT tick, MSpaceI& space);

private:
	void CreateDuplicates(	MSequence* originals,
							MSequence* duplicates,
							float distance);
};

#endif
