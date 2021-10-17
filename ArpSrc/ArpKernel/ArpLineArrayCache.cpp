/* ArpLineArrayCache.cpp
 */
#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpLineArrayCache.h"

/***************************************************************************
 * ARP-LINE-ARRAY-CACHE
 ***************************************************************************/
ArpLineArrayCache::ArpLineArrayCache()
		: mTarget(0), mArraySize(128), mLineCount(0), mInArray(false)
{
}

ArpLineArrayCache::ArpLineArrayCache(BView* target)
		: mTarget(target), mArraySize(128), mLineCount(0), mInArray(false)
{
}

ArpLineArrayCache::~ArpLineArrayCache()
{
}

void ArpLineArrayCache::SetTarget(BView* target)
{
	if ( (mTarget != 0) && (mInArray) ) EndLineArray();
	mTarget = target;
}

void ArpLineArrayCache::BeginLineArray(int32 arraySize)
{
	if (mTarget == 0) return;
	
	if (mInArray) EndLineArray();
	mArraySize = arraySize;
	mInArray = true;
	mTarget->BeginLineArray(mArraySize);
}

void ArpLineArrayCache::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
	if (mTarget == 0) return;

	mTarget->AddLine(pt0, pt1, col);
	mLineCount++;
	if (mLineCount >= mArraySize) {
		EndLineArray();
		BeginLineArray(mArraySize);
	}		
}

void ArpLineArrayCache::AddLine(BPoint pt0, BPoint pt1, uchar r, uchar g, uchar b, uchar a)
{
	rgb_color	c;
	c.red = r; c.green = g; c.blue = b; c.alpha = a;
	AddLine(pt0, pt1, c);
}

void ArpLineArrayCache::EndLineArray()
{
	if (mTarget == 0) return;

	if (mInArray) mTarget->EndLineArray();
	mInArray = false;
	mLineCount = 0;
}
