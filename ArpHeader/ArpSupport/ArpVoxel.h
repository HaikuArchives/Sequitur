/* ArpVoxel.h
 * Copyright (c)2002 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.09.10			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef ARPSUPPORT_ARPVOXEL_H
#define ARPSUPPORT_ARPVOXEL_H

#include <be/support/SupportDefs.h>
#include <ArpMath/ArpDefs.h>
class BMessage;

/***************************************************************************
 * ARP-VOXEL
 * An element with r, g, b, a and depth components.
 ****************************************************************************/
class ArpVoxel
{
public:
	uint8			r, g, b, a, z;

	ArpVoxel();
	ArpVoxel(const ArpVoxel& o);
	ArpVoxel(uint8 r, uint8 g, uint8 b, uint8 a = 255, uint8 z = 0);

	ArpVoxel&		operator=(const ArpVoxel& o);

	status_t		ReadFrom(const BMessage& msg);
	status_t		WriteTo(BMessage& msg) const;

	/* Utilities
	 */
	/* Mix in the supplied color - a mix of 0 makes no change, a mix
	 * of 1 completely replaces me.
	 */
	void			MixRgb(const ArpVoxel& v, float mix);
	void			MixRgb(uint8 cr, uint8 cg, uint8 cb, float mix);
	void			MixRgba(const ArpVoxel& v, float mix);
	void			MixRgbaz(const ArpVoxel& v, float mix);

/* Debugging
 */
public:
	void			Print(uint32 tabs = 0) const;
};

/*-------------------------------------------------------------*/
/*----- inline implementations --------------------------------*/
inline void ArpVoxel::MixRgb(const ArpVoxel& v, float mix)
{
	r = arp_clip_255(r + ((int16(v.r) - r) * mix));
	g = arp_clip_255(g + ((int16(v.g) - g) * mix));
	b = arp_clip_255(b + ((int16(v.b) - b) * mix));
}
	
inline void ArpVoxel::MixRgb(uint8 cr, uint8 cg, uint8 cb, float mix)
{
	r = arp_clip_255(r + ((int16(cr) - r) * mix));
	g = arp_clip_255(g + ((int16(cg) - g) * mix));
	b = arp_clip_255(b + ((int16(cb) - b) * mix));
}

inline void ArpVoxel::MixRgba(const ArpVoxel& v, float mix)
{
	r = arp_clip_255(r + ((int16(v.r) - r) * mix));
	g = arp_clip_255(g + ((int16(v.g) - g) * mix));
	b = arp_clip_255(b + ((int16(v.b) - b) * mix));
	a = arp_clip_255(a + ((int16(v.a) - a) * mix));
}

inline void ArpVoxel::MixRgbaz(const ArpVoxel& v, float mix)
{
	r = arp_clip_255(r + ((int16(v.r) - r) * mix));
	g = arp_clip_255(g + ((int16(v.g) - g) * mix));
	b = arp_clip_255(b + ((int16(v.b) - b) * mix));
	a = arp_clip_255(a + ((int16(v.a) - a) * mix));
	z = arp_clip_255(z + ((int16(v.z) - z) * mix));
}

#endif
