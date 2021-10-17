#include <ArpCore/ArpCoreDefs.h>

#ifndef MAC_OS_X
#include <malloc.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#endif

#include <InterfaceKit.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpSupport/ArpIntFormatterI.h>
#include <ArpSupport/ArpIntToStringMapI.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpInlineTextView.h>
#include <ArpInterface/ArpIntControl.h>
#include <ArpInterface/ArpIntControlMotions.h>
#include <ArpInterface/ViewTools.h>
#include <ArpInterface/ArpPrefs.h>

static const BBitmap		*bgImage = 0;
static const uint32			EDIT_START_MSG	= 'strt';

/*************************************************************************
 * ARP-INT-CONTROL
 *************************************************************************/
ArpIntControl::ArpIntControl(	BRect frame,
								const char* name,
								const BString16* label,
								BMessage* message,
								uint32 rmask,
								uint32 flags)
		: inherited(frame, name, label, message, rmask, flags),
		  mMotion(0), mFlags(0), mDivider(0), mMin(-99), mMax(99),
		  mMap(NULL), mFormatter(NULL), mKeyStep(1), mKeyCount(0),
		  mTextCtrl(NULL), mEditRunner(NULL)
{
	if (label) {
		mLabel = label;
		mDivider = StringWidth(label);
		if ( mDivider > frame.Width() ) mDivider = frame.Width();
	}
	SetMotion( new ArpIntControlSmallMotion() );
//	if (!bgImage && HasImageManager()) bgImage = ImageManager().FindBitmap("BG");
//	SetBodyFill(ArpWest);
}

/*
	ArpIntControl(	const char* name,
					const BString16* label,
					BMessage* message,
					int32 min,
					int32 max);

ArpIntControl::ArpIntControl(	const char* name,
								const BString16* label,
								BMessage* message,
								int32 min,
								int32 max)
		: ArpLayoutControl(name, label, message),
		  mMotion(0), mFlags(0), mDivider(0), mMin(min), mMax(max),
		  mMap(NULL), mFormatter(NULL), mKeyStep(1), mKeyCount(0),
		  mTextCtrl(NULL), mEditRunner(NULL)
{
	if (label) {
		mLabel = label;
		mDivider = StringWidth(label);
	}
	SetMotion( new ArpIntControlSmallMotion() );
	if (!bgImage && HasImageManager())
		bgImage = ImageManager().FindBitmap("BG");
	SetBodyFill(ArpWest);
}
*/

ArpIntControl::~ArpIntControl()
{
	if (mMap) {
		mMap->Release();
		mMap = 0;
	}
	delete mFormatter;
	delete mMotion;
}

void ArpIntControl::SetStringMap(ArpIntToStringMapI* map)
{
	if (mMap) mMap->Release();
	mMap = map;
}

void ArpIntControl::SetFormatter(ArpIntFormatterI* formatter)
{
	delete mFormatter;
	mFormatter = formatter;
}

void ArpIntControl::SetLimits(int32 min, int32 max)
{
	mMin = min;
	mMax = max;
}

void ArpIntControl::SetMotion(ArpIntControlMotionI* motion)
{
	if (mMotion != 0) delete mMotion;
	mMotion = motion;
	if (mMotion == 0) mMotion = new ArpIntControlSmallMotion();
	if (mMotion != 0) mMotion->CacheData(this);
}

void ArpIntControl::SetDivider(float dividing_line)
{
	if (dividing_line != mDivider) {
		mDivider = dividing_line;
		Invalidate();
	}
}

int32 ArpIntControl::Min() const
{
	return mMin;
}

int32 ArpIntControl::Max() const
{
	return mMax;
}

void ArpIntControl::GetValueLabel(int32 value, BString16& str) const
{
	if (value < mMin) value = mMin;
	else if (value > mMax) value = mMax;
	
	BString16			str8;
	
	if (mMap) {
		BString16		name;
		if (mMap->NameForId(value, &name) == B_OK) str8 << name;
	} else if (mFormatter) {
		mFormatter->FormatInt(value, str8);
	}
	if (str8.Length() < 1) str8 << value;

	str = str8;
}


