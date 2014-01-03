/* ArpRangeControl.cpp
 */
#include <assert.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <app/Message.h>
#include <interface/Bitmap.h>
#include <interface/Region.h>
#include <interface/Screen.h>
#include <interface/Shape.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpRangeControl.h"

#define I_BG_COLOR		(B_MENU_BACKGROUND_COLOR)

/* This represents either end of the horizontal view.  It is two pixels
 * of shading, plus two blank pixels.
 */
static const float	H_BORDER_X	= 4;
/* This represents the top (or bottom) shading of the horizontal view.
 * It is two pixels of shading, put one pixel of blank space.
 */
static const float	H_BORDER_Y	= 3;

static const float	V_BORDER_Y	= 4;
static const float	V_BORDER_X	= 3;

static const BBitmap*	mMini0 = NULL;
static const BBitmap*	mMini1 = NULL;
static const BBitmap*	mMini2 = NULL;
static const BBitmap*	mMini3 = NULL;
static const BBitmap*	mMini4 = NULL;
static const BBitmap*	mMini5 = NULL;
static const BBitmap*	mMini6 = NULL;
static const BBitmap*	mMini7 = NULL;
static const BBitmap*	mMini8 = NULL;
static const BBitmap*	mMini9 = NULL;

// Forward references
class ArpSetRangeView;
class _ArpShowTextView;

/* A convenience method -- answer the smallest of the three values.
 */
static float smallest(float x, float y, float z)
{
	float	xyMin = (x < y) ? x : y;
	if( z < xyMin ) return z;
	return xyMin;
}

/* A convenience method -- answer the largest of the three values.
 */
static float largest(float x, float y, float z)
{
	float	xyMax = (x > y) ? x : y;
	if( z > xyMax ) return z;
	return xyMax;
}


/*************************************************************************
 * _ARP-SET-VALUE-WINDOW
 *************************************************************************/
class _ArpSetValueWindow : public BWindow,
						public ArpBitmapCache
{
public:
	_ArpSetValueWindow(	BRect noChangeZone,
						orientation direction,
						float initialZoom,
						float min, float max,
						const ArpRangeControl& control);

	float	StartWidth(float zoom, const band_vec& bands) const;
	float	StopWidth(float zoom, const band_vec& bands) const;

	/* Answer my current zoom value.
	 */
	float Zoom();
	void SetZoom(float value);
	void SetZoomToPixel(BPoint where);
	
private:
	ArpSetRangeView	*mView;	
};

/*************************************************************************
 * ARP-SET-RANGE-VIEW
 *************************************************************************/
class ArpSetRangeView : public BView
{
public:
	ArpSetRangeView(BRect frame,
					orientation direction,
					BRect noChangeZone,
					float initialZoom,
					float minZoom, float maxZoom,
					const ArpRangeControl& control);
	virtual ~ArpSetRangeView();
	
	/* Answer my current zoom value.
	 */
	float Zoom();
	virtual void Draw(BRect updateRect);
	virtual void DrawOn(BRect updateRect, BView* view);
	virtual void SetZoom(float value);
	/* Take the supplied pixel and set my zoom value accordingly.
	 * where is expected to be in screen coordinates.
	 */
	virtual void SetZoomToPixel(BPoint where) = 0;
	
protected:
	const ArpRangeControl&	mControl;
	orientation				mDirection;
	BRect					mNoChangeZone;
	float					mInitialZoom, mZoom;
	float					mMinZoom, mMaxZoom;
	/* This is the current pixel position of the value indicator.
	 */
	BPoint					mPositionIndicator;
	/* This variable is filled whenever this view is opened with
	 * a fixed-size band that contains the initial value.  In this
	 * case, the size of the band should not be factored into the
	 * method that calculates the current value from the current
	 * mouse position.
	 */
	float					mNoChangeBandSize;
	/* This is the background color taken from the range control.
	 */
	rgb_color				mBackgroundColor;
	
	// Not very well named -- this method cleans up the image a little
	// for my specific orientation.
	virtual void	DrawDetailsOn(BRect updateRect, BView *view) = 0;
	// Draw the triangle indicating what the percent range is/
	virtual void	DrawRangeOn(BView *view) = 0;
	// Draw the position indicator for what the current percent is.
	virtual void	DrawIndicatorOn(BView *view) = 0;

	float			ValueFromPixel(float pixel) const;
};

/*************************************************************************
 * _ARP-SET-VALUE-X-VIEW
 *************************************************************************/
class _ArpSetValueXView : public ArpSetRangeView
{
public:
	_ArpSetValueXView(	BRect frame,
					orientation direction,
					BRect noChangeZone,
					float initialZoom,
					float minZoom, float maxZoom,
					const ArpRangeControl& control);

	virtual void	SetZoomToPixel(BPoint where);

protected:
	// Not very well named -- this method cleans up the image a little
	// for my specific orientation.
	virtual void	DrawDetailsOn(BRect updateRect, BView *view);
	// Draw the triangle indicating what the percent range is/
	virtual void	DrawRangeOn(BView *view);
	void			DrawRangeTriangleOn(BView* view);
	// Draw the position indicator for what the current percent is.
	virtual void	DrawIndicatorOn(BView *view);
};

/*************************************************************************
 * _ARP-SET-VALUE-Y-VIEW
 *************************************************************************/
class _ArpSetValueYView : public ArpSetRangeView
{
public:
	_ArpSetValueYView(	BRect frame,
					orientation direction,
					BRect noChangeZone,
					float initialZoom,
					float minZoom, float maxZoom,
					const ArpRangeControl& control);

	virtual void SetZoomToPixel(BPoint where);

protected:
	// Not very well named -- this method cleans up the image a little
	// for my specific orientation.
	virtual void	DrawDetailsOn(BRect updateRect, BView *view);
	// Draw the triangle indicating what the percent range is/
	virtual void	DrawRangeOn(BView *view);
	void			DrawRangeTriangleOn(BView* view);
	// Draw the position indicator for what the current percent is.
	virtual void	DrawIndicatorOn(BView *view);
};

/*************************************************************************
 * _ARP-VALUE-TEXT-WINDOW
 *************************************************************************/
class _ArpValueTextWindow : public BWindow,
							 public ArpBitmapCache
{
public:
	_ArpValueTextWindow(BPoint atPoint,
						 float minX, float maxX, float initialX,
						 float minY, float maxY, float initialY,
						 const char* prefix, const char* suffix,
						 rgb_color bg);

	void SetZoom(float zoomX, float zoomY);

private:
	_ArpShowTextView	*mView;
};

/*************************************************************************
 * _ARP-SHOW-TEXT-VIEW
 *************************************************************************/
class _ArpShowTextView : public BView
{
public:
	/* This class sizes itself appropriately in the constructor.
	 */
	_ArpShowTextView(	float minX, float maxX,	float initialX,
						float minY, float maxY,	float initialY,
						const char* prefix, const char* suffix,
						rgb_color bg);
	virtual ~_ArpShowTextView();
	
	virtual void Draw(BRect updateRect);
	void SetZoom(float zoomX, float zoomY);
	
private:
	float			mMinX, mMaxX,
					mMinY, mMaxY,
					mZoomX, mZoomY;
	BString			mPrefix, mSuffix;
	// This is the number of pixels between the x and y percent strings.
	// It will be zero if only one of the values is being displayed.
	float			mSeparator;
	BPoint			mXPoint, mYPoint;
	/* This is the background color taken from the range control.
	 */
	rgb_color				mBackgroundColor;
	
	float FontHeight();
	bool ShowX()			{ return mMinX != mMaxX; }
	bool ShowY()			{ return mMinY != mMaxY; }

	void FreeMemory();
	
	// Rendering
	BBitmap*	mBitmap;
	BView*		mDrawView;

	void DrawOn(BRect updateRect, BView *view);
};

/*************************************************************************
 * ARP-RANGE-CONTROL
 *************************************************************************/
