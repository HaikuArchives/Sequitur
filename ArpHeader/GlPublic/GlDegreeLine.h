/* GlDegreeLine.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.09.24				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLDEGREELINE_H
#define GLPUBLIC_GLDEGREELINE_H

#include <support/SupportDefs.h>
class GlLineCache;

/*******************************************************
 * GL-DEGREE-LINE
 * This simple object takes a degree (0 to 360) and
 * translates into instructions for drawing a line.
 *******************************************************/
class GlDegreeLine
{
public:
	GlDegreeLine(	float angle, float x = 0.0, float y = 0.0,
					int8 xPos = 0, int8 yPos = 0);
	GlDegreeLine(	const GlDegreeLine& o);

	GlDegreeLine&	operator=(const GlDegreeLine& o);

	/* Set all, including the x and y sign.  The line is assuming
	 * to be radiating out from a center point, so degree 0 - 90
	 * have positive x and y, 91 - 180 have negative x, pos y, etc.
	 */
	void			SetAngle(float angle);
	void			SetOrigin(float x, float y);
	/* If value is < 0, sign is negative.  If > 0, positive.
	 * If 0, no change.
	 */
	void			SetSigns(int8 xPos, int8 yPos);
	void			Set(float angle, float x, float y, int8 xPos, int8 yPos);

	void			GetNext(int32* outX, int32* outY);
	void			PeekNext(int32* outX, int32* outY);
	void			GetEnd(float radius, float* outX, float* outY);
	/* Write myself out to the size of the cache.
	 */
	status_t		CacheLine(GlLineCache& cache);

	/* Get the info that can be used to generate the line.
	 */
	void			GetSlope(float* slope, int8* signX, int8* signY) const;
	
private:
	float			mSlope;
	float			mX, mY;
	float			mCurX, mCurY;
	float			mCounter;
	int8			mSignX, mSignY;		// either 1 or -1
	
	void			SlopeFromDegree(float degrees);

public:
// Obsolete
	GlDegreeLine(	float degree, bool xPositive,
					bool yPositive);
// Obsolete, use SetSigns()
	void			SetXPositive(bool pos);
	void			SetYPositive(bool pos);
// OBSOLETE, use SetAngle()
	void			SetAll(float angle);
};

#endif
