/* SeqTempoControl.cpp
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmDefs.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqTempoControl.h"

/* Number of pixels between each digit.
 */
static const float			LARGE_SPACE_X	= 5;
static const float			SMALL_SPACE_X	= 2;
static float				LARGE_W			= 0;
static float				SMALL_W			= 0;

static const BBitmap*		gBg = NULL;

static const BBitmap*		gLarge0 = NULL;
static const BBitmap*		gLarge1 = NULL;
static const BBitmap*		gLarge2 = NULL;
static const BBitmap*		gLarge3 = NULL;
static const BBitmap*		gLarge4 = NULL;
static const BBitmap*		gLarge5 = NULL;
static const BBitmap*		gLarge6 = NULL;
static const BBitmap*		gLarge7 = NULL;
static const BBitmap*		gLarge8 = NULL;
static const BBitmap*		gLarge9 = NULL;

static const BBitmap*		gSmall0 = NULL;
static const BBitmap*		gSmall1 = NULL;
static const BBitmap*		gSmall2 = NULL;
static const BBitmap*		gSmall3 = NULL;
static const BBitmap*		gSmall4 = NULL;
static const BBitmap*		gSmall5 = NULL;
static const BBitmap*		gSmall6 = NULL;
static const BBitmap*		gSmall7 = NULL;
static const BBitmap*		gSmall8 = NULL;
static const BBitmap*		gSmall9 = NULL;

static const BBitmap* large_bitmap_for(char value)
{
	int32	val = value - 48;
	if (val == 0) return gLarge0;
	else if (val == 1) return gLarge1;
	else if (val == 2) return gLarge2;
	else if (val == 3) return gLarge3;
	else if (val == 4) return gLarge4;
	else if (val == 5) return gLarge5;
	else if (val == 6) return gLarge6;
	else if (val == 7) return gLarge7;
	else if (val == 8) return gLarge8;
	else if (val == 9) return gLarge9;
	return NULL;
}

static const BBitmap* small_bitmap_for(char value)
{
	int32	val = value - 48;
	if (val == 0) return gSmall0;
	else if (val == 1) return gSmall1;
	else if (val == 2) return gSmall2;
	else if (val == 3) return gSmall3;
	else if (val == 4) return gSmall4;
	else if (val == 5) return gSmall5;
	else if (val == 6) return gSmall6;
	else if (val == 7) return gSmall7;
	else if (val == 8) return gSmall8;
	else if (val == 9) return gSmall9;
	return NULL;
}

/*************************************************************************
 * SEQ-TEMPO-CONTROL
 *************************************************************************/
SeqTempoControl::SeqTempoControl(	BPoint leftTop,
									const char* name,
									BMessage* message,
									int32 resizeMask,
									float minTempo,
									float maxTempo,
									float initialTempo)
		: BView(	BRect(leftTop.x, leftTop.y, leftTop.x, leftTop.y),
					name, resizeMask, B_WILL_DRAW),
		mMinTempo(minTempo), mMaxTempo(maxTempo),
		mMessage(message),
		mMouseDown(0)
{	
	if (!gBg) gBg = Resources().FindBitmap(B_MESSAGE_TYPE, "Tempo LCD");

	if (!gLarge0) gLarge0 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 0");
	if (!gLarge1) gLarge1 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 1");
	if (!gLarge2) gLarge2 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 2");
	if (!gLarge3) gLarge3 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 3");
	if (!gLarge4) gLarge4 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 4");
	if (!gLarge5) gLarge5 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 5");
	if (!gLarge6) gLarge6 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 6");
	if (!gLarge7) gLarge7 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 7");
	if (!gLarge8) gLarge8 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 8");
	if (!gLarge9) gLarge9 = Resources().FindBitmap(B_MESSAGE_TYPE, "Large Digit 9");

	if (!gSmall0) gSmall0 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 0");
	if (!gSmall1) gSmall1 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 1");
	if (!gSmall2) gSmall2 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 2");
	if (!gSmall3) gSmall3 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 3");
	if (!gSmall4) gSmall4 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 4");
	if (!gSmall5) gSmall5 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 5");
	if (!gSmall6) gSmall6 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 6");
	if (!gSmall7) gSmall7 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 7");
	if (!gSmall8) gSmall8 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 8");
	if (!gSmall9) gSmall9 = Resources().FindBitmap(B_MESSAGE_TYPE, "Small Digit 9");

	if (mMinTempo < 1) mMinTempo = 1;
	SetTempo(initialTempo);

	if (gLarge0) LARGE_W = gLarge0->Bounds().Width();
	if (gSmall0) SMALL_W = gSmall0->Bounds().Width();
	mWholeBounds.Set(0, 1, 40, 17);
	mFractionalBounds.Set(48, 7, 64, 17);
	if (gBg) ResizeTo(gBg->Bounds().Width(), gBg->Bounds().Height() );
}

SeqTempoControl::~SeqTempoControl()
{
	delete mMessage;
}

