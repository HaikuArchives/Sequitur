/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * ----------------------------------------------------------------------
 *
 * ArpLayoutControl.h
 *
 * A BControl that is also an ArpBaseLayout.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * Dec 6, 1998:
 *	First public release.
 *
 */

/**
@author Dianne Hackborn
@package ArpLayout
**/
 
#ifndef ARPLAYOUT_ARPLAYOUTCONTROL_H
#define ARPLAYOUT_ARPLAYOUTCONTROL_H

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTHOOKS_H
#include <ArpLayout/ArpLayoutHooks.h>
#endif

#ifndef _CONTROL_H
#include <interface/Control.h>
#endif

/** -----------------------------------------------------------------------

ArpLayoutControl is-a mix-in of a BControl and an ArpBaseLayout.

@description
Essentially, it
is the moral equivalent of BControl, but for classes that want to implement
controls that automatically work with the ArpLayout architecture. When
implementing a subclass, you should implement all of the normal BControl stuff
(possibly except GetPreferredSize()), as well as the ArpLayoutable methods
you need to override.

In particular, you should always override
ComputeDimens() to return the appropriate dimensions of your object. Layout 
managers also must override LayoutView() to correctly place their children.

	----------------------------------------------------------------------- */
class _EXPORT ArpLayoutControl : public BControl, public ArpBaseLayout
{
public:
	/** Create a new instance of the class, with the given name and
		view flags.  The second form, with frame and resizeMask
		arguments, should rarely be used -- the initial
		frame and resizing behavior are entirely handled by the
		ArpLayout classes.
	 **/
  	ArpLayoutControl(const char* name, const char* label,
  					BMessage* message,
					uint32 flags=B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
  	//* @see ArpLayoutControl()
	ArpLayoutControl(BRect frame, const char* name,
					const char* label, BMessage* message, uint32 resizeMask,
					uint32 flags=B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_NAVIGABLE);
			
	static BArchivable* Instantiate(BMessage* archive);
	
	// Inplement some standard BControl<->ArpLayoutable interaction.
	ARPLAYOUT_VIEWHOOKS(BControl);
	ARPLAYOUT_HANDLERHOOKS(BControl);
	ARPLAYOUT_SUITEHOOKS(BControl);
	ARPLAYOUT_ARCHIVEHOOKS(ArpLayoutControl, BControl, false);
	virtual void GetPreferredSize(float* width, float* height);
	
protected:
  	void ComputeDimens(ArpDimens& dimens);
  	
private:
	void initialize();
};

#endif