ArpRangeControl::ArpRangeControl(	BRect frame,
									const char* name,
									uint32 resizeMask,
									float initialX,
									float initialY,
									uint32 displayFlags)
		: BView(frame, name, resizeMask, B_WILL_DRAW),
		mDisplayFlags(displayFlags),
		mUpdatedMsg(0), mFinishedMsg(0),
		mZoomX(initialX), mZoomY(initialY),
		mMinX(0), mMaxX(0),
		mMinY(0), mMaxY(0),
		mTextX(100), mTextY(100), mSuffix("%"),
		mSetH(0), mSetV(0), mShowZoom(0), mTracking(false), mSticky(false)
{
	/* There's a bug that lets menus have alpha colour components, which
	 * causes alpha images to display incorrectly.  This is fixed in an
	 * upcoming version, but for now we make sure the alpha is the default.
	 */
	rgb_color		c = ui_color(I_BG_COLOR);
	c.alpha = 255;
	SetViewColor(c);
	SetLowColor(c);
	mRangeC = tint_color( c, B_DARKEN_3_TINT );

	if (!mMini0)	mMini0	= ImageManager().FindBitmap( MINI_DIGIT_0_STR );
	if (!mMini1)	mMini1	= ImageManager().FindBitmap( MINI_DIGIT_1_STR );
	if (!mMini2)	mMini2	= ImageManager().FindBitmap( MINI_DIGIT_2_STR );
	if (!mMini3)	mMini3	= ImageManager().FindBitmap( MINI_DIGIT_3_STR );
	if (!mMini4)	mMini4	= ImageManager().FindBitmap( MINI_DIGIT_4_STR );
	if (!mMini5)	mMini5	= ImageManager().FindBitmap( MINI_DIGIT_5_STR );
	if (!mMini6)	mMini6	= ImageManager().FindBitmap( MINI_DIGIT_6_STR );
	if (!mMini7)	mMini7	= ImageManager().FindBitmap( MINI_DIGIT_7_STR );
	if (!mMini8)	mMini8	= ImageManager().FindBitmap( MINI_DIGIT_8_STR );
	if (!mMini9)	mMini9	= ImageManager().FindBitmap( MINI_DIGIT_9_STR );
}

ArpRangeControl::~ArpRangeControl()
{
	delete mUpdatedMsg;
	delete mFinishedMsg;
	for( uint32 k = 0; k < mHorizontals.size(); k++ ) delete mHorizontals[k];
	for( uint32 k = 0; k < mVerticals.size(); k++ ) delete mVerticals[k];
}

uint32 ArpRangeControl::DisplayFlags() const
{
	return mDisplayFlags;
}

void ArpRangeControl::SetDisplayFlags(uint32 displayFlags)
{
	mDisplayFlags = displayFlags;
}

void ArpRangeControl::AddHorizontalBand(float start, float stop, float pixelSize)
{
	ArpRangeBand*	band = new ArpRangeBand( start, stop, pixelSize );
	if( band ) AddHorizontalBand( band );
}

void ArpRangeControl::AddHorizontalBand(ArpRangeBand* band)
{
	mHorizontals.push_back( band );
	/* Set my minimum and maximum values to be the smallest and largest
	 * start / stop values in all my bands.
	 */
	mMinX = (band->mStart < band->mStop) ? band->mStart : band->mStop;
	mMaxX = (band->mStop > band->mStart) ? band->mStop : band->mStart;
	for( uint32 k = 0; k < mHorizontals.size(); k++ ) {
		float	start = mHorizontals[k]->mStart;
		float	stop = mHorizontals[k]->mStop;
		mMinX = smallest( start, stop, mMinX );
		mMaxX = largest( start, stop, mMaxX );
	}
}

const band_vec& ArpRangeControl::HorizontalBands() const
{
	return mHorizontals;
}

void ArpRangeControl::AddVerticalBand(float start, float stop, float pixelSize)
{
	ArpRangeBand*	band = new ArpRangeBand( start, stop, pixelSize );
	if( band ) AddVerticalBand( band );
}

void ArpRangeControl::AddVerticalBand(ArpRangeBand* band)
{
	mVerticals.push_back( band );
	/* Set my minimum and maximum values to be the smallest and largest
	 * start / stop values in all my bands.
	 */
	mMinY = (band->mStart < band->mStop) ? band->mStart : band->mStop;
	mMaxY = (band->mStop > band->mStart) ? band->mStop : band->mStart;
	for( uint32 k = 0; k < mVerticals.size(); k++ ) {
		float	start = mVerticals[k]->mStart;
		float	stop = mVerticals[k]->mStop;
		mMinY = smallest( start, stop, mMinY );
		mMaxY = largest( start, stop, mMaxY );
	}
}

const band_vec& ArpRangeControl::VerticalBands() const
{
	return mVerticals;
}

void ArpRangeControl::AddHorizontalIcon(float start, float stop, const BBitmap* image)
{
	AddHorizontalIcon(start, stop, image, start, stop);
}

void ArpRangeControl::AddHorizontalIcon(float start, float stop, const BBitmap* image,
										float labelStart, float labelStop)
{
	float	size = 10;
	if( image ) size = image->Bounds().Width();
	ArpBitmapRangeBand*	band = new ArpBitmapRangeBand( start, stop, size, image, labelStart, labelStop );
	if( band ) AddHorizontalBand( band );
}

void ArpRangeControl::AddVerticalIcon(float start, float stop, const BBitmap* image)
{
	AddVerticalIcon(start, stop, image, start, stop);
}

void ArpRangeControl::AddVerticalIcon(float start, float stop, const BBitmap* image,
										float labelStart, float labelStop)
{
	float	size = 10;
	if( image ) size = image->Bounds().Height();
	ArpBitmapRangeBand*	band = new ArpBitmapRangeBand( start, stop, size, image, labelStart, labelStop );
	if( band ) AddVerticalBand( band );
}

void ArpRangeControl::AddHorizontalIntermediate(float start, float stop, float pixelSize)
{
	ArpIntermediateRangeBand*	band = new ArpIntermediateRangeBand( start, stop, pixelSize );
	if( band ) AddHorizontalBand( band );
}

void ArpRangeControl::AddVerticalIntermediate(float start, float stop, float pixelSize)
{
	ArpIntermediateRangeBand*	band = new ArpIntermediateRangeBand( start, stop, pixelSize );
	if( band ) AddVerticalBand( band );
}

void ArpRangeControl::SetUpdatedMessage(BMessage* msg)
{
	mUpdatedMsg = msg;
}

BMessage* ArpRangeControl::UpdatedMessage() const
{
	return mUpdatedMsg;
}

void ArpRangeControl::SetFinishedMessage(BMessage* msg)
{
	mFinishedMsg = msg;
}

BMessage* ArpRangeControl::FinishedMessage() const
{
	return mFinishedMsg;
}

void ArpRangeControl::SetTextScale(float x, float y)
{
	mTextX = x;
	mTextY = y;
}

void ArpRangeControl::SetTextContext(const char* prefix, const char* suffix)
{
	mPrefix = prefix;
	mSuffix = suffix;
}

void ArpRangeControl::SetRangeColor(rgb_color c)
{
	mRangeC = c;
}

rgb_color ArpRangeControl::RangeColor() const
{
	return mRangeC;
}

bool ArpRangeControl::CanZoomX() const
{
	return mHorizontals.size() > 0;
}

bool ArpRangeControl::CanZoomY() const
{
	return mVerticals.size() > 0;
}

float ArpRangeControl::ZoomX() const
{
	return mZoomX;
}

float ArpRangeControl::ZoomY() const
{
	return mZoomY;
}

void ArpRangeControl::SetZoomX(float x)
{
	if (x != mZoomX) {
		mZoomX = x;
		Invalidate();
	}
}

void ArpRangeControl::SetZoomY(float y)
{
	if (y != mZoomY) {
		mZoomY = y;
		Invalidate();
	}
}

