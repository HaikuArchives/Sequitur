/* SeqTempoControl.h
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
 * 05.08.00		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQTEMPOCONTROL_H
#define SEQUITUR_SEQTEMPOCONTROL_H

#include <be/interface/Bitmap.h>
#include <be/interface/Control.h>

/***************************************************************************
 * SEQ-TEMPO-CONTROL
 * This control displays an LCD-style tempo value, and can be edited by
 * clicking and dragging.  It assumes that an ArpImageManagerI object has
 * been implemented, and that it stores images LDigit0 through LDigit9.
 ***************************************************************************/

class SeqTempoControl : public BView
{
public:
	SeqTempoControl(BPoint leftTop,
					const char* name,
					BMessage* message,
					int32 resizeMask,
					float minTempo,
					float maxTempo,
					float initialTempo);
	virtual ~SeqTempoControl();

	float			Tempo() const;
	void			SetTempo(float tempo);

	virtual void	AttachedToWindow();
	virtual	void	Draw(BRect clip);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint pt);
	virtual	void	MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage* msg);
	
	/* Return true if the user is currently operating
	 * this control.
	 */
	bool			IsTracking() const;
	
private:
	typedef BView		inherited;
	float				mMinTempo, mMaxTempo;
	float				mTempo;
	/* The message sent out whenever the mouse moves.
	 */
	BMessage*			mMessage;

	BRect				mWholeBounds;
	BRect				mFractionalBounds;	
	/* When the user presses the mouse, record which
	 * area they've clicked (if any).
	 */
	enum {
		WHOLE			= 0x00000001,
		FRACTIONAL		= 0x00000002
	};
	uint32				mMouseDown;
	int32				mWholePart;
	int32				mFractionPart;
	/* This BRect stores the area around the user's view of the int I am
	 * displaying.  It is used during the mouse move -- if the user moves
	 * back into the original area, the value will be what it was when they
	 * clicked down.
	 */
	BRect				mBaseRect;

	void	DrawOn(BView* view, BRect clip);
	void	DrawWholeTempoOn(BView* view, BPoint leftTop);
	void	DrawFractionalTempoOn(BView* view, BPoint leftTop);
	void	Invoke();
};

#endif
