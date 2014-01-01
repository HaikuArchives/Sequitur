/* GlPixel.h
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
#ifndef GLPUBLIC_GLPIXEL_H
#define GLPUBLIC_GLPIXEL_H

#include <be/support/SupportDefs.h>
class GlPlanes;

enum {
	GL_PIXEL_R_MASK		= 0x00000001,
	GL_PIXEL_G_MASK		= 0x00000002,
	GL_PIXEL_B_MASK		= 0x00000004,
	GL_PIXEL_A_MASK		= 0x00000008,
	GL_PIXEL_Z_MASK		= 0x00000010,
	GL_PIXEL_DIFF_MASK	= 0x00000020,
	GL_PIXEL_SPEC_MASK	= 0x00000040,
	GL_PIXEL_D_MASK		= 0x00000080,
	GL_PIXEL_C_MASK		= 0x00000100,
	GL_PIXEL_F_MASK		= 0x00000200
};
#define GL_PIXEL_RGB	(GL_PIXEL_R_MASK | GL_PIXEL_G_MASK | GL_PIXEL_B_MASK)
#define GL_PIXEL_RGBA	(GL_PIXEL_R_MASK | GL_PIXEL_G_MASK | GL_PIXEL_B_MASK | GL_PIXEL_A_MASK)
#define GL_PIXEL_RGBAZ	(GL_PIXEL_R_MASK | GL_PIXEL_G_MASK | GL_PIXEL_B_MASK | GL_PIXEL_A_MASK | GL_PIXEL_Z_MASK)
#define GL_PIXEL_ALL	(GL_PIXEL_R_MASK | GL_PIXEL_G_MASK | GL_PIXEL_B_MASK | GL_PIXEL_A_MASK \
						 | GL_PIXEL_Z_MASK | GL_PIXEL_DIFF_MASK | GL_PIXEL_SPEC_MASK \
						 | GL_PIXEL_D_MASK | GL_PIXEL_C_MASK | GL_PIXEL_F_MASK)

#define GL_PIXEL_COLOR		(GL_PIXEL_R_MASK | GL_PIXEL_G_MASK | GL_PIXEL_B_MASK | GL_PIXEL_A_MASK | GL_PIXEL_Z_MASK)
#define GL_PIXEL_LIGHT		(GL_PIXEL_DIFF_MASK | GL_PIXEL_SPEC_MASK)
#define GL_PIXEL_MATERIAL	(GL_PIXEL_D_MASK | GL_PIXEL_C_MASK | GL_PIXEL_F_MASK)

/***************************************************************************
 * GL-PIXEL
 * A utility that stores a single value from each plane of a GlPlanes.
 ****************************************************************************/
class GlPixel
{
public:
	uint32			size;
	uint8*			c;

	GlPixel(uint32 size = 0);
	GlPixel(const GlPlanes& planes, int32 pix);
	GlPixel(const GlPixel& o);
	virtual ~GlPixel();

	bool				operator==(const GlPixel& o) const;
	
	virtual GlPixel*	Clone() const;

	status_t			SetSize(uint32 cSize);
	status_t			SetTo(const GlPlanes& planes, int32 pix);

protected:
	void				Free();

public:
	void				Print(uint32 tabs = 0) const;
};

#endif