static void build_bitmap_vec(BString str, vector<const BBitmap*>& vec)
{
	int32		len = str.Length();
	for(int32 k = 0; k < len; k++) {
		/* As soon as I hit a non-digit, fail.
		 */
		int32	val = str.ByteAt(k) - 48;
		if (val == 0) vec.push_back(mMini0);
		else if (val == 1) vec.push_back(mMini1);
		else if (val == 2) vec.push_back(mMini2);
		else if (val == 3) vec.push_back(mMini3);
		else if (val == 4) vec.push_back(mMini4);
		else if (val == 5) vec.push_back(mMini5);
		else if (val == 6) vec.push_back(mMini6);
		else if (val == 7) vec.push_back(mMini7);
		else if (val == 8) vec.push_back(mMini8);
		else if (val == 9) vec.push_back(mMini9);
		else return;
	}
}

static void draw_value_label(BRect frame, BView* view, float value)
{
	BString		str;
	str << value;
	vector<const BBitmap*>	vec;
	build_bitmap_vec(str, vec);
	
	BRect		r(0, 0, 0, 0);
	for (uint32 k = 0; k < vec.size(); k++) {
		if (vec[k]) {
			BRect	b = vec[k]->Bounds();
			r.right += b.Width() + 2;
			if ( b.bottom > r.bottom ) r.bottom = b.bottom;
		}
	}
	if (r.right == 0 || r.bottom == 0) return;

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	float	left = frame.left + ((frame.Width() - r.Width()) / 2);
	if (left <= frame.left) left = frame.left + 1;
	float	top = frame.top + ((frame.Height() - r.Height()) / 2);
	for (uint32 k = 0; k < vec.size(); k++) {
		if (vec[k]) {
			BRect	b = vec[k]->Bounds();
			if (left + b.Width() > frame.right) {
				view->SetHighColor(0, 0, 0);
				view->StrokeLine(BPoint(left, top + r.Height()), BPoint(left, top + r.Height()));
				view->StrokeLine(BPoint(left + 2, top + r.Height()), BPoint(left + 2, top + r.Height()));
				view->StrokeLine(BPoint(left + 4, top + r.Height()), BPoint(left + 4, top + r.Height()));
				break;
			}
			view->DrawBitmapAsync( vec[k], BPoint(left, top) );
			left += 2 + b.Width();
		}
	}
	if (view->Window()) view->Sync();
	view->SetDrawingMode(mode);
}

void ArpRangeControl::Draw(BRect clip)
{
	inherited::Draw(clip);
	rgb_color		bg = LowColor();
	SetHighColor( bg );
	FillRect(clip);
	DrawBorder(clip);

	BRect	bandBounds = Bounds();
	bandBounds.left += 2;
	bandBounds.top += 2;
	bandBounds.right -= 2;
	bandBounds.bottom -= 2;
	BRegion r(bandBounds);
	ConstrainClippingRegion(&r);
	if( !DrawLabelFromBands(bandBounds, this) ) { 
		draw_value_label(bandBounds, this, mZoomY);
	}
	ConstrainClippingRegion(NULL);
}

bool ArpRangeControl::DrawLabelFromBands(BRect clip, BView* view)
{
	for( uint32 k = 0; k < mHorizontals.size(); k++ ) {
		if( mHorizontals[k]->UseAsLabel( mZoomX ) ) {
			mHorizontals[k]->DrawControlForeground( clip, view, mZoomX );
			return true;
		} else if( mHorizontals[k]->UseAsIntermediate( mZoomX ) ) {
			mHorizontals[k]->DrawControlForeground( clip, view, mZoomX );
			return true;
#if 0
			float space = clip.Width();
			float off = (mHorizontals[k]->StartToBisectionPixel(mZoomX) * space)
					  / mHorizontals[k]->mPixelSize;
			if (k > 0)
				mHorizontals[k-1]->DrawForeground( clip.OffsetByCopy(BPoint(off-space, 0)), view, mZoomX );
			if (k < (mHorizontals.size()-1))
				mHorizontals[k+1]->DrawForeground( clip.OffsetByCopy(BPoint(off, 0)), view, mZoomX );
			return true;
#endif
		}
	}
	for( uint32 k = 0; k < mVerticals.size(); k++ ) {
		if( mVerticals[k]->UseAsLabel( mZoomY ) ) {
			mVerticals[k]->DrawControlForeground(clip, view, mZoomY);
			return true;
		} else if( mVerticals[k]->UseAsIntermediate( mZoomY ) ) {
			mVerticals[k]->DrawControlForeground(clip, view, mZoomY);
			return true;
#if 0
			float space = clip.Height();
			float off = (mVerticals[k]->StartToBisectionPixel(mZoomY) * space)
					  / mVerticals[k]->mPixelSize;
			if (k > 0)
				mVerticals[k-1]->DrawForeground( clip.OffsetByCopy(BPoint(0, off-space)), view );
			if (k < (mVerticals.size()-1))
				mVerticals[k+1]->DrawForeground( clip.OffsetByCopy(BPoint(0, off)), view );
			return true;
#endif
		}
	}
	return false;
}

void ArpRangeControl::DrawBorder(BRect clip)
{
	BRect		b = Bounds();
	const rgb_color bg = LowColor();
	SetHighColor( tint_color(bg, B_DARKEN_3_TINT) );
	StrokeRect(b);
	SetHighColor(tint_color(bg, B_DARKEN_2_TINT));
	StrokeLine(BPoint(b.left + 2, b.bottom - 1), BPoint(b.right - 1, b.bottom - 1));
	StrokeLine(BPoint(b.right - 1, b.top + 2), BPoint(b.right - 1, b.bottom - 2));
	SetHighColor(255, 255, 255);
	StrokeLine(BPoint(b.left + 1, b.top + 1), BPoint(b.left + 1, b.bottom - 2));
	StrokeLine(BPoint(b.left + 2, b.top + 1), BPoint(b.right - 2, b.top + 1));
}

