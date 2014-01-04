#include <ArpKernel/ArpDebug.h>
#include <ArpMath/ArpDefs.h>
#include <ArpMath/ArpTriangle2d.h>

/***************************************************************************
 * Miscellanous functions
 ****************************************************************************/
bool arp_circumcircle(	double xp, double yp, double a_0, double a_1,
						double b_0, double b_1, double c_0, double c_1,
						double* p_0, double* p_1, double* r)
{
	double		d = (a_0 - c_0) * (b_1 - c_1) - (b_0 - c_0) * (a_1 - c_1);
	*p_0 = (((a_0 - c_0) * (a_0 + c_0) + (a_1 - c_1) * (a_1 + c_1)) / 2 * (b_1 - c_1)
		  - ((b_0 - c_0) * (b_0 + c_0) + (b_1 - c_1) * (b_1 + c_1)) / 2 * (a_1 - c_1))
			/ d;
	*p_1 = (((b_0 - c_0) * (b_0 + c_0) + (b_1 - c_1) * (b_1 + c_1)) / 2 * (a_0 - c_0)
		  - ((a_0 - c_0) * (a_0 + c_0) + (a_1 - c_1) * (a_1 + c_1)) / 2 * (b_0 - c_0))
		    / d;
	*r = ARP_DISTANCE(*p_0, *p_1, c_0, c_1);
	if (ARP_DISTANCE(xp, yp, *p_0, *p_1) <= *r) return true;
	else return false;
}
