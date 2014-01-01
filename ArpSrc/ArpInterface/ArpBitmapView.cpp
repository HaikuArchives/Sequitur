#include <be/interface/Window.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpBitmapView.h>
#include <ArpInterface/ArpPrefs.h>

/*************************************************************************
 * OS-BITMAP-VIEW
 *************************************************************************/
ArpBitmapView::ArpBitmapView(BRect frame, const char* name, uint32 resizeMask)
		: inherited(frame, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
		  mBitmap(0), mFlags(0)
{
}

ArpBitmapView::~ArpBitmapView()
{
	DeleteAll();
}

void ArpBitmapView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void ArpBitmapView::SetBitmap(const ArpBitmap* bitmap)
{
	DeleteAll();
	if (bitmap) mBitmap = new ArpBitmap(*bitmap);
	if (Window()) Window()->PostMessage(ARP_INVALIDATE_MSG, this);
}

void ArpBitmapView::TakeBitmap(ArpBitmap* bitmap)
{
	DeleteAll();
	mBitmap = bitmap;
	if (Window()) Window()->PostMessage(ARP_INVALIDATE_MSG, this);
}

void ArpBitmapView::Draw(BRect clip)
{
	BView*				into = this;
	ArpPOLISH("I'm getting some sort of RTTI error with the dynamic_cast (on window that's not inheriting ArpBC, at least)");
	ArpBitmapCache*		cache = NULL;
//	ArpBitmapCache*		cache = dynamic_cast<ArpBitmapCache*>(Window());
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void ArpBitmapView::SetFlags(uint32 flags)
{
	mFlags = flags;
}

void ArpBitmapView::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(Prefs().GetColor(ARP_BG_C));
	view->FillRect(clip);

	BBitmap*			bm;
	if (mBitmap && (bm = mBitmap->mBitmap) != NULL) {
		BPoint			at(0, 0);
		if (mFlags&CENTER_F) {
			BRect		bitmapB = mBitmap->Bounds(),
						viewB = Bounds();
			float		bitmapW = bitmapB.Width(), bitmapH = bitmapB.Height(),
						viewW = viewB.Width(), viewH = viewB.Height();
			if (bitmapW < viewW) at.x = (viewW - bitmapW) * float(0.5);
			if (bitmapH < viewH) at.y = (viewH - bitmapH) * float(0.5);
		}
		view->DrawBitmap(bm, at);
	}
/*
	if (!mCachedBitmap) CacheBitmap();
	if (mCachedBitmap) {
drawing_mode		mode = view->DrawingMode();
view->SetDrawingMode(B_OP_ALPHA);
view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->DrawBitmap(mCachedBitmap, BPoint(0, 0));
view->SetDrawingMode(mode);	
	}
*/
}

bool ArpBitmapView::HasDrawingBitmap() const
{
	return mBitmap != 0;
}

void ArpBitmapView::DeleteAll()
{
	delete mBitmap;
	mBitmap = 0;
}