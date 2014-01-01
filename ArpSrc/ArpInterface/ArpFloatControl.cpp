#include <ArpCore/ArpCoreDefs.h>

#ifndef MAC_OS_X
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <be/InterfaceKit.h>
#include <ArpCore/ArpDebug.h>
#include <ArpInterface/ArpBitmapCache.h>
#include <ArpInterface/ArpInlineTextView.h>
#include <ArpInterface/ArpFloatControl.h>
#include <ArpInterface/ViewTools.h>
#include <ArpInterface/ArpPrefs.h>

static const BBitmap		*bgImage = 0;
static const uint32			EDIT_START_MSG	= 'strt';

/*************************************************************************
 * ARP-INT-CONTROL
 *************************************************************************/
ArpFloatControl::ArpFloatControl(	BRect frame,
									const char* name,
									const BString16* label,
									BMessage* message,
									uint32 rmask,
									uint32 flags)
		: inherited(frame, name, rmask, flags),
		  mValue(0), mFlags(ARP_IS_ENABLED), mDivider(0),
		  mSteps(0.1f), mMin(0), mMax(1.0f),
		  mKeyStep(1), mKeyCount(0), mFinishedMsg(0),
		  mTextCtrl(NULL), mEditRunner(NULL),
		  mMouseDown(false), mBaseValue(0),
		  mPixels(2)
{
	if (label) {
		mLabel = label;
		mDivider = StringWidth(label) + 5;
		if (mDivider > frame.Width() ) mDivider = frame.Width();
	}
	if (message) SetMessage(message);
//	if (!bgImage && HasImageManager()) bgImage = ImageManager().FindBitmap("BG");
}

ArpFloatControl::~ArpFloatControl()
{
	delete mFinishedMsg;
}

bool ArpFloatControl::IsEnabled() const
{
	return mFlags&ARP_IS_ENABLED;
}

void ArpFloatControl::SetEnabled(bool enabled)
{
	if (enabled) mFlags |= ARP_IS_ENABLED;
	else mFlags &= ~ARP_IS_ENABLED;
}

void ArpFloatControl::SetSteps(float steps)
{
	mSteps = steps;
}

float ArpFloatControl::Value() const
{
	return mValue;
}

void ArpFloatControl::SetValue(float value)
{
	mValue = value;
	Invalidate();
	Invoke();
}

void ArpFloatControl::SetLimits(float min, float max)
{
	mMin = min;
	mMax = max;
}

void ArpFloatControl::SetDivider(float dividing_line)
{
	if (dividing_line != mDivider) {
		mDivider = dividing_line;
		Invalidate();
	}
}

float ArpFloatControl::Min() const
{
	return mMin;
}

float ArpFloatControl::Max() const
{
	return mMax;
}

void ArpFloatControl::GetValueLabel(float value, BString16& str) const
{
	char	z[32];
	sprintf(z, "%.4f", value);
	str << z;
}

BRect ArpFloatControl::ControlBounds() const
{
	BRect	b = Bounds();
	if (mDivider > 0) b.left = mDivider;
	return b;
}

void ArpFloatControl::SetFinishedMessage(BMessage* msg)
{
	delete mFinishedMsg;
	mFinishedMsg = msg;
}

void ArpFloatControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetFontSize(10);
	if (Parent() ) mViewColor = Parent()->ViewColor();
	SetViewColor(B_TRANSPARENT_COLOR);
	SetTarget(BMessenger(this) );
}

void ArpFloatControl::Draw(BRect updateRect)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, updateRect);
	
	DrawOn(updateRect, into);
	if (cache) cache->FinishDrawing(into);
}

static void get_control_spacing(ArpFloatControl* ctrl, float* minW, float* maxW)
{
	BString16		minStr;
	ctrl->GetValueLabel(ctrl->Min(), minStr);
	*minW = 0;
	if (minStr.Length() > 0) *minW = arp_get_string_width(ctrl, minStr.String());
	BString16		maxStr;
	ctrl->GetValueLabel(ctrl->Max(), maxStr);
	*maxW = 0;
	if (maxStr.Length() > 0) *maxW = arp_get_string_width(ctrl, maxStr.String());
}

void ArpFloatControl::GetPreferredSize(float *width, float *height)
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

void ArpFloatControl::KeyDown(const char* bytes, int32 numBytes)
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

void ArpFloatControl::KeyUp(const char *bytes, int32 numBytes)
{
	inherited::KeyUp(bytes, numBytes);
	mKeyStep = 1;
	mKeyCount = 0;
}

void ArpFloatControl::MouseDown(BPoint pt)
{
	mMouseDown = false;
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
	if (ControlBounds().Contains(pt) ) {
		MakeFocus(true);
		SetMouseEventMask(	B_POINTER_EVENTS,
							B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY);
		mMouseDown = true;
		mBaseValue = Value();
		mBaseRect = ControlBounds();
	}
}

void ArpFloatControl::MouseUp(BPoint pt)
{
	if (IsEnabled() == false) return;

	mMouseDown = false;
	if (mDownPt == pt && Bounds().Contains(pt) ) {
		/* Start the edit timer in case the user is invoking an inline
		 * text view.
		 */
		bigtime_t	doubleClickTime;
		get_click_speed(&doubleClickTime);
		doubleClickTime *= 2;
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		StartTimer(BMessage(EDIT_START_MSG), doubleClickTime);
	} else {
		if (mFinishedMsg) Invoke(mFinishedMsg);
		else Invoke();
	}
}

