#include <ArpCore/ArpCoreDefs.h>

#ifndef MAC_OS_X
#include <cstdio>
#include <cstring>
#include <malloc.h>
#endif

#include <interface/Window.h>
#include <StorageKit.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpFileNameControl.h>
#include <ArpInterface/ViewTools.h>

static rgb_color			BG_C;

/*************************************************************************
 * ARP-FILE-NAME-CONTROL
 *************************************************************************/
ArpFileNameControl::ArpFileNameControl(	BRect frame, const char* name,
										uint32 resizeMask)
		: inherited(frame, name, NULL, NULL, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS)
{
//	if (fileName) mFileName = fileName;
	BG_C.red = BG_C.green = BG_C.blue = 180;
	BG_C.alpha = 255;
}
ArpFileNameControl::~ArpFileNameControl()
{
}

void ArpFileNameControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void ArpFileNameControl::Draw(BRect clip)
{
	BView* into = this;
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}


void ArpFileNameControl::MessageReceived(BMessage* msg)
{
	if (msg->WasDropped() || msg->what == B_REFS_RECEIVED) {
		BString16		newFn(mFileName);
		entry_ref fileRef;
		if (msg->FindRef("refs", &fileRef) == B_OK) {
			BEntry		e(&fileRef);
			if (e.InitCheck() == B_OK && e.Exists()) {
				BPath	p(&e);
				if (p.InitCheck() == B_OK) {
					newFn = p.Path();
				}
			}
		}
		if (newFn != mFileName) {
			mFileName = newFn;
/*
			BMessenger		mg(mTarget);
			if (!mg.IsValid()) mg = BMessenger(this);
			if (mInvokedMsg && mg.IsValid()) {
*/
				Invoke();
/*
				BMessage		m(*mInvokedMsg);
				m.AddString("arp:file name", mFileName);
				mg.SendMessage(&m);
			}
*/
			Invalidate();
		}
		return;
	}
	
	inherited::MessageReceived(msg);
}

BString16 ArpFileNameControl::FileName() const
{
	return mFileName;
}

void ArpFileNameControl::SetFileName(const BString16* fileName)
{
	mFileName = fileName;
	Invoke();
}

void ArpFileNameControl::SetFileNameNoInvoke(const BString16* fileName)
{
	mFileName = fileName;
}

void ArpFileNameControl::DrawOn(BView* view, BRect clip)
{
	view->SetHighColor(BG_C);
	view->FillRect(clip);

	view->SetLowColor(BG_C);
	view->SetHighColor(0, 0, 0);

	BPoint			pt(0, 10);
	BString16		s;
	if (mFileName.Length() < 1) s = "Drop file here";
	else s = mFileName;
	float			w = arp_get_string_width(view, s.String());
	float			bw = Bounds().Width();
	if (w >= bw) view->TruncateString(&s, B_TRUNCATE_MIDDLE, bw);

	view->DrawString(s.String(), pt);
}