float SeqTempoControl::Tempo() const
{
	return mTempo;
}

void SeqTempoControl::SetTempo(float tempo)
{
	mTempo = tempo;
	if (mTempo < mMinTempo) mTempo = mMinTempo;
	else if (mTempo > mMaxTempo) mTempo = mMaxTempo;
	Invalidate();
}

void SeqTempoControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
}

void SeqTempoControl::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>(Window());
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void SeqTempoControl::MouseDown(BPoint pt)
{
	MakeFocus(true);
	SetMouseEventMask(B_POINTER_EVENTS,
					  B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY);
	if (mWholeBounds.Contains(pt) ) {
		mMouseDown = WHOLE;
		mBaseRect = mWholeBounds;
		mWholePart = (int32)floor(Tempo());
		mFractionPart = int32(floor( (Tempo() - mWholePart) * 100));
	} else if (mFractionalBounds.Contains(pt) ) {
		mMouseDown = FRACTIONAL;
		mBaseRect = mFractionalBounds;
		mWholePart = (int32)floor(Tempo());
		mFractionPart = int32(floor( (Tempo() - mWholePart) * 100));
	}
}

void SeqTempoControl::MouseUp(BPoint pt)
{
	mMouseDown = 0;
	Invoke();
}

void SeqTempoControl::MouseMoved(BPoint pt,
								uint32 code,
								const BMessage* msg)
{
	if (mMouseDown == 0) return;

	float	pixels = 2;
	float	xChange, yChange;
	
	if (pt.x < mBaseRect.left)
		xChange = (pt.x - mBaseRect.left) / pixels;
	else if (pt.x > mBaseRect.right)
		xChange = (pt.x - mBaseRect.right) / pixels;
	else
		xChange = 0;
			
	if (pt.y < mBaseRect.top)
		yChange = (pt.y - mBaseRect.top) / pixels;
	else if (pt.y > mBaseRect.bottom)
		yChange = (pt.y - mBaseRect.bottom) / pixels;
	else
		yChange = 0;

	float	newValue;
	if (mMouseDown&WHOLE)
		newValue = float(mWholePart + (xChange - yChange)) + (float(mFractionPart) / 100);
	else
		newValue = float(mWholePart) + (float(mFractionPart + xChange - yChange) / 100);
	if (newValue < mMinTempo) newValue = mMinTempo;
	if (newValue > mMaxTempo) newValue = mMaxTempo;

	if (newValue != Tempo()) {
		SetTempo(float(int32(newValue)));
		Invoke();
	}
}

bool SeqTempoControl::IsTracking() const
{
	return mMouseDown != 0;
}

void SeqTempoControl::DrawOn(BView* view, BRect clip)
{
	if (gBg) view->DrawBitmapAsync(gBg);
	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	DrawWholeTempoOn(view, BPoint(0, 1) );
	DrawFractionalTempoOn(view, BPoint(48, 7) );

	view->SetDrawingMode(mode);
}

void SeqTempoControl::DrawWholeTempoOn(BView* view, BPoint leftTop)
{
	char		buf[32];
	sprintf(buf, "%ld", (int32)floor(Tempo()) );
	size_t		len = strlen(buf);
	size_t		digits = 3;
	if (len > digits) len = digits;
	size_t		blen = digits - len;
	float		left = leftTop.x, top = leftTop.y;
	for (size_t k = 0; k < blen; k++) {
		left += LARGE_W + LARGE_SPACE_X;
	}
	for (size_t k = blen; k < digits; k++) {
		const BBitmap		*b = large_bitmap_for(buf[k - blen]);
		if (b != 0) view->DrawBitmapAsync(b, BPoint(left, top));
		left += LARGE_W + LARGE_SPACE_X;
	}
}

void SeqTempoControl::DrawFractionalTempoOn(BView* view, BPoint leftTop)
{
	char		buf[32];
	float		whole = floor(Tempo());
	int32		fraction = int32(floor( (Tempo() - whole) * 100));
	sprintf(buf, "%ld", fraction);
	size_t		len = strlen(buf);
	size_t		digits = 2;
	size_t		stop = digits;	
	if (len < digits) stop = len;
	float		left = leftTop.x, top = leftTop.y;
	for (size_t k = 0; k < stop; k++) {
		const BBitmap*		b = small_bitmap_for(buf[k]);
		if (b) view->DrawBitmapAsync(b, BPoint(left, top));
		left += SMALL_W + SMALL_SPACE_X;
	}
	if (len < digits) {
		for (size_t k = len; k < digits; k++) {
			if (gSmall0) view->DrawBitmapAsync(gSmall0, BPoint(left, top));
			left += SMALL_W + SMALL_SPACE_X;
		}
	}
}

void SeqTempoControl::Invoke()
{
	if (!mMessage || !Window() ) return;
	BMessage*	msg = new BMessage(*mMessage);
	msg->AddFloat("tempo", Tempo() );
	Window()->PostMessage(msg);
	delete msg;
}
