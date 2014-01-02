#include <ArpCore/ArpCoreDefs.h>
#include <stdio.h>
#include <be/interface/Window.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpPainter.h>
#include <ArpInterface/ArpPrefs.h>
#include <GlPublic/GlImage.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlResultCache.h>
#include <Glasslike/GlResultView.h>
#include <Glasslike/SZ.h>

/*************************************************************************
 * GL-RESULT-VIEW
 *************************************************************************/
GlResultView::GlResultView(BRect frame, GlResultCache& result)
		: inherited(frame, "preview", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),// | B_ACCEPTS_DROPS),
		  mResult(result), mBitmap(0), mInited(false)
{
}

GlResultView::~GlResultView()
{
	delete mBitmap;
}

void GlResultView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	mResult.SetTarget(BMessenger(this));
}

void GlResultView::MessageReceived(BMessage* msg)
{
	if (msg->WasDropped() || msg->what == B_REFS_RECEIVED) {
		if (Window()) {
			msg->what = GL_PREVIEW_IMAGE_DROPPED;
			Window()->PostMessage(msg);
		}
		return;
	}
	
	switch (msg->what) {
		case GL_RESULT_CHANGED_MSG:
			ResultChanged();
			break;
		case GL_RECACHE_MSG:
			if (mBitmap) {
				delete mBitmap;
				mBitmap = 0;
				this->Invalidate();
			}
			break;
		case GL_INVALIDATE_MSG:
			this->Invalidate();
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

status_t GlResultView::SetBitmap(const ArpBitmap* bm)
{
	mInited = true;
	delete mBitmap;
	mBitmap = NULL;
	if (bm) {
		mBitmap = new ArpBitmap(*bm);
		if (!mBitmap) return B_NO_MEMORY;
	}
	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
	return B_OK;
}

status_t GlResultView::TakeBitmap(ArpBitmap* bm)
{
	mInited = true;
	delete mBitmap;
	mBitmap = bm;
	if (Window()) Window()->PostMessage(GL_INVALIDATE_MSG, this);
	return B_OK;
}

status_t GlResultView::SaveImage(const BString16& filename, int32 format)
{
	if (!mBitmap) return B_OK;
	return mBitmap->Save(filename, format);
}

status_t GlResultView::ResultChanged()
{
	return TakeBitmap(mResult.NewDisplayedBitmap());
}

void GlResultView::Draw(BRect clip)
{
	BView*				into = this;
	ArpPOLISH("I'm getting some sort of RTTI error with the dynamic_cast (on window that's not inheriting ArpBC, at least)");
	ArpBitmapCache*		cache = NULL;
//	ArpBitmapCache*		cache = dynamic_cast<ArpBitmapCache*>(Window());
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, BRect(clip));
	if (cache) cache->FinishDrawing(into);
}

void GlResultView::DrawOn(BView* view, BRect clip)
{
	view->SetViewColor(Prefs().GetColor(ARP_BG_C));
	view->SetLowColor(Prefs().GetColor(ARP_BG_C));
	view->SetHighColor(Prefs().GetColor(ARP_BG_C));
	view->FillRect(clip);
	view->SetHighColor(Prefs().GetColor(ARP_FG_C));

	if (mBitmap && mBitmap->mBitmap) {
		view->DrawBitmap(mBitmap->mBitmap, BPoint(5, 5));
	} else if (!mInited) {
		float		fh = arp_get_font_height(this);
		float		padX = Prefs().GetFloat(ARP_PADX_F),
					padY = Prefs().GetFloat(ARP_PADY_F);
		view->DrawString(SZ(SZ_drop_image_here)->String(), BPoint(padX, padY + fh));
	}
}