void ArpFloatControl::MouseMoved(	BPoint pt,
									uint32 code,
									const BMessage* msg)
{
	BRect	cb = ControlBounds();
	if (!(cb.Contains(pt))) {
		StopEdit();
		StopTimer();
	}

	if (IsEnabled() == false) return;

	if (mMouseDown == false) return;

	float	xChange, yChange;
	float	newValue;
	
	if (pt.x < mBaseRect.left) {
		xChange = (pt.x - mBaseRect.left) / mPixels;
	} else if (pt.x > mBaseRect.right) {
		xChange = (pt.x - mBaseRect.right) / mPixels;
	} else {
		xChange = 0;
	}
			
	if (pt.y < mBaseRect.top) {
		yChange = (pt.y - mBaseRect.top) / mPixels;
	} else if (pt.y > mBaseRect.bottom) {
		yChange = (pt.y - mBaseRect.bottom) / mPixels;
	} else {
		yChange = 0;
	}
	newValue = mBaseValue + ( (xChange - yChange) * mSteps);
	SafeSetValue(newValue);
}

void ArpFloatControl::MessageReceived(BMessage *msg)
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

void ArpFloatControl::StartEdit()
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
	mTextCtrl->SetViewColor(Prefs().GetColor(ARP_INT_BGF_C));
	mTextCtrl->SetHighColor(Prefs().GetColor(ARP_INT_FGF_C));
	BString16		str;
	GetValueLabel(Value(), str);
	mTextCtrl->SetText( str.String() );
	AddChild(mTextCtrl);
}

void ArpFloatControl::StopEdit(bool keepChanges)
{
	if (mTextCtrl) {

		if (keepChanges && mTextCtrl->HasChanged() ) {
			const BString16*	text = mTextCtrl->Text();
			if (text) {
				float	val;
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

void ArpFloatControl::NotifyHook(float newValue)
{
	BMessage	msg(ARPMSG_FLOAT_CONTROL_CHANGED);
	msg.AddFloat("value", newValue );
	SendNotices(ARPMSG_FLOAT_CONTROL_CHANGED, &msg);
}

void ArpFloatControl::SafeSetValue(float newValue)
{
	if (newValue < Min() ) newValue = Min();
	if (newValue > Max() ) newValue = Max();

	if (newValue != Value() ) NotifyHook(newValue);
	SetValue(newValue);	
}

void ArpFloatControl::SetValueQuiet(float value)
{
	float			nv = value;
	if (nv < Min()) nv = Min();
	else if (nv > Max()) nv = Max();
	if (mValue == nv) return;

	mValue = nv;
	this->Invalidate();
}

void ArpFloatControl::AddAscii(char byte)
{
	float	newValue = Value();

	if (byte == B_UP_ARROW) newValue = Value() + mKeyStep;
	else if (byte == B_DOWN_ARROW) newValue = Value() - mKeyStep;
	else if (byte == '+') newValue = fabs(Value() );
	else if (byte == '-') newValue = 0 - fabs(Value() );
	else if (byte == B_SPACE) StartEdit();
	else return;

	if (newValue < Min()) newValue = Min();
	if (newValue > Max()) newValue = Max();
	if (newValue != Value()) {
		SetValue(newValue);
		NotifyHook(newValue);
	}
}

status_t ArpFloatControl::ValueFromLabel(const char* label, float* answer) const
{
	float	val = float(atof(label));
	*answer = val;
	return B_OK;
}

status_t ArpFloatControl::ValueFromLabel(const BString16* label, float* answer) const
{
	ArpVALIDATE(label, return B_ERROR);
	char*		l = label->AsAscii();
	if (!l) return B_ERROR;

	float	val = float(atof(l));
	delete[] l;
	*answer = val;
	return B_OK;
}

void ArpFloatControl::DrawOn(BRect updateRect, BView* view)
{
	BRect		cb = ControlBounds();
	float		textY = cb.top + Prefs().GetInt32(ARP_FULLFONT_Y);
	if (mDivider > 0) {
		view->SetHighColor(mViewColor);
		view->FillRect( BRect(0, cb.top, cb.left, cb.bottom) );
		if (mLabel.Length() > 0) {
			if ( IsFocus() ) view->SetHighColor( Prefs().GetColor(ARP_INT_FGF_C) );
			else if ( !IsEnabled() ) view->SetHighColor( tint_color(Prefs().GetColor(ARP_INT_FG_C), B_LIGHTEN_1_TINT) );
			else view->SetHighColor( Prefs().GetColor(ARP_INT_FG_C));
			view->SetLowColor(mViewColor);
			view->DrawString( mLabel.String(), BPoint(0, textY) );
		}
	}
	DrawControlBackgroundOn(cb, view);

	BPoint		drawPt(cb.left + 3, textY);
	if (IsEnabled() == false) {
		drawPt.x++;
		drawPt.y++;
	}
	BString16		str;
	GetValueLabel(Value(), str);
	view->DrawString(str.String(), drawPt);
}

void ArpFloatControl::DrawControlBackgroundOn(BRect bounds, BView* view)
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
	} else if (IsFocus()) {
		bg = Prefs().GetColor(ARP_INT_BGF_C);
		fg = Prefs().GetColor(ARP_INT_FGF_C);
		view->SetHighColor(bg);
		GratuitousShadeOn(view, bounds, bg, -3);
	} else {
		bg = Prefs().GetColor(ARP_INT_BG_C);
		fg = Prefs().GetColor(ARP_INT_FG_C);
		view->SetHighColor(bg);
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
	view->SetLowColor(bg);
}

void ArpFloatControl::GratuitousShadeOn(BView* view,
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

void ArpFloatControl::DrawBitmapBackgroundOn(	BRect bounds,
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

void ArpFloatControl::StartTimer(const BMessage& msg, bigtime_t delay)
{
	StopTimer();
	mEditRunner = new BMessageRunner(BMessenger(this), &msg, delay, 1);
}

void ArpFloatControl::StopTimer()
{
	delete mEditRunner;
	mEditRunner = NULL;
}
