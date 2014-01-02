/* ArpFloatControl.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.07.01			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPVIEWS_ARPFLOATCONTROL_H
#define ARPVIEWS_ARPFLOATCONTROL_H

#include <app/Invoker.h>
#include <app/MessageRunner.h>
#include <interface/View.h>
#include <support/String.h>
class ArpInlineTextView;

enum {
	ARPMSG_FLOAT_CONTROL_CHANGED	= 'aICC'
};

/***************************************************************************
 * EX-INT-CONTROL
 * This class presents the user with a text representation of a number,
 * and allows them to change the value by clicking on it and dragging.
 ***************************************************************************/
class ArpFloatControl : public BView,
						public BInvoker
{
public:
	ArpFloatControl(BRect frame,
					const char* name,
					const char* label,
					BMessage* message,
					uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	virtual ~ArpFloatControl();

	enum Flags {
		ARP_IS_ENABLED			= 0x00000001,
		ARP_EXPLICITLY_DISABLED	= 0x00000002
	};

	bool			IsEnabled() const;
	void			SetEnabled(bool enabled);
	/* This method sets the minimum and maximum numeric value that the int
	 * control will display.  Normally, the control takes these values from
	 * its param.  However, in certain situations, clients might want
	 * to force a specific range.  Once this method has been called, the control
	 * will never again use its param to find its minimum and maximum values.
	 */
	void			SetLimits(float min, float max);
	/* Set the step increment used as the user drags the mouse.
	 */
	void			SetSteps(float steps);
	/* Get and set the value.
	 */
	float			Value() const;
	void			SetValue(float value);
	/* Set the number of pixels alloted to the label.  By default, it's
	 * the length of the label (or 0 if no label).
	 */
	virtual	void	SetDivider(float dividing_line);
	float			Min() const;
	float			Max() const;
	/* Fill the string with one of my values, formatted as it appears
	 * in the control.
	 */
	void			GetValueLabel(float value, BString& str) const;
	BRect			ControlBounds() const;
	
	virtual	void	AttachedToWindow();
	virtual	void	Draw(BRect updateRect);
	virtual	void	GetPreferredSize(float *width, float *height);
	virtual	void	KeyDown(const char *bytes, int32 numBytes);
	virtual	void	KeyUp(const char *bytes, int32 numBytes);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint pt);
	virtual	void	MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage *msg);
	virtual void	MessageReceived(BMessage *msg);

	void			StartEdit();
	void			StopEdit(bool keepChanges = true);

	/* Called whenever my value changes.  Subclasses can implement this
	 * to notify any interested dependents of the change.  The default
	 * implementation sends out an ARPMSG_FLOAT_CONTROL_CHANGED to any
	 * observers.
	 */
	virtual void	NotifyHook(float newValue);
	/* Make sure the value falls within my limits, send out notification.
	 */
	void			SafeSetValue(float newValue);
	
protected:
	/* If subclasses want to override the drawing, here's where.
	 */
	virtual void	DrawOn(BRect updateRect, BView* view);
	/* This draws the background and sets the low and high colors appropriately,
	 */
	void			DrawControlBackgroundOn(BRect bounds, BView* view);
	void			GratuitousShadeOn(	BView* view,
										BRect bounds,
										rgb_color color,
										int16 delta);
	void 			DrawBitmapBackgroundOn(	BRect bounds,
											BView* view,
											const BBitmap* bitmap);

	void			AddAscii(char byte);
	/* This method takes whatever value the user typed into the inline
	 * edit control and translates it to an int for this control.  By
	 * default, if there's a string map, that's used.  If there isn't,
	 * then the int value of the current ASCII text is found.
	 */
	virtual status_t ValueFromLabel(const char* label, float* answer) const;
	
private:
	typedef BView			inherited;
	float					mValue;
	int32					mFlags;
	BString					mLabel;
	float					mDivider;
	// These are the limits set by the SetLimits() method.  They are
	// ignored unless that method has been called.
	float					mSteps;
	float					mMin, mMax;
	rgb_color				mViewColor;	
	/* These are used to facilitate key presses.
	 */
	int32					mKeyStep, mKeyCount;

	/* This is the text control I use to allow users to type in new values.
	 */
	ArpInlineTextView*		mTextCtrl;
	/* Timer for bringing up text edit box after
	 * double-click time elapses.
	 */
	BMessageRunner*			mEditRunner;
	BPoint					mDownPt;
	void					StartTimer(const BMessage& msg, bigtime_t delay);
	void					StopTimer();

	/* This is all for tracking the mouse when the user
	 * clicks and drags me.
	 */
	bool		mMouseDown;
	float		mBaseValue;
	BRect		mBaseRect;
	/* The number of pixels to track before the value changes.
	 */
	float		mPixels;
};

#endif
