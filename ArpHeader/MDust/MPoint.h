/* MPoint.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 04.15.00		hackborn
 * Created this file.
 */

#ifndef MDUST_MPOINT_H
#define MDUST_MPOINT_H

#include <be/interface/Point.h>

/***************************************************************************
 * M-POINT
 * A 3-dimensional point.
 ***************************************************************************/
class MPoint : public BPoint
{
public:
	float	z;
	
	MPoint();
	MPoint(float X, float Y, float Z);
	MPoint(const MPoint& pt);

	MPoint		&operator=(const MPoint &from);
	void		Set(float X, float Y, float Z);

	void		PrintToStream() const;
			
	MPoint		operator+(const MPoint&) const;
	MPoint		operator-(const MPoint&) const;
	MPoint		operator*(const MPoint&) const;

	MPoint		operator*(const float) const;

	MPoint&		operator+=(const MPoint&);
	MPoint&		operator-=(const MPoint&);

	bool		operator!=(const MPoint&) const;
	bool		operator==(const MPoint&) const;
};

#endif
