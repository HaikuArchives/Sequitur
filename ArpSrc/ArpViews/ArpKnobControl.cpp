/* ArpKnobControl.cpp
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <interface/Bitmap.h>
#include <interface/Screen.h>
#include <interface/StringView.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpKnobControl.h"
#include "ArpViews/ArpIntControl.h"

static const BBitmap		*bgImage = 0;

static const int32			KNOB_MAX = 36;
static bool					attemptedLoad = false;
static const BBitmap*		knobs[KNOB_MAX];
static const BBitmap*		knob_ring = 0;
static const BBitmap*		knob_ring_tight = 0;

/*************************************************************************
 * EX-KNOB-CONTROL
 *************************************************************************/
ArpKnobControl::ArpKnobControl(	BRect frame,
								const char *name,
								BMessage *message,
								int32 minValue,
								int32 maxValue,
								uint32 displayFlags,
								uint32 rmask,
								uint32 flags)
		: inherited(frame, name, 0, message, rmask, flags),
		  BToolTipable(*(BView*)this),
		  mIndex(0), mIntCtrl(0),
		  mMinValue(minValue), mMaxValue(maxValue),
		  mDegree(ARP_DEFAULT_MIN_DEGREE),
		  mMinDegree(ARP_DEFAULT_MIN_DEGREE), mMaxDegree(ARP_DEFAULT_MAX_DEGREE),
		  mFocusTarget(0),
		  mIsObserving(false),
		  mMouseDown(false), mDisplayFlags(displayFlags),
		  mModificationMessage(0),
		  mFormatter(0)
{
	ArpASSERT( mMaxValue >= mMinValue );
//	if (bgImage == 0) bgImage = ImageManager().FindBitmap("BG");

	if (!attemptedLoad) {
		attemptedLoad = true;
		knobs[0] = ImageManager().FindBitmap( KNOB_000_IMAGE_STR );
		knobs[1] = ImageManager().FindBitmap( KNOB_010_IMAGE_STR );
		knobs[2] = ImageManager().FindBitmap( KNOB_020_IMAGE_STR );
		knobs[3] = ImageManager().FindBitmap( KNOB_030_IMAGE_STR );
		knobs[4] = ImageManager().FindBitmap( KNOB_040_IMAGE_STR );
		knobs[5] = ImageManager().FindBitmap( KNOB_050_IMAGE_STR );
		knobs[6] = ImageManager().FindBitmap( KNOB_060_IMAGE_STR );
		knobs[7] = ImageManager().FindBitmap( KNOB_070_IMAGE_STR );
		knobs[8] = ImageManager().FindBitmap( KNOB_080_IMAGE_STR );
		knobs[9] = ImageManager().FindBitmap( KNOB_090_IMAGE_STR );
		knobs[10] = ImageManager().FindBitmap(KNOB_100_IMAGE_STR );
		knobs[11] = ImageManager().FindBitmap(KNOB_110_IMAGE_STR );
		knobs[12] = ImageManager().FindBitmap(KNOB_120_IMAGE_STR );
		knobs[13] = ImageManager().FindBitmap(KNOB_130_IMAGE_STR );
		knobs[14] = ImageManager().FindBitmap(KNOB_140_IMAGE_STR );
		knobs[15] = ImageManager().FindBitmap(KNOB_150_IMAGE_STR );
		knobs[16] = ImageManager().FindBitmap(KNOB_160_IMAGE_STR );
		knobs[17] = ImageManager().FindBitmap(KNOB_170_IMAGE_STR );
		knobs[18] = ImageManager().FindBitmap(KNOB_180_IMAGE_STR );
		knobs[19] = ImageManager().FindBitmap(KNOB_190_IMAGE_STR );
		knobs[20] = ImageManager().FindBitmap(KNOB_200_IMAGE_STR );
		knobs[21] = ImageManager().FindBitmap(KNOB_210_IMAGE_STR );
		knobs[22] = ImageManager().FindBitmap(KNOB_220_IMAGE_STR );
		knobs[23] = ImageManager().FindBitmap(KNOB_230_IMAGE_STR );
		knobs[24] = ImageManager().FindBitmap(KNOB_240_IMAGE_STR );
		knobs[25] = ImageManager().FindBitmap(KNOB_250_IMAGE_STR );
		knobs[26] = ImageManager().FindBitmap(KNOB_260_IMAGE_STR );
		knobs[27] = ImageManager().FindBitmap(KNOB_270_IMAGE_STR );
		knobs[28] = ImageManager().FindBitmap(KNOB_280_IMAGE_STR );
		knobs[29] = ImageManager().FindBitmap(KNOB_290_IMAGE_STR );
		knobs[30] = ImageManager().FindBitmap(KNOB_300_IMAGE_STR );
		knobs[31] = ImageManager().FindBitmap(KNOB_310_IMAGE_STR );
		knobs[32] = ImageManager().FindBitmap(KNOB_320_IMAGE_STR );
		knobs[33] = ImageManager().FindBitmap(KNOB_330_IMAGE_STR );
		knobs[34] = ImageManager().FindBitmap(KNOB_340_IMAGE_STR );
		knobs[35] = ImageManager().FindBitmap(KNOB_350_IMAGE_STR );
		knob_ring = ImageManager().FindBitmap(KNOB_RING_IMAGE_STR);
		knob_ring_tight = ImageManager().FindBitmap(KNOB_RING_TIGHT_IMAGE_STR);
	}
	SetText("Knob Tip");
}

