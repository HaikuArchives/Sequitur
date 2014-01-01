#include <ArpCore/ArpDebug.h>
#include <ArpMath/ArpPoint2d.h>

/***************************************************************************
 * ARP-POINT-2D
 ****************************************************************************/
ArpPoint2d::ArpPoint2d()
		: x(0), y(0)
{
}

ArpPoint2d::ArpPoint2d(float inX, float inY)
		: x(inX), y(inY)
{
}

ArpPoint2d::ArpPoint2d(const ArpPoint2d& o)
		: x(o.x), y(o.y)
{
}

bool ArpPoint2d::operator==(const ArpPoint2d& o) const
{
	return x == o.x && y == o.y;
}

ArpPoint2d& ArpPoint2d::operator=(const ArpPoint2d& o)
{
	x = o.x;
	y = o.y;
	return *this;
}

ArpPoint2d ArpPoint2d::operator+(const ArpPoint2d& pt) const
{
	return ArpPoint2d(x + pt.x, y + pt.y);
}

ArpPoint2d ArpPoint2d::operator-(const ArpPoint2d& pt) const
{
	return ArpPoint2d(x - pt.x, y - pt.y);
}
