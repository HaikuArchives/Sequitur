/* GlImagePool.h
 * Copyright (c)2003 by Magic Box.
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.04.22				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLIMAGEPOOL_H
#define GLASSLIKE_GLIMAGEPOOL_H

#include <support/Locker.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlImage.h>
class GlImagePoolEntry;

/***************************************************************************
 * GL-IMAGE-POOL
 ***************************************************************************/
class GlImagePool
{
public:
	GlImagePool();
	~GlImagePool();

	gl_image_id					Acquire(const BString16& filename);
	gl_image_id					Acquire(GlImage* image);
	gl_image_id					Acquire(gl_image_id id);
	status_t					Release(gl_image_id id);
	GlImage*					Clone(gl_image_id id);
	ArpBitmap*					CloneBitmap(gl_image_id id);
	const GlImage*				Source(gl_image_id id);

private:
	std::vector<GlImagePoolEntry*>	mEntries;
	
	mutable BLocker				mAccess;

	GlImagePoolEntry*			GetEntry(const BString16& filename, uint32* index);
	GlImagePoolEntry*			GetEntry(gl_image_id id, uint32* index);
};

#endif
