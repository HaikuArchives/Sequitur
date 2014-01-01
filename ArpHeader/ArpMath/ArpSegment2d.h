/* ArpSegment2d.h
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
#ifndef ARPMATH_ARPSEGMENT2D_H
#define ARPMATH_ARPSEGMENT2D_H

#include <ArpMath/ArpPoint2d.h>

/***************************************************************************
 * ARP-SEGMENT-2D
 * A line segment.
 ****************************************************************************/
class ArpSegment2d
{
public:
	ArpPoint2d		pt0, pt1;
	
	ArpSegment2d();
	ArpSegment2d(float x0, float y0, float x1, float y1);
	ArpSegment2d(const ArpPoint2d& inPt0, const ArpPoint2d& inPt1);
	ArpSegment2d(const ArpSegment2d& o);

	ArpSegment2d&	operator=(const ArpSegment2d& o);

	float			Slope() const;
	/* Answer true of the point is within my bounds --
	 * that is, inside my bounding rect.  This is obviously not a test
	 * to see if the point lies on the segment, but it can be used to see
	 * if the point DOESN'T lie on the segment, which is faster and in
	 * certain situations completely adequate.
	 */
	bool			InBounds(const ArpPoint2d& pt) const;

	/* Taking pt0 as the origin and my slope, set pt1 to a new
	 * value that is newDist away from pt0.
	 */
	void			Resize(float newDist);

	/* Not sure where these were being used but they aren't
	 * very useful -- they're doing equality checks on floats.
	 */
	bool			IsPoint() const;
	bool			IsVertical() const;
	bool			IsHorizontal() const;
	bool			IsParallel(const ArpSegment2d& seg) const;
};

#endif
