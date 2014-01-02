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
 * ArpRootLayout.h
 *
 * This is the top level of an ArpLayoutable hierarchy.  It may
 * be used either as the root child of a BWindow, or embedded
 * somewhere farther down a traditional BView heirarchy.
 *
 * This object takes care of global attributes of the layout
 * hierarchy: it contains the global parameters, manages the
 * LayoutInhibit() flag based on whether or not it is attached
 * to a window, and forces its children to be layed out within
 * its Bounds() frame.
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

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#define ARPLAYOUT_ARPROOTLAYOUT_H

#ifndef ARPLAYOUT_ARPLAYOUTVIEW_H
#include <ArpLayout/ArpLayoutView.h>
#endif

#ifndef ARPLAYOUT_ARPGLOBALSET_H
#include "ArpLayout/ArpGlobalSet.h"
#endif

// The root layout handles this message as a new set of global values.
enum {
	ARP_GLOBALS_MSG = 'lglb',
};

class _EXPORT ArpRootLayout : public ArpLayoutView, public ArpGlobalSetI {
private:
	typedef ArpLayoutView inherited;

public:
  	ArpRootLayout(BRect frame, const char* name,
			 uint32 resizeMask=B_FOLLOW_ALL,
			 uint32 flags=B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS);
  	ArpRootLayout(BMessage* data, bool final=true);
  	~ArpRootLayout();

	static BArchivable* Instantiate(BMessage* archive);
	virtual status_t Archive(BMessage* data, bool deep=true) const;
	virtual void AttachedToWindow();
	virtual	void AllAttached();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void WindowActivated(bool state);
	virtual void MessageReceived(BMessage* message);
	
	virtual	void Draw(BRect updateRect);
	virtual	void FrameMoved(BPoint new_position);

	// Implement interface to global values.
	virtual const BMessage* GlobalValues() const;
	virtual status_t AddGlobals(const BMessage* values);
	virtual status_t UpdateGlobals(const BMessage* values);
	virtual bool IsGlobalUpdate() const;
	
	virtual int 		LayoutChildSpace() const;
	
	// Call this to update the containing window's size limits,
	// based on the dimensions of the layout.
	void SetWindowLimits();
	
protected:
	void MakeDefaultGlobals(BMessage& globals);
	
private:
	void initialize();
	
	BMessage mGlobals;
};

#endif
