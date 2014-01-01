/* ArpInterfaceDefs.h
 */
#ifndef ARPINTERFACE_ARPINTERFACEDEFS_H
#define ARPINTERFACE_ARPINTERFACEDEFS_H

#include <be/support/SupportDefs.h>

typedef void*	arp_image_id;

enum ArpColourConstant {
	ARP_BACKGROUND_COLOUR	= 1,
	ARP_FOREGROUND_COLOUR	= 2
};

/* The rules determine how to change out-of-bounds pixels
 */
enum ArpPixelRules {
	ARP_NO_PIXEL_RULES	= 0,
	ARP_CE				= 1,		// Circular extension
	ARP_SE				= 2,		// Symmetric extension
	ARP_TILED			= 3			// Tiled
};

#define ARP_CIRCULAR_EXTENSION(x, y, w, h) \
	if ((x) < 0) (x) = (x) + (w); \
	else if ((x) >= (w)) (x) = (x) - (w); \
	if ((y) < 0) (y) = (y) + (h); \
	else if ((y) >= (h)) (y) = (y) - (h);

#define ARP_SYMMETRIC_EXTENSION(x, y, w, h) \
	if ((x) < 0) (x) = 0 - (x); \
	else if ((x) >= (w)) (x) = 2 * (w) - (x) - 2; \
	if ((y) < 0) (y) = 0 - (y); \
	else if ((y) >= (h)) (y) = 2 * (h) - (y) - 2;

inline void arp_apply_pixel_rules(	int32* x, int32* y,
									int32 w, int32 h,
									ArpPixelRules rules)
{
	if (rules == ARP_CE) {
		ARP_CIRCULAR_EXTENSION(*x, *y, w, h);
	} else if (rules == ARP_SE) {
		ARP_SYMMETRIC_EXTENSION(*x, *y, w, h);
	} else if (rules == ARP_TILED) {
		// Optimization for color images
		if (w == 1 && h == 1) {
			*x = 0;
			*y = 0;
		} else {
			if (*x >= w) *x = *x % w;
			if (*y >= h) *y = *y % h;
		}
	}
}


#endif
