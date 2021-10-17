/* ArpHrzViewManager.cpp
* I hold a list of views that I stack vertically inside myself.
* These views must be subclasses of ArpView, and are kept in the mViewList.
*
* I exhibit basic behaviour for adding new views to the list (AddMiniView() always
* adds at the bottom), inserting at a given point, and removing.
*/
#include <cstdio>
#include <interface/Window.h>
#include "ArpViews/ArpHrzViewManager.h"

/*************************************************************************
* ArpHrzViewManager
* Abstract superclass for any view that will contain a sequential list of
* MiniViews.
*************************************************************************/

ArpHrzViewManager::ArpHrzViewManager(BRect frame, const char *name, uint32 resizingMode)
		: inherited(frame, name, resizingMode, B_WILL_DRAW | B_FRAME_EVENTS),
		mHsb(0), mSeparation(0)
{
	SetDefaultViewHeight(0);
	SetViewColor( tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT) );
}

// NOTE: it's not necessary to delete any of the views in the mViewList, since
// they are always my children.
ArpHrzViewManager::~ArpHrzViewManager()
{
}

void ArpHrzViewManager::InitializeHorizontalScrollBar()
{
	if( mHsb ) {
		SetHSBRange();
		SetHSBSteps();
	}
}

void ArpHrzViewManager::AttachedToWindow()
{
}

int32 ArpHrzViewManager::IndexOf( view_id id )
{
	return mViewList.IndexOf( id );
}

BView* ArpHrzViewManager::ItemAt(int32 index) const
{
	return (BView*)mViewList.ItemAt(index);
}

void ArpHrzViewManager::FrameResized(float new_width, float)
{
	BView*	v;
	for( int32 k = 0; (v = ItemAt(k)); k++ ) {
		float	height = v->Bounds().Height();
		v->ResizeTo(new_width, height);
	}
	if( mHsb ) {
		SetHSBRange();
		SetHSBSteps();
	}
}

// When the mHsb is added, it needs to sync itself up with everyone already
// in the list.
void ArpHrzViewManager::SetHorizontalScrollBar(ArpMultiScrollBar* sb)
{
	mHsb = sb;

	// The mHsb must target every child view, but NEVER myself -- this
	// causes the child view to be scrolled WITH the parent, so that
	// the bounds would have to be constantly updated or else the
	// child would just be scrolled out of the view.
	if (mHsb != 0) {
		BView		*view;
		for (int32 i=0; (view = (BView*)mViewList.ItemAt(i)) != 0; i++) {
			mHsb->AddTarget(view);
		}
	}
}


//************************************************************************
// Public - Adding
//------------------------------------------------------------------------

// Add a mini view to the end of the list.  We don't want to alter the height
// of the view to be added, but the position and width are fair game for
// making the view fit inside us.
BView* ArpHrzViewManager::AddMiniView(BView *view)
{
	return InsertMiniView(view, mViewList.CountItems() + 1);
}

// Remove and delete the miniview at position, and replace it with view.
// If position is invalid, return false.
bool ArpHrzViewManager::ReplaceMiniView(BView *view, int32 position)
{
	BView*	oldAv = (BView*)mViewList.ItemAt(position);
	if( !oldAv ) return false;

	float	top = oldAv->Frame().top;
//printf("replacing view in %s at pos %ld whose top is %f\n", Name(), position, top );
//printf("my bounds top is %f\n", Bounds().top);
	do_RemoveMiniView(oldAv);
	InsertMiniView( view, position, top );
	return true;
}

// Insert the view in position posArg of the mViewList.
// If pos = 0, insert as list head.  If pos > mViewList size or < 0, insert as
// list tail.  Otherwise, insert at pos.
// topArg is the top of the Bounds() for the new BView.  Use B_ERROR
// to find the value dynamically.  This is mostly a convenience when replacing
// elements.
// Return 0 if we fail, otherwise the added view.
BView* ArpHrzViewManager::InsertMiniView(BView *view, int32 posArg, float topArg)
{
	if( !view ) return 0;
	bool		unlock = false;
	if( Window() && !(Window()->IsLocked()) ) {
		if( !(Window()->Lock()) ) return 0;
		unlock = true;
	}
	int32				pos, top = 0;
	int32				itemCount = mViewList.CountItems();

	if ( (posArg < 0) || (posArg >= itemCount) ) {
		pos = B_ERROR;
		// This is a small trick to make things work correctly -- we
		// need a valid posArg because it will get used below in the
		// ListTopAt() func.  posArg could potentially be negative,
		// which would have undesireable effects on adding the view.
		posArg = itemCount + 1;
	} else {
		pos = posArg;
	}

	float				offset = Bounds().top;
	if (topArg == B_ERROR) {
		if ((top = ListTopAt(posArg)) == B_ERROR) {
			if( unlock && Window() ) Window()->Unlock();
			return 0;
		}
		view->MoveTo(0, top + offset);
	} else {
		view->MoveTo(0, topArg + offset);
	}

	float	dummyPreferredWidth, preferredHeight;
	view->GetPreferredSize(&dummyPreferredWidth, &preferredHeight);
	if (preferredHeight == 0) preferredHeight = mDefaultViewHeight;
	view->ResizeTo(Bounds().Width(), preferredHeight);

	// FIX: this is a kludge so the ChannelViews can set their
	// menus to the proper size.  Perhaps that view SHOULD be
	// initialized to a certain size.
	// LATER:  Huh?  Why doesn't channel view listen to frame resizes?
	// You CAN set the resize flag without affecting other views in
	// the hierarchy, ya know.
	view->FrameResized(view->Bounds().Width(), view->Bounds().Height());

	PreInsertMiniView(view);
	(pos == B_ERROR)
			? (mViewList.AddItem((void*)view))
			: (mViewList.AddItem((void*)view, pos));
	AddChild(view);

	PostInsertMiniView(view);
	RepositionMiniViewsFrom(mViewList.IndexOf(view) + 1);
	ManagerOperationFinished( INSERT_OP );

	if( unlock && Window() ) Window()->Unlock();
	return view;
}