BRect ArpIntControl::ControlBounds() const
{
	BRect	b = Bounds();
	if (mDivider > 0) b.left = mDivider;
	return b;
}

void ArpIntControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetFontSize(10);
	if ( Parent() ) mViewColor = Parent()->ViewColor();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void ArpIntControl::Draw(BRect updateRect)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, updateRect);
	
	DrawOn(updateRect, into);
	if (cache) cache->FinishDrawing(into);
}

static void get_control_spacing(ArpIntControl* ctrl, float* minW, float* maxW)
{
	BString16		minStr;
	ctrl->GetValueLabel(ctrl->Min(), minStr);
	*minW = 0;
	if (minStr.Length() > 0) *minW = ctrl->StringWidth( minStr.String() );
	BString16		maxStr;
	ctrl->GetValueLabel(ctrl->Max(), maxStr);
	*maxW = 0;
	if (maxStr.Length() > 0) *maxW = ctrl->StringWidth( maxStr.String() );
}

void ArpIntControl::GetPreferredSize(float *width, float *height)
{
	float		fh = arp_get_font_height(this);
	/* Hmm...  I use the INT_CTRL_Y size to determine the height of
	 * all  my int controls, but for some reason, the layout engine,
	 * which is the only thing that makes use of this method, ends up
	 * with controls that are one pixel smaller than all the other instaces.
	 */
	float		ih = float(Prefs().GetInt32(ARP_INTCTRL_Y) + 1);
	*height = (fh > ih) ? fh : ih;
	
	float		minW, maxW;
	get_control_spacing(this, &minW, &maxW);
	if (minW > maxW) maxW = minW;
	*width = maxW + mDivider;
}

void ArpIntControl::KeyDown(const char* bytes, int32 numBytes)
{
	if (IsEnabled() == false) return;
	// Single byte values can be tested against ascii character codes...
	if (numBytes == 1) {
		AddAscii(bytes[0]);
	}
	inherited::KeyDown(bytes, numBytes);

	/* The mTransStep is the number of steps to take in the transform function.
	 * This is a simple little tweaked algorithm designed to work well both for
	 * small values and large.
	 */
	if( mKeyCount == 15 ) mKeyStep += 1;
	else if( mKeyCount == 25 ) mKeyStep += 5;
	else if( mKeyCount >= 35 ) mKeyStep += 5;
	if( mKeyStep > 1000 ) mKeyStep = 1000;
	mKeyCount++;
}

void ArpIntControl::KeyUp(const char *bytes, int32 numBytes)
{
	inherited::KeyUp(bytes, numBytes);
	mKeyStep = 1;
	mKeyCount = 0;
}

void ArpIntControl::MouseDown(BPoint pt)
{
	// We watch all mouse events while the text editor is shown
	// (to remove it when the mouse moves out of this view), so
	// ignore any button presses during that time.
	if (mTextCtrl) return;
	StopTimer();
	mDownPt = pt;
	if (IsEnabled() == false) return;

	/* Prepare the mouse motion sensor in case the user is going to
	 * start dragging.
	 */
	if ( ControlBounds().Contains(pt) ) {
		MakeFocus(true);
		SetMouseEventMask(	B_POINTER_EVENTS,
							B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY);
		if (mMotion) mMotion->MouseDown(pt);
	}
}

void ArpIntControl::MouseUp(BPoint pt)
{
	if (IsEnabled() == false) return;

	if (mMotion) mMotion->MouseUp(pt);
	if ( mDownPt == pt && Bounds().Contains(pt) ) {
		/* Start the edit timer in case the user is invoking an inline
		 * text view.
		 */
		bigtime_t	doubleClickTime;
		get_click_speed(&doubleClickTime);
		doubleClickTime *= 2;
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		StartTimer(BMessage(EDIT_START_MSG), doubleClickTime);
	} else Invoke();
}

