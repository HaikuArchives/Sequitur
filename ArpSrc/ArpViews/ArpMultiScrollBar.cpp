/* ArpMultiScrollBar.cpp
 */
#include <cassert>
#include <cstdio>
#include <interface/Window.h>
#include "ArpViews/ArpMultiScrollBar.h"

/*************************************************************************
 * ARP-MULTI-SCROLL-BAR
 *************************************************************************/
ArpMultiScrollBar::ArpMultiScrollBar(BRect frame, const char *name,
				BView *target, int32 min, int32 max,
				orientation direction)
		: inherited(frame, name, target, min, max, direction)
{
}

ArpMultiScrollBar::~ArpMultiScrollBar()
{
}

void ArpMultiScrollBar::ValueChanged(float newValue)
{
	inherited::ValueChanged(newValue);

	if( !Window() ) return;
	Window()->DisableUpdates();
	Window()->BeginViewTransaction();

	BView		*item;
	for (int32 i=0; (item = (BView*)mTargetList.ItemAt(i)) != 0; i++) {
		if (Orientation() == B_HORIZONTAL) {
			item->ScrollTo(BPoint(newValue, item->Bounds().top));
		} else {
			item->ScrollTo(BPoint(item->Bounds().left, newValue));
		}
	}

	Window()->EndViewTransaction();
	Window()->EnableUpdates();
}

void ArpMultiScrollBar::AddTarget(BView *target)
{
	assert( target );
	mTargetList.AddItem((void*)target);
	if( Orientation() == B_HORIZONTAL ) {
		target->ScrollTo( Value(), target->Bounds().top );
	} else {
		target->ScrollTo( target->Bounds().left, Value() );
	}
}

bool ArpMultiScrollBar::RemoveTarget(BView *target)
{
	return mTargetList.RemoveItem((void*)target);
}

void ArpMultiScrollBar::ClearTargets()
{
	SetTarget( (BView*)0 );
	mTargetList.MakeEmpty();
}