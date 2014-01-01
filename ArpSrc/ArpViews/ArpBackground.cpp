/* ArpBackground.cpp
 */
#include "ArpViews/ArpBackground.h"
#include "ArpKernel/ArpDebug.h"

/*************************************************************************
 * ARP-BACKGROUND
 *************************************************************************/
ArpBackground::ArpBackground()
		: next(0)
{
}

ArpBackground::~ArpBackground()
{
	delete next;
}

status_t ArpBackground::AddTail(ArpBackground *tail)
{
	if( next ) return next->AddTail( tail );
	tail->next = 0;
	next = tail;
	return B_OK;
}

void ArpBackground::DrawAllOn(BView* view, BRect clip)
{
	DrawOn( view, clip );
	if( next ) next->DrawAllOn( view, clip );
}

/*************************************************************************
 * ARP-CENTER-BACKGROUND
 *************************************************************************/
ArpCenterBackground::ArpCenterBackground(BView* boundsView, rgb_color lineColor)
		: mBoundsView(boundsView), mLineColor(lineColor)
{
}

void ArpCenterBackground::DrawOn(BView* view, BRect clip)
{
	assert( mBoundsView );
	float	middle = (mBoundsView->Bounds().bottom - mBoundsView->Bounds().top) / 2;

	view->SetHighColor( mLineColor );
	view->StrokeLine(	BPoint(clip.left, middle),
						BPoint(clip.right, middle) );
}

/*************************************************************************
 * ARP-FLOOR-BACKGROUND
 *************************************************************************/
ArpFloorBackground::ArpFloorBackground(BView* boundsView, rgb_color lineColor)
		: mBoundsView(boundsView), mLineColor(lineColor)
{
}

void ArpFloorBackground::DrawOn(BView* view, BRect clip)
{
	ArpASSERT( mBoundsView );
	BPoint	startPt, endPt;
	BRect	bounds = mBoundsView->Bounds();
	
	view->SetHighColor( mLineColor );
	// FIX this
	if ((bounds.bottom >= clip.top)
			&& (bounds.bottom <= clip.bottom)) {

		startPt.y = endPt.y = bounds.bottom ;
		startPt.x = clip.left;
		endPt.x = clip.right;
		view->StrokeLine( startPt, endPt );
	}
}