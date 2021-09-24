/* MCube.cpp
 */
#include <cstdio>
#include <cassert>
#include <cstring>
#include "MDust/MCube.h"

/***************************************************************************
 * M-CUBE
 ***************************************************************************/
MCube::MCube()
		: left(0), top(0), right(0), bottom(0), front(0), back(0)

{
}

MCube::MCube(float l, float t, float r, float btm, float f, float bck)
		: left(l), top(t), right(r), bottom(btm), front(f), back(bck)
{
}

MCube::MCube(const MCube& cube)
{
	left = cube.left;
	top = cube.top;
	right = cube.right;
	bottom = cube.bottom;
	front = cube.front;
	back = cube.back;
}

bool MCube::Contains(MPoint pt) const
{
	if ( (pt.x < left) || (pt.x > right) ) return false;
	if ( (pt.y < top) || (pt.y > bottom) ) return false;
	if ( (pt.z < back) || (pt.z > front) ) return false;
	return true;
}

#if 0
MPoint& MPoint::operator=(const MPoint &from)
{
	x = from.x;
	y = from.y;
	z = from.z;
	return *this;
}

void MPoint::Set(float X, float Y, float Z)
{
	x = X;
	y = Y;
	z = Z;
}

void MPoint::PrintToStream() const
{
	printf("MPoint x %f y %f z %f\n", x, y, z); fflush(stdout);
}
			
MPoint MPoint::operator+(const MPoint& rhv) const
{
	return MPoint(x + rhv.x, y + rhv.y, z + rhv.z);
}

MPoint MPoint::operator-(const MPoint& rhv) const
{
	return MPoint(x - rhv.x, y - rhv.y, z - rhv.z);
}

MPoint MPoint::operator*(const MPoint& rhv) const
{
	return MPoint(x * rhv.x, y * rhv.y, z * rhv.z);
}

MPoint MPoint::operator*(const float rhv) const
{
	return MPoint(x * rhv, y * rhv, z * rhv);
}

MPoint& MPoint::operator+=(const MPoint& rhv)
{
	x += rhv.x;
	y += rhv.y;
	z += rhv.z;
	return *this;
}

MPoint&	MPoint::operator-=(const MPoint& rhv)
{
	x -= rhv.x;
	y -= rhv.y;
	z -= rhv.z;
	return *this;
}

bool MPoint::operator!=(const MPoint& rhv) const
{
	return (x != rhv.x)
			|| (y != rhv.y)
			|| (z != rhv.z);
}

bool MPoint::operator==(const MPoint& rhv) const
{
	return (x == rhv.x)
			&& (y == rhv.y)
			&& (z == rhv.z);
}
#endif