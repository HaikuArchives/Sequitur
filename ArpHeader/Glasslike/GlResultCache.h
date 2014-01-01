/* GlResultCache.h
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
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.04.22		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_GLRESULTCACHE_H
#define GLASSLIKE_GLRESULTCACHE_H

#include <be/app/Messenger.h>
#include <be/interface/MenuItem.h>
#include <be/support/Locker.h>
class ArpBitmap;
class GlImage;

/***************************************************************************
 * GL-RESULT-CACHE
 * I am a cache on the current result image -- actually, on two images.
 * The frozen image is the basis for any new processing, the displayed image
 * is what is currently being displayed.  This doesn't REALLY make sense,
 * does it?  The displayed image should just be handled by the result view.
 * Maybe that'll get ironed out.
 ***************************************************************************/
class GlResultCache
{
public:
	GlResultCache();
	~GlResultCache();

	void			SetTarget(const BMessenger& target);
	void			SetPreviewSize(int32 w, int32 h);
	status_t		GetFileName(BString16& str) const;

	GlImage*		NewFrozenImage() const;
	ArpBitmap*		NewDisplayedBitmap() const;
	GlImage*		NewPreviewImage() const;
	ArpBitmap*		NewPreviewBitmap() const;
	
	/* Take ownership of the new displayed image and send out notification.
	 */
	status_t		UpdateImage(GlImage* img);
	/* Freeze the current display image, if any.  Do NOT send update notification.
	 */
	status_t		FreezeDisplay();
	/* Take ownership of the new frozen image.  Do NOT send update notification.
	 */
	status_t		FreezeImage(GlImage* img);
	/* Answer the bitmap used to instantiate the image from the file -- the
	 * caller assumes ownership.
	 */
	ArpBitmap*		FreezeImage(BMessage* msg);
	ArpBitmap*		FreezeImage(const BString16& fn);

	status_t		Rewind();

private:
	GlImage*		mFrozenImage;
	GlImage*		mDisplayedImage;
	int32			mPreviewW, mPreviewH;
	GlImage*		mPreviewImage;
			
	BMessenger		mTarget;

	BString16		mFileName;

	mutable BLocker	mAccess;

	status_t		CachePreview();
};

#endif