void ArpRangeControl::MouseDown(BPoint pt)
{
	// If the mouse is being pressed in sticky mode, commit the currently
	// selected values.
	if (mSticky) {
		mSticky = false;
		mTracking = false;
		SetEventMask(0, 0);
		CommitValues();
		return;
	}
	
	if (!mTracking) {
		mInitPoint = ConvertToScreen(pt);
		mTracking = true;
		SetMouseEventMask(B_POINTER_EVENTS,
						  B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
	
		BRect	b = Bounds();	
		BRect	noChangeZone( ConvertToScreen(b.LeftTop()),
							  ConvertToScreen(b.RightBottom()) );
		if( CanZoomX() )
			mSetH = new _ArpSetValueWindow( noChangeZone, B_HORIZONTAL, mZoomX, mMinX, mMaxX, *this );
		if( CanZoomY() )
			mSetV = new _ArpSetValueWindow( noChangeZone, B_VERTICAL, mZoomY, mMinY, mMaxY, *this );
		if( DisplayFlags()&ARP_DISPLAY_TEXT )
			mShowZoom = new _ArpValueTextWindow(ConvertToScreen(b.RightBottom() + BPoint(10, 10)),
												mMinX * mTextX, mMaxX * mTextX, mZoomX * mTextX,
												mMinY * mTextY, mMaxY * mTextY, mZoomY * mTextY,
												mPrefix.String(), mSuffix.String(),
												LowColor() );
		if( mSetH ) mSetH->Show();
		if( mSetV ) mSetV->Show();
		if( mShowZoom ) mShowZoom->Show();
	}
}

void ArpRangeControl::MouseUp(BPoint where)
{
	if (mTracking) {
		// If the mouse is released inside if the control, then we should move
		// into "sticky" mode -- keep the popups shown until a button is pressed.
		if (Bounds().Contains(where)) {
			mSticky = true;
			SetEventMask(B_POINTER_EVENTS,
						 B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
			return;
		}
		
		mTracking = false;
		CommitValues();
	}
}

void ArpRangeControl::MouseMoved(BPoint pt, uint32 code, const BMessage *msg)
{
	if( !mTracking ) return;
	
	const BRect b(Bounds());
	
	float	newZoomX = 0, newZoomY = 0;
	float	oldZoomX = mZoomX, oldZoomY = mZoomY;
	if( mSetH ) {
		const float h = b.Height();
		// We want to reset back to the initial value if there is no
		// Y axis, and the current location is outside of the X axis
		// "active" area.
		if (mSetV || (pt.y > (b.top-h) && pt.y < (b.bottom+h))) {
			mSetH->SetZoomToPixel( ConvertToScreen(pt) );
		} else {
			mSetH->SetZoomToPixel( mInitPoint );
		}
		newZoomX = mSetH->Zoom();
	}
	if( mSetV ) {
		const float w = b.Width();
		// We want to reset back to the initial value if there is no
		// X axis, and the current location is outside of the Y axis
		// "active" area.
		if (mSetH || (pt.x > (b.left-w) && pt.x < (b.right+w))) {
			mSetV->SetZoomToPixel( ConvertToScreen(pt) );
		} else {
			mSetV->SetZoomToPixel( mInitPoint );
		}
		newZoomY = mSetV->Zoom();
	}
	if( newZoomX !=  0) mZoomX = newZoomX;
	if( newZoomY != 0 ) mZoomY = newZoomY;
	if( mShowZoom != 0 ) mShowZoom->SetZoom( mZoomX * mTextX, mZoomY * mTextY );

	if ( (oldZoomX != mZoomX) || (oldZoomY != mZoomY) ) {
		if( mUpdatedMsg ) {
			BMessage	copy(*mUpdatedMsg);
			copy.AddFloat( "x_value", mZoomX );
			copy.AddFloat( "y_value", mZoomY );
			SendNotices( mUpdatedMsg->what, &copy );
		}
	}
}

void ArpRangeControl::CommitValues()
{
	float		xValue = 0, yValue = 0;

	if( mSetH ) {
		xValue = mSetH->Zoom();
		mSetH->PostMessage( B_QUIT_REQUESTED );
		mSetH = 0;
	}
	if( mSetV ) {
		yValue = mSetV->Zoom();
		mSetV->PostMessage( B_QUIT_REQUESTED );
		mSetV = 0;
	}
	if( mShowZoom ) {
		mShowZoom->PostMessage( B_QUIT_REQUESTED );
		mShowZoom = 0;
	}

	if( mFinishedMsg && ( xValue != 0 || yValue != 0 ) ) {
		BMessage	copy(*mFinishedMsg);
		if (xValue) copy.AddFloat( "x_value", xValue );
		if (yValue) copy.AddFloat( "y_value", yValue );
		SendNotices(mFinishedMsg->what, &copy);
		/* Also notify whatever parent contains me.  This is a
		 * quick hack to get the AmDurationControl working without
		 * clients watching it.
		 */
		if ( Window() && Parent() ) {
			Window()->PostMessage( &copy, Parent() );
		}
	}
}

void ArpRangeControl::GetPreferredSize(float* width, float* height)
{
	*width = 0;
	for( size_t i=0; i<mHorizontals.size(); i++ ) {
		if( *width < ceil(mHorizontals[i]->mPixelSize) ) {
			*width = ceil(mHorizontals[i]->mPixelSize);
		}	
	}
	for( size_t i=0; i<mVerticals.size(); i++ ) {
		if( *width < ceil(mVerticals[i]->mPixelSize) ) {
			*width = ceil(mVerticals[i]->mPixelSize);
		}	
	}
	(*width) += 4;
	*height = *width;
}

bool ArpRangeControl::LargeToSmall(orientation direction) const
{
	const band_vec&		bands = (direction == B_HORIZONTAL) ? mHorizontals : mVerticals;
	if( bands.size() < 1 ) return true;
	float	start = bands[0]->mStart;
	for( uint32	k = 0; k < bands.size(); k++ ) {
		if( start < bands[k]->mStop ) return false;
		if( start > bands[k]->mStop ) return true;
	}
	return true;
}

/*************************************************************************
 * _ARP-SET-VALUE-WINDOW
 *************************************************************************/
_ArpSetValueWindow::_ArpSetValueWindow(	BRect noChangeZone,
										orientation direction,
										float initialValue,
										float min, float max,
										const ArpRangeControl& control)
		: BWindow(noChangeZone, "zoom",
				  B_NO_BORDER_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL, 0)
{
	BRect	viewZone;
	if( direction == B_HORIZONTAL ) {
		float	leftWidth = StartWidth( initialValue, control.HorizontalBands() ) + (H_BORDER_X * 2) - 1;
		float	rightWidth = StopWidth( initialValue, control.HorizontalBands() ) + (H_BORDER_X * 2) - 1;
		MoveTo( BPoint(noChangeZone.left - leftWidth, noChangeZone.top) );
		ResizeTo( noChangeZone.Width() + leftWidth + rightWidth, noChangeZone.Height() );
		viewZone.Set( leftWidth, 0, leftWidth + noChangeZone.Width(), 0 + noChangeZone.Height() );
	} else {
		float	topHeight = StartWidth( initialValue, control.VerticalBands() ) + (V_BORDER_Y * 2) - 1;
		float	bottomHeight = StopWidth( initialValue, control.VerticalBands() ) + (V_BORDER_Y * 2) - 1;
		MoveTo( BPoint(noChangeZone.left, noChangeZone.top - topHeight) );
		ResizeTo( noChangeZone.Width(), noChangeZone.Height() + topHeight + bottomHeight );
		viewZone.Set( 0, topHeight, 0 + noChangeZone.Width(), topHeight + noChangeZone.Height() );
	}

	BRect			viewFrame(0, 0, Bounds().Width(), Bounds().Height());
	if( direction == B_HORIZONTAL ) 
		mView = new _ArpSetValueXView(viewFrame, direction, viewZone,
								   initialValue, min, max, control);
	else
		mView = new _ArpSetValueYView(viewFrame, direction, viewZone,
								   initialValue, min, max, control);
	if (mView) AddChild(mView);
}

float _ArpSetValueWindow::StartWidth(float value, const band_vec& bands) const
{
	float	pixel = 0;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		if( bands[k]->Contains( value ) ) {
			pixel += bands[k]->StartToBisectionPixel( value );
			return pixel;
		}
		pixel += bands[k]->mPixelSize;
	}
	return pixel;
}

float _ArpSetValueWindow::StopWidth(float value, const band_vec& bands) const
{
	float	pixel = 0;
	bool	found = false;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		if( bands[k]->Contains( value ) ) {
			pixel += bands[k]->BisectionToStopPixel( value );
			found = true;
		} else if( found ) pixel += bands[k]->mPixelSize;
	}
	return pixel;
}

float _ArpSetValueWindow::Zoom()
{
	if( !mView ) return 0;
	return mView->Zoom();
}

void _ArpSetValueWindow::SetZoom(float value)
{
	if( mView ) mView->SetZoom(value);
}

void _ArpSetValueWindow::SetZoomToPixel(BPoint where)
{
	if( mView ) mView->SetZoomToPixel(where);
}

/*************************************************************************
 * ARP-SET-RANGE-VIEW
 *************************************************************************/
ArpSetRangeView::ArpSetRangeView(	BRect frame,
									orientation direction,
									BRect noChangeZone,
									float initialZoom,
									float minZoom, float maxZoom,
									const ArpRangeControl& control)
		: BView(frame, "zoom", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		mControl(control), mDirection(direction), mNoChangeZone(noChangeZone),
		mInitialZoom(initialZoom), mZoom(initialZoom),
		mMinZoom(minZoom), mMaxZoom(maxZoom),
		mPositionIndicator(0, 0),
		mNoChangeBandSize(0)
{
	SetViewColor( B_TRANSPARENT_COLOR );
	SetLowColor(control.LowColor());
	mBackgroundColor = control.LowColor();
}

ArpSetRangeView::~ArpSetRangeView()
{
}

float ArpSetRangeView::Zoom()
{
	return mZoom;
}

void ArpSetRangeView::Draw(BRect updateRect)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if( cache ) into = cache->StartDrawing( this, updateRect );
	
	DrawOn(updateRect, into);
	if( cache ) cache->FinishDrawing( into );
}

