/* SeqNoView.cpp
 */
#include <cstdio>
#include <cstdlib>
#include "AmPublic/AmPrefsI.h"
#include "ArpKernel/ArpDebug.h"
#include "Sequitur/SeqNoView.h"

/*************************************************************************
 * SEQ-NO-VIEW 
 *************************************************************************/
SeqNoView::SeqNoView(BRect frame,
					const char *name,
					uint32 resizeMask,
					uint32 flags)
	: inherited(frame, name, resizeMask, flags)
{
}

void SeqNoView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor( Prefs().Color(AM_CONTROL_BG_C) );
}

void SeqNoView::Draw(BRect updateRect)
{
	inherited::Draw(updateRect);
	BPoint	start, end;
	BRect	bounds = Bounds();

	// Shade the top a little
	if( bounds.top <= updateRect.top ) {
		start.Set(	updateRect.left,	bounds.top);
		end.Set(	updateRect.right,	bounds.top);
		SetHighColor( tint_color(ViewColor(), B_LIGHTEN_2_TINT) );
		StrokeLine(start, end);
	}

	// Shade the bottom a little
	if( updateRect.bottom >= (bounds.bottom-1) ) {
		start.Set(	updateRect.left,	bounds.bottom-1);
		end.Set(	updateRect.right,	bounds.bottom-1);
		SetHighColor( tint_color(ViewColor(), B_DARKEN_2_TINT) );
		StrokeLine(start, end);
	}
	if( updateRect.bottom >= bounds.bottom ) {
		start.Set(	updateRect.left,	bounds.bottom);
		end.Set(	updateRect.right,	bounds.bottom);
		SetHighColor( 0, 0, 0 );
		StrokeLine(start, end);
	}
}
