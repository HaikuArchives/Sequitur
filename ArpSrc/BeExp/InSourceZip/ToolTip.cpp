#include <ToolTip.h>

#include <Message.h>
#include <MessageRunner.h>

#include <Bitmap.h>
#include <Picture.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>

#include <Autolock.h>
#include <Debug.h>
#include <String.h>

#include <cstdlib>
#include <cstdio>
#include <memory>

#include <experimental/BitmapTools.h>

namespace BPrivate {

// --------------------------- TipView ---------------------------

class TipView : public BView
{
public:
	TipView(BRect frame, const char* name, const BToolTipInfo* info,
			uint32 resizeMask = B_FOLLOW_NONE,
			uint32 flags = B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
		: BView(frame, name, resizeMask, flags)
	{
		if( info ) {
			fTip = info->Text();
			SetFont(info->Font());
			SetLowColor(info->FillColor());
			SetHighColor(info->TextColor());
		}
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	virtual	void Draw(BRect /*updateRect*/)
	{
		static rgb_color shine = { 255, 255, 255, 255 };
		static rgb_color shadow = { 0, 0, 0, 255 };
		rgb_color text = HighColor();

		BRect b(Bounds());

		SetHighColor(shadow);
		StrokeRect(b);
		b.InsetBy(1, 1);

		SetHighColor(mix_color(LowColor(), shine, 128+64));
		StrokeLine(BPoint(b.left, b.bottom), BPoint(b.left, b.top));
		StrokeLine(BPoint(b.left, b.top), BPoint(b.right, b.top));
		SetHighColor(mix_color(LowColor(), shadow, 128-64));
		StrokeLine(BPoint(b.right, b.top), BPoint(b.right, b.bottom));
		StrokeLine(BPoint(b.right, b.bottom), BPoint(b.top+1, b.bottom));

		FillRect(BRect(b.left+1, b.top+1, b.right-1, b.bottom-1), B_SOLID_LOW);

		font_height fh;
		GetFontHeight(&fh);
		SetHighColor(text);

		DrawString(fTip.String(), BPoint(b.left+2, b.top+1+fh.ascent));
	}

	virtual void GetPreferredSize(float* width, float* height)
	{
		font_height fh;
		GetFontHeight(&fh);
		*height = floor( fh.ascent+fh.descent + 4 + .5 );
		*width = floor( StringWidth(fTip.String()) + 6 + .5 );
	}

	BPoint TextOrigin() const
	{
		font_height fh;
		GetFontHeight(&fh);
		return BPoint(3, floor(fh.ascent+2+.5));
	}

private:
	BString fTip;
};

// --------------------------- TipWindow ---------------------------

class TipWindow : public BWindow
{
public:
	TipWindow(BToolTip& owner)
		: BWindow(BRect(-100, -100, -90, -90),
				  "Tool Tip",
				  B_NO_BORDER_WINDOW_LOOK,
				  B_FLOATING_ALL_WINDOW_FEEL,
				  B_NOT_MOVABLE|B_NOT_CLOSABLE|B_NOT_ZOOMABLE
				  |B_NOT_MINIMIZABLE|B_NOT_RESIZABLE
				  |B_AVOID_FOCUS|B_NO_WORKSPACE_ACTIVATION
				  |B_WILL_ACCEPT_FIRST_CLICK|B_ASYNCHRONOUS_CONTROLS),
		  fOwner(owner),
		  fDrawer(Bounds(), "drawer", B_FOLLOW_ALL, B_WILL_DRAW),
		  fState(S_OFF),
		  fShowTime(1500*1000), fHideTime(10000*1000), fSettleTime(1000*1000),
		  fStateTimer(0), fAnim(0), fTip(0),
		  fBackPic(0), fForePic(0), fMixPic(0),
          fInline(false), fCurAlpha(0), fDestAlpha(0)
	{
		fDrawer.SetViewColor(B_TRANSPARENT_COLOR);
		AddChild(&fDrawer);
		Run();
	}
	~TipWindow()
	{
		fOwner.WindowGone(this);
		fDrawer.RemoveSelf();
		StopStateTimer();
		DestroyTip(true);
	}

	void ShowTip(BMessenger source)
	{
		BAutolock l(this);
		switch( fState ) {
			case S_OFF:
				PRINT(("ShowTip(): S_OFF to S_HOVER\n"));
				fState = S_HOVER;
				StartStateTimer(fShowTime);
				break;
			case S_HOVER:
				PRINT(("ShowTip(): S_HOVER to S_HOVER\n"));
				StartStateTimer(fShowTime);
				break;
			case S_REQUEST:
			case S_SHOWN:
			case S_SETTLE:
				PRINT(("ShowTip(): S_REQUEST or S_SHOWN or S_SETTLE to S_SETTLE\n"));
				fState = S_REQUEST;
				StartStateTimer(fHideTime);
				RequestTipInfo();
				break;
			default:
				TRESPASS();
		}

		fSource = source;
	}

	void SetToolTipInfo(BRect region, BToolTipInfo* info)
	{
		BAutolock l(this);
		if( fState != S_REQUEST ) {
			// If this wasn't called in response to a request, just
			// perform a show immediately.
			StartStateTimer(fHideTime);
		}
		fState = S_SHOWN;
		CreateTip(region, info);
	}

	void CursorMoved(BPoint /*where*/, BPoint delta)
	{
		if( fState != S_HOVER ) return;

		BAutolock l(this);
		if( delta.x > 3 || delta.x < -3 || delta.y > 3 || delta.y < -3 ) {
			PRINT(("Cursor moved enough to restart tool tip.\n"));
			StartStateTimer(fShowTime);
		}
	}

	void HideTip()
	{
		BAutolock l(this);
		switch( fState ) {
			case S_OFF:
				PRINT(("HideTip(): S_OFF to S_OFF\n"));
				StopStateTimer();
				return;
			case S_HOVER:
				PRINT(("HideTip(): S_HOVER to S_OFF\n"));
				fState = S_OFF;
				StopStateTimer();
				break;
			case S_REQUEST:
			case S_SHOWN:
				PRINT(("HideTip(): S_REQUEST or S_SHOWN to S_SETTLE\n"));
				fState = S_SETTLE;
				StartStateTimer(fSettleTime);
				break;
			case S_SETTLE:
				PRINT(("HideTip(): S_SETTLE to S_SETTLE\n"));
				break;
			default:
				TRESPASS();
		}

		DestroyTip();
	}

	void KillTip()
	{
		BAutolock l(this);
		switch( fState ) {
			case S_OFF:
				PRINT(("KillTip(): S_OFF to S_OFF\n"));
				StopStateTimer();
				return;
			case S_HOVER:
				PRINT(("KillTip(): S_HOVER to S_OFF\n"));
				fState = S_OFF;
				StopStateTimer();
				break;
			case S_REQUEST:
			case S_SHOWN:
			case S_SETTLE:
				PRINT(("KillTip(): S_REQUEST or S_SHOWN or S_SETTLE to S_OFF\n"));
				fState = S_OFF;
				StopStateTimer();
				break;
			default:
				TRESPASS();
		}

		DestroyTip(true);
	}

	void TransitionState()
	{
		StopStateTimer();

		bigtime_t next_time = 0;

		switch( fState ) {
			case S_OFF:
				PRINT(("Transition: S_OFF to S_OFF\n"));
				DestroyTip(true);
				break;
			case S_HOVER:
				PRINT(("Transition: S_HOVER to S_REQUEST\n"));
				RequestTipInfo();
				fState = S_REQUEST;
				next_time = fHideTime;
				break;
			case S_REQUEST:
			case S_SHOWN:
				PRINT(("Transition: S_REQUEST or S_SHOWN to S_SETTLE\n"));
				DestroyTip();
				fState = S_SETTLE;
				next_time = fSettleTime;
				break;
			case S_SETTLE:
				PRINT(("Transition: S_SETTLE to S_OFF\n"));
				DestroyTip(true);
				fState = S_OFF;
				break;
			default:
				TRESPASS();
		}

		if( next_time > 0 ) StartStateTimer(next_time);
	}

	void RequestTipInfo()
	{
		if( fSource.IsValid() ) {
			BMessage msg(B_REQUEST_TOOL_INFO);
			fSource.SendMessage(&msg);
		}
	}

	void StartStateTimer(bigtime_t when)
	{
		StopStateTimer();
		if( when <= 0 ) return;
		fStateTimer = new BMessageRunner(BMessenger(this),
										 new BMessage('puls'),
										 when);
	}

	void StopStateTimer()
	{
		delete fStateTimer;
		fStateTimer = 0;
	}

	virtual void DispatchMessage(BMessage* msg, BHandler* handler)
	{
		switch( msg->what ) {
			case B_MOUSE_MOVED: {
				BPoint pnt;
				if( msg->FindPoint("where", &pnt) == B_OK ) {
					ConvertToScreen(&pnt);
					PRINT(("Checking if (%.2f,%.2f) is in (%.2f,%.2f)-(%.2f-%.2f)\n",
							pnt.x, pnt.y,
							fInlineRegion.left, fInlineRegion.top,
							fInlineRegion.right, fInlineRegion.bottom));
					if( !fInlineRegion.Contains(pnt) ) {
						if( fDestAlpha != 0.0 ) HideTip();
						return;
					}
				}
				if( fInline && fState == S_SETTLE && !IsHidden() ) {
					PRINT(("Mouse move in inline tip: showing.\n"));
					fDestAlpha = 1.0;
					fState = S_SHOWN;
					StartStateTimer(fHideTime);
					StartAnimation();
				}
			} break;

			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
				// TO DO: Forward message to underlying window.
				KillTip();
				return;
		}

		inherited::DispatchMessage(msg, handler);
	}

	virtual void MessageReceived(BMessage* msg)
	{
		switch( msg->what ) {
			case 'anim':
				if( fDestAlpha > 0.5 ) {
					fCurAlpha += .1;
					if( fCurAlpha >= fDestAlpha ) {
						StopAnimation();
						fCurAlpha = 1.0;
						return;
					}
				} else {
					fCurAlpha -= .15;
					if( fCurAlpha <= fDestAlpha ) {
						StopAnimation();
						fCurAlpha = 0.0;
						return;
					}
				}

				PRINT(("Drawing with alpha=%f, tip=%p\n",
						fCurAlpha, fTip));
				if( fMixPic && fBackPic && fForePic ) {
					mix_bitmaps(fMixPic, fBackPic, fForePic,
								(uint8)(255*fCurAlpha+.5));
					fDrawer.DrawBitmap(fMixPic);
				}
				break;

			case 'puls':
				TransitionState();
				break;

			default:
				inherited::MessageReceived(msg);
				break;
		}
	}

	void CreateTip(BRect region, BToolTipInfo* info, bool now=false)
	{
		// Grab the current screen bitmap, in case our new tip
		// overlaps it anywhere -- we can use it to recover what was
		// originally on the screen at that point.
		BBitmap* prev_pic = fBackPic;
		fBackPic = 0;

		fDestAlpha = 0.0;
		StopAnimation();

		/*if( info->View() ) {
			fTip = info->DetachView();
		} else*/ if( info->Text() && *info->Text() ) {
			fTip = new TipView(Bounds(), "TipView", info);
		} else {
			DestroyTip();
		}

		fInline = info->Inline();
		fInlineRegion = region;

		BScreen s(this);
		BRect sb(s.Frame());

		float w, h;
		fTip->GetPreferredSize(&w, &h);
		if( w > sb.Width() ) w = sb.Width();
		if( h > sb.Height() ) h = sb.Height();

		float x = (region.left+region.right)/2 - w/2;
		float y = region.bottom + 6;

		TipView* tipView = NULL;
		if (info->HasOrigin() && (tipView=dynamic_cast<TipView*>(fTip)) != NULL) {
			// Tool tip should be placed with text at this
			// location.
			const BPoint tipOrigin = tipView->TextOrigin();
			x = region.left + info->Origin().x - tipOrigin.x;
			y = region.top + info->Origin().y - tipOrigin.y;
		}

		#if 0
		if( info->Inline() ) {
			x = region.left;
			y = region.top;
			if( w < region.Width() ) w = region.Width();
			if( h < region.Height() ) h = region.Height();
		}
		#endif

		ResizeTo(w, h);
		fTip->ResizeTo(w, h);

		if( x < sb.left ) x = sb.left;
		if( (x+w) > sb.right ) x = sb.right-w;
		if( y < sb.top ) y = sb.top;
		if( info->Inline() ) {
			if( (y+h) > sb.bottom ) y = sb.bottom-h;
		} else {
			if( (y+h) > sb.bottom ) y = region.top - h - 6;
		}
		if( y < sb.top ) y = sb.top;
		x = floor(x+.5);
		y = floor(y+.5);

		delete info;
		info = 0;

		MoveTo(x, y);

		fDestAlpha = 1.0;

		BRect bbnd(Bounds());
		bbnd.bottom++;

		BRect wfrm(Frame());
		fBackPic = new BBitmap(bbnd, 0,
							   s.ColorSpace(), B_ANY_BYTES_PER_ROW,
							   s.ID());
		s.ReadBitmap(fBackPic, false, &wfrm);
		if( !fBackPic ) {
			PRINT(("*** ERROR GETTING SCREEN BITMAP.\n"));
			fCurAlpha = fDestAlpha;
			StopAnimation();
			delete prev_pic;
			return;
		}

		// If there was still a tool tip displayed, and the new tip overlaps
		// the previous one, copy the screen bitmap we had for that area into
		// our new screen bitmap.
		if( prev_pic ) {
			if( wfrm.Intersects(fBackRegion) ) {
				BRect i = wfrm&fBackRegion;
				BPoint p(0, 0);
				if( wfrm.left < fBackRegion.left ) {
					p.x = fBackRegion.left-wfrm.left;
					i.OffsetTo(0, i.top);
				} else {
					i.OffsetTo(wfrm.left-fBackRegion.left, i.top);
				}
				if( wfrm.top < fBackRegion.top ) {
					p.y = fBackRegion.top-wfrm.top;
					i.OffsetTo(i.left, 0);
				} else {
					i.OffsetTo(i.left, wfrm.top-fBackRegion.top);
				}
				copy_bitmap(fBackPic, prev_pic, i, p);
			}
			delete prev_pic;
			prev_pic = 0;
		}

		fBackRegion = wfrm;

		fForePic = new BBitmap(bbnd,
							   B_BITMAP_CLEAR_TO_WHITE|B_BITMAP_ACCEPTS_VIEWS,
							   fBackPic->ColorSpace(), B_ANY_BYTES_PER_ROW,
							   s.ID());
		fForePic->Lock();
		fForePic->AddChild(fTip);
		fTip->PushState();
		fTip->Draw(fTip->Bounds());
		fTip->PopState();
		fTip->Sync();
		fTip->RemoveSelf();
		fForePic->Unlock();

		fMixPic = new BBitmap(bbnd, 0, fBackPic->ColorSpace(),
							  B_ANY_BYTES_PER_ROW, s.ID());

		if( !now ) {
			StartAnimation();
		} else {
			fCurAlpha = 1.0;
			StopAnimation();
		}
	}

	void DestroyTip(bool now=false)
	{
		if( fTip ) {
			fTip->RemoveSelf();
			delete fTip;
			fTip = 0;
		}

		fDestAlpha = 0.0;

		if( IsHidden() ) return;

		if( !now ) {
			StartAnimation();
		} else {
			fCurAlpha = 0.0;
			StopAnimation();
		}
	}

	void StartAnimation()
	{
		delete fAnim;
		fAnim = 0;
		fAnim = new BMessageRunner(BMessenger(this),
									new BMessage('anim'), 50*1000);
		if( IsHidden() ) Show();
	}

	void StopAnimation()
	{
		delete fAnim;
		fAnim = 0;

		if( fDestAlpha > 0.5 ) {
			if( fTip ) {
				if( fTip->Window() ) {
					fTip->RemoveSelf();
				}
				fDrawer.AddChild(fTip);
				if( IsHidden() ) Show();
			}

		} else {
			if( !IsHidden() ) Hide();

			delete fBackPic;
			fBackPic = 0;
			delete fForePic;
			fForePic = 0;
			delete fMixPic;
			fMixPic = 0;
		}
	}

private:
	typedef BWindow inherited;

	BToolTip& fOwner;
	BView fDrawer;

	enum tip_state {
		S_OFF,
		S_HOVER,
		S_REQUEST,
		S_SHOWN,
		S_SETTLE
	};

	tip_state fState;
	bigtime_t fShowTime;
	bigtime_t fHideTime;
	bigtime_t fSettleTime;

	BMessenger fSource;

	BMessageRunner* fStateTimer;
	BMessageRunner* fAnim;
	BView* fTip;
	BBitmap* fBackPic;
	BBitmap* fForePic;
	BBitmap* fMixPic;
	BBitmap* fPrevPic;
	BRect fBackRegion;
	BRect fInlineRegion;
	bool fInline;
	float fCurAlpha;
	float fDestAlpha;
};

}	// namespace BPrivate

using namespace BPrivate;

// --------------------------- BToolTipInfo ---------------------------

BToolTipInfo::BToolTipInfo()
	: fFont(*be_plain_font), fInline(false), fHasOrigin(false), fView(0)
{
	fFillColor.red = 255;
	fFillColor.green = 255;
	fFillColor.blue = 0;
	fFillColor.alpha = 255;

	fTextColor.red = fTextColor.green = fTextColor.blue = 0;
	fTextColor.alpha = 255;
}

BToolTipInfo::~BToolTipInfo()
{
	delete fView;
}

void BToolTipInfo::SetText(const char* text)
{
	fText = text;
}

const char* BToolTipInfo::Text() const
{
	return fText.String();
}

void BToolTipInfo::SetFont(const BFont* font)
{
	fFont = *font;
}

const BFont* BToolTipInfo::Font() const
{
	return &fFont;
}

void BToolTipInfo::SetFillColor(rgb_color color)
{
	fFillColor = color;
}

rgb_color BToolTipInfo::FillColor() const
{
	return fFillColor;
}

void BToolTipInfo::SetTextColor(rgb_color color)
{
	fTextColor = color;
}

rgb_color BToolTipInfo::TextColor() const
{
	return fTextColor;
}

void BToolTipInfo::SetInline(bool state)
{
	fInline = state;
}

bool BToolTipInfo::Inline() const
{
	return fInline;
}

void BToolTipInfo::SetOrigin(BPoint origin)
{
	fOrigin = origin;
	fHasOrigin = true;
}

void BToolTipInfo::ClearOrigin()
{
	fHasOrigin = false;
}

bool BToolTipInfo::HasOrigin() const
{
	return fHasOrigin;
}

BPoint BToolTipInfo::Origin() const
{
	return fOrigin;
}

void BToolTipInfo::SetView(BView* view)
{
	delete fView;
	fView = view;
}

BView* BToolTipInfo::View() const
{
	return fView;
}

BView* BToolTipInfo::DetachView()
{
	BView* v = fView;
	fView = 0;
	return v;
}

void BToolTipInfo::_ReservedToolTipInfo1() {}
void BToolTipInfo::_ReservedToolTipInfo2() {}
void BToolTipInfo::_ReservedToolTipInfo3() {}
void BToolTipInfo::_ReservedToolTipInfo4() {}
void BToolTipInfo::_ReservedToolTipInfo5() {}
void BToolTipInfo::_ReservedToolTipInfo6() {}
void BToolTipInfo::_ReservedToolTipInfo7() {}
void BToolTipInfo::_ReservedToolTipInfo8() {}
void BToolTipInfo::_ReservedToolTipInfo9() {}
void BToolTipInfo::_ReservedToolTipInfo10() {}
void BToolTipInfo::_ReservedToolTipInfo11() {}
void BToolTipInfo::_ReservedToolTipInfo12() {}
void BToolTipInfo::_ReservedToolTipInfo13() {}
void BToolTipInfo::_ReservedToolTipInfo14() {}
void BToolTipInfo::_ReservedToolTipInfo15() {}
void BToolTipInfo::_ReservedToolTipInfo16() {}

// --------------------------- BToolTipable ---------------------------

BToolTipable::BToolTipable(BView& owner, const char* text)
	: fOwner(owner), fText(text)
{
}
BToolTipable::~BToolTipable()
{
}

void BToolTipable::SetText(const char* text)
{
	fText = text;
}

const char* BToolTipable::Text() const
{
	return fText.String();
}

status_t BToolTipable::GetToolTipInfo(BPoint /*where*/, BRect* out_region,
									  BToolTipInfo* out_info)
{
	if( fText.Length() <= 0 ) {
		*out_region = BRect();
		return B_OK;
	}

	*out_region = fOwner.Frame().OffsetToCopy(0, 0);
	if( out_info ) out_info->SetText(fText.String());
	return B_OK;
}

// --------------------------- BToolTip ---------------------------

BToolTip::BToolTip()
	: fTip(0)
{
}

BToolTip::~BToolTip()
{
	if (fTip && fTip->Lock()) {
		fTip->Close();
	}
}

static BToolTip global_tip;

BToolTip* BToolTip::Default()
{
	return &global_tip;
}

status_t BToolTip::ShowTip(BMessenger who)
{
	BAutolock l(fAccess);
	HideTip(BMessenger());
	if (who.IsValid()) {
		fCurrentOwner = who;
		Tip()->ShowTip(fCurrentOwner);
	}
	return B_OK;
}

status_t BToolTip::CursorMoved(BMessenger who, BPoint where, BPoint delta)
{
	BAutolock l(fAccess);
	if( fCurrentOwner != who ) return B_BAD_VALUE;
	if( fTip ) fTip->CursorMoved(where, delta);
	return B_OK;
}

BToolTipInfo* BToolTip::NewToolTipInfo() const
{
	BToolTipInfo* info = new BToolTipInfo();
	return info;
}

status_t BToolTip::SetToolTipInfo(BMessenger who, BRect region, BToolTipInfo* info)
{
	BAutolock l(fAccess);
	if( fCurrentOwner != who ) {
		delete info;
		return B_BAD_VALUE;
	}
	fTip->SetToolTipInfo(region, info);
	return B_OK;
}

status_t BToolTip::HideTip(BMessenger who)
{
	BAutolock l(fAccess);
	if( who.IsValid() && fCurrentOwner != who ) return B_BAD_VALUE;
	if( fTip ) fTip->HideTip();
	fCurrentOwner = BMessenger();
	return B_OK;
}

status_t BToolTip::KillTip(BMessenger who)
{
	BAutolock l(fAccess);
	if( who.IsValid() && fCurrentOwner != who ) return B_BAD_VALUE;
	if( fTip ) fTip->KillTip();
	fCurrentOwner = BMessenger();
	return B_OK;
}

status_t BToolTip::RemoveOwner(BMessenger who)
{
	BAutolock l(fAccess);
	if( fCurrentOwner == who ) {
		if( fTip ) fTip->HideTip();
		fCurrentOwner = BMessenger();
		return B_OK;
	}
	return B_BAD_VALUE;
}

TipWindow* BToolTip::Tip()
{
	if( !fTip ) fTip = new TipWindow(*this);
	return fTip;
}

void BToolTip::WindowGone(TipWindow* /*w*/)
{
	fTip = 0;
}

void BToolTip::_WatchMyToolTip1() {}
void BToolTip::_WatchMyToolTip2() {}
void BToolTip::_WatchMyToolTip3() {}
void BToolTip::_WatchMyToolTip4() {}
void BToolTip::_WatchMyToolTip5() {}
void BToolTip::_WatchMyToolTip6() {}
void BToolTip::_WatchMyToolTip7() {}
void BToolTip::_WatchMyToolTip8() {}
void BToolTip::_WatchMyToolTip9() {}
void BToolTip::_WatchMyToolTip10() {}
void BToolTip::_WatchMyToolTip11() {}
void BToolTip::_WatchMyToolTip12() {}
void BToolTip::_WatchMyToolTip13() {}
void BToolTip::_WatchMyToolTip14() {}
void BToolTip::_WatchMyToolTip15() {}
void BToolTip::_WatchMyToolTip16() {}

// --------------------------- BToolTipFilter ---------------------------

BToolTipFilter::BToolTipFilter(BToolTip& tip)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	  fTip(tip), fButtons(0), fShower(0)
{
}

BToolTipFilter::BToolTipFilter()
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	  fTip(*BToolTip::Default()), fButtons(0), fShower(0)
{
}

BToolTipFilter::~BToolTipFilter()
{
	fTip.RemoveOwner(fLooper);
}

filter_result BToolTipFilter::Filter(BMessage *message, BHandler **target)
{
	if( message->what == B_MOUSE_MOVED || message->what == B_MOUSE_DOWN
			|| message->what == B_MOUSE_UP ) {
		int32 but;
		if( message->FindInt32("buttons", &but) == B_OK ) {
			fButtons = but;
		}
	}

	switch( message->what ) {
		case B_REQUEST_TOOL_INFO: {
			SendToolTipInfo();
		} break;

		case B_MOUSE_MOVED: {
			//message->PrintToStream();
			if( fButtons ) {
				// While buttons are down, always hide tip.
				KillTip();
			} else {
				BPoint pnt;
				if( message->FindPoint("where", &pnt) == B_OK ) {
					BWindow* w = dynamic_cast<BWindow*>(Looper());
					if( w ) w->ConvertToScreen(&pnt);
					BView* v = dynamic_cast<BView*>(*target);
					MoveCursor(v, pnt);
				}
			}
		} break;

		case B_WINDOW_ACTIVATED:
		case B_SCREEN_CHANGED:
		case B_WINDOW_MOVED:
		case B_WINDOW_RESIZED:
		case B_WORKSPACES_CHANGED:
		case B_WORKSPACE_ACTIVATED:
		case B_ZOOM: {
			//message->PrintToStream();
			HideTip();
		} break;

		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED:
		case B_MOUSE_WHEEL_CHANGED: {
			//message->PrintToStream();
			KillTip();
		} break;
	}
	return B_DISPATCH_MESSAGE;
}

status_t BToolTipFilter::SendToolTipInfo()
{
	BAutolock l(Looper());

	if( !fShower ) return B_NO_INIT;

	// make sure the tipped view is still attached to the window.
	BWindow* w = dynamic_cast<BWindow*>(Looper());
	if( !w || !find_view(w->ChildAt(0), fShower) ) return B_BAD_VALUE;
	if( !w->IsActive() ) return B_OK;

	// try to get a BTipable interface for this view.
	BToolTipable* tipable = dynamic_cast<BToolTipable*>(fShower);
	if( !tipable ) return B_OK;

	// retrieve tip information.
	status_t err;
	BRect region;
	BToolTipInfo* info = fTip.NewToolTipInfo();
	err = tipable->GetToolTipInfo(fShower->ConvertFromScreen(fCursor),
								  &fRegion, info);
	fShower->ConvertToScreen(&fRegion);
	region = fRegion;

	if( err == B_OK ) fTip.SetToolTipInfo(fLooper, region, info);
	else HideTip();

	return err;
}

void BToolTipFilter::MoveCursor(BView* v, BPoint screen_loc)
{
	BWindow* w = dynamic_cast<BWindow*>(Looper());
	if( !w || !w->IsActive() ) return;

	BPoint last_loc = fCursor;
	fCursor = screen_loc;

#if 0
	PRINT(("last_loc=(%.2f,%.2f), screen_loc=(%.2f,%.2f), fCursor=(%.2f,%.2f)\n",
			last_loc.x, last_loc.y, screen_loc.x, screen_loc.y,
			fCursor.x, fCursor.y));
#endif

	if( fShower ) {
		// currently displaying a tool tip.
		if( fRegion.Contains(screen_loc) ) {
			// still in tip area -- let tip handle any cursor movement.
			fTip.CursorMoved(fLooper, screen_loc, screen_loc-last_loc);
			return;
		} else {
			// out of shown tip area -- hide.
			HideTip();
		}
	}

	// find the view under the cursor.
	BPoint window_loc(w->ConvertFromScreen(screen_loc));
	if( !v ) v = w->FindView(window_loc);
	BToolTipable* tipable = dynamic_cast<BToolTipable*>(v);
	while( v && !tipable ) {
		v = v->Parent();
		tipable = dynamic_cast<BToolTipable*>(v);
	}
	if( !v || !tipable ) return;

#if 0
	PRINT(("v=%p (%s), window_loc=(%.2f,%.2f), view_loc=(%.2f,%.2f)\n",
			v, v ? v->Name() : "--", window_loc.x, window_loc.y,
			v->ConvertFromScreen(screen_loc).x,
			v->ConvertFromScreen(screen_loc).y));
#endif

	status_t err;
	err = tipable->GetToolTipInfo(v->ConvertFromScreen(screen_loc), &fRegion);
	if( err != B_OK ) return;
	v->ConvertToScreen(&fRegion);
	if( !fRegion.Contains(screen_loc) ) return;

	fShower = v;
	if (!fLooper.IsValid()) fLooper = BMessenger(Looper());
	fTip.ShowTip(fLooper);
}

void BToolTipFilter::HideTip()
{
	fTip.HideTip(fLooper);
	fRegion = BRect();
	fShower = 0;
}

void BToolTipFilter::KillTip()
{
	fTip.KillTip(fLooper);
	fRegion = BRect();
	fShower = 0;
}

bool BToolTipFilter::find_view(BView* root, BView* which)
{
	while( root ) {
		if( root == which ) return true;
		BView* c = root->ChildAt(0);
		if( c && find_view(c, which) ) return true;
		root = root->NextSibling();
	}

	return false;
}
