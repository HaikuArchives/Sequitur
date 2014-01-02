/* AmMotionI.h
 * Copyright (c)2001 by Angry Red Planet and Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.05.10			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef AMPUBLIC_AMMOTIONI_H
#define AMPUBLIC_AMMOTIONI_H

#include <support/String.h>
#include "AmPublic/AmDefs.h"

/*************************************************************************
 * AM-MOTION-I
 * This is the interface for a simple object that describes a motion.
 * A motion is just two points, which can be used to describe one hit
 * of a rhythm, one note in a progression, two stages of an envelope, etc.
 *************************************************************************/
class AmMotionI
{
public:
	virtual ~AmMotionI();

	/* Generally, the config message is one that was created by
	 * calling the rhythm's WriteTo() message.
	 */
	static AmMotionI*		NewMotion(const BMessage& config);

	virtual BString			Label() const = 0;
	virtual uint32			CountHits() const = 0;
	virtual uint32			CountMeasures() const = 0;
	virtual status_t		GetHit(	uint32 number,
									BPoint* pt, float* end = NULL) const = 0;
	/* Answer the y at the given x.  x must be >= 0, and <= to my
	 * last hit.x value.  If I am in envelope mode, answer an
	 * interpolated Y based on the envelope.
	 */
	virtual status_t		GetHitY(float x, float* outY) const = 0;
	virtual bool			IsEmpty() const = 0;
	virtual AmMotionMode	EditingMode() const = 0;
	virtual AmMotionI*		Copy() const = 0;
	virtual status_t		WriteTo(BMessage& config) const = 0;
};

#endif 