ArpKnobControl::~ArpKnobControl()
{
	delete mModificationMessage;
	delete mFormatter;
}

void ArpKnobControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( Parent() ) mViewColor = Parent()->ViewColor();
	SetViewColor( B_TRANSPARENT_COLOR );
}

void ArpKnobControl::Draw(BRect updateRect)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if( cache ) into = cache->StartDrawing(this, updateRect);
	
	DrawOn(updateRect, into);
	if( cache ) cache->FinishDrawing(into);
}

void ArpKnobControl::GetPreferredSize(float* width, float* height)
{
	BRect	b = Bounds();
	*width = b.Width();
	*height = b.Height();
}

void ArpKnobControl::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			{
				int32 origWhat = 0;
				msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &origWhat);
				if (origWhat == ARPMSG_INT_CONTROL_CHANGED) {
					int32	val;
					if (msg->FindInt32("value", &val) == B_OK) {
						SetValue(val);
						Invoke();
					}
				}
			}
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void ArpKnobControl::MouseDown(BPoint pt)
{
	if (mFocusTarget != 0) mFocusTarget->MakeFocus(true);
	SetMouseEventMask(B_POINTER_EVENTS);
	mMouseDown = true;
	mLastPoint = pt;
}

void ArpKnobControl::MouseUp(BPoint pt)
{
	mMouseDown = false;
	Invoke();
}

void ArpKnobControl::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	if( mMouseDown ) {
		/* Operate on degrees so that motions are consistent across
		 * knobs with different limits.
		 */
//		int32	degree = Degree(), newDegree;
		int32	degree = mDegree, newDegree;
		float	delta = ((pt.x - mLastPoint.x) * 2) + ((mLastPoint.y - pt.y) * 2);
		newDegree = (int32)(delta + degree);
		if (newDegree < mMinDegree) newDegree = mMinDegree;
		else if (newDegree > mMaxDegree) newDegree = mMaxDegree;
		if (newDegree != degree) {
			int32	oldValue = Value();
			int32	value = DegreeToValue( newDegree );
			SetValue(value);
			if (Value() != oldValue && mModificationMessage) {
				BMessage	msg(*mModificationMessage);
				msg.AddInt32("be:value", Value() );
				Invoke(&msg);
			}
		}
		mDegree = newDegree;
	}
	mLastPoint = pt;
}

void ArpKnobControl::SetModificationMessage(BMessage* message)
{
	delete mModificationMessage;
	mModificationMessage = message;
}

BMessage* ArpKnobControl::ModificationMessage() const
{
	return mModificationMessage;
}

void ArpKnobControl::SetValueRange(int32 minValue, int32 maxValue)
{
	ArpASSERT( maxValue >= minValue );
	if ( (mMinValue == minValue) && (mMaxValue == maxValue) ) return;
	mMinValue = minValue;
	mMaxValue = maxValue;
	int32		value = Value();
	if( value < mMinValue ) value = mMinValue;
	if( value > mMaxValue ) value = mMaxValue;
	SetValue( value );
}

