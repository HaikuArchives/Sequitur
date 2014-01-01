/* ArpBitmapView.h
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
 * 2002.08.05			hackborn@angryredplanet.com
 * Created this file.
 */
 
#ifndef ARPINTERFACE_ARPBITMAPVIEW_H
#define ARPINTERFACE_ARPBITMAPVIEW_H

#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ViewTools.h>

/*******************************************************
 * ARP-BITMAP-VIEW
 * Display a bitmap.
 *******************************************************/
class ArpBitmapView : public BView
{
public:
	ArpBitmapView(	BRect frame, const char* name,
					uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~ArpBitmapView();

	virtual void		AttachedToWindow();

	void				SetBitmap(const ArpBitmap* bitmap);
	/* A little performance convenience -- the view takes
	 * ownership of the bitmap.
	 */
	void				TakeBitmap(ArpBitmap* bitmap);

	virtual void		Draw(BRect clip);

	enum {
		CENTER_F		= 0x00000001
	};
	void				SetFlags(uint32 flags);

protected:
	ArpBitmap*			mBitmap;

	virtual void		DrawOn(BView* view, BRect clip);
	bool				HasDrawingBitmap() const;
	
private:
	typedef BView		inherited;
	uint32				mFlags;
	
	void				DeleteAll();
};

#endif