void ArpIntControl::MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage* msg)
{
	BRect	cb = ControlBounds();
	if (!(cb.Contains(pt))) {
		StopEdit();
		StopTimer();
	}

	if (IsEnabled() == false) return;
	if (mMotion) mMotion->MouseMoved(pt, code, msg);
}

void ArpIntControl::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case EDIT_START_MSG:
			StartEdit();
			break;
		case INLINE_FINALUPDATE_MSG:
			StopEdit();
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void ArpIntControl::StartEdit()
{
	// If I've lost the active state while waiting, don't start editing.
	if ( !Window() || !Window()->IsActive() ) return;
	
	StopEdit(false);
	StopTimer();
	
	BRect		b = ControlBounds();
	float		borderL = b.left;
	float		bottomIndent = 0;
	float		stringIndent = 3;
	BPoint		origin( borderL + 3, b.bottom - bottomIndent - stringIndent );
	
	if (!mTextCtrl) {
		BFont font;
		GetFont(&font);
		mTextCtrl = new ArpInlineTextView(BMessenger(this),
										  "text:edit", &font,
										  origin.x, b.right-2, origin.y);
	}
	if (!mTextCtrl) return;
	
	SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	
	mTextCtrl->MoveOver(origin.x, b.right-2, origin.y);
	mTextCtrl->SetViewColor( Prefs().GetColor(ARP_INT_BGF_C));
	mTextCtrl->SetHighColor( Prefs().GetColor(ARP_INT_FGF_C));
	BString16		str;
	GetValueLabel(Value(), str);
	mTextCtrl->SetText( str.String() );
	AddChild(mTextCtrl);
}

void ArpIntControl::StopEdit(bool keepChanges)
{
	if (mTextCtrl) {

		if( keepChanges && mTextCtrl->HasChanged() ) {
			const BString16*	text = mTextCtrl->Text();
			if (text) {
				int32			val;
				if (ValueFromLabel(text, &val) == B_OK)
					SafeSetValue(val);
			}
		}
	
		SetEventMask(0, 0);
		
		mTextCtrl->RemoveSelf();
		delete mTextCtrl;
		mTextCtrl = NULL;
	}
}

void ArpIntControl::NotifyHook(int32 newValue)
{
	BMessage	msg(ARPMSG_INT_CONTROL_CHANGED);
	msg.AddInt32( "value", newValue );
	SendNotices(ARPMSG_INT_CONTROL_CHANGED, &msg);
}

void ArpIntControl::SafeSetValue(int32 newValue)
{
	if ( newValue < Min() ) newValue = Min();
	if ( newValue > Max() ) newValue = Max();

	if ( newValue != Value() ) NotifyHook(newValue);
	SetValue(newValue);	
}

void ArpIntControl::AddAscii(char byte)
{
	int32	newValue = Value();

	if (byte == B_UP_ARROW) newValue = Value() + mKeyStep;
	else if (byte == B_DOWN_ARROW) newValue = Value() - mKeyStep;
	else if (byte == '+') newValue = abs( Value() );
	else if (byte == '-') newValue = 0 - abs( Value() );
	else if (byte == B_SPACE) StartEdit();
	else return;

	if (newValue < Min()) newValue = Min();
	if (newValue > Max()) newValue = Max();
	if (newValue != Value()) {
		SetValue(newValue);
		NotifyHook(newValue);
	}
}

static bool is_all_zeroes(const BString16* str)
{
	ArpASSERT(str);
	int32		len = str->Length();
	/* Flippin' OS X and it's hidden string class means I don't
	 * know of a good way to access the characters, so I hardcode
	 * the common cases and default to the ugly way out.
	 */
	if (len < 1) return true;
	else if (len == 1) {
		if (BString16("0") == str) return true;
	} else if (len == 2) {
		if (BString16("00") == str) return true;
	} else if (len == 3) {
		if (BString16("000") == str) return true;
	}

	char*   t = str->AsAscii();
	if (!t) return true;
	bool		ans = true;
	for (int32 k = 0; k < len; k++) {
		if (t[k] != '0') {
			ans = false;
			break;
		}
	}
	delete[] t;
	return ans;

/*
	size_t	len = strlen(str);
	for (size_t k = 0; k < len; k++)
		if (str[k] != '0') return false;
	return true;
*/
}