void ArpKnobControl::SetValue(int32 value)
{
	int32	newValue = value;
	if( newValue < mMinValue ) newValue = mMinValue;
	if( newValue > mMaxValue ) newValue = mMaxValue;

	int32	oldValue = Value();
	inherited::SetValue( newValue );
	/* It would be nice to only draw this if the value has
	 * changed.  However, this causes problems for knobs who have
	 * fewer valus than degrees -- they rotate in a very unfriendly
	 * fashion.  So I'm forced to update whenever anyone tries
	 * to set the value.
	 */
	Invalidate();
	if( newValue != oldValue ) {
		BString		tip;
		if( mFormatter ) mFormatter->FormatInt( newValue, tip );
		else tip << newValue;
		SetText( tip.String() );
	}
	if( mIntCtrl ) mIntCtrl->SetValue( value );
	mDegree = Degree();
}

void ArpKnobControl::SetDegreeRange(int32 minDegree, int32 maxDegree)
{
	ArpASSERT( maxDegree >= minDegree );
	mMinDegree = minDegree;
	mMaxDegree = maxDegree;
	int32		degree = Degree();
	if( degree < mMinDegree ) SetValue( DegreeToValue( mMinDegree ) );
	if( degree > mMaxDegree ) SetValue( DegreeToValue( mMaxDegree ) );	
}

int32 ArpKnobControl::Degree() const
{
	return ValueToDegree( Value() );
}

void ArpKnobControl::SetTipFormatter(ArpIntFormatterI* formatter)
{
	delete mFormatter;
	mFormatter = formatter;
}

void ArpKnobControl::SetIntControl(ArpIntControl* intCtrl)
{
	mIntCtrl = intCtrl;
	mFocusTarget = intCtrl;
}

void ArpKnobControl::DrawOn(BRect updateRect, BView *view)
{
	BRect		b = Bounds();
	if( bgImage != 0 ) {
		view->SetDrawingMode(B_OP_COPY);
		if ( mDisplayFlags&ARP_KNOB_BITMAP_BG && bgImage )
			DrawBackgroundOn(b, view);
		else {
			view->SetHighColor( mViewColor );
			view->FillRect(b);
		}
	} else {
		view->SetHighColor( mViewColor );
		view->FillRect( b );
	}

	int32	degree = Degree();
//	int32	degree = mDegree;
	int32	num = degree / 10;
	if( num < 0 ) num = 0;
	if( num > 35 ) num = 35;
	float	left = 0, top = 0;
	if( knobs[num] != 0 ) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		if( mDisplayFlags&ARP_RING_ADORNMENT && knob_ring ) {
			view->DrawBitmapAsync(knob_ring, BPoint(left, top));
			left += 5;
			top += 4;
		} else if( mDisplayFlags&ARP_TIGHT_RING_ADORNMENT && knob_ring_tight ) {
			view->DrawBitmapAsync(knob_ring_tight, BPoint(left, top));
			left += 1;
			top += 1;
		}
		view->DrawBitmapAsync(knobs[num], BPoint(left, top));
		view->SetDrawingMode(B_OP_COPY);
	}
}