//************************************************************************
// Private - Updating
//------------------------------------------------------------------------

void ArpHrzViewManager::RemoveMiniView(view_id id)
{
	int32 position;
	if( (position = do_RemoveMiniView( id )) != B_ERROR )
		RepositionMiniViewsFrom(position);
	ManagerOperationFinished( REMOVE_OP );
}

BView* ArpHrzViewManager::ViewForId( view_id id ) const
{
	void*	item;
	for( int32 k = 0; (item = mViewList.ItemAt(k)); k++ ) {
		if( item == id ) return (BView*)item;
	}
	return 0;
}

int32 ArpHrzViewManager::do_RemoveMiniView(view_id id)
{
	int32	position;
	BView*	view = ViewForId( id );
	if( !view ) return B_ERROR;

	PreRemoveMiniView( view );
	position = mViewList.IndexOf( id );
	mViewList.RemoveItem( id );
	RemoveChild( view );
	PostRemoveMiniView( view );

	delete view;
	return position;
}

void ArpHrzViewManager::PostInsertMiniView(BView* view)
{
	if (mHsb != 0) {
		mHsb->AddTarget(view);
		SetHSBRange();
		SetHSBSteps();
	}
}

void ArpHrzViewManager::PostRemoveMiniView(BView *view)
{
	if (mHsb != 0) {
		mHsb->RemoveTarget(view);
		SetHSBRange();
		SetHSBSteps();
	}
}

void ArpHrzViewManager::ManagerOperationFinished(uint32 op)
{
}

// Reassign a frame and invalidate all the wrappers in the view list from
// position to the end of the list.
void ArpHrzViewManager::RepositionMiniViewsFrom(int32 position)
{
	BView		*view;
	float		bottom = -1;

	if (position == B_ERROR) return;

	for (int32 i = 0;
			(view = (BView*)mViewList.ItemAt(i)) != 0; i++) {

		if (i >= position) view->MoveTo(0, bottom + 1 + mSeparation);
		bottom = view->Frame().bottom;
	}
}

//  Return the top pixel value for the item at pos, or B_ERROR if something
// goes wrong.
int32 ArpHrzViewManager::ListTopAt(int32 position)
{
	if (ListHeadConditions(position)) return 0;

	if (ListTailConditions(position)) {
		BView	*v;
		if ((v = (BView*)(mViewList.LastItem())) == 0) return B_ERROR;
		return (int32)(v->Frame().bottom + 1 + mSeparation);
	}

	BView	*v;
	if ((v = (BView*)(mViewList.ItemAt(position - 1))) == 0) return B_ERROR;
	return (int32)(v->Frame().bottom + 1 + mSeparation);	
}

bool ArpHrzViewManager::ListHeadConditions(int32 position)
{
	if ((mViewList.IsEmpty()) || (position <= 0)) return true;
	return false;
}

bool ArpHrzViewManager::ListTailConditions(int32 position)
{
	if (position >= mViewList.CountItems()) return true;
	return false;
}

void ArpHrzViewManager::InvalidateAll()
{
	BView		*view;
	
	for (int32 i = 0;
			(view = (BView*)mViewList.ItemAt(i)) != 0; i++) {
		view->Invalidate();
	}
}

void ArpHrzViewManager::SetHSBRange()
{
	float 	width = HorizontalWidth();

	if (width <= Frame().Width()) width = 0;
	else width = width - Frame().Width();
	mHsb->SetRange(0, width);
}

void ArpHrzViewManager::SetHSBSteps()
{
	float	min, max;
	BRect	bounds = Bounds();
	
	float 	width = HorizontalWidth();

	mHsb->GetRange(&min, &max);
	if ((min == 0) && (max == 0)) return;

	int32 bigStep = BigHorizontalStep();
	int32 smallStep = SmallHorizontalStep();

	if (bigStep > 20) bigStep -= smallStep;
	if (bigStep > width - bounds.Width()) bigStep = (int32)(width - bounds.Width());
	if ((bounds.right + smallStep) > width) smallStep = (int32)(width - bounds.right);

	mHsb->SetSteps(smallStep, bigStep);
	
	float	prop = bounds.Width() / width;
	if( prop > 1 ) prop = 1;
	if( prop < 0 ) prop = 0;
	mHsb->SetProportion(prop);
}