status_t ArpIntControl::ValueFromLabel(const BString16* label, int32* answer) const
{
	ArpASSERT(label);
	if (mMap) {
		int32	val;
		if (mMap->IdForName(label, &val) == B_OK) {
			*answer = val;
			return B_OK;
		}
	}
	/* Since the atol function returns zero to mean that the string
	 * could not be translated, I need to do some special processing:
	 * if the label is nothing but zeroes, then the answer should be 0.
	 */
	if ( is_all_zeroes(label) ) {
		*answer = 0;
		return B_OK;
	}
//	int32	val = atol(label);
	int32	val = label->AsInt32();
	if (val == 0) return B_ERROR;
	*answer = val;
	return B_OK;
}

#if 0
void ArpIntControl::ComputeDimens(ArpDimens& cur_dimens)
{
	float vw=0, vh=0;
	GetPreferredSize(&vw,&vh);
	
	const float divw = BasicFont()->StringWidth(Label())
					 + BasicFont()->StringWidth("  ");
	vw = divw+12+StringWidth("WWWWWW");
	
	cur_dimens.X().SetTo(divw, vw-divw-12, vw-divw-12, ArpAnySize, 0);
	cur_dimens.Y().SetTo(0, vh, vh, vh, 0);
	
	//printf("Left: %f, Width: %f, Divider: %f\n",
	//		Frame().left, Frame().Width(), Divider());
	ArpUniDimens& dx = cur_dimens.X();
	float minb = dx.MinBody();
	float prefb = dx.PrefBody();
	float maxb = dx.MaxBody();
	minb = StringWidth("");
	prefb = StringWidth("WWW");
	maxb = StringWidth("WWWWWW");
	
	dx.SetBody(minb, prefb, maxb);
	dx.AddBody(12);
}

void ArpIntControl::LayoutView()
{
	inherited::LayoutView();
	SetDivider(BodyFrame().left - LayoutFrame().left);
}
#endif

void ArpIntControl::DrawOn(BRect updateRect, BView* view)
{
	BRect		cb = ControlBounds();
	float		textY = cb.top + Prefs().GetInt32(ARP_FULLFONT_Y);
	if (mDivider > 0) {
		view->SetHighColor(mViewColor);
		view->FillRect( BRect(0, cb.top, cb.left, cb.bottom) );
		if (mLabel.Length() > 0) {
			if ( IsFocus() ) view->SetHighColor( Prefs().GetColor(ARP_INT_FGF_C));
			else if ( !IsEnabled() ) view->SetHighColor( tint_color(Prefs().GetColor(ARP_INT_FG_C), B_LIGHTEN_1_TINT) );
			else view->SetHighColor( Prefs().GetColor(ARP_INT_FG_C) );
			view->SetLowColor(mViewColor);
			view->DrawString( mLabel.String(), BPoint(0, textY) );
		}
	}
	DrawControlBackgroundOn(cb, view);

	BPoint			drawPt(cb.left + 3, textY);
	if (IsEnabled() == false) {
		drawPt.x++;
		drawPt.y++;
	}
	BString16		str;
	GetValueLabel(Value(), str);
	SetValueColor(view);
	view->DrawString( str.String(), drawPt );
}

