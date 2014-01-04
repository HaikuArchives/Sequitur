#include <ArpKernel/ArpDebug.h>
#include <ArpMath/ArpDefs.h>
#include <ArpMath/ArpSegment2d.h>

/***************************************************************************
 * ARP-SEGMENT-2D
 ****************************************************************************/
ArpSegment2d::ArpSegment2d()
{
}

ArpSegment2d::ArpSegment2d(	float x0, float y0,
							float x1, float y1)
		: pt0(x0, y0), pt1(x1, y1)
{
}

ArpSegment2d::ArpSegment2d(const ArpPoint2d& inPt0, const ArpPoint2d& inPt1)
		: pt0(inPt0), pt1(inPt1)
{
}

ArpSegment2d::ArpSegment2d(const ArpSegment2d& o)
		: pt0(o.pt0), pt1(o.pt1)
{
}

ArpSegment2d& ArpSegment2d::operator=(const ArpSegment2d& o)
{
	pt0 = o.pt0;
	pt1 = o.pt1;
	return *this;
}

float ArpSegment2d::Slope() const
{
	return arp_slope(pt0.x, pt0.y, pt1.x, pt1.y);
}

bool ArpSegment2d::InBounds(const ArpPoint2d& pt) const
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

void ArpSegment2d::Resize(float newDist)
{
	if (pt0.x == pt1.x) {
		if (pt0.y < pt1.y) pt1.y = pt0.y + newDist;
		else pt1.y = pt0.y - newDist;
		return;
	} else if (pt0.y == pt1.y) {
		if (pt0.x < pt1.x) pt1.x = pt0.x + newDist;
		else pt1.x = pt0.x - newDist;
		return;
	}

	float		m = ARP_SLOPE(pt0.x, pt0.y, pt1.x, pt1.y);
	/* This is basically a reverse engineering of the distance formula
	 * 		dist = sqrt(SQR(x2 - x1) + SQR(y2 - 1))
	 * It works by taking the final result -- the requested new distance,
	 * and deconstructing it into the xPart (SQR(x2 - x1) and the yPart,
	 * the sqrt of which can be added to the x and y values.
	 *
	 * The only trick is finding the xPart, which is just solved via
	 * the equation x + (x + (slope * slope)) = newDistance.
	 */
	float		xPart = (ARP_SQR(newDist) * (1 / (m * m))) / ( (1 / (m * m)) + 1);
	float		yPart = xPart * (m * m);

	if (pt1.x >= pt0.x) {
		// Quad 1
		if (pt1.y <= pt0.y) {
			pt1.x = pt0.x + sqrt(xPart);
			pt1.y = pt0.y - sqrt(yPart);
		// Quad 4
		} else {
			pt1.x = pt0.x + sqrt(xPart);
			pt1.y = pt0.y + sqrt(yPart);
		}
	} else {
		// Quad 2
		if (pt1.y <= pt0.y) {
			pt1.x = pt0.x - sqrt(xPart);
			pt1.y = pt0.y - sqrt(yPart);
		// Quad 3
		} else {
			pt1.x = pt0.x - sqrt(xPart);
			pt1.y = pt0.y + sqrt(yPart);
		}
	}
}

bool ArpSegment2d::IsPoint() const
{
	return pt0.x == pt1.x && pt0.y == pt1.y;
}

bool ArpSegment2d::IsVertical() const
{
	return pt0.x == pt1.x;
}

bool ArpSegment2d::IsHorizontal() const
{
	return pt0.y == pt1.y;
}

bool ArpSegment2d::IsParallel(const ArpSegment2d& seg) const
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
