/* ArpTriangle3d.h
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
#ifndef ARPMATH_ARPTRIANGLE3D_H
#define ARPMATH_ARPTRIANGLE3D_H

#include <ArpMath/ArpPoint3d.h>

/***************************************************************************
 * ARP-TRIANGLE-3D
 ****************************************************************************/
class ArpTriangle3d
{
public:
	ArpPoint3d		v0, v1, v2;
	
	ArpTriangle3d();
	ArpTriangle3d(	float x0, float y0, float z0,
					float x1, float y1, float z1,
					float x2, float y2, float z2);
	ArpTriangle3d(	const ArpPoint3d& in0, const ArpPoint3d& in1,
					const ArpPoint3d& in2);

	void			MakeClockwise();
	void			MakeCounterClockwise();

	inline void		Set(float x0, float y0, float z0,
						float x1, float y1, float z1,
						float x2, float y2, float z2)
	{
		v0.x = x0;	v0.y = y0; v0.z = z0;
		v1.x = x1;	v1.y = y1; v1.z = z1;
		v2.x = x2;	v2.y = y2; v2.z = z2;
	}

	void			Print(uint32 tabs = 0) const;
};


#endif
