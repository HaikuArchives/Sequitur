/*
 * Copyright (c)1999 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpDocumentButton.h
 *
 * A button control for placement to the right of a window's menu bar,
 * which provides one-press access to the document settings, a drag
 * and drop interface to save/load, and (in the future) a pop-up menu for
 * common document operations.
 *
 * For drag and drop operations, this class handles much of the grunge
 * work for implementing the drag and drop protocol.  To make use of it,
 * you must supply two "prototype" BMessage objects to the class:
 *
 * The "drag prototype" (SetDragPrototype()) is the message that is passed
 * to DragMessage() to initiate a drag operation.  It should contain the
 * data types, file types, and actions that you can supply to a drop target.
 * See to Be Developer Newsletter article "R4 Drag-and-Drop" for details
 * on what is contained in this message.  When the target of the drag
 * operation responds with the action and data it desires, this message
 * is forwarded to your own target for processing.  In other words,
 * you should implement cases in your message handler for all of the
 * actions you supplied in "be:actions", handling either refs and/or
 * data [depending on which kinds of transfers the drag prototype included]
 * that need to be supplied information.
 *
 * The "drop prototype" (SetDropPrototype()) is a message that describes
 * the kinds of types you accept drops for.  This is formatted identically
 * to the drag prototype, but instead used to determine which kinds of
 * data types, file types, and actions you can accept.  When a conforming
 * drop occurs, you will either receive a B_REFS_RECEIVED message if the
 * drop was supplied as a reference to a file, or B_MIME_DATA if the
 * dropped data is being supplied within the actual message.
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
 * Mar 8, 1999:
 *	Created this file.
 *
 */

#ifndef ARPKERNEL_ARPDOCUMENTBUTTON_H
#define ARPKERNEL_ARPDOCUMENTBUTTON_H

#ifndef _CONTROL_H
#include <interface/Control.h>
#endif

#ifndef _BITMAP_H
#include <interface/Bitmap.h>
#endif

class ArpDocumentButton : public BControl
{
public:
	ArpDocumentButton(BRect frame, const char *name, BBitmap* icon,
					  BMessage* pressMsg,
					  uint32 resizeMask = B_FOLLOW_LEFT|B_FOLLOW_TOP,
					  uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE
					  				| B_FULL_UPDATE_ON_RESIZE);
	~ArpDocumentButton();
	
	// Set and get the prototypical drag message, describing all of the
	// types of drag operations you can supply to a drop target.
	void SetDragPrototype(const BMessage& prototype);
	const BMessage& DragPrototype() const;
	
	// Set and get the prototypical drop message, describing all of the
	// types of drag operations you can accept as a drop target.  These should
	// be listed in the message in order of preference.
	void SetDropPrototype(const BMessage& prototype);
	const BMessage& DropPrototype() const;
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void GetPreferredSize(float* width, float* height);
	
	//virtual void FrameResized(float new_width, float new_height);
	virtual void Draw(BRect rect);
	virtual void MouseDown(BPoint point);
	virtual void MouseMoved(BPoint point, uint32 transit,
							const BMessage* message);
	virtual void MouseUp(BPoint point);
	virtual void MessageReceived(BMessage *message);
	
	// Like Invoke(), but cause any reply the target makes to the given
	// message go to its original sender.
	status_t InvokeReply(BMessage* msg);
	
private:
	typedef BControl inherited;
	
	void FreeMemory();
	bool IsPointIn(BPoint point);
	
	status_t GetDropRef(entry_ref* out, const BMessage* in,
						bool always=false) const;
	status_t GetDropAction(BMessage* out, const BMessage* in,
						   bool always=false) const;
	
	enum {
		FrameWidth=2, FrameHeight=2,
		BorderWidth=1, BorderHeight=1
	};
	
	BMessage mDragPrototype;
	BMessage mDropPrototype;
	
	BBitmap* mDocIcon;
	BBitmap* mSmallIcon;
	
	enum ActiveState {
		Inactive, ButtonPressed, MenuPopped
	};
	
	ActiveState mAction;
	bool mPressed;
	bool mMenued;
	bool mDropped;
};

#endif
