/* GlSkinCache.h
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
 * 2003.01.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLSKINCACHE_H
#define GLASSLIKE_GLSKINCACHE_H

//#include "GlPublic/GlNodeIo.h"
//#include "GlPublic/GlSkinI.h"
class ArpBitmap;
class GlChainCache;

/***************************************************************************
 * GL-SKIN-CACHE
 * I cache the pieces used to construct the skin -- the grids for data
 * generation, and the data themselves.
 ***************************************************************************/
class GlSkinCache
{
public:
	GlSkinCache();
	virtual ~GlSkinCache();

	/*---------------------------------------------------------
	 * IMAGES
	 *---------------------------------------------------------*/
	virtual const ArpBitmap*	AcquireChain(const BString16& text);
	virtual void				ReleaseChain(const ArpBitmap* bm);

	/*---------------------------------------------------------
	 * UTILITY
	 *---------------------------------------------------------*/
	virtual status_t			Initialize();
	
private:
	GlChainCache*		mChainCache;

	void				Free();
};


#endif
