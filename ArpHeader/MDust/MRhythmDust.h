/* MRhythmDust.h
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

#ifndef MDUST_MRHYTHMDUST_H
#define MDUST_MRHYTHMDUST_H

#include "MDust/MStar.h"

/***************************************************************************
 * M-RHYTHM-DUST
 * This dust carves out a rhtyhm with volume control changes.
 ***************************************************************************/
class MRhythmDust : public MStar
{
public:
	MRhythmDust(float startX,
				float startY,
				float startZ,
				float mass);
	virtual ~MRhythmDust();

	virtual void Process(ArpMidiT tick, MSpaceI& space);

private:
	ArpMidiT	mCounter;
	int32		mIndex;
};

#endif
