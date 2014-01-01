/* ArpIntControlMotions.h
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This file defines an interface used to track mouse movements
 * for the ExIntControl.  In addition, it defines several concrete
 * convenience classes.
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
 * 05.08.00		hackborn
 * Created this file.
 */

#ifndef ARPVIEWS_ARPINTCONTROLMOTIONS_H
#define ARPVIEWS_ARPINTCONTROLMOTIONS_H

#include "ArpViews/ArpIntControl.h"

/***************************************************************************
 * ARP-INT-CONTROL-MOTION-I
 * A class that can be plugged into the ArpIntControl that defines how
 * that control's value gets assigned when the mouse moves.  Subclasses
 * are responsible for sending out realtime info during the mouse moved,
 * but they are not responsible for setting the mouse event mask in the mouse
 * down.
 ***************************************************************************/
class ArpIntControlMotionI
{
public:
	virtual ~ArpIntControlMotionI()		{ }

	/* Implementors are asked by the int control to cache all of this
	 * information, so that the motion object has whatever it might
	 * possibly need, and the int control doesn't have to pass it over
	 * with every call to mouse moved.
	 */
	virtual void CacheData(ArpIntControl* control);

	virtual	void MouseDown(BPoint pt) = 0;
	virtual	void MouseUp(BPoint pt) = 0;
	virtual	void MouseMoved(BPoint pt,
							uint32 code,
							const BMessage *msg) = 0;

protected:
	ArpIntControl*		mControl;
};

/***************************************************************************
 * ARP-INT-CONTROL-SMALL-MOTION
 * This class is intended for when the int control has a small range of
 * possible values, say somewhere between 0 and 300.
 ***************************************************************************/
class ArpIntControlSmallMotion : public ArpIntControlMotionI
{
public:
	ArpIntControlSmallMotion(float pixelsBeforeChange = 3);
	virtual ~ArpIntControlSmallMotion();

	virtual	void MouseDown(BPoint pt);
	virtual	void MouseUp(BPoint pt);
	virtual	void MouseMoved(BPoint pt,
							uint32 code,
							const BMessage *msg);

private:
	bool		mMouseDown;
	int32		mBaseValue;
	BRect		mBaseRect;
	/* The number of pixels to track before the value changes.
	 */
	float		mPixels;
};

/***************************************************************************
 * ARP-INT-CONTROL-MEDIUM-MOTION
 * This class is intended for when the int control has a medium range of
 * possible values, say somewhere between 0 and 5000.
 ***************************************************************************/
class ArpIntControlMediumMotion : public ArpIntControlMotionI
{
public:
	ArpIntControlMediumMotion();
	virtual ~ArpIntControlMediumMotion();

	virtual	void MouseDown(BPoint pt);
	virtual	void MouseUp(BPoint pt);
	virtual	void MouseMoved(BPoint pt,
							uint32 code,
							const BMessage *msg);

private:
	bool		mMouseDown;
	int32		mBaseValue;
	BRect		mBaseRect;
	BPoint		mPrevPt;

	int32 NewValue(float xChange, float yChange);
};

#endif
