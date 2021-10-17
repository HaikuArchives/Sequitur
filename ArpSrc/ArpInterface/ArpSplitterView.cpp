#include <cassert>
#include <cstdio>
#include <app/Cursor.h>
#include <app/Message.h>
#include <interface/Window.h>
#include <support/Errors.h>
#include <ArpKernel/ArpDebug.h>
#include "ArpInterface/ArpSplitterView.h"

static const BCursor*	gHrzCursor = 0;	
static const BCursor*	gVrtCursor = 0;	

/*************************************************************************
 * SEQ-SPLITTER-VIEW
 *************************************************************************/
ArpSplitterView::ArpSplitterView(	BRect frame,
									const char* name,
									uint32 resizeMask,
									uint32 flags,
									orientation direction)
		: inherited(frame, name, resizeMask, flags),
		  mDirection(direction), mDrawingFlags(0),
		  mMouseDown(false), mPointDown(0, 0), mFrameDown(0, 0, 0, 0)
{
#if 0
	if (!gHrzCursor) gHrzCursor = Resources().FindCursor("Hrz Splitter");
	if (!gVrtCursor) gVrtCursor = Resources().FindCursor("Vrt Splitter");
#endif
}

ArpSplitterView::~ArpSplitterView()
{
}

void ArpSplitterView::AttachedToWindow()
{
	inherited::AttachedToWindow();
//	SetViewColor( Prefs().Color(AM_DATA_BACKDROP_C) );
}

void ArpSplitterView::Draw(BRect clip)
{
	inherited::Draw(clip);
	if (mDrawingFlags&NO_DRAWING_FLAG) return;
	
	BRect		b = Bounds();
	if (mDirection == B_VERTICAL) {
		SetHighColor( 175, 175, 175 );
		StrokeLine( BPoint(1, clip.top), BPoint(1, clip.bottom) );
		SetHighColor( 0, 0, 0 );
		StrokeLine( BPoint(0, clip.top), BPoint(0, clip.bottom) );
		StrokeLine( BPoint(b.right, clip.top), BPoint(b.right, clip.bottom) );
		if (mDrawingFlags&CAP_ENDS_FLAG && clip.top <= b.top)
			StrokeLine(BPoint(0, b.top), BPoint(b.right, b.top));
		if (mDrawingFlags&CAP_ENDS_FLAG && clip.bottom >= b.bottom)
			StrokeLine(BPoint(0, b.bottom), BPoint(b.right, b.bottom));
	} else {
		SetHighColor( 175, 175, 175 );
		StrokeLine( BPoint(clip.left, 1), BPoint(clip.right, 1) );
		SetHighColor( 0, 0, 0 );
		StrokeLine( BPoint(clip.left, 0), BPoint(clip.right, 0) );
		StrokeLine( BPoint(clip.left, b.bottom), BPoint(clip.right, b.bottom) );
		if (mDrawingFlags&CAP_ENDS_FLAG && clip.left <= b.left)
			StrokeLine(BPoint(b.left, 0), BPoint(b.left,  b.bottom));
		if (mDrawingFlags&CAP_ENDS_FLAG && clip.right >= b.right)
			StrokeLine(BPoint(b.right, 0), BPoint(b.right, b.bottom));
	}
}

void ArpSplitterView::MouseDown(BPoint where)
{
	mMouseDown = true;
	mPointDown = ConvertToScreen( where );
	mFrameDown = Frame();
	SetMouseEventMask( B_POINTER_EVENTS,
					   B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY );
}

void ArpSplitterView::MouseUp(BPoint where)
{
	mMouseDown = false;
}

