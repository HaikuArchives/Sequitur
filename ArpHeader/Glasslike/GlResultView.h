/* GlResultView.h
 * Copyright (c)2004 by Eric Hackborn.
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
 *	-¢ None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2004.01.31				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLASSLIKE_RESULTVIEW_H
#define GLASSLIKE_RESULTVIEW_H

#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ViewTools.h>
class GlResultCache;

/***************************************************************************
 * GL-RESULT-VIEW
 * I display the results of the current node.
 ***************************************************************************/
class GlResultView : public BView
{
public:
	GlResultView(BRect frame, GlResultCache& result);
	virtual ~GlResultView();
	
	virtual	void		AttachedToWindow();
	virtual void		MessageReceived(BMessage* msg);

	status_t			SetBitmap(const ArpBitmap* bm);
	// Take ownership of the bitmap
	status_t			TakeBitmap(ArpBitmap* bm);
	status_t			SaveImage(const BString16& filename, int32 format);
	
protected:
	virtual void		DrawOn(BView* view, BRect clip);
	
private:
	typedef BView		inherited;
	GlResultCache&		mResult;
	ArpBitmap*			mBitmap;
	bool				mInited;

	status_t			ResultChanged();

	status_t			SetFrom(BString16& fn);

public:
	virtual void		Draw(BRect clip);
};

#endif
