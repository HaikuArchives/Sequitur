/*
	ArpLayoutControl.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.

	A basic BControl that is also an ArpBaseLayout.
*/

#ifndef ARPLAYOUT_ARPLAYOUTCONTROL_H
#include "ArpLayout/ArpLayoutControl.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

#include <be/support/Autolock.h>
#include <float.h>

ArpMOD();

/* ------ ArpLayoutControl constructor and destructor ------
 *
 * The various ways to create and destroy ArpLayoutControl
 * objects.
 */
 
ArpLayoutControl::ArpLayoutControl(const char* name, const char* label,
									BMessage* message, uint32 flags)
	: BControl(BRect(0,0,1,1),name,label,message,B_FOLLOW_NONE,flags)
{
}

ArpLayoutControl::ArpLayoutControl(BRect frame, const char* name,
							const char* label, BMessage* message,
							 uint32 resizeMask, uint32 flags)
	: BControl(frame,name,label,message,resizeMask,flags)
{
}

/* ------------ ArpLayoutControl archiving ------------
 *
 * Archiving and retrieving ArpLayoutControl objects.
 */
BArchivable* ArpLayoutControl::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "ArpLayoutControl") ) 
		return new ArpLayoutControl(archive); 
	return NULL;
}

void ArpLayoutControl::GetPreferredSize(float* width, float* height)
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

void ArpLayoutControl::ComputeDimens(ArpDimens& dimens)
{
	ArpD(cdb << ADH << "Computing ArpLayoutControl dimensions...\n");
	ArpBaseLayout::ComputeDimens(dimens);
}

void ArpLayoutControl::initialize() {}

ARPLAYOUT_VIEWHOOKS_SOURCE(ArpLayoutControl, BControl);
ARPLAYOUT_HANDLERHOOKS_SOURCE(ArpLayoutControl, BControl);
ARPLAYOUT_SUITEHOOKS_SOURCE(ArpLayoutControl, BControl);
ARPLAYOUT_ARCHIVEHOOKS_SOURCE(ArpLayoutControl, BControl, false);
