#include <cmath>
#include <ArpKernel/ArpDebug.h>
#include <ArpMath/ArpDefs.h>
#include <ArpMath/ArpSegment3d.h>

/***************************************************************************
 * ARP-SEGMENT-3D
 ****************************************************************************/
ArpSegment3d::ArpSegment3d()
		: pt0(0, 0, 0), pt1(0, 0, 0)
{
}

ArpSegment3d::ArpSegment3d(	float x0, float y0, float z0,
							float x1, float y1, float z1)
		: pt0(x0, y0, z0), pt1(x1, y1, z1)
{
}

ArpSegment3d::ArpSegment3d(const ArpPoint3d& pt0, const ArpPoint3d& pt1)
		: pt0(pt0), pt1(pt1)
{
}

ArpSegment3d::ArpSegment3d(const ArpSegment3d& o)
		: pt0(o.pt0), pt1(o.pt1)
{
}

ArpSegment3d& ArpSegment3d::operator=(const ArpSegment3d& o)
{
	pt0 = o.pt0;
	pt1 = o.pt1;
	return *this;
}

float ArpSegment3d::Slope() const
{
	return arp_slope(pt0.x, pt0.y, pt1.x, pt1.y);
}

bool ArpSegment3d::IsPoint() const
{
	return pt0.x == pt1.x && pt0.y == pt1.y && pt0.z == pt1.z;
}

bool ArpSegment3d::IsVertical() const
{
	return pt0.x == pt1.x;
}

bool ArpSegment3d::IsHorizontal() const
{
	return pt0.y == pt1.y;
}

bool ArpSegment3d::IsParallel(const ArpSegment3d& seg) const
{
	/* Infinity slope is a special case.
	 */
	if (seg.pt0.x == seg.pt1.x) {
		if (pt0.x == pt1.x) return true;
		return false;
	}
	if (pt0.x == pt1.x) return false;
	return arp_slope(pt0.x, pt0.y, pt1.x, pt1.y)
			== arp_slope(seg.pt0.x, seg.pt0.y, seg.pt1.x, seg.pt1.y);
}

bool ArpSegment3d::InBounds(const ArpPoint3d& pt) const
{
	if (pt0.x <= pt1.x) {
		if (pt.x < pt0.x || pt.x > pt1.x) return false;
	} else {
		if (pt.x < pt1.x || pt.x > pt0.x) return false;
	}
	if (pt0.y <= pt1.y) {
		if (pt.y < pt0.y || pt.y > pt1.y) return false;
	} else {
		if (pt.y < pt1.y || pt.y > pt0.y) return false;
	}
	return true;
}

float ArpSegment3d::Distance2D(const ArpPoint3d& pt, ArpPoint3d* outClosest) const
{
	ArpPoint3d	v = pt1 - pt0;
	ArpPoint3d	w = pt - pt0;

	float		c1 = ARP_DOT_2D(w, v);
	if (c1 <= 0) {
		if (outClosest) *outClosest = pt0;
		return ARP_D_2D(pt, pt0);
	}
	
	float		c2 = ARP_DOT_2D(v, v);
	if (c2 <= c1) {
		if (outClosest) *outClosest = pt1;
		return ARP_D_2D(pt, pt1);
	}
	
	float		b = c1 / c2;
	ArpPoint3d	pb = pt0 + ArpPoint3d(b * v.x, b * v.y, 0);
	if (outClosest) *outClosest = pb;
    return ARP_D_2D(pt, pb);
}

status_t ArpSegment3d::UnboundedIntersection2D(	const ArpPoint3d& from,
												const ArpPoint3d& to,
												ArpPoint3d* outPt) const
{
	/* If I'm just a point, that's my only possible point of
	 * intersection.
	 */
	if (IsPoint()) {
		outPt->x = pt0.x;
		outPt->y = pt0.y;
		return B_OK;
	}
	/* If the supplied segment's slope is infinity...
	 */
	if (from.x == to.x) {
		if (pt0.x == pt1.x) return B_ERROR;	// parallel
		float		m = arp_slope(pt0.x, pt0.y, pt1.x, pt1.y);
		float		b = arp_y_intercept(pt0.x, pt0.y, m);
		outPt->x = from.x;
		outPt->y = (m * from.x) + b;
		return B_OK;
	}
	/* If my slope is infinity...
	 */
	if (pt0.x == pt1.x) {
		float		m = arp_slope(from.x, from.y, to.x, to.y);
		float		b = arp_y_intercept(from.x, from.y, m);
		outPt->x = pt0.x;
		outPt->y = (m * pt0.x) + b;
		return B_OK;		
	}
	/* Otherwise find the intercept
	 */
	float			slope0 = arp_slope(pt0.x, pt0.y, pt1.x, pt1.y),
					slope1 = arp_slope(from.x, from.y, to.x, to.y);	
	if (slope0 == slope1) return B_ERROR;
	float	yIntercept0 = arp_y_intercept(pt0.x, pt0.y, slope0),
			yIntercept1 = arp_y_intercept(from.x, from.y, slope1);
	outPt->x = (yIntercept0 - yIntercept1) / -(slope0 - slope1);
	outPt->y = (slope0 * (outPt->x)) + yIntercept0;
	return B_OK;
}
