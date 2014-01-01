/* GlPlanes.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.09.11			hackborn@angryredplanet.com
 * Extracted from GlPixels
 */
#ifndef GLPUBLIC_GLPLANES_H
#define GLPUBLIC_GLPLANES_H

#include <be/support/SupportDefs.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlDefs.h>

/* Someone must call this (globally, once) to initialize the value
 * look up tables.  If it isn't called, then the Value() method will answer
 * with the max of the r, g, b components, which isn't really correct --
 * it tends to produce images that are too bright.
 */
void gl_pixel_init_value();

/***************************************************************************
 * GL-PLANES
 * I store an array of bit planes.
 ****************************************************************************/
class GlPlanes
{
public:
	int32			w, h;
	uint32			size;
	uint8**			plane;
	/* This info may or may not be present, depending on where the planes
	 * came from.  If they were created by a GlImage, then planeSrc will be
	 * the same size as plane and give a mask value (GL_PIXEL_R_MASK etc.)
	 * describing where each plane came from.  Also, the named planes don't
	 * respect the targetPixels that created the GlPlanes -- every one
	 * that exists in the image will be there.
	 */
	uint32*			planeSrc;
	uint8*			r;
	uint8*			g;
	uint8*			b;
	uint8*			a;
	uint8*			z;
	uint8*			diff;
	uint8*			spec;
	uint8*			d;
	uint8*			c;
	uint8*			f;

	GlPlanes(int32 w = 0, int32 h = 0);
	/* If the s (size) is > 0 then I allocate and deallocate a plane
	 * for each size.  The planes are unitialized.
	 */
	GlPlanes(int32 w, int32 h, uint32 s);
	GlPlanes(const GlPlanes& o);
	virtual ~GlPlanes();

	static uint8		RgbValue(uint8 r, uint8 g, uint8 b);

	virtual GlPlanes*	Clone() const;
	void				Free();

	status_t			SetSize(uint32 planeSize);

	/* THE FOLLOWING IS ONLY APPROPRIATE IF THE GLPLANES WAS
	 * CREATED BY A GLIMAGE.
	 */
	 
	/* Get the hue, saturation and value of this RGB color.
	 * h = 0 - 360, s = 0 - 1, v = 0 - 1.
	 * If s == 0, then h = -1 (undefined)
	 * If all you want is the value, you can supply 0 to h and s.
	 * This is in ArpColour, but also here for performance.
	 * Note that the value returned here is different from that
	 * returned by Value() -- that method is colour-corrected,
	 * and looks a little nicer.
	 */
	void				GetHsv(int32 pix, float* h, float* s, float* v) const;
	void				SetHsv(int32 pix, float h, float s, float v);
	/* These are mostly for convenience for clients who want a simple,
	 * compatible accessing method for performance reasons.  Note
	 * that gl_pixel_init_value() should be called on program startup for the
	 * Value() operation to look good.
	 */
	uint8				Hue(int32 pix) const;
	uint8				Saturation(int32 pix) const;
	uint8				Value(int32 pix) const;

	/* Answer true if my r, g, b, a, z components are there.
	 * They still might not be in the plane array, this is basically
	 * just asserting against an error condition.
	 */
	bool				HasColor() const;
	/* Answer true if every plane specified in the mask exists.
	 */
	bool				HasPlanes(uint32 mask) const;

	/* Utilities
	 */
	/* Mix in the supplied color - a mix of 0 makes no change, a mix
	 * of 1 completely replaces me.
	 */
	inline void MixRgb(int32 destPix, const GlPlanes& src, int32 srcPix, float mix)
	{
		ArpASSERT(HasColor() && src.HasColor());
		ArpASSERT(destPix >= 0 && destPix < w * h);
		ArpASSERT(srcPix >= 0 && srcPix < src.w * src.h);

		r[destPix] = arp_clip_255(r[destPix] + ((int16(src.r[srcPix]) - r[destPix]) * mix));
		g[destPix] = arp_clip_255(g[destPix] + ((int16(src.g[srcPix]) - g[destPix]) * mix));
		b[destPix] = arp_clip_255(b[destPix] + ((int16(src.b[srcPix]) - b[destPix]) * mix));
	}

	inline void MixRgb(int32 destPix, uint8 cr, uint8 cg, uint8 cb, float mix)
	{
		ArpASSERT(HasColor() && w > 0 && h > 0);
		ArpASSERT(destPix >= 0 && destPix < w * h);

		r[destPix] = arp_clip_255(r[destPix] + ((int16(cr) - r[destPix]) * mix));
		g[destPix] = arp_clip_255(g[destPix] + ((int16(cg) - g[destPix]) * mix));
		b[destPix] = arp_clip_255(b[destPix] + ((int16(cb) - b[destPix]) * mix));
	}

	/* MORPHING
	   ---------- */

	/* This method tells the image to increment its current frame -- only
	 * morphing images actually have frames.  The total arg indicates the
	 * total number of frames, but some clients don't actually know how
	 * many frames they have (unfortunate!!) and will just supply 0.
	 * Answer is true if the increment caused a change, false otherwise.
	 */
	virtual bool	IncFrame(uint32 total = 0);
	/* Return myself to the original state.
	 */
	virtual void	Rewind();

	/* FILLING
	   ---------- */
	void			Fill(GlFillType ft);

	void			ColorWheel(int32 l, int32 t, int32 r, int32 b);
	void			Black();

private:
	bool			mOwner;		// True if I own the planes and need to
								// delete -- currently only via the result
								// of Clone() / the copy constructor.
};

#endif