void ArpSetRangeView::DrawOn(BRect updateRect, BView* view)
{
	BRect		b = Bounds();
	const rgb_color bg = mBackgroundColor;
	view->SetViewColor(bg);
	view->SetHighColor(bg);
	view->FillRect(b);
	view->SetHighColor(0, 0, 0);
	view->StrokeRect(b);
	
	//----DRAW THE BORDERS----
	view->SetHighColor(tint_color(bg, B_LIGHTEN_2_TINT));
	view->StrokeLine(BPoint(b.left + 1, b.top + 1), BPoint(b.left + 1, b.bottom - 2));
	view->StrokeLine(BPoint(b.left + 2, b.top + 1), BPoint(b.right - 2, b.top + 1));

	view->SetHighColor(tint_color(bg, B_DARKEN_2_TINT));
	view->StrokeLine(BPoint(b.left + 2, b.bottom - 1), BPoint(b.right - 1, b.bottom - 1));
	view->StrokeLine(BPoint(b.right - 1, b.top + 2), BPoint(b.right - 1, b.bottom - 1));

	//----DRAW THE NO CHANGE ZONE----
	view->SetHighColor(tint_color(bg, B_DARKEN_3_TINT));
	view->FillRect(mNoChangeZone);

	view->SetHighColor(0, 0, 0);
	view->StrokeRect(mNoChangeZone);

	//----DRAW THE SUBCLASS-SPECIFIC INFO----
	DrawDetailsOn(updateRect, view);
	DrawRangeOn(view);
	if( mControl.DisplayFlags()& ARP_DISPLAY_VALUE_BAR )
		DrawIndicatorOn(view);
}

float ArpSetRangeView::ValueFromPixel(float pixel) const
{
	const band_vec&		bands = (mDirection == B_HORIZONTAL)
								? mControl.HorizontalBands()
								: mControl.VerticalBands();

	if( bands.size() < 1 ) return 0;
	if( pixel < 0 ) return bands[0]->mStart;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		if( pixel < bands[k]->mPixelSize ) {
			return bands[k]->ZoomFromPixel( pixel );
		}
		pixel -= bands[k]->mPixelSize;
	}
	return bands[ bands.size() -1 ]->mStop;
}

void ArpSetRangeView::SetZoom(float value)
{
	if( LockLooper() == false ) return;
	mZoom = value;
	Invalidate();
	UnlockLooper();
}

/*************************************************************************
 * _ARP-SET-VALUE-X-VIEW
 *************************************************************************/
_ArpSetValueXView::_ArpSetValueXView(	BRect frame,
										orientation direction,
										BRect noChangeZone,
										float initialZoom,
										float minZoom, float maxZoom,
										const ArpRangeControl& control)
		: ArpSetRangeView(frame, direction, noChangeZone, initialZoom, minZoom, maxZoom, control)
{
}

void _ArpSetValueXView::SetZoomToPixel(BPoint where)
{
	if( LockLooper() == false ) return;
	BPoint		pt = ConvertFromScreen(where);
	BPoint		prevIndicator = mPositionIndicator;
	mPositionIndicator = pt;
	if ( prevIndicator.x == mPositionIndicator.x ) {
		UnlockLooper();
		return;
	}
	
	float	newZoom = 0;
	if( pt.x < (mNoChangeZone.left - 3) )
		newZoom = ValueFromPixel( pt.x - H_BORDER_X );
	else if( pt.x > (mNoChangeZone.right + 3) )
		newZoom = ValueFromPixel( pt.x + H_BORDER_X - mNoChangeZone.Width() - 5 );
	else
		newZoom = mInitialZoom;
#if 0
//This is from the y, which seems to be more correct
	float	newZoom = 0;
	if( pt.y < (mNoChangeZone.top - 3) )
		newZoom = ValueFromPixel( pt.y - V_BORDER_Y );
	else if( pt.y > (mNoChangeZone.bottom + 3) )
		newZoom = ValueFromPixel( pt.y - V_BORDER_Y - mNoChangeZone.Height() - 6 + mNoChangeBandSize );
	else
		newZoom = mInitialZoom;
#endif
	SetZoom(newZoom);
	UnlockLooper();
}

void _ArpSetValueXView::DrawDetailsOn(BRect updateRect, BView *view)
{
	BRect	b = Bounds();
	// If the initial zoom value is greater than the minimum, then there
	// will always be at least a little bit of the view that extends to
	// the left of the no change zone.
	if (mInitialZoom > mMinZoom) {
		view->SetHighColor(tint_color(mBackgroundColor, B_DARKEN_2_TINT));
		view->StrokeLine(BPoint(mNoChangeZone.left - 1, b.top + 2), BPoint(mNoChangeZone.left - 1, b.bottom - 2));
	}
	// Same deal, but now the view that extends right.
	if (mInitialZoom < mMaxZoom) {
		view->SetHighColor(tint_color(mBackgroundColor, B_LIGHTEN_2_TINT));
		view->StrokeLine(BPoint(mNoChangeZone.right + 1, b.top + 2), BPoint(mNoChangeZone.right + 1, b.bottom - 2));
	}
}

void _ArpSetValueXView::DrawRangeOn(BView *view)
{
	const band_vec&		bands = mControl.HorizontalBands();
	BRect				b = Bounds();

	/* Give the bands an opportunity to draw into the background.
	 */
	float	left = H_BORDER_X;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		ArpRangeBand&	band = *( bands[k] );
		/* If the band has the initial zoom, then it is either the no change zone
		 * or it's been bisected.  Either way, we don't give it a chance to draw.
		 */
		if( band.Contains( mInitialZoom ) ) {
			if( band.mStart == band.mStop ) {
				left = mNoChangeZone.right + 4;
				mNoChangeBandSize = band.mPixelSize;
			} else left += band.mPixelSize + mNoChangeZone.Width() + 6;
		} else {
			bool	selected = (mPositionIndicator.x >= left && mPositionIndicator.x < left + band.mPixelSize);
			band.DrawBackground( BRect(left, H_BORDER_Y, left + band.mPixelSize - 1, b.bottom - H_BORDER_Y), view, selected );
			left += band.mPixelSize;
		}
	}

	/* Draw the value range.
	 */
	if( mControl.DisplayFlags()&ARP_DISPLAY_RANGE )
		DrawRangeTriangleOn( view );

	/* Give the bands an opportunity to draw into the foreground.
	 */
	left = H_BORDER_X;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		ArpRangeBand&	band = *( bands[k] );
		/* If the band has the initial zoom, then it is either the no change zone
		 * or it's been bisected.  Either way, we don't give it a chance to draw.
		 */
		if( band.Contains( mInitialZoom ) ) {
			if( band.mStart == band.mStop ) left = mNoChangeZone.right + 4;
			else left += band.mPixelSize + mNoChangeZone.Width() + 6;
		} else {
			band.DrawForeground( BRect(left, H_BORDER_Y, left + band.mPixelSize - 1, b.bottom - H_BORDER_Y), view, 0 );
			left += band.mPixelSize;
		}
	}

	/* One last iteration, to give any band that wants to be the label
	 * a chance to draw itself.
	 */
	b = mNoChangeZone;
	b.left += 2;
	b.top += 2;
	b.right -= 2;
	b.bottom -= 2;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		if( bands[k]->UseAsLabel( mInitialZoom ) ) {
			bands[k]->DrawForeground( b, view, 0 );
			break;
		}
	}
}

