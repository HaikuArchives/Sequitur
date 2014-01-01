#ifndef ARPMATH_ARPSEGMENT3D_H
#define ARPMATH_ARPSEGMENT3D_H


#include <ArpMath/ArpPoint3d.h>

/***************************************************************************
 * ARP-SEGMENT-3D
 * A floating line segment.
 ****************************************************************************/
class ArpSegment3d
{
public:
	ArpPoint3d		pt0, pt1;

	ArpSegment3d();
	ArpSegment3d(float x0, float y0, float z0, float x1, float y1, float z1);
	ArpSegment3d(const ArpPoint3d& pt0, const ArpPoint3d& pt1);
	ArpSegment3d(const ArpSegment3d& o);

	ArpSegment3d&	operator=(const ArpSegment3d& o);

	float			Slope() const;

	bool			IsPoint() const;
	bool			IsVertical() const;
	bool			IsHorizontal() const;
	bool			IsParallel(const ArpSegment3d& seg) const;
	/* Answer true of the point is within my bounds --
	 * that is, inside my bounding rect.  This is obviously not a test
	 * to see if the point lies on the segment, but it can be used to see
	 * if the point DOESN'T lie on the segment, which is faster and in
	 * certain situations completely adequate.
	 */
	bool			InBounds(const ArpPoint3d& pt) const;

	float			Distance2D(const ArpPoint3d& pt, ArpPoint3d* outClosest = 0) const;
	/* Get the point of intersection between the line described by from-to
	 * and this segment.  The intersection may not actually be on the segment --
	 * it is just where they would intersect if both segments were actual lines.
	 */
	status_t		UnboundedIntersection2D(const ArpPoint3d& from, const ArpPoint3d& to,
											ArpPoint3d* outPt) const;
};


#endif
