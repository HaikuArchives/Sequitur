/* ArpSplitterView.h
 * 
 * Copyright (c)2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com> or <hackborn@genomica.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- Only works vertically.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 06.16.00		hackborn
 * Created this file.
 */
 
#ifndef ARPINTERFACE_ARPSPLITTERVIEW_H
#define ARPINTERFACE_ARPSPLITTERVIEW_H

#include <interface/View.h>

/*************************************************************************
 * SEQ-SPLITTER-VIEW
 * A view you can drag to resize its neighbors.
 *************************************************************************/
class ArpSplitterView : public BView
{
public:
	ArpSplitterView(BRect frame,
					const char* name,
					uint32 resizeMask,
					uint32 flags,
					orientation direction);
	virtual	~ArpSplitterView();

	virtual	void	AttachedToWindow();
	virtual	void	Draw(BRect clip);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseUp(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage* message);

	enum {
		NO_DRAWING_FLAG		= 0x00000001,
		CAP_ENDS_FLAG		= 0x00000002
	};
	void			SetDrawingFlags(uint32 flags);

	void			MoveVerticalSplitter(float left);
	void			MoveHorizontalSplitter(float top);

private:
	typedef	BView	inherited;
	orientation		mDirection;
	uint32			mDrawingFlags;
	/* This is set to true in the mouse down, false in the mouse up.  Used
	 * to track if this was the view that the mouse motion was initiated in.
	 */
	bool			mMouseDown;
	/* The point at which the mouse down occurred.
	 */
	BPoint			mPointDown;
	BRect			mFrameDown;
};

#endif 