void _ArpSetValueXView::DrawRangeTriangleOn(BView* view)
{
	BRect	b = Bounds();
	float	noChangeL = mNoChangeZone.left - 3;
	float	noChangeR = mNoChangeZone.right + 3;
	float	leftWidth = noChangeL - (b.left + H_BORDER_X);
	float	rightWidth = b.right - H_BORDER_X - noChangeR;
	bool	largeToSmall = mControl.LargeToSmall( B_HORIZONTAL );
	
	if( leftWidth == 0 && rightWidth == 0 ) return;
	view->SetHighColor( mControl.RangeColor() );
	if( leftWidth <= 0 ) {
		float	x = ( largeToSmall ) ? noChangeR : b.right - H_BORDER_X;
		view->FillTriangle(	BPoint(noChangeR, b.bottom - H_BORDER_Y),
							BPoint(b.right - H_BORDER_X, b.bottom - H_BORDER_Y),
							BPoint(x, H_BORDER_Y) );
	} else if( rightWidth <= 0 ) {
		float	x = ( largeToSmall ) ? H_BORDER_X : noChangeL;
		view->FillTriangle(	BPoint(H_BORDER_X, b.bottom - H_BORDER_Y),
							BPoint(noChangeL, b.bottom - H_BORDER_Y),
							BPoint(x, H_BORDER_Y) );
	} else {
		float	height = b.bottom - (H_BORDER_Y * 2);
		float	y = (height * noChangeL) / (leftWidth + rightWidth);
		if( largeToSmall ) {
			y = height - y;
			view->FillTriangle(	BPoint(H_BORDER_X, H_BORDER_Y + y),
								BPoint(noChangeL, H_BORDER_Y + y),
								BPoint(H_BORDER_X, H_BORDER_Y) );
			view->FillRect( BRect(H_BORDER_X, H_BORDER_Y + y, noChangeL, b.bottom - H_BORDER_Y) );
			view->FillTriangle(	BPoint(noChangeR, H_BORDER_Y + y),
								BPoint(noChangeR, b.bottom - H_BORDER_Y),
								BPoint(b.right - H_BORDER_X, b.bottom - H_BORDER_Y) );
		} else {
			view->FillTriangle(	BPoint(H_BORDER_X, b.bottom - H_BORDER_Y),
								BPoint(noChangeL, b.bottom - H_BORDER_Y),
								BPoint(noChangeL, H_BORDER_Y + y) );
			view->FillTriangle(	BPoint(noChangeR, H_BORDER_Y + y),
								BPoint(b.right - H_BORDER_X, H_BORDER_Y + y),
								BPoint(b.right - H_BORDER_X, H_BORDER_Y) );
			view->FillRect( BRect(noChangeR, H_BORDER_Y + y, b.right - H_BORDER_X, b.bottom - H_BORDER_Y) );
		}
	}
}

void _ArpSetValueXView::DrawIndicatorOn(BView *view)
{
	if( mPositionIndicator.x >= (mNoChangeZone.left - 3)
			&& mPositionIndicator.x <= (mNoChangeZone.right + 3) )
		return;
	float	leftLimit = H_BORDER_X;
	float	rightLimit = Bounds().right - H_BORDER_X;
	float	x = mPositionIndicator.x;
	if( x < leftLimit ) x = leftLimit;
	else if( x > rightLimit ) x = rightLimit;

	BRect	b = Bounds();
	BRect	f(x - 2, b.top + 4, x + 2, b.bottom - 4);
	view->SetHighColor(0, 0, 0);
	view->StrokeRect(f);
	view->StrokeLine(BPoint(x, b.top + 2), BPoint(x, b.bottom - 2));
	// Fill in the indicator
	view->SetHighColor(255, 255, 255);
	view->FillRect(BRect(f.left + 1, f.top + 1, f.right - 2, f.bottom - 2));
	view->SetHighColor(150, 150, 150);
	view->StrokeLine(BPoint(f.right - 1, f.top + 1), BPoint(f.right - 1, f.bottom - 1));
	view->StrokeLine(BPoint(f.left + 1, f.bottom - 1), BPoint(f.right - 1, f.bottom - 1));
}

/*************************************************************************
 * _ARP-SET-VALUE-Y-VIEW
 *************************************************************************/
_ArpSetValueYView::_ArpSetValueYView(	BRect frame,
								orientation direction,
								BRect noChangeZone,
								float initialZoom,
								float minZoom, float maxZoom,
								const ArpRangeControl& control)
		: ArpSetRangeView(frame, direction, noChangeZone, initialZoom, minZoom, maxZoom, control)
{
}

void _ArpSetValueYView::SetZoomToPixel(BPoint where)
{
	if( LockLooper() == false ) return;
	BPoint		pt = ConvertFromScreen(where);
	BPoint		prevIndicator = mPositionIndicator;
	mPositionIndicator = pt;
	if ( prevIndicator.y == mPositionIndicator.y ) {
		UnlockLooper();
		return;
	}
	
	float	newZoom = 0;
	if( pt.y < (mNoChangeZone.top - 3) )
		newZoom = ValueFromPixel( pt.y - V_BORDER_Y );
	else if( pt.y > (mNoChangeZone.bottom + 3) )
		newZoom = ValueFromPixel( pt.y - V_BORDER_Y - mNoChangeZone.Height() - 6 + mNoChangeBandSize );
	else
		newZoom = mInitialZoom;

	SetZoom(newZoom);
	UnlockLooper();
}

void _ArpSetValueYView::DrawDetailsOn(BRect updateRect, BView *view)
{
	BRect	b = Bounds();
	// If the initial zoom value is greater than the minimum, then there
	// will always be at least a little bit of the view that extends to
	// the top of the no change zone.
	if (mInitialZoom > mMinZoom) {
		view->SetHighColor(tint_color(mBackgroundColor, B_DARKEN_2_TINT));
		view->StrokeLine(BPoint(b.left + 2, mNoChangeZone.top - 1), BPoint(b.right - 2, mNoChangeZone.top - 1));
	}
	// Same deal, but now the view that extends down.
	if (mInitialZoom < mMaxZoom) {
		view->SetHighColor(tint_color(mBackgroundColor, B_LIGHTEN_2_TINT));
		view->StrokeLine(BPoint(b.left + 2, mNoChangeZone.bottom + 1), BPoint(b.right - 2, mNoChangeZone.bottom + 1));
	}
}

void _ArpSetValueYView::DrawRangeOn(BView *view)
{
	const band_vec&		bands = mControl.VerticalBands();
	BRect				b = Bounds();

	/* Give the bands an opportunity to draw into the background.
	 */
	float	top = V_BORDER_Y;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		ArpRangeBand&	band = *( bands[k] );
		/* If the band has the initial zoom, then it is either the no change zone
		 * or it's been bisected.  Either way, we don't give it a chance to draw.
		 */
		if( band.Contains( mInitialZoom ) ) {
			if( band.mStart == band.mStop ) {
				top = mNoChangeZone.bottom + 4;
				mNoChangeBandSize = band.mPixelSize;
			} else top += band.mPixelSize + mNoChangeZone.Height() + 6;
		} else {
			bool	selected = (mPositionIndicator.y >= top && mPositionIndicator.y < top + band.mPixelSize);
			band.DrawBackground( BRect(V_BORDER_X, top, b.right - V_BORDER_X, top + band.mPixelSize - 1), view, selected );
			top += band.mPixelSize;
		}
	}

	/* Draw the value range.
	 */
	if( mControl.DisplayFlags()&ARP_DISPLAY_RANGE )
		DrawRangeTriangleOn( view );

	/* Give the bands an opportunity to draw into the foreground.
	 */
	top = V_BORDER_Y;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		ArpRangeBand&	band = *( bands[k] );
		/* If the band has the initial zoom, then it is either the no change zone
		 * or it's been bisected.  Either way, we don't give it a chance to draw.
		 */
		if( band.Contains( mInitialZoom ) ) {
			if( band.mStart == band.mStop ) top = mNoChangeZone.bottom + 4;
			else top += band.mPixelSize + mNoChangeZone.Height() + 6;
		} else {
			band.DrawForeground( BRect(V_BORDER_X, top, b.right - V_BORDER_X, top + band.mPixelSize - 1), view, 0 );
			top += band.mPixelSize;
		}
	}

	/* One last iteration, to give any band that wants to be the label
	 * a chance to draw itself.
	 */
	b = mNoChangeZone;
	b.left += 2;
	b.top += 2;
	b.right -= 2;
	b.bottom -= 2;
	for( uint32 k = 0; k < bands.size(); k++ ) {
		if( bands[k]->UseAsLabel( mInitialZoom ) ) {
			bands[k]->DrawForeground( b, view, 0 );
			break;
		}
	}
}

