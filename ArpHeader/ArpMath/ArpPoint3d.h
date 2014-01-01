/* ArpPoint3d.h
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
 * 2003.04.03			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPMATH_ARPPOINT3D_H
#define ARPMATH_ARPPOINT3D_H

#include <math.h>
#include <be/app/Message.h>

/***************************************************************************
 * ARP-POINT-3D
 ****************************************************************************/
class ArpPoint3d
{
public:
	float		x, y, z;

	ArpPoint3d();
	ArpPoint3d(float x, float y, float z);
	ArpPoint3d(const ArpPoint3d& o);

	bool				operator==(const ArpPoint3d& o) const;

	ArpPoint3d&			operator=(const ArpPoint3d& o);
	ArpPoint3d			operator+(const ArpPoint3d& pt) const;
	ArpPoint3d			operator-(const ArpPoint3d& pt) const;

	status_t			ReadFrom(const BMessage& msg, const char* name);
	status_t			WriteTo(BMessage& msg, const char* name) const;

	inline void			Set(float inX, float inY, float inZ)
	{
		x = inX;
		y = inY;
		z = inZ;
	}

	inline ArpPoint3d	CrossProduct(const ArpPoint3d& b)
	{
		return ArpPoint3d(	(y * b.z) - (b.y * z),
							(z * b.x) - (b.z * x),
							(x * b.y) - (b.x * y) );
	}

	inline float		Length() const
	{
		return sqrt(x * x + y * y + z * z);
	}

	inline float		YIntercept2D(float m) const
	{
		return y - (m * x);
	}
	/* Answer the midpoint between myself and the supplied point.
	 */
	inline ArpPoint3d	Midpoint(const ArpPoint3d& pt) const
	{
		return ArpPoint3d((x + pt.x) / 2, (y + pt.y) / 2, (z + pt.z) / 2);
	}
};


#endif
