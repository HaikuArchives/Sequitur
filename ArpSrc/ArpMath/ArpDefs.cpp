#include <ArpCore/ArpCoreDefs.h>
#include <ArpCore/ArpDebug.h>
#include <ArpMath/ArpDefs.h>

status_t arp_intersection(	float x0, float y0, float slope0,
							float x1, float y1, float slope1,
							float* outX, float* outY)
{
	if (slope0 == slope1) return B_ERROR;
	float	yIntercept0 = y0 - (slope0 * x0),
			yIntercept1 = y1 - (slope1 * x1);
	*outX = (yIntercept0 - yIntercept1) / -(slope0 - slope1);
	*outY = (slope0 * (*outX)) + yIntercept0;
	return B_OK;
}

float arp_degree(float x2, float y2)
{
	if (y2 == 0) {
		if (x2 > 0) return 0.0;
		else if (x2 < 0) return 180.0;
		else return 0;
	}
	if (x2 == 0) {
		if (y2 > 0) return 90.0;
		else if (y2 < 0) return 270.0;
	}

	float	x1 = 0, y1 = 0;
	float	m1 = 0;
	float	m2 = arp_slope(x1, y1, x2, y2);
	float	tangent = (m2 - m1) / (1 + (m1 * m2));
	float	radians = float(atan(tangent));
	float	degree = float(radians * 57.296);

	if (x2 < 0 && y2 > 0) degree += 180;
	else if (x2 < 0 && y2 < 0) degree += 180;
	else if (x2 > 0 && y2 < 0) degree += 360;

	ArpASSERT(degree >= 0 && degree < 361);

	return degree;
}	

#if 0
/* Distance between two points.
 */
float	arp_distance(float x0, float y0, float x1, float y1);

float arp_distance(float x0, float y0, float x1, float y1)
{
	float	xd = x1 - x0;
	float	yd = y1 - y0;
	return float(sqrt((xd * xd) + (yd * yd)));
}
#endif

void arp_new_pt_on_line(	float x0, float y0, float x1, float y1, float dist,
						float* newX, float* newY)
{
	if (x0 == x1) {
		if (y0 < y1) *newY = y0 + dist;
		else *newY = y0 - dist;
		*newX = x1;
		return;
	} else if (y0 == y1) {
		if (x0 < x1) *newX = x0 + dist;
		else *newX = x0 - dist;
		*newY = y1;
		return;
	}

	float		m = arp_slope(x0, y0, x1, y1);
	/* This is basically a reverse engineering of the distance formula
	 * 		dist = sqrt(SQR(x2 - x1) + SQR(y2 - 1))
	 * It works by taking the final result -- the requested new distance,
	 * and deconstructing it into the xPart (SQR(x2 - x1) and the yPart,
	 * the sqrt of which can be added to the x and y values.
	 *
	 * The only trick is finding the xPart, which is just solved via
	 * the equation x + (x + (slope * slope)) = newDistance.
	 */
	float		xPart = (ARP_SQR(dist) * (1 / (m * m))) / ( (1 / (m * m)) + 1);
	float		yPart = xPart * (m * m);

	if (x1 >= x0) {
		// Quad 1
		if (y1 <= y0) {
			*newX = x0 + sqrt(xPart);
			*newY = y0 - sqrt(yPart);
		// Quad 4
		} else {
			*newX = x0 + sqrt(xPart);
			*newY = y0 + sqrt(yPart);
		}
	} else {
		// Quad 2
		if (y1 <= y0) {
			*newX = x0 - sqrt(xPart);
			*newY = y0 - sqrt(yPart);
		// Quad 3
		} else {
			*newX = x0 - sqrt(xPart);
			*newY = y0 + sqrt(yPart);
		}
	}
}

void arp_new_pt_on_line(float x, float y, float  m, float dist,
						float* newX, float* newY)
{
	if (m == 0) {
		*newX = x + dist;
		*newY = y;
		return;
// FIX: MS has a different way of determining infinite than
// everyone else, need to generalize this
//	} else if (isinf(x)) {
	} else if (ARP_IS_INFINITE(x)) {
		*newX = x;
		*newY = y + dist;
		return;
	}

	/* This is basically a reverse engineering of the distance formula
	 * 		dist = sqrt(SQR(x2 - x1) + SQR(y2 - 1))
	 * It works by taking the final result -- the requested new distance,
	 * and deconstructing it into the xPart (SQR(x2 - x1) and the yPart,
	 * the sqrt of which can be added to the x and y values.
	 *
	 * The only trick is finding the xPart, which is just solved via
	 * the equation x + (x + (slope * slope)) = newDistance.
	 */
	float		xPart = (ARP_SQR(dist) * (1 / (m * m))) / ( (1 / (m * m)) + 1);
	float		yPart = xPart * (m * m);

	// Hmmm... just guessing...
	if (m < 0) {
		if (dist < 0) {
			*newX = x + sqrt(xPart);
			*newY = y - sqrt(yPart);
		} else {
			*newX = x - sqrt(xPart);
			*newY = y + sqrt(yPart);
		}
	} else {
		if (dist < 0) {
			*newX = x - sqrt(xPart);
			*newY = y - sqrt(yPart);
		} else {
			*newX = x + sqrt(xPart);
			*newY = y + sqrt(yPart);
		}
	}
}

#if 0
/* Distance from a point to a line.  UMM, THIS IS ACTUALLY JUST
 * CALCULATING IN 2D, RIGHT NOW.
 */
//float	arp_distance(const GlPoint3D& pt, const GlPoint3D& linePt0, const GlPoint3D& linePt1);

float arp_distance(	const GlPoint3D& pt, const GlPoint3D& linePt0,
					const GlPoint3D& linePt1)
{
	GlPoint3D	v = linePt1 - linePt0;
	GlPoint3D	w = pt - linePt0;

	float		c1 = dot(w, v);
	if (c1 <= 0) return float(d(pt, linePt0));

	float		c2 = dot(v, v);
	if (c2 <= c1) return float(d(pt, linePt1));

	float		b = c1 / c2;
	GlPoint3D	pb = linePt0 + scalar_x_pt(b, v);
    return float(d(pt, pb));
}
#endif

float arp_rand_depth(float min, float max, float orig, float depth)
{
	return arp_rand_depth(min, max, orig, depth, ARP_RAND);
}

float arp_rand_depth(float min, float max, float orig, float depth, float rand)
{
	if (max < min) {
		float	t = min;
		min = max;
		max = t;
	}
	float		range = ARP_ABS(max - min) * depth;
	float		low = orig - (range / 2),
				high = orig + (range / 2);
	if (low < min) {
		low = min;
		high = low + range;
	} else if (high > max) {
		high = max;
		low = high - range;
	}
	ArpASSERT(low >= min && high <= max);
	if (low < min) low = min;
	if (high > max) high = max;

	float		ans = low + ((high - low) * rand);
	if (ans < min) ans = min;
	else if (ans > max) ans = max;
	return ans;	
}
