/*
	ArpLayoutView.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	A basic BView that is also an ArpBaseLayout.
*/

#ifndef ARPLAYOUT_ARPLAYOUTVIEW_H
#include "ArpLayout/ArpLayoutView.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#include <be/support/Autolock.h>
#include <float.h>

ArpMOD();

/* ------ ArpLayoutView constructor and destructor ------
 *
 * The various ways to create and destroy ArpLayoutView
 * objects.
 */
 
ArpLayoutView::ArpLayoutView(const char* name, uint32 flags)
	: BView(BRect(0,0,1,1),name,B_FOLLOW_NONE,flags)
{
}

ArpLayoutView::ArpLayoutView(BRect frame, const char* name,
							 uint32 resizeMask, uint32 flags)
	: BView(frame,name,resizeMask,flags)
{
}

/* ------------ ArpLayoutView archiving ------------
 *
 * Archiving and retrieving ArpLayoutView objects.
 */
BArchivable* ArpLayoutView::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpLayoutView") ) 
		return new ArpLayoutView(archive); 
	return NULL;
}

void ArpLayoutView::GetPreferredSize(float* width, float* height)
{
	// Because of limitations in some Be classes, we can't report
	// a valid layout if we aren't attached to a window.
	if( !Window() ) {
		if( width ) *width = Bounds().right;
		if( height ) *height = Bounds().bottom;
		return;
	}
	
	// Retrieve layout dimensions from this tree, and return
	// the 'preferred size' constraint.
	const ArpDimens& dimens = LayoutDimens();
	if( width ) *width = dimens.X().TotalPref();
	if( height ) *height = dimens.Y().TotalPref();
}

void ArpLayoutView::ComputeDimens(ArpDimens& dimens)
{
	ArpD(cdb << ADH << "Computing ArpLayoutView dimensions...\n");
	ArpBaseLayout::ComputeDimens(dimens);
}

void ArpLayoutView::initialize() {}

ARPLAYOUT_VIEWHOOKS_SOURCE(ArpLayoutView, BView);
ARPLAYOUT_HANDLERHOOKS_SOURCE(ArpLayoutView, BView);
ARPLAYOUT_SUITEHOOKS_SOURCE(ArpLayoutView, BView);
ARPLAYOUT_ARCHIVEHOOKS_SOURCE(ArpLayoutView, BView, false);
