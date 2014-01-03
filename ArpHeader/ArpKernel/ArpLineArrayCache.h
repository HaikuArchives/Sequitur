/* ArpLineArrayCache.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 05.10.00		hackborn
 * Extracted this from the ArpView.  ArpView is going away, but this
 * line array behaviour is still useful at times.
 */
 
#ifndef ARPKERNEL_ARPLINEARRAY_H
#define ARPKERNEL_ARPLINEARRAY_H

#include <interface/View.h>

/***************************************************************************
 * ARP-LINE-ARRAY-CACHE
 * This is a simple object that manages line arrays for a specific view.
 * All it does is set a maximum size for its arrays.  When the current array
 * grows too large (or is ended), it flushes the changes.
 ***************************************************************************/
class ArpLineArrayCache
{
public:
	ArpLineArrayCache();
	ArpLineArrayCache(BView* target);
	virtual ~ArpLineArrayCache();

	void SetTarget(BView* target);

	void BeginLineArray(int32 arraySize = 128);
	void AddLine(BPoint pt0, BPoint pt1, rgb_color col);
	void AddLine(BPoint pt0, BPoint pt1, uchar r, uchar g, uchar b, uchar a = 255);
	void EndLineArray();

private:
	/* The view that the lines will be drawn on.
	 */
	BView*		mTarget;
	/* The number of lines that can be added to an array before
	 * it is flushed.
	 */
	int32		mArraySize;
	/* Track how many lines have been added to the array.
	 */
	int32		mLineCount;
	/* True if BeginLineArray has been called but EndLineArray hasn't.
	 */
	bool		mInArray;
};

#endif
