/* AmSineSweep.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.02.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMKERNEL_AMSINESWEEP_H
#define AMKERNEL_AMSINESWEEP_H

#include "AmPublic/AmSweep.h"

/***************************************************************************
 * AM-SINE-SWEEP
 ***************************************************************************/
class AmSineSweep : public AmSweep
{
public:
	AmSineSweep(float c, float p);
	AmSineSweep(const AmSineSweep& o);

	virtual AmSweep*	Clone() const;

	virtual void		HandleEvent(AmEvent* event);

protected:
	virtual float		Process(float v) const;

private:
	typedef AmSweep		inherited;
	float				mCycle, mPhase;
};



#endif
