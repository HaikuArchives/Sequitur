/* ArpTriangle2d.h
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
#ifndef ARPMATH_ARPTRIANGLE2D_H
#define ARPMATH_ARPTRIANGLE2D_H

/***************************************************************************
 * ARP-TRIANGLE-2D
 ****************************************************************************/

/***************************************************************************
 * Miscellanous functions
 ****************************************************************************/

/* Answer true if the point is in the triangle's circumcircle, 0 otherwise.
 * Fill the circle center and radius values.
 */
bool arp_circumcircle(	double xp, double yp, double a_0, double a_1,
						double b_0, double b_1, double c_0, double c_1,
						double* p_0, double* p_1, double* r);

#endif
