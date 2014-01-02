#include <interface/View.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpInterface/ArpPainter.h>

#if 1
/***************************************************************************
 * ARP-PAINTER
 ****************************************************************************/
ArpPainter::ArpPainter(ArpBitmap* bitmap)
		: mArpBitmap(bitmap), mViewBm(0), mView(0), mBm(0), mFlags(0)
{
	mBgC.red = mBgC.green = mBgC.blue = mBgC.alpha = 255;
	mFgC.red = mFgC.green = mFgC.blue = 0;
	mFgC.alpha = 255;
	if (mArpBitmap) mBm = mArpBitmap->mBitmap;
}

#if 0
	/* This method acts as a pass through to the view -- when this is the
	 * constructor, I don't own the view and won't delete it.  The bitmap
	 * related methods, like set/get pixel and resize, are not available.
	 */
	ArpPainter(BView* view);

ArpPainter::ArpPainter(BView* view)
		: mArpBitmap(0), mViewBm(0), mView(view), mBm(0), mFlags(VIEW_PAINTER)
{
}
#endif

ArpPainter::~ArpPainter()
{
ArpFINISH();
// How do I want to draw to bitmaps?
/*
	if (mViewBm) {
		mViewBm->Unlock();
		if (mView) {
			mViewBm->RemoveChild(mView);
			delete mView;
		}
		if (mArpBitmap) {
			delete mArpBitmap->mBitmap;
			mArpBitmap->mBitmap = new BBitmap(mViewBm);
		}
		delete mViewBm;
	}
*/
	mArpBitmap = 0;
	mBm = 0;
}

void ArpPainter::SetColour(ArpColourConstant constant, const rgb_color& c)
{
	if (constant == ARP_BACKGROUND_COLOUR) {
		mBgC = c;
		if (mView) mView->SetLowColor(mBgC);
	} else if (constant == ARP_FOREGROUND_COLOUR) {
		mFgC = c;
		if (mView) mView->SetHighColor(mFgC);
	}
}

void ArpPainter::SetFont(const ArpFont* font)
{
	if (!font) return;
	mFont = *font;
	if (mView) mView->SetFont(&(mFont.mFont));
}

BRect ArpPainter::StringBounds(const BString16& str) const
{
	BRect				b(0, 0, 0, 0);
ArpFINISH();
/*
	b.right = b.left + (mFont.mFont.StringWidth(str.String()));

	font_height			fh;
	mFont.mFont.GetHeight(&fh);
	b.bottom = b.top + (fh.ascent + fh.descent);
*/
	return b;
}

#if 0
BRect ArpPainter::StringBounds(const BString16& str) const
{
	BRect				b(0, 0, 0, 0);
	const char*			strs[1];
	escapement_delta	deltas[1];
	BRect				rects[1];
	strs[0] = str.String();
	/* For some reason, the bounding box isn't getting the width correctly.
	 */
	mFont.mFont.GetBoundingBoxesForStrings(strs, 1, B_SCREEN_METRIC, deltas, rects);
	b.SetW(mFont.mFont.StringWidth(str.String()));
	b.SetH(rects[0].Height());
	return b;
}
#endif

status_t ArpPainter::Resize(float w, float h)
{
	ArpVALIDATE(mArpBitmap, return B_ERROR);
	Uncache();
	BRect		b(0, 0, w, h);
	return Cache(new BBitmap(b, B_RGBA32, true));
}

status_t ArpPainter::DrawBitmap(const ArpBitmap& bm, const BPoint& pt)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;

	if (!bm.mBitmap) return B_ERROR;
// Do I ALWAYS want this?
drawing_mode		mode = mView->DrawingMode();
mView->SetDrawingMode(B_OP_ALPHA);
mView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	mView->DrawBitmap(bm.mBitmap, pt);
mView->SetDrawingMode(mode);
	return B_OK;
}

status_t ArpPainter::DrawBitmap(const ArpBitmap& bm, const BRect& srcR,
								const BRect& destR)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;

	if (!bm.mBitmap) return B_ERROR;

// Do I ALWAYS want this?
drawing_mode		mode = mView->DrawingMode();
mView->SetDrawingMode(B_OP_ALPHA);
mView->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	mView->DrawBitmap(bm.mBitmap, srcR, destR);
mView->SetDrawingMode(mode);	
	return B_OK;
}

status_t ArpPainter::DrawLine(const BPoint& pt1, const BPoint& pt2)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;
	mView->StrokeLine(pt1, pt2);
	return B_OK;
}

status_t ArpPainter::DrawString(const BString16& str, float x,
								float top, float bottom)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;
// This should totally NOT be necessary.  For some reason, the
// Set...Color() calls to mView in SetColour() are one step behind
// what they should be, but Flush() isn't doing anything.
/* FIX:  I Just noticed that SetColour() ISN'T SETTING THE color to
 * the incoming color -- that's probably the problem.
 */
mView->SetLowColor(mBgC);
mView->SetHighColor(mFgC);
	mView->DrawString(str.String(), BPoint(x, bottom));
	return B_OK;
}

status_t ArpPainter::StrokeRect(BRect r)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;
	
//	rgb_color		oldC = mView->HighColor();
//	mView->SetHighColor(c.mC);
	mView->StrokeRect(r);
//	mView->SetHighColor(oldC);
	return B_OK;
}

status_t ArpPainter::StrokeEllipse(BPoint center, float xRadius, float yRadius)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;
ArpFINISH();
/*
	mView->StrokeEllipse(center, xRadius, yRadius);
*/
	return B_OK;
}

status_t ArpPainter::FillRect(BRect r, const rgb_color& c)
{
	status_t		err = Cache();
	if (err != B_OK) return err;
	if (!mView) return B_ERROR;
	
	rgb_color		oldC = mView->HighColor();
	mView->SetHighColor(c);
	mView->FillRect(r);
	mView->SetHighColor(oldC);
	return B_OK;
}

status_t ArpPainter::Cache()
{
	if (mFlags&VIEW_PAINTER) {
		if (mView) return B_OK;
		return B_ERROR;
	}
	
	if (mViewBm) return B_OK;
	if (!mBm) return B_ERROR;
	return Cache(new BBitmap(mBm, true));
}

status_t ArpPainter::Cache(BBitmap* bm)
{
ArpFINISH();
return B_ERROR;
/*
	if (mFlags&VIEW_PAINTER) {
		if (mView) return B_OK;
		return B_ERROR;
	}
	
	mViewBm = bm;
	if (!mViewBm) return B_NO_MEMORY;
	mView = new BView(mViewBm->Bounds(), "v", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!mView) {
		delete mViewBm;
		mViewBm = 0;
		return B_NO_MEMORY;
	}
	mViewBm->AddChild(mView);
	mViewBm->Lock();
	mView->SetLowColor(mBgC);
	mView->SetHighColor(mFgC);
	mView->SetFont(&(mFont.mFont));
	return B_OK;
*/
}

void ArpPainter::Uncache()
{
	mBm = 0;
ArpFINISH();
/*
	if (mViewBm) {
		mViewBm->Unlock();
		if (mView) {
			mViewBm->RemoveChild(mView);
			delete mView;
			mView = 0;
		}
		delete mViewBm;
		mViewBm = 0;
	}
*/
}
#endif