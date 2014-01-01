/* MCube.h
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
 * 04.26.00		hackborn
 * Created this file.
 */

#ifndef MDUST_MCUBE_H
#define MDUST_MCUBE_H

#include "MDust/MPoint.h"

/***************************************************************************
 * M-CUBE
 * A 3-dimensional cube.
 ***************************************************************************/
class MCube
{
public:
	float	left;
	float	top;
	float	right;
	float	bottom;
	float	front;
	float	back;
	
	MCube();
	MCube(float l, float t, float r, float btm, float f, float bck);
	MCube(const MCube& cube);

	/* The cube needs to be using the same coordinate system as
	 * the point for this test to be valid.  I.e., the point x
	 * is aligned to cube left and right, point y is cube top
	 * and bottom, and point z is cube front and back.
	 */
	bool Contains(MPoint) const;

/*
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
*/
};

#endif
