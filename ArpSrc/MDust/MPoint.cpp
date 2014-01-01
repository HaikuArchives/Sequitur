/* MSpace.cpp
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "MDust/MPoint.h"

/***************************************************************************
 * M-POINT
 ***************************************************************************/
MPoint::MPoint()
		: z(0)
{
	x = y = 0;
}

MPoint::MPoint(float X, float Y, float Z)
		: BPoint(X, Y),
		z(Z)
{
}

MPoint::MPoint(const MPoint& pt)
		: z(pt.z)
{
	x = pt.x;
	y = pt.y;
}

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
