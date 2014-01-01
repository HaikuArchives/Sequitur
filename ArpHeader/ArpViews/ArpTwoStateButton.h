/* ArpTwoStateButton.h
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
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 11.08.00		hackborn
 * Created this file.
 */

#ifndef ARPVIEWS_ARPTWOSTATEBUTTON_H
#define ARPVIEWS_ARPTWOSTATEBUTTON_H

#include <be/interface/Control.h>

/***************************************************************************
 * ARP-TWO-STATE-BUTTON
 * Be doesn't have a normal two state button that displays bitmaps, so this
 * one's a quick hack.
 ***************************************************************************/
class ArpTwoStateButton : public BControl
{
public:
	ArpTwoStateButton(	BRect frame, const char* name,
						const char* label,
						BMessage* message,
						const BBitmap* bmNormal = NULL,
						const BBitmap* bmOver = NULL,
						const BBitmap* bmPressed = NULL,
						const BBitmap* bmDisabled = NULL,
						const BBitmap* bmDisabledPressed = NULL,
						uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
						
	virtual	void	AttachedToWindow();
	virtual void	Draw(BRect clip);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage* message);
	virtual	void	MouseUp(BPoint where);
	/* Quick hack, right now I really don't use much from BControl.
	 */
	void			SetButtonState(bool pressed);
	bool			ButtonState() const;
	
private:
	typedef BControl	inherited;
	const BBitmap*		mBmNormal;
	const BBitmap*		mBmPressed;

	bool				mPressed;
	bool				mSwitched;
	bool				mDrawFromSwitched;
};

#endif
