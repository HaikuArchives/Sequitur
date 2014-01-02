/* ArpIntControl.h
 * Copyright (c)2002 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
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
 * 2002.07.15			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPINTERFACE_ARPINTCONTROL_H
#define ARPINTERFACE_ARPINTCONTROL_H

#include <ArpCore/String16.h>
#include <app/MessageRunner.h>
#include <interface/Control.h>
class ArpInlineTextView;
class ArpIntControlMotionI;
class ArpIntFormatterI;
class ArpIntToStringMapI;
class BBitmap;

enum {
	ARPMSG_INT_CONTROL_CHANGED	= 'aICC'
};

/***************************************************************************
 * ARP-INT-CONTROL
 * This class presents the user with a text representation of a number,
 * and allows them to change the value by clicking on it and dragging.
 ***************************************************************************/
class ArpIntControl : public BControl
{
public:
	ArpIntControl(	BRect frame,
					const char* name,
					const BString16* label,
					BMessage* message,
					uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	virtual ~ArpIntControl();

	enum Flags {
		ARP_EXPLICITLY_DISABLED	= 0x00000001
	};

	/* Set an object that will cause certain integer values to be displayed
	 * as strings, instead of the original integer.  For example, a string
	 * map could be supplied that answers "Preset" for each integer of -1,
	 * and returns with an error for all other integers.  In this case, whenever
	 * my value is -1, I will display "Preset" instead of "-1", but I will
	 * still display numbers for all other values.
	 *
	 * Note that I am the owner of the map, and will delete it accordingly.
	 */
	void			SetStringMap(ArpIntToStringMapI* map);
	/* The formatter is the last line of defense between the user and a
	 * raw, unadorned number.  If there's no string map or the map doesn't
	 * handle the current value(), then the number is given to the int
	 * formatter, if any.
	 */
	void			SetFormatter(ArpIntFormatterI* formatter);
	/* This method sets the minimum and maximum numeric value that the int
	 * control will display.  Normally, the control takes these values from
	 * its param.  However, in certain situations, clients might want
	 * to force a specific range.  Once this method has been called, the control
	 * will never again use its param to find its minimum and maximum values.
	 */
	void			SetLimits(int32 min, int32 max);
	/* Set the object that tracks mouse motions and assigns new values based
	 * on them.  I own the object, and am responsible for deleting it.
	 */
	void			SetMotion(ArpIntControlMotionI* motion);
	/* Set the number of pixels alloted to the label.  By default, it's
	 * the length of the label (or 0 if no label).
	 */
	virtual	void	SetDivider(float dividing_line);
	/* Answer the min and max.  If the ARP_CLIENT_LIMITS flags has
	 * been set, use mMin and mMax, otherwise take the values from
	 * the param.
	 */
	int32			Min() const;
	int32			Max() const;
	/* Fill the string with one of my values, formatted as it appears
	 * in the control.
	 */
	void			GetValueLabel(int32 value, BString16& str) const;
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
	 * implementation sends out an ARPMSG_INT_CONTROL_CHANGED to any
	 * observers.
	 */
	virtual void	NotifyHook(int32 newValue);
	/* Make sure the value falls within my limits, send out notification.
	 */
	void			SafeSetValue(int32 newValue);
	
protected:
	/* If subclasses want to override the drawing, here's where.
	 */
	virtual void	DrawOn(BRect updateRect, BView* view);
	/* This draws the background and sets the low and high colors appropriately,
	 */
	virtual void	DrawControlBackgroundOn(BRect bounds, BView* view);
	virtual void	GratuitousShadeOn(	BView* view,
										BRect bounds,
										rgb_color color,
										int16 delta);
	void 			DrawBitmapBackgroundOn(	BRect bounds,
											BView* view,
											const BBitmap* bitmap);

	/* Only applies to the value label.  Do nothing to leave it as the
	 * foreground colour.
	 */
	virtual void	SetValueColor(BView* v);

	void			AddAscii(char byte);
	/* This method takes whatever value the user typed into the inline
	 * edit control and translates it to an int for this control.  By
	 * default, if there's a string map, that's used.  If there isn't,
	 * then the int value of the current ASCII text is found.
	 */
	virtual status_t ValueFromLabel(const BString16* label, int32* answer) const;
	
private:
	typedef BControl		inherited;
	ArpIntControlMotionI*	mMotion;
	int32					mFlags;
	BString16				mLabel;
	float					mDivider;
	// These are the limits set by the SetLimits() method.  They are
	// ignored unless that method has been called.
	int32					mMin, mMax;
	rgb_color				mViewColor;	
	/* This string map will determine if some (or all) of the displayed
	 * integers get displayed as strings, instead.  It can be null.
	 */
	ArpIntToStringMapI*		mMap;
	/* This formatter is the last line of defense when displaying ints:
	 * If there was no map, or the map did not handle the int, then if
	 * there IS a formatter, the int gets run through that.
	 */
	ArpIntFormatterI*		mFormatter;
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
};

#endif
