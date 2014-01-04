/* ExBitmapCache.cpp
 */
 
#include <interface/Bitmap.h>
#include <interface/Screen.h>
#include <interface/View.h>
#include <interface/Window.h>

#include <support/String.h>

#include "MDust/ExBitmapCache.h"

/*************************************************************************
 * EX-BITMAP-CACHE
 *************************************************************************/

ExBitmapCache::ExBitmapCache()
	: mCurWidth(0), mCurHeight(0),
	  mBitmap(0), mDrawView(0),
	  mOwner(0)
{
}

ExBitmapCache::~ExBitmapCache()
{
	delete mBitmap;
}

BView* ExBitmapCache::StartDrawing(BView* owner)
{
	if( mOwner ) {
		debugger("StartDrawing() called twice");
		return owner;
	}
	
	if( !owner->Window() ) return owner;
	
	BScreen screen(owner->Window());
	if( !mBitmap
			|| owner->Bounds().Width() > mCurWidth
			|| owner->Bounds().Height() > mCurHeight
			|| mBitmap->ColorSpace() != screen.ColorSpace() ) {
			
		if( mBitmap && mBitmap->ColorSpace() != screen.ColorSpace() ) {
			// reset dimensions.
			mCurWidth = mCurHeight = 0;
		}
		
		delete mBitmap;
		mBitmap = 0;
		mDrawView = 0;
		
		if( mCurWidth < owner->Bounds().Width() ) {
			mCurWidth = owner->Bounds().Width();
		}
		if( mCurHeight < owner->Bounds().Height() ) {
			mCurHeight = owner->Bounds().Height();
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
	
	mOwner = owner;
	if( mDrawView->Bounds().LeftTop() != owner->Bounds().LeftTop() ) {
		mDrawView->ScrollTo(owner->Bounds().LeftTop());
		//mDrawView->Sync();
	}
	
	return mDrawView;
}

void ExBitmapCache::FinishDrawing(BView* offscreen)
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
	mOwner->DrawBitmap(mBitmap, mOwner->Bounds().OffsetToSelf(0, 0),
					   mOwner->Bounds());
	
	mBitmap->Unlock();
	mOwner = 0;
}
