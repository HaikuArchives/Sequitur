/* GlCache2d.h
 * Copyright (c)2002-2004 by Eric Hackborn.
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
 * 2004.04.29			hackborn@angryredplanet.com
 * Updated from GlMask.
 *
 * 2002.09.03			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLCACHE2D_H
#define GLPUBLIC_GLCACHE2D_H

#include <support/SupportDefs.h>
class GlAlgo2d;
class GlPlanes;
class GlProcessStatus;

/***************************************************************************
 * GL-CACHE-2D
 * I am essentially a data cache.  I collaborate with images, who provide
 * my size, and surfaces, who populate the data.
 ****************************************************************************/
class GlCache2d
{
public:
	GlCache2d();
	GlCache2d(int32 w, int32 h);
	virtual ~GlCache2d();

	int32			Width() const;
	int32			Height() const;
	uint8*			Data(int32* outWidth = 0, int32* outHeight = 0);

	/* Given the image (represented by the pixels and dimensions) and a surface,
	 * generate data large enough for the image and let the surface process it.
	 * The pixels are there for dimensions, but they might not have any planes.
	 */
	uint8*			Make(	const GlPlanes& pixels, GlAlgo2d* a,
							GlProcessStatus* status = 0);

	/* Make my data the same size as the dimensions and answer it.  The
	 * answered data is uninitialized.
	 */
	uint8*			SetDimensions(int32 w, int32 h);

protected:
	friend class	GlAlgo2dWrap;

	status_t		mStatus;	// If there's an error than don't answer my data
	
private:
	uint8*			mData;
	int32			mW, mH;
};

#endif
