/* ArpBitmapCache.cpp
 */
#include "ArpKernel/ArpBitmapCache.h"

#include <Bitmap.h>
#include <Region.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>
#include <String.h>

/*************************************************************************
 * ARP-BITMAP-CACHE
 *************************************************************************/
ArpBitmapCache::ArpBitmapCache()
	: mCurWidth(0), mCurHeight(0),
	  mBitmap(0), mDrawView(0),
	  mOwner(0)
{
}

ArpBitmapCache::~ArpBitmapCache()
{
	delete mBitmap;
}

BView* ArpBitmapCache::StartDrawing(BView* owner, BRect updateRect)
{
	if( mOwner ) {
		debugger("StartDrawing() called twice");
		return owner;
	}
	
	if( !owner->Window() ) return owner;
	
	BScreen screen(owner->Window());
	if( !mBitmap
			|| updateRect.Width() > mCurWidth
			|| updateRect.Height() > mCurHeight
			|| mBitmap->ColorSpace() != screen.ColorSpace() ) {
			
		if( mBitmap && mBitmap->ColorSpace() != screen.ColorSpace() ) {
			// reset dimensions.
			mCurWidth = mCurHeight = 0;
		}
		
		delete mBitmap;
		mBitmap = 0;
		mDrawView = 0;
		
		if( mCurWidth < updateRect.Width() ) {
			mCurWidth = updateRect.Width();
		}
		if( mCurHeight < updateRect.Height() ) {
			mCurHeight = updateRect.Height();
		}
		
		mBitmap = new BBitmap(BRect(0, 0, mCurWidth, mCurHeight),
							  screen.ColorSpace(), true);
		BString name(owner->Window()->Name());
		name << " " << "BackingStore";
		mDrawView = new BView(mBitmap->Bounds(), name.String(),
							  B_FOLLOW_NONE, B_WILL_DRAW);
		mBitmap->AddChild(mDrawView);
		
	}
	
	if( !mBitmap || !mDrawView || !mBitmap->Lock() ) return owner;
	
	mDrawView->PushState();
	mDrawView->SetHighColor(owner->HighColor());
	mDrawView->SetLowColor(owner->LowColor());
	mDrawView->SetPenSize(owner->PenSize());
	BFont font;
	owner->GetFont(&font);
	mDrawView->SetFont(&font);
	
	mOwner = owner;
	mUpdateRect = updateRect;
	if( mDrawView->Bounds().LeftTop() != updateRect.LeftTop() ) {
		mDrawView->ScrollTo(updateRect.LeftTop());
		//mDrawView->Sync();
	}
	
	BRegion region(updateRect);
	mDrawView->ConstrainClippingRegion(&region);
	
	return mDrawView;
}

void ArpBitmapCache::FinishDrawing(BView* offscreen)
{
	if( !offscreen || offscreen != mDrawView ) {
		// the caller was not drawing on our bitmap -- just
		// skip it.
		return;
	}
	
	if( !mOwner ) {
		debugger("FinishDrawing() called twice");
		return;
	}
	
	offscreen->Sync();
	mOwner->DrawBitmapAsync(mBitmap, mUpdateRect.OffsetToCopy(0, 0), mUpdateRect);
	
	mDrawView->PopState();
	mOwner->Sync();
	
	mBitmap->Unlock();
	mOwner = 0;
}