void _ArpSetValueYView::DrawRangeTriangleOn(BView* view)
{
	BRect	b = Bounds();
	float	noChangeT = mNoChangeZone.top - 3;
	float	noChangeB = mNoChangeZone.bottom + 3;
	float	topHeight = noChangeT - (b.top + V_BORDER_Y);
	float	bottomHeight = b.bottom - V_BORDER_Y - noChangeB;
	bool	largeToSmall = mControl.LargeToSmall( B_VERTICAL );
	
	if( topHeight == 0 && bottomHeight == 0 ) return;
	view->SetHighColor( mControl.RangeColor() );
	if( topHeight <= 0 ) {
		float	y = ( largeToSmall ) ? noChangeB : b.bottom - V_BORDER_Y;
		view->FillTriangle(	BPoint(V_BORDER_X, noChangeB),
							BPoint(V_BORDER_X, b.bottom - V_BORDER_Y),
							BPoint(b.right - V_BORDER_X, y) );
	} else if( bottomHeight <= 0 ) {
		float	y = ( largeToSmall ) ? b.top + V_BORDER_Y : noChangeT;
		view->FillTriangle(	BPoint(V_BORDER_X, b.top + V_BORDER_Y),
							BPoint(V_BORDER_X, noChangeT),
							BPoint(b.right - V_BORDER_X, y) );
	} else {
		float	width = b.right - (V_BORDER_X * 2);
		float	x = (width * noChangeT) / (topHeight + bottomHeight);
		if( largeToSmall ) {
			x = width - x;
			view->FillTriangle(	BPoint(V_BORDER_X + x, V_BORDER_Y),
								BPoint(V_BORDER_X + x, noChangeT),
								BPoint(b.right - V_BORDER_X, V_BORDER_Y) );
			view->FillRect( BRect(V_BORDER_X, V_BORDER_Y, V_BORDER_X + x, noChangeT) );
			view->FillTriangle(	BPoint(V_BORDER_X, noChangeB),
								BPoint(V_BORDER_X, b.bottom - V_BORDER_Y),
								BPoint(V_BORDER_X + x, noChangeB) );
		} else {
			view->FillTriangle(	BPoint(V_BORDER_X, V_BORDER_Y),
								BPoint(V_BORDER_X, noChangeT),
								BPoint(V_BORDER_X + x, noChangeT) );
			view->FillTriangle(	BPoint(V_BORDER_X + x, noChangeB),
								BPoint(V_BORDER_X + x, b.bottom - V_BORDER_Y),
								BPoint(b.right - V_BORDER_X, b.bottom - V_BORDER_Y) );
			view->FillRect( BRect(V_BORDER_X, noChangeB, V_BORDER_X + x, b.bottom - V_BORDER_Y) );
		}
	}
}

void _ArpSetValueYView::DrawIndicatorOn(BView *view)
{
	if( mPositionIndicator.y >= (mNoChangeZone.top - 3)
			&& mPositionIndicator.y <= (mNoChangeZone.bottom + 3) )
		return;
	float	topLimit = V_BORDER_Y;
	float	bottomLimit = Bounds().bottom - V_BORDER_Y;
	float	y = mPositionIndicator.y;
	if( y < topLimit ) y = topLimit;
	else if( y > bottomLimit ) y = bottomLimit;

	BRect	b = Bounds();
	BRect	f(b.left + 4, y - 2, b.right - 4, y + 2);
	view->SetHighColor(0, 0, 0);
	view->StrokeRect(f);
	view->StrokeLine(BPoint(b.left + 2, y), BPoint(b.right - 2, y));
	// Fill in the indicator
	view->SetHighColor(255, 255, 255);
	view->FillRect(BRect(f.left + 1, f.top + 1, f.right - 2, f.bottom - 2));
	view->SetHighColor(150, 150, 150);
	view->StrokeLine(BPoint(f.right - 1, f.top + 1), BPoint(f.right - 1, f.bottom - 1));
	view->StrokeLine(BPoint(f.left + 1, f.bottom - 1), BPoint(f.right - 1, f.bottom - 1));
}

/*************************************************************************
 * _ARP-VALUE-TEXT-WINDOW
 *************************************************************************/
_ArpValueTextWindow::_ArpValueTextWindow(	BPoint atPoint,
											float minX, float maxX,	float initialX,
											float minY, float maxY,	float initialY,
											const char* prefix, const char* suffix,
											rgb_color bg)
		: BWindow(BRect(atPoint, atPoint), "zoom_text",
				  B_NO_BORDER_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
				  B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	mView = new _ArpShowTextView(	minX, maxX, initialX,
									minY, maxY, initialY,
									prefix, suffix, bg);
	if (mView == 0) return;
	ResizeTo(mView->Bounds().Width(), mView->Bounds().Height());
	AddChild(mView);
}

void _ArpValueTextWindow::SetZoom(float zoomX, float zoomY)
{
	if (mView == 0) return;
	if (Lock()) {
		mView->SetZoom(zoomX, zoomY);
		Unlock();
	}
}

/*************************************************************************
 * _ARP-SHOW-TEXT-VIEW
 *************************************************************************/