void ArpIntControl::DrawControlBackgroundOn(BRect bounds, BView* view)
{
	rgb_color	bg, fg;
	if (IsEnabled() == false) {
		bg = mViewColor;
		fg = tint_color(Prefs().GetColor(ARP_INT_FG_C), B_LIGHTEN_1_TINT);
		if (bgImage != 0) {
			DrawBitmapBackgroundOn(bounds, view, bgImage);
		} else {
			view->SetHighColor(bg);
			view->FillRect(bounds);
		}
		view->SetLowColor(bg);
	} else if (IsFocus()) {
		bg = Prefs().GetColor(ARP_INT_BGF_C);
		fg = Prefs().GetColor(ARP_INT_FGF_C);
		view->SetHighColor(bg);
		view->SetLowColor(bg);
		// Changed this so GratuitousShadeOn() can set the low colour...
		GratuitousShadeOn(view, bounds, bg, -3);
	} else {
		bg = Prefs().GetColor(ARP_INT_BG_C);
		fg = Prefs().GetColor(ARP_INT_FG_C);
		view->SetHighColor(bg);
		view->SetLowColor(bg);
		// Changed this so GratuitousShadeOn() can set the low colour...
		GratuitousShadeOn(view, bounds, bg, -3);
	}
	BRect		r = bounds;

	if (IsEnabled() == false) {
		view->SetHighColor( tint_color(bg, B_DARKEN_2_TINT) );
		view->StrokeRect(bounds);

		view->StrokeLine(BPoint(r.left+1, r.top+1), BPoint(r.right-1, r.top+1));
		view->StrokeLine(BPoint(r.left+1, r.top+2), BPoint(r.left+1, r.bottom-1));
	} else {
		view->SetHighColor( tint_color( tint_color(bg, B_DARKEN_3_TINT), B_DARKEN_2_TINT ) );
		view->StrokeRect(bounds);

		view->StrokeLine(BPoint(r.left+1, r.bottom-1), BPoint(r.left+1, r.bottom-1));
		view->StrokeLine(BPoint(r.right-1, r.top+1), BPoint(r.right-1, r.top+1));
		view->SetHighColor(tint_color(bg, B_DARKEN_3_TINT));
		view->StrokeLine(BPoint(r.left+2, r.bottom-1), BPoint(r.right-1, r.bottom-1));
		view->StrokeLine(BPoint(r.right-1, r.bottom-1), BPoint(r.right-1, r.top+2));
	}

	view->SetHighColor(fg);
//	view->SetLowColor(bg);
}

void ArpIntControl::GratuitousShadeOn(	BView* view,
										BRect bounds,
										rgb_color color,
										int16 delta)
{
	color.red += (uint8)(bounds.top * delta);
	color.green += (uint8)(bounds.top * delta);
	color.blue += (uint8)(bounds.top * delta);
	BPoint	left(bounds.left, bounds.top), right(bounds.right, bounds.top);
	while (left.y <= bounds.bottom) {
		view->SetHighColor(color);
		view->StrokeLine(left, right);
		color.red += delta;
		color.green += delta;
		color.blue += delta;
		left.y++;
		right.y++;
	}
}

void ArpIntControl::DrawBitmapBackgroundOn(	BRect bounds,
											BView* view,
											const BBitmap* bitmap)
{
	BRect		f = Frame(), sb = bitmap->Bounds();
	float		srcLeft = fmod( f.left, sb.Width() +1 );
	float		srcTop = fmod( f.top, sb.Height() +1 );
	float		srcRight = sb.Width(), srcBottom = sb.Height();
	float		destLeft = 0, destTop = 0;
	float		destRight = srcRight - srcLeft;
	float		destBottom = srcBottom - srcTop;
	BRect		src(srcLeft, srcTop, srcRight, srcBottom),
				dest(destLeft, destTop, destRight, destBottom);
	
	while (dest.top <= bounds.bottom) {
		while (dest.left <= bounds.right) {
			view->DrawBitmapAsync(bitmap, src, dest);
			src.left = 0;
			src.right = sb.Width();
			dest.left = dest.right + 1;
			dest.right = dest.left + sb.Width();
		}
		src.left = srcLeft;
		src.right = srcRight;
		src.top = 0;
		src.bottom = sb.Height();
		dest.left = destLeft;
		dest.right = destRight;
		dest.top = dest.bottom + 1;
		dest.bottom = dest.top + sb.Height();
	}
}

void ArpIntControl::SetValueColor(BView* v)
{
}

void ArpIntControl::StartTimer(const BMessage& msg, bigtime_t delay)
{
	StopTimer();
	mEditRunner = new BMessageRunner(BMessenger(this), &msg, delay, 1);
}

void ArpIntControl::StopTimer()
{
	delete mEditRunner;
	mEditRunner = NULL;
}
