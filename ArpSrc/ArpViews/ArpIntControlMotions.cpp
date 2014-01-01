/* ArpIntControlMotions.cpp
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "ArpViews/ArpIntControlMotions.h"

/*************************************************************************
 * ARP-INT-CONTROL-MOTION-I
 *************************************************************************/
void ArpIntControlMotionI::CacheData(ArpIntControl* control)
{
	mControl = control;
}

/*************************************************************************
 * ARP-INT-CONTROL-SMALL-MOTION
 *************************************************************************/
ArpIntControlSmallMotion::ArpIntControlSmallMotion(float pixelsBeforeChange)
		: mMouseDown(false), mBaseValue(0),
		mPixels(pixelsBeforeChange)
{
}

ArpIntControlSmallMotion::~ArpIntControlSmallMotion()
{
}

void ArpIntControlSmallMotion::MouseDown(BPoint pt)
{
	mMouseDown = true;
	mBaseValue = mControl->Value();
	
	mBaseRect = mControl->ControlBounds();
#if 0
	BRect	b = mControl->ControlBounds();
	char	str[10];
	float	width;
	sprintf(str, "%ld", mBaseValue);
	width = mControl->StringWidth(str);
	mBaseRect.left = b.left + 2;
	mBaseRect.right = mBaseRect.left + width;
	mBaseRect.top = b.top + 2;
	mBaseRect.bottom = b.bottom - 2;
#endif
}

void ArpIntControlSmallMotion::MouseUp(BPoint pt)
{
	mMouseDown = false;
}

void ArpIntControlSmallMotion::MouseMoved(	BPoint pt,
											uint32 code,
											const BMessage *msg)
{
	if (mMouseDown == false) return;

	float	xChange, yChange;
	int32	newValue;
	
	if (pt.x < mBaseRect.left) {
		xChange = (pt.x - mBaseRect.left) / mPixels;
	} else if (pt.x > mBaseRect.right) {
		xChange = (pt.x - mBaseRect.right) / mPixels;
	} else {
		xChange = 0;
	}
			
	if (pt.y < mBaseRect.top) {
		yChange = (pt.y - mBaseRect.top) / mPixels;
	} else if (pt.y > mBaseRect.bottom) {
		yChange = (pt.y - mBaseRect.bottom) / mPixels;
	} else {
		yChange = 0;
	}
	newValue = (int32)(mBaseValue + (xChange - yChange));
	mControl->SafeSetValue(newValue);
}

/***************************************************************************
 * ARP-INT-CONTROL-MEDIUM-MOTION
 ***************************************************************************/
ArpIntControlMediumMotion::ArpIntControlMediumMotion()
		: mMouseDown(false), mBaseValue(0), mPrevPt(0, 0)
{
}

ArpIntControlMediumMotion::~ArpIntControlMediumMotion()
{
}

void ArpIntControlMediumMotion::MouseDown(BPoint pt)
{
	mMouseDown = true;
	mBaseValue = mControl->Value();
	mBaseRect = mControl->ControlBounds();
	mPrevPt = pt;
}

void ArpIntControlMediumMotion::MouseUp(BPoint pt)
{
	mMouseDown = false;
}

void ArpIntControlMediumMotion::MouseMoved(	BPoint pt,
											uint32 code,
											const BMessage *msg)
{
	if (mMouseDown) {
		if (mBaseRect.Contains(pt)) {
			mControl->NotifyHook(mBaseValue);
			mControl->SetValue(mBaseValue);
		} else {
			float	xChange = pt.x - mPrevPt.x,
					yChange = pt.y - mPrevPt.y,
					newValue;
			newValue = NewValue(xChange, yChange);
			mControl->SafeSetValue(newValue);
		}
	}
	mPrevPt = pt;
}

int32 ArpIntControlMediumMotion::NewValue(float xChange, float yChange)
{
//	int32	realMin = mControl->Min(), realMax = mControl->Max();

//	int32	range = realMax + (0 - realMin);
	// Case one -- a small range, simply add the offsets together and
	// offset the current value by that.
//	if (range < 150) {
//		return (int32)(Value() + (xChange + yChange));
//	}
	// Case two -- a medium range, add the offsets together and raise
	// that to two, then offset the current value by the answer.
	bool	neg = false;
	if ((xChange - yChange) < 0) neg = true;
	int32	delta = (int32)((xChange - yChange) * (xChange - yChange));
	if (neg && (delta > 0)) delta = 0 - delta;
	return (int32)(mControl->Value() + delta);
}
