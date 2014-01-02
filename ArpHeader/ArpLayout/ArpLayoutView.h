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
 * ArpLayoutView.h
 *
 * A basic BView that is also an ArpBaseLayout.
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
 
#ifndef ARPLAYOUT_ARPLAYOUTVIEW_H
#define ARPLAYOUT_ARPLAYOUTVIEW_H

#ifndef ARPLAYOUT_ARPBASELAYOUT_H
#include <ArpLayout/ArpBaseLayout.h>
#endif

#ifndef ARPLAYOUT_ARPLAYOUTHOOKS_H
#include <ArpLayout/ArpLayoutHooks.h>
#endif

#ifndef _VIEW_H
#include <interface/View.h>
#endif

/** -----------------------------------------------------------------------

ArpLayoutView is-a mix-in of a BView and an ArpBaseLayout.

@description
Essentially, it
is the moral equivalent of BView, but for classes that want to implement
controls that automatically work with the ArpLayout architecture. When
implementing a subclass, you should implement all of the normal BView stuff
(possibly except GetPreferredSize()), as well as the ArpLayoutable methods
you need to override.

In particular, you should always override
ComputeDimens() to return the appropriate dimensions of your object. Layout 
managers also must override LayoutView() to correctly place their children.

	----------------------------------------------------------------------- */
class _EXPORT ArpLayoutView : public BView, public ArpBaseLayout
{
public:
	/** Create a new instance of the class, with the given name and
		view flags.  The second form, with frame and resizeMask
		arguments, should rarely be used -- the initial
		frame and resizing behavior are entirely handled by the
		ArpLayout classes.
	 **/
  	ArpLayoutView(const char* name,
				  uint32 flags=B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);
  	//* @see ArpLayoutView()
	ArpLayoutView(BRect frame, const char* name, uint32 resizeMask,
				  uint32 flags=B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE);
			
	static BArchivable* Instantiate(BMessage* archive);
	
	// Inplement some standard BView<->ArpLayoutable interaction.
	ARPLAYOUT_VIEWHOOKS(BView);
	ARPLAYOUT_HANDLERHOOKS(BView);
	ARPLAYOUT_SUITEHOOKS(BView);
	ARPLAYOUT_ARCHIVEHOOKS(ArpLayoutView, BView, false);
	virtual void GetPreferredSize(float* width, float* height);
	
protected:
  	void ComputeDimens(ArpDimens& dimens);
  	
private:
	void initialize();
};

#endif
