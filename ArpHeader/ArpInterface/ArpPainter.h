/* ArpPainter.h
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
 * 2002.07.17			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef ARPINTERFACE_ARPPAINTER_H
#define ARPINTERFACE_ARPPAINTER_H

#include <ArpCore/String16.h>
#include <be/interface/View.h>
#include <ArpInterface/ArpInterfaceDefs.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpFont.h>

/***************************************************************************
 * ARP-PAINTER
 * I perform graphic operations on bitmaps.  I lock the bitmap -- no one
 * else can use it while I've got it.  I do not own the bitmap I'm supplied.
 ****************************************************************************/
#if 1
class ArpPainter
{
public:
	ArpPainter(ArpBitmap* bitmap);
	virtual ~ArpPainter();

	virtual void	SetColour(ArpColourConstant constant, const rgb_color& c);
	virtual void	SetFont(const ArpFont* font);
	
	BRect			StringBounds(const BString16& str) const;

	/* Currently, the new bitmap is blank.
	 */
	status_t		Resize(float w, float h);

	status_t		DrawBitmap(	const ArpBitmap& bm, const BPoint& pt);
	status_t		DrawBitmap(	const ArpBitmap& bm, const BRect& srcR,
								const BRect& destR);
	status_t		DrawLine(	const BPoint& pt1, const BPoint& pt2);
	status_t		DrawString(	const BString16& str, float x,
								float top, float bottom);

	status_t		StrokeRect(BRect r);
	status_t		StrokeEllipse(BPoint center, float xRadius, float yRadius);
/* Temp
 */
 	status_t		FillRect(BRect r, const rgb_color& c);

protected:
	rgb_color		mBgC, mFgC;
	ArpFont			mFont;

private:
	ArpBitmap*		mArpBitmap;
	/* The view bm exists for the drawing commands -- it is the main
	 * bm, so all operations should check it first.  If it doesn't
	 * exist, the check the regular bm.  When this class destructs,
	 * if there's a view bm, it needs to be copied (so that it doesn't
	 * accept views) and placed back in the ArpBitmap.
	 */
	BBitmap*		mViewBm;
	BView*			mView;
	BBitmap*		mBm;

	enum {
		VIEW_PAINTER	= 0x00000001
	};
	uint32			mFlags;


	status_t		Cache();
	status_t		Cache(BBitmap* bm);
	void			Uncache();
};
#endif

#endif