void ArpKnobControl::DrawBackgroundOn(BRect updateRect, BView *view)
{
	BRect		f = Frame(), sb = bgImage->Bounds();
	float		srcLeft = fmod( f.left, sb.Width() +1 );
	float		srcTop = fmod( f.top, sb.Height() +1 );
	float		srcRight = sb.Width(), srcBottom = sb.Height();
	float		destLeft = 0, destTop = 0;
	float		destRight = srcRight - srcLeft;
	float		destBottom = srcBottom - srcTop;
	BRect		src(srcLeft, srcTop, srcRight, srcBottom),
				dest(destLeft, destTop, destRight, destBottom);
	
	while (dest.top <= updateRect.bottom) {
		while (dest.left <= updateRect.right) {
			view->DrawBitmapAsync(bgImage, src, dest);
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

int32 ArpKnobControl::ValueToDegree(int32 value) const
{
	int32	degreeRange = mMaxDegree - mMinDegree;
	int32	valueRange = mMaxValue - mMinValue;
	if( valueRange == 0 ) return mMinDegree;
	int32	adjustedLimit = value - mMinValue;
	int32 	answer = (int32) (mMinDegree + 1 + ((adjustedLimit * degreeRange) / valueRange));
	if( answer > 360 ) answer -= 360;
	return answer;
}

int32 ArpKnobControl::DegreeToValue(int32 degree) const
{
	int32	degreeRange = mMaxDegree - mMinDegree;
	int32	adjustedDegree = degree - mMinDegree;
	int32	valueRange = mMaxValue - mMinValue;
	int32	answer = (int32) (mMinValue + ((valueRange * adjustedDegree) / degreeRange));
	// Everything looks good on my test routines, but I force the
	// answer into my limits just to make sure nothing funny happens.
	if( answer < mMinValue ) return mMinValue;
	if( answer > mMaxValue ) return mMaxValue;
	return answer;
}

/***************************************************************************
 * ARP-KNOB-PANEL
 * This class constructs a knob along with an optional label and/or int
 * control.  Part of its usefulness is that it hides hooking up the knob
 * and int controls to each other, which is behaviour that will probably
 * change at some point.
 ***************************************************************************/
ArpKnobPanel::ArpKnobPanel(	const char* name, const char* label,
							BMessage* message, int32 minValue, int32 maxValue,
							bool showIntControl, orientation layout, uint32 knobFlags,
							float labelWidth, float intControlWidth, uint32 rmask, uint32 flags)
		: inherited( BRect( 0, 0, 0, 0 ), "knob_panel", rmask, flags )
{
	if( layout == B_VERTICAL )
		LayoutVertical(name, label, message, minValue, maxValue, showIntControl, knobFlags, labelWidth, intControlWidth);
	else
		LayoutHorizontal(name, label, message, minValue, maxValue, showIntControl, knobFlags, labelWidth, intControlWidth);

	float		width = 0, height = 0;
	BView*		view;
	for( view = ChildAt(0); view; view = view->NextSibling() ) {
		BRect	f = view->Frame();
		if( f.right > width ) width = f.right;
		if( f.bottom > height ) height = f.bottom;
	}
	ResizeTo( width, height );
}

ArpKnobPanel::~ArpKnobPanel()
{
}

ArpKnobControl* ArpKnobPanel::KnobControl() const
{
	BView*		view;
	for( view = ChildAt(0); view; view = view->NextSibling() ) {
		ArpKnobControl*	knob = dynamic_cast<ArpKnobControl*>( view );
		if( knob ) return knob;
	}
	return 0;
}

ArpIntControl* ArpKnobPanel::IntControl() const
{
	return dynamic_cast<ArpIntControl*>( FindView("int_control") );
}

void ArpKnobPanel::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( Parent() ) SetViewColor( Parent()->ViewColor() );

	ArpKnobControl*	knobCtrl = KnobControl();
	ArpIntControl*	intCtrl = IntControl();
	if( knobCtrl && intCtrl ) intCtrl->StartWatching( knobCtrl, ARPMSG_INT_CONTROL_CHANGED );
}

void ArpKnobPanel::DetachedFromWindow()
{
	inherited::DetachedFromWindow();

	ArpKnobControl*	knobCtrl = KnobControl();
	ArpIntControl*	intCtrl = IntControl();
	if( knobCtrl && intCtrl ) intCtrl->StopWatching( knobCtrl, ARPMSG_INT_CONTROL_CHANGED );
}

void ArpKnobPanel::GetPreferredSize(float* width, float* height)
{
	float	w = 0, h = 0;
	BView*	view;
	for( view = ChildAt(0); view != 0; view = view->NextSibling() ) {
		BRect	b = view->Frame();
		if( b.right > w ) w = b.right;
		if( b.bottom > h ) h = b.bottom;
	}
	*width = w;
	*height = h;
}

BHandler* ArpKnobPanel::LayoutHandler()
{
	return this;
}

const BHandler* ArpKnobPanel::LayoutHandler() const
{
	return this;
}

void ArpKnobPanel::LayoutHorizontal(const char* name, const char* label, BMessage* message,
									int32 minValue, int32 maxValue,
									bool showIntControl, uint32 knobFlags, float labelWidth,
									float intControlWidth)
{
	const BFont*	font = be_plain_font;
	float			fh = arp_get_font_height(font);
	float			spaceX = 8;
	float			left = 0;
	/* Add label
	 */
	BStringView*	sv = 0;
	if( label ) {
		float		w = (labelWidth >= 0) ? labelWidth : font->StringWidth( label );
		sv = new BStringView(	BRect( left, 0, left + w, 0 + fh ),
								"label", label );
		if( sv ) AddChild( sv );
		left += w + spaceX;
	}
	/* Add knob
	 */
	float			knobW, knobH;
	if( knobFlags&ARP_RING_ADORNMENT ) {
		knobW = Prefs().Size(KNOB_RING_X);
		knobH = Prefs().Size(KNOB_RING_Y);
	} else if( knobFlags&ARP_TIGHT_RING_ADORNMENT ) {
		knobW = Prefs().Size(KNOB_RING_TIGHT_X);
		knobH = Prefs().Size(KNOB_RING_TIGHT_Y);
	} else {
		knobW = Prefs().Size(KNOB_X);
		knobH = Prefs().Size(KNOB_Y);
	}
	ArpKnobControl* knob = new ArpKnobControl(	BRect(left, 0, left + knobW, 0 + knobH ),
												name, message, minValue, maxValue, knobFlags);
	if( knob ) {
		AddChild( knob );
		left += knobW + spaceX;
	}
	/* Add int control
	 */
	ArpIntControl*	intCtrl = 0;
	if( showIntControl ) {
		float		w = (intControlWidth >= 0) ? intControlWidth : knobW;
		intCtrl = new ArpIntControl( BRect(left, 0, left + w, 0 + Prefs().Size(INT_CTRL_Y) ),
									"int_control", 0, 0);
		if( intCtrl ) {
			intCtrl->SetLimits( minValue, maxValue );
			if( knob ) knob->SetIntControl( intCtrl );
			AddChild( intCtrl );
		}
	}
	/* Now center everything based on the tallest view.
	 */
	if( !sv && !intCtrl ) return;
	float	tallest = (knobH > fh) ? knobH : fh;

	BView*	view;
	for( view = ChildAt(0); view; view = view->NextSibling() ) {
		BRect	f = view->Frame();
		if( f.Height() < tallest )
			view->MoveTo( f.left, (tallest - f.Height()) / 2 );
	}		
}

void ArpKnobPanel::LayoutVertical(	const char* name, const char* label, BMessage* message,
									int32 minValue, int32 maxValue,
									bool showIntControl, uint32 knobFlags, float labelWidth,
									float intControlWidth)
{
	const BFont*	font = be_plain_font;
	float			fh = arp_get_font_height(font);
	float			spaceY = 3;
	float			top = 0;
	float			widest = 0;		// Cache the widest view just to save ourselves an
									// extra iteration over the views
	/* Add label
	 */
	BStringView*	sv = 0;
	if( label ) {
		float		w = (labelWidth >= 0) ? labelWidth : font->StringWidth( label );
		sv = new BStringView(	BRect( 0, top, 0 + w, top + fh ),
								"label", label );
		if( sv ) {
			AddChild( sv );
			if( w > widest ) widest = w;
		}
		top += fh + spaceY;
	}
	/* Add knob
	 */
	float			knobW, knobH;
	if( knobFlags&ARP_RING_ADORNMENT ) {
		knobW = Prefs().Size(KNOB_RING_X);
		knobH = Prefs().Size(KNOB_RING_Y);
	} else if( knobFlags&ARP_TIGHT_RING_ADORNMENT ) {
		knobW = Prefs().Size(KNOB_RING_TIGHT_X);
		knobH = Prefs().Size(KNOB_RING_TIGHT_Y);
	} else {
		knobW = Prefs().Size(KNOB_X);
		knobH = Prefs().Size(KNOB_Y);
	}
	ArpKnobControl* knob = new ArpKnobControl(	BRect(0, top, knobW, top + knobH ),
												name, message, minValue, maxValue, knobFlags);
	if( knob ) {
		AddChild( knob );
		if( knobW > widest ) widest = knobW;
		top += knobH + spaceY;
	}
	/* Add int control
	 */
	ArpIntControl*	intCtrl = 0;
	if( showIntControl ) {
		float		w = (intControlWidth >= 0) ? intControlWidth : knobW;
		intCtrl = new ArpIntControl( BRect(0, top, w, top + Prefs().Size(INT_CTRL_Y) ),
									"int_control", 0, 0);
		if( intCtrl ) {
			intCtrl->SetLimits( minValue, maxValue );
			if( knob ) knob->SetIntControl( intCtrl );
			AddChild( intCtrl );
			if( w > widest ) widest = w;
		}
	}
	/* Now center everything based on the widest view.
	 */
	if( !sv && !intCtrl ) return;
	if( widest <= 0 ) return;

	BView*	view;
	for( view = ChildAt(0); view; view = view->NextSibling() ) {
		BRect	f = view->Frame();
		if( f.Width() < widest )
			view->MoveTo( (widest - f.Width()) / 2, f.top );
	}		
}
