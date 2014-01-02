/* ArpMath.h
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
 * 2003.04.08			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPMATH_ARPDEFS_H
#define ARPMATH_ARPDEFS_H

#include <stdlib.h>
#include <math.h>
#include <support/SupportDefs.h>
#include <ArpCore/ArpDebug.h>

/* absolute value of a */
#define ARP_ABS(a)			(((a)<0) ? -(a) : (a))
/* answer the rounded integer of the (presumably floating) value,
 * so that 1.1 answers 1, 1.6 answers 2.
 */
#define ARP_ROUND(a)		((a) >= 0 ? (int32)((a) + 0.5) : (int32)((a) - 0.5))
/* take sign of a, either -1, 0, or 1 */
#define ARP_ZSGN(a)			(((a)<0) ? -1 : (a)>0 ? 1 : 0)	
/* take binary sign of a, either -1, or 1 if >= 0 */
#define ARP_SGN(a)			(((a)<0) ? -1 : 1)
/* square a */
#define ARP_SQR(a)			(float((a)*(a)))	

#define ARP_DOT_2D(u,v)		((u).x * (v).x + (u).y * (v).y)	// dot product, 2D, on points
#define ARP_NORM_2D(v)		sqrt(ARP_DOT_2D(v,v))			// norm = length of vector
#define ARP_D_2D(u,v)		ARP_NORM_2D(u-v)				// distance = norm of difference

#define ARP_DTOR			0.017453	/* convert degrees to radians */

#define	ARP_SLOPE(x1, y1, x2, y2)				( ((y2) - (y1)) / ((x2) - (x1)) )

#define ARP_MIN(v1, v2)		(((v1) <= (v2)) ? (v1) : (v2))
#define ARP_MAX(v1, v2)		(((v1) >= (v2)) ? (v1) : (v2))
// Constrain v to 0 to 255.
#define ARP_CLIP_255(v)		uint8(v > 255 ? 255 : v < 0 ? 0 : v)
// Constrain v to 0.0 - 1.0. That's a 'One' not an 'L'.
#define ARP_CLIP_1(v)		(v > 1.0f ? 1.0f : v < 0 ? 0 : v)

#define		arp_slope(x1, y1, x2, y2)				( ((y2) - (y1)) / ((x2) - (x1)) )
#define		arp_perpendicular_slope(x1, y1, x2, y2)	( -(((x2) - (x1)) / ((y2) - (y1))) )
#define		arp_y_intercept(x, y, m)				((y) - ((m) * (x)))

/* Answer the intersection of two lines.  If the lines are parallel,
 * answer with an error.
 */
status_t	arp_intersection(	float x0, float y0, float slope0,
								float x1, float y1, float slope1,
								float* outX, float* outY);
/* Answer a degree of 0 - 359.  The center is always 0, 0, the
 * supplied point rotates around that.  The coordinates are expected to
 * be cartesian.
 */
float		arp_degree(float x2, float y2);

#define		ARP_DISTANCE(x1, y1, x2, y2)			(sqrt( ARP_SQR((x2) - (x1)) + ARP_SQR((y2) - (y1)) ))
#define		ARP_DISTANCE_3D(x1, y1, x2, y2, z1, z2)	(sqrt( ARP_SQR((x2) - (x1)) + ARP_SQR((y2) - (y1)) + ARP_SQR((z2) - (z1)) ))

/* A new point that lies on the line formed by (x0, y0), (x1, y1) and
 * is dist out from (x0, y9).
 */
void		arp_new_pt_on_line(	float x0, float y0, float x1, float y1, float dist,
								float* newX, float* newY);
/* Same but line is epxressed in pt-slope form.
 */
void		arp_new_pt_on_line(	float x, float y, float  m, float dist,
								float* newX, float* newY);

// Constrain v to 0 to 255.
#define		arp_clip_255(v)	uint8(v > 255 ? 255 : v < 0 ? 0 : v)
// Constrain v to 0.0 - 1.0.
#define		arp_clip_1(v)	(v > 1.0f ? 1.0f : v < 0 ? 0 : v)

/* Convenient pixel macros -- images and masks store flattened data.
 * The ARP_PIXEL() macros requires the x, y, be in bounds.  ARP_PIXEL_CE
 * performs circular extension and ARP_PIXEL_SE performs symmetric extension.
 */
#define		ARP_PIXEL(x,y,w)		((y) * (w) + (x))
#define 	ARP_PIXEL_CE(x,y,w,h)	(ARP_PIXEL(	(x < 0) ? ((x) + (w)) : ((x) >= w) ? ((x) - (w)) : x, \
											(y < 0) ? ((y) + (h)) : ((y) >= h) ? ((y) - (h)) : y, w))
#define 	ARP_PIXEL_SE(x,y,w,h)	(ARP_PIXEL(	(x < 0) ? (-(x)) : (x >= w) ? (2 * (w) - (x) - 2) : x, \
											(y < 0) ? (-(y)) : (y >= h) ? (2 * (h) - (y) - 2) : y, w))
/* Go the other way.  Mostly for debugging.
 */
#define		ARP_X_FROM_PIXEL(pix, w)		((pix) % (w))
#define		ARP_Y_FROM_PIXEL(pix, w)		(((pix) - ARP_X_FROM_PIXEL(pix, w)) / w)

/************ RANDOMIZING ************/
/* Answer a random value between 0 and 1.
 */
#define	ARP_RAND		(float(rand()) / (RAND_MAX))
/* Answer a random value between -max and max.
 */
#define	ARP_RAND_OFFSET(max) \
	(float(max) - (float(rand()) / (RAND_MAX / ((max) * 2))))
/* Answer a value between min and max
 */
#define	ARP_RAND_RANGE(min, max) \
	((min) + (((max) - (min)) * ARP_RAND))
/* Answer a random number between min and max centered on orig.  At
 * a depth of 0, answer is always orig, at 1, it's anywhere between
 * min and max.
 */
float arp_rand_depth(float min, float max, float orig, float depth);
float arp_rand_depth(float min, float max, float orig, float depth, float rand);

/************ F(x) OPERATIONS ************/
#define ARP_SIN_PERIOD		(1.56999)

/* Given an x of 0 to 1, answer a value of 0 to 1
 * that's smoothed towards the edges -- a standard S curve.
 */
#define ARP_S_CURVE(x)		((x) * (x) * (3.0 - 2.0 * (x)))
/* Given an x of 0 to 1:
 * f(0) = 0, f(0.5) = 1, f(1) = 0.  All interpolation is linera.
 */
#define ARP_PEAK_CURVE(x)	((x <= 0 || x >= 1) ? 0 : (x <= 0.5) ? (x) * 2 : 1 - ((x) - 0.5) * 2)
/* Given an x of 0 to 1:
 * f(0) = 0, f(0.5) = 1, f(1) = 0.  The center is smoothed,
 * tapering off rapidly at the ends.
 */ 
// Way too centric...
//#define ARP_HUMP_CURVE(x)	((x <= 0 || x >= 1) ? 0 : sin(x * GL_SIN_PERIOD * 2))
#define ARP_HUMP_CURVE(x)	((x <= 0 || x >= 1) ? 0 : (x <= 0.5) ? ARP_S_CURVE((x) * 2) : ARP_S_CURVE(1 - ((x) - 0.5) * 2))

/************ MEASURING ************/
#define arp_dot(u,v)			((u).x * (v).x + (u).y * (v).y)	// dot product, 2D
#define arp_norm(v)				sqrt(arp_dot(v,v))				// norm = length of vector
#define arp_d(u,v)				arp_norm(u-v)					// distance = norm of difference

#endif
