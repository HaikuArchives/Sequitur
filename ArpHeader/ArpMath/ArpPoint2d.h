/* ArpPoint2d.h
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
 * 2003.04.30			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPMATH_ARPPOINT2D_H
#define ARPMATH_ARPPOINT2D_H

/***************************************************************************
 * ARP-POINT-2D
 ****************************************************************************/
class ArpPoint2d
{
public:
	float		x, y;

	ArpPoint2d();
	ArpPoint2d(float x, float y);
	ArpPoint2d(const ArpPoint2d& o);

	bool				operator==(const ArpPoint2d& o) const;

	ArpPoint2d&			operator=(const ArpPoint2d& o);
	ArpPoint2d			operator+(const ArpPoint2d& pt) const;
	ArpPoint2d			operator-(const ArpPoint2d& pt) const;

	inline void			Set(float inX, float inY)
	{
		x = inX;
		y = inY;
	}

	inline float		YIntercept(float m) const
	{
		return y - (m * x);
	}
	/* Answer the midpoint between myself and the supplied point.
	 */
	inline ArpPoint2d	Midpoint(const ArpPoint2d& pt) const
	{
		return ArpPoint2d((x + pt.x) / 2, (y + pt.y) / 2);
	}
};


#endif
