/* ExKnobControl.h
 * Copyright (c)1999 by Eric Hackborn.
 * All rights reserved.
 *
 * This class implements a control that acts as a typical analog-style
 * knob.
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
 * 07.04.99		hackborn
 * Created this file.
 */

#ifndef ARPVIEWS_ARPKNOBCONTROL_H
#define ARPVIEWS_ARPKNOBCONTROL_H

#include <be/interface/Control.h>
#include "BeExp/ToolTip.h"
#include <ArpLayout/ArpBaseLayout.h>
#include "ArpViewsPublic/ArpIntToStringMapI.h"
#include "ArpViews/ArpIntControl.h"
class ArpIntControl;
class ArpIntFormatterI;

/* These are the default values for the degree range.  They set
 * the knob to be a traditional, non-freely rotating knob whose low
 * value is facing down and a little left, and whose  high value
 * is facing down and a little right.
 */
enum {
	ARP_DEFAULT_MIN_DEGREE		= 40,
	ARP_DEFAULT_MAX_DEGREE		= 320
};

#define ARP_RING_ADORNMENT			(0x00000001)
#define ARP_TIGHT_RING_ADORNMENT	(0x00000002)
#define ARP_KNOB_BITMAP_BG			(0x00000100)

/***************************************************************************
 * ARP-KNOB-CONTROL
 * This class displays a standard analog-style knob that the user can
 * manipulate.  Degree 0 is pointing straight down, degree 90 points left,
 * 180 points straight up, etc.  This class operates on values, which are
 * independent of degrees.  Clients are required to supply a value range in
 * the constructor, which then get mapped to the minimum and maximum degrees.
 ***************************************************************************/
class ArpKnobControl : public BControl,
					   public BToolTipable
{
public:
	ArpKnobControl(	BRect frame,
					const char* name,
					BMessage* message,
					int32 minValue,
					int32 maxValue,
					uint32 displayFlags = 0,
					uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual ~ArpKnobControl();
	
	virtual	void	AttachedToWindow();
	virtual	void	Draw(BRect updateRect);
	virtual	void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint pt);
	virtual	void	MouseMoved(BPoint pt, uint32 code, const BMessage *msg);

	/* Set and answer a message that is delivered as the knob is twisted.
	 */
	virtual	void	SetModificationMessage(BMessage* message);
	BMessage*		ModificationMessage() const;

	/* Set the object that will receive focus whenver I am activated.
	 */
	void			SetFocusTarget(BView *view)					{ mFocusTarget = view; }
	/* The lowest and highest value allowed for the knob.  Values are translated
	 * to degrees such that the minimum value equals the minimum degree, and the
	 * maximum value equals the maximum degree.
	 *
	 * This method will cause the Value() to change if it no longer falls
	 * within the allowable limits.  Additionally, the knob will be redrawn
	 * to display the new pointer value in relation to the new limits.
	 *
	 * Using this method replaces the min and max values supplied in the
	 * constructor with the new ones.  Changing this dynamically doesn't
	 * make a whole lot of sense, and might be removed in the future.
	 */
	void			SetValueRange(int32 minValue, int32 maxValue);
	/* The value can be from minValue to maxValue (as defined in the constructor).
	 */
	virtual	void	SetValue(int32 value);
	/* The lowest and highest degree allowed on the knob.  The range moves
	 * clockwise, starting with 0 pointing straight down.  Logically, this
	 * is probably something that should only be set during construction.
	 * That might actually become an enforced rule.
	 */
	void			SetDegreeRange(int32 minDegree, int32 maxDegree);
	/* Answer my current value as a degree.
	 */
	int32			Degree() const;
	/* This is an object that can be used to format the display of
	 * my value for the tooltip.
	 */
	void			SetTipFormatter(ArpIntFormatterI* formatter);
	 
	/* This is all preliminary, a quick hack to get the int and knob
	 * talking.
	 */
	void SetIntControl(ArpIntControl* intCtrl);

protected:
	/* The index value sent to the param in its ReadSysEx(), WriteSysEx(),
	 * and SendRealTime() methods.
	 */
	uint32				mIndex;

	void DrawOn(BRect updateRect, BView *view);
	/* This method fills in the background bitmap, tiled correctly
	 * assuming that the bitmap is aligned with the upper left corner
	 * of this view's parent.
	 */
	void DrawBackgroundOn(BRect updateRect, BView *view);

	ArpIntControl*		mIntCtrl;

private:
	typedef BControl	inherited;
	rgb_color			mViewColor;
	int32				mMinValue, mMaxValue;
	int32				mDegree;
	int32				mMinDegree, mMaxDegree;
	// This view cannot have focus, but frequently it has an int control
	// associated with it that can.  Whenever this view is activated,
	// focus is given to its current mFocusTarget (typically the aforementioned
	// int control.
	BView				*mFocusTarget;
	// Set to true once I've started observing my window.
	bool				mIsObserving;		
	// Set to true while the moust is down
	bool				mMouseDown;
	BPoint				mLastPoint;
	uint32				mDisplayFlags;	
	BMessage*			mModificationMessage;
	ArpIntFormatterI*	mFormatter;
	
	int32				DegreeToValue(int32 value) const;
	int32				ValueToDegree(int32 degree) const;
};

/***************************************************************************
 * ARP-KNOB-PANEL
 * This class constructs a knob along with an optional label and/or int
 * control.  Part of its usefulness is that it hides hooking up the knob
 * and int controls to each other, which is behaviour that will probably
 * change at some point.
 ***************************************************************************/
class ArpKnobPanel : public BView,
					 public ArpBaseLayout
{
public:
	ArpKnobPanel(	const char* name,
					const char* label,
					BMessage* message,
					int32 minValue,
					int32 maxValue,
					bool showIntControl,
					orientation layout = B_HORIZONTAL,
					uint32 knobFlags = ARP_TIGHT_RING_ADORNMENT,
					float labelWidth = -1,
					float intControlWidth = -1,
					uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
	virtual ~ArpKnobPanel();

	ArpKnobControl*	KnobControl() const;
	ArpIntControl*	IntControl() const;

	virtual void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual	void	GetPreferredSize(float* width, float* height);

	virtual BHandler* LayoutHandler()				{ return this; }
	virtual const BHandler* LayoutHandler() const	{ return this; }
 
private:
	typedef BView	inherited;

	void			LayoutHorizontal(	const char* name, const char* label, BMessage* message,
										int32 minValue, int32 maxValue,
										bool showIntControl, uint32 knobFlags, float labelWidth,
										float intControlWidth);
	void			LayoutVertical(		const char* name, const char* label, BMessage* message,
										int32 minValue, int32 maxValue,
										bool showIntControl, uint32 knobFlags, float labelWidth,
										float intControlWidth);
};

#endif
