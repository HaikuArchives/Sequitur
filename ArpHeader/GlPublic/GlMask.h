/* GlMask.h
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
 * 2002.09.03			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLMASK_H
#define GLPUBLIC_GLMASK_H

#include <support/SupportDefs.h>
class GlAlgo2d;
class GlPlanes;
class GlProcessStatus;

/***************************************************************************
 * GL-MASK
 * I am essentially a data cache.  I collaborate with images, who provide
 * my size, and surfaces, who populate the data.
 ****************************************************************************/
class GlMask
{
public:
	GlMask();
	GlMask(uint8* mask, int32 w, int32 h);
	GlMask(const GlPlanes& pixels, GlAlgo2d* s);
	virtual ~GlMask();

	/* Given the image (represented by the pixels and dimensions) and a surface,
	 * generate data large enough for the image and let the surface process it.
	 * The pixels are there for dimensions, but they might not have any planes.
	 */
	uint8*			Make(	const GlPlanes& pixels, GlAlgo2d* a,
							GlProcessStatus* status = 0);
	/* Copy the mask into me.  If my data isn't large enough for it, grow me.
	 */
	status_t		Make(uint8* mask, int32 w, int32 h);
	/* Answer my data.  No size info is supplied because the mask must
	 * have been made for a specific image, and unless the Make() command
	 * failed, the data will conform to that size.
	 */
	uint8*			Data(int32* outWidth = 0, int32* outHeight = 0);
	
private:
	uint8*			mData;
	int32			mW, mH;
	bool			mSuppress;
};

#endif
