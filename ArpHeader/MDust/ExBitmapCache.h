/* ExBitmapCache.h
 * Copyright (c)1999 by Eric Hackborn.
 * All rights reserved.
 *
 * Multiply inherit from this in your BWindow; then controls
 * attached to the window can use its bitmap for double buffering.
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
 *	• None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 07.01.99		hackborn
 * Created this file.
 */

#ifndef EXKERNEL_EXBITMAPCACHE_H
#define EXKERNEL_EXBITMAPCACHE_H

class BBitmap;
class BView;

class ExBitmapCache
{
public:
	ExBitmapCache();
	virtual ~ExBitmapCache();

	BView*				StartDrawing(BView* owner);
	void				FinishDrawing(BView* offscreen);
	
private:
	float				mCurWidth, mCurHeight;
	BBitmap*			mBitmap;
	BView*				mDrawView;
	BView*				mOwner;
};

#endif
