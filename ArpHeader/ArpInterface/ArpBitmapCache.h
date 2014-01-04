/*
 * Copyright (c)2000 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpBitmapCache.h
 *
 * Multiply inherit from this in your BWindow; then controls
 * attached to the window can use its bitmap for double buffering.
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
 * 05.01.00		hackbod
 * Created this file.
 */

#ifndef ARPINTERFACE_ARPBITMAPCACHE_H
#define ARPINTERFACE_ARPBITMAPCACHE_H

#include <interface/Rect.h>
class BView;

/*******************************************************
 * ARP-BITMAP-CACHE
 * The bitmap cache handles double buffering, which is
 * done automatically by OS X.  So this class does
 * nothing.  Nice.
 *******************************************************/
class ArpBitmapCache
{
public:
	ArpBitmapCache();
	virtual ~ArpBitmapCache();

	BView*				StartDrawing(BView* owner, BRect updateRect);
	void				FinishDrawing(BView* offscreen);
};

#endif