void ArpSplitterView::MouseMoved(	BPoint where,
									uint32 code,
									const BMessage* message)
{
	if (code == B_ENTERED_VIEW) {
		if (mDirection == B_VERTICAL && gVrtCursor) SetViewCursor(gVrtCursor);
		if (mDirection == B_HORIZONTAL && gHrzCursor) SetViewCursor(gHrzCursor);
	}
	if( !mMouseDown || !Window() || !Window()->CurrentMessage() ) return;
	BView*	prev = PreviousSibling();
	BView*	next = NextSibling();
	if( !prev || !next ) return;
	
	// The mouse moved message's "where" field is in window
	// coordinates.  We need to use that instead of view
	// coordinates because the changes in this view's frame
	// are asynchronous with the mouse events we receive.
	BPoint	screenWhere;
ArpFINISH();
//	Window()->CurrentMessage()->FindPoint("where", &screenWhere);
	Window()->ConvertToScreen(&screenWhere);
	BPoint	delta = screenWhere - mPointDown;
	
	bool	locked = false;
//printf("recived mouse of %f\n", where.x);

if( Window() && Window()->Lock() ) {
	locked = true;
	Window()->BeginViewTransaction();
}
	if( mDirection == B_VERTICAL ) {
		/* Move me
		 */
		float	prevLeft = prev->Frame().left;
		float	nextRight = next->Frame().right;
		float	x = mFrameDown.left + delta.x;
		float	y = mFrameDown.top;
		if( x < prevLeft ) x = prevLeft;
		if( x + Bounds().Width() > nextRight ) x = nextRight - Bounds().Width();
		MoveTo( x, y );
		/* Move prev
		 */
		float	height = prev->Bounds().Height();
		float	prevRight = Frame().left - 1;
		prev->ResizeTo( prevRight - prevLeft, height );
		/* Move next
		 */
		height = next->Bounds().Height();
		float	nextTop = next->Frame().top;
		float	nextLeft = Frame().right + 1;
//printf("\tsending out move to %f\n", nextLeft);
		BRect	f = next->Frame();
		if( f.left != nextLeft ) next->MoveTo( nextLeft, nextTop );
		if( f.Width() != nextRight - nextLeft ) next->ResizeTo( nextRight - nextLeft, height );
#if 0
		next->MoveTo( nextLeft, nextTop );
		next->ResizeTo( nextRight - nextLeft, height );
#endif
	} else {
		/* Move me
		 */
		float	prevTop = prev->Frame().top;
		float	nextBottom = next->Frame().bottom;
		float	x = mFrameDown.left;
		float	y = mFrameDown.top + delta.y;
		if( y < prevTop ) y = prevTop;
		if( y + Bounds().Height() > nextBottom ) y = nextBottom - Bounds().Height();
		MoveTo( x, y );
		/* Move prev
		 */
		float	width = prev->Bounds().Width();
		float	prevBottom = Frame().top - 1;
		prev->ResizeTo( width, prevBottom - prevTop );
		/* Move next
		 */
		width = next->Bounds().Width();
		float	nextLeft = next->Frame().left;
		float	nextTop = Frame().bottom + 1;
		next->MoveTo( nextLeft, nextTop );
		next->ResizeTo( width, nextBottom - nextTop );
	}
if( locked ) {
	Window()->EndViewTransaction();
	Window()->Unlock();
}
}

void ArpSplitterView::SetDrawingFlags(uint32 flags)
{
	mDrawingFlags = flags;
}

void ArpSplitterView::MoveVerticalSplitter(float left)
{
	ArpVALIDATE(mDirection == B_VERTICAL, return);
	
	BView*		prev = PreviousSibling();
	BView*		next = NextSibling();
	if (!prev || !next) return;
	/* Move me
	 */
	float		prevLeft = prev->Frame().left;
	float		nextRight = next->Frame().right;
	float		x = left;
	float		y = Frame().top;
	if (x < prevLeft) x = prevLeft;
	if (x + Bounds().Width() > nextRight ) x = nextRight - Bounds().Width();
	MoveTo(x, y);
	/* Move prev
	 */
	float		height = prev->Bounds().Height();
	float		prevRight = Frame().left - 1;
	prev->ResizeTo(prevRight - prevLeft, height);
	/* Move next
	 */
	height = next->Bounds().Height();
	float		nextTop = next->Frame().top;
	float		nextLeft = Frame().right + 1;
	BRect		f = next->Frame();
	if (f.left != nextLeft) next->MoveTo(nextLeft, nextTop);
	if (f.Width() != nextRight - nextLeft) next->ResizeTo(nextRight - nextLeft, height);
}

void ArpSplitterView::MoveHorizontalSplitter(float top)
{
	ArpVALIDATE(mDirection == B_HORIZONTAL, return);
	
	BView*		prev = PreviousSibling();
	BView*		next = NextSibling();
	if (!prev || !next) return;
	/* Move me
	 */
	float		prevTop = prev->Frame().top;
	float		nextBottom = next->Frame().bottom;
	float		x = Frame().left;
	float		y = top;
	if (y < prevTop) y = prevTop;
	if (y + Bounds().Height() > nextBottom) y = nextBottom - Bounds().Height();
	MoveTo(x, y);
	/* Move prev
	 */
	float		width = prev->Bounds().Width();
	float		prevBottom = Frame().top - 1;
	prev->ResizeTo(width, prevBottom - prevTop);
	/* Move next
	 */
	width = next->Bounds().Width();
	float		nextLeft = next->Frame().left;
	float		nextTop = Frame().bottom + 1;
	next->MoveTo(nextLeft, nextTop);
	next->ResizeTo(width, nextBottom - nextTop);
}