_ArpShowTextView::_ArpShowTextView(	float minX, float maxX,	float initialX,
										float minY, float maxY,	float initialY,
										const char* prefix, const char* suffix,
										rgb_color bg)
		: BView(BRect(0, 0, 0, 0), "zoom_text", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		mMinX(minX), mMaxX(maxX),
		mMinY(minY), mMaxY(maxY),
		mZoomX(initialX), mZoomY(initialY),
		mPrefix(prefix), mSuffix(suffix),
		mSeparator(0), mBackgroundColor(bg),
		mBitmap(0), mDrawView(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(mBackgroundColor);
	
	float	width = 0, height = FontHeight();
	if (ShowX()) {
		mXPoint.Set(width + 4, height);
		BString s;
		s << mPrefix << ((int32)maxX) << mSuffix;
		width += StringWidth(s.String());
	}
	if (ShowY()) {
		// Only assign a separation value if I will display both my
		// x and y zoom values.
		if (ShowX()) {
			mSeparator = height;
			width += mSeparator;
		}
		mYPoint.Set(width + 4, height);
		BString s;
		s << mPrefix << ((int32)maxY) << mSuffix;
		width += StringWidth(s.String());
	}

	ResizeTo(width + 8, height + 4);
}

_ArpShowTextView::~_ArpShowTextView()
{
	FreeMemory();
}

void _ArpShowTextView::Draw(BRect updateRect)
{
	// Set up the backing store bitmap.
	{
		BScreen screen(Window());
		if( !mBitmap || mBitmap->Bounds() != Bounds()
			|| mBitmap->ColorSpace() != screen.ColorSpace() ) {
			Sync();
			delete mBitmap;
			mBitmap = 0;
			mDrawView = 0;
			mBitmap = new BBitmap(Bounds(), screen.ColorSpace(), true);
			mDrawView = new BView(Bounds(), "BackingStore",
								  B_FOLLOW_ALL, B_WILL_DRAW);
			mBitmap->AddChild(mDrawView);
		}
	}
	
	if( !mBitmap || !mDrawView ) return;
	if( !mBitmap->Lock() ) return;

	DrawOn(updateRect, mDrawView);

	mDrawView->Sync();
	DrawBitmapAsync(mBitmap, BPoint(0,0));
	mBitmap->Unlock();
}

void _ArpShowTextView::DrawOn(BRect updateRect, BView *view)
{
	BRect		b = Bounds();
	view->SetHighColor(mBackgroundColor);
	view->FillRect(b);
	view->SetHighColor(0, 0, 0);
	view->StrokeRect(b);
	
	//----DRAW THE BORDERS----
	view->SetHighColor(tint_color(mBackgroundColor, B_LIGHTEN_2_TINT));
	view->StrokeLine(BPoint(b.left + 1, b.top + 1), BPoint(b.left + 1, b.bottom - 2));
	view->StrokeLine(BPoint(b.left + 2, b.top + 1), BPoint(b.right - 2, b.top + 1));

	view->SetHighColor(tint_color(mBackgroundColor, B_DARKEN_2_TINT));
	view->StrokeLine(BPoint(b.left + 2, b.bottom - 1), BPoint(b.right - 1, b.bottom - 1));
	view->StrokeLine(BPoint(b.right - 1, b.top + 2), BPoint(b.right - 1, b.bottom - 1));

	//----DRAW THE TEXT----
	view->SetHighColor(0, 0, 0);
	view->SetLowColor(mBackgroundColor);
	if (mMinX != mMaxX) {
		BString s;
		s << mPrefix << ((int32)mZoomX) << mSuffix;
		view->DrawString(s.String(), mXPoint);
	}
	if (mMinY != mMaxY) {
		BString s;
		s << mPrefix << ((int32)mZoomY) << mSuffix;
		view->DrawString(s.String(), mYPoint);
	}
}

void _ArpShowTextView::SetZoom(float zoomX, float zoomY)
{
	mZoomX = zoomX;
	mZoomY = zoomY;
	Invalidate();
}

float _ArpShowTextView::FontHeight()
{
	font_height		heightStruct;
	GetFontHeight(&heightStruct);
	return ceil(heightStruct.ascent + heightStruct.descent + heightStruct.leading);
}

void _ArpShowTextView::FreeMemory()
{
	if (mBitmap != 0) delete mBitmap;
}

/***************************************************************************
 * ARP-RANGE-BAND
 ***************************************************************************/
ArpRangeBand::ArpRangeBand(	float start,
							float stop,
							float pixelSize )
		: mStart(start), mStop(stop), mPixelSize(pixelSize)
{
	if( start < 0 || stop < 0 )
		debugger("Start and stop must be zero or greater");
	if( pixelSize < 0 )
		debugger("Pixel size must be zero or greater");
}

ArpRangeBand& ArpRangeBand::operator=(const ArpRangeBand &r)
{
	mStart = r.mStart;
	mStop = r.mStop;
	mPixelSize = r.mPixelSize;
	return *this;
}

bool ArpRangeBand::Contains(float value) const
{
	if( mStart == mStop && mStart == value ) return true;
	if( mStart < mStop ) return ( value >= mStart && value <= mStop );
	return ( value >= mStop && value <= mStart );
}

float ArpRangeBand::ZoomFromPixel(float pixel) const
{
	if( mStart == mStop ) return mStart;
	if( pixel >= mPixelSize ) return mStop;

	float	min = (mStart < mStop) ? mStart : mStop;
	float	max = (mStart < mStop) ? mStop : mStart;
	max -= min;

	float	answer = (max * pixel) / mPixelSize;
	if( mStart > mStop ) return mStart - answer;
	return min + answer;
}

float ArpRangeBand::StartToBisectionPixel(float value) const
{
	if( mStart == mStop ) return 0;
	
	float	max;
	if( mStart < mStop ) {
		max = mStop - mStart;
		value = value - mStart;
	} else {
		max = mStart - mStop;
		value = value - mStop;
	}
	return (mPixelSize * value) / max;
}

float ArpRangeBand::BisectionToStopPixel(float value) const
{
	if( mStart == mStop ) return 0;
	return mPixelSize - StartToBisectionPixel( value );
}

void ArpRangeBand::DrawBackground(BRect frame, BView* view, bool selected)
{
	if( selected && mStart == mStop ) {
		view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_1_TINT ) );
		view->FillRect( frame );
	} else {
		rgb_color	c = view->LowColor();
		view->SetHighColor( c.red, c.green, c.blue );
		view->FillRect( frame );
	}
//	view->SetHighColor( 0, 255, 0 );
//	view->StrokeRect( frame );
}

void ArpRangeBand::DrawForeground(BRect frame, BView* view, float value)
{
//	view->SetHighColor( 0, 255, 0 );
//	view->StrokeRect( frame );
}

void ArpRangeBand::DrawControlForeground(BRect frame, BView* view, float value)
{
	DrawForeground(frame, view, value);
}

bool ArpRangeBand::UseAsLabel(float value) const
{
	return false;
}

bool ArpRangeBand::UseAsIntermediate(float value) const
{
	return false;
}

/*************************************************************************
 * ARP-BITMAP-RANGE-BAND
 *************************************************************************/
ArpBitmapRangeBand::ArpBitmapRangeBand(	float start,
										float stop,
										float pixelSize,
										const BBitmap* image,
										float labelStart,
										float labelStop)
		: inherited( start, stop, pixelSize),
		mImage( image ), mLabelStart(labelStart), mLabelStop(labelStop)
{
}

void ArpBitmapRangeBand::DrawBackground(BRect frame, BView* view, bool selected)
{
	if( selected ) {
		view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_1_TINT ) );
		view->FillRect( frame );
	} else {
		inherited::DrawBackground( frame, view, selected );
	}
}

void ArpBitmapRangeBand::DrawForeground(BRect frame, BView* view, float value)
{
	inherited::DrawForeground(frame, view, value);
	if( !mImage ) return;

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode( B_OP_ALPHA );
	view->SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_COMPOSITE );
	view->DrawBitmapAsync( mImage, frame.LeftTop() );
	if (view->Window()) view->Sync();
	view->SetDrawingMode( mode );
}

bool ArpBitmapRangeBand::UseAsLabel(float value) const
{
	if( !mImage ) return false;
	return (value <= mLabelStart) && (value >= mLabelStop);
}

/*************************************************************************
 * ARP-INTERMEDIATE-RANGE-BAND
 *************************************************************************/
ArpIntermediateRangeBand::ArpIntermediateRangeBand(	float start,
													float stop,
													float pixelSize)
		: inherited( start, stop, pixelSize)
{
}

void ArpIntermediateRangeBand::DrawBackground(BRect frame, BView* view, bool selected)
{
	if( selected ) {
		view->SetHighColor( tint_color( view->LowColor(), B_DARKEN_1_TINT ) );
		view->FillRect( frame );
	} else {
		inherited::DrawBackground( frame, view, selected );
	}
}

void ArpIntermediateRangeBand::DrawControlForeground(BRect frame, BView* view, float value)
{
	draw_value_label(frame, view, value);
}

bool ArpIntermediateRangeBand::UseAsIntermediate(float value) const
{
	return (value <= mStart) && (value >= mStop);
}

/*************************************************************************
 * ARP-ZOOM-CONTROL
 *************************************************************************/
const BBitmap		*zoomImage = 0;

ArpZoomControl::ArpZoomControl(	BRect frame,
								const char* name,
								uint32 resizeMask,
								float initialX,
								float initialY,
								uint32 displayFlags)
		: inherited(frame, name, resizeMask, initialX, initialY, displayFlags)
{
	if (!zoomImage) zoomImage = ImageManager().FindBitmap(MAGNIFY_SMALL_IMAGE_STR);
}

ArpZoomControl::~ArpZoomControl()
{
}

void ArpZoomControl::Draw(BRect clip)
{
	inherited::Draw(clip);
	rgb_color		bg = LowColor();
	SetHighColor( bg );
	FillRect(clip);
	DrawBorder(clip);

	BRect	bandBounds = Bounds();
	bandBounds.left += 2;
	bandBounds.top += 2;
	bandBounds.right -= 2;
	bandBounds.bottom -= 2;
	BRegion r(bandBounds);
	ConstrainClippingRegion(&r);
	if (zoomImage) {
		drawing_mode	mode = DrawingMode();
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		DrawBitmapAsync(zoomImage, bandBounds.LeftTop());
		if (Window()) Sync();
		SetDrawingMode(mode);
	}
	ConstrainClippingRegion(NULL);
}
