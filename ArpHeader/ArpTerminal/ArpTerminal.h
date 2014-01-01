/*
 * Copyright (c)1999 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * ----------------------------------------------------------------------
 *
 * ArpTerminal.h
 *
 * The ArpTerminal class adds all of the functionality to
 * ArpCoreTerminal that is needed to make a fully usable
 * terminal widget.  In particular, it allows the user to
 * highlight regions with the mouse and perform cut&copy
 * operations, implements an interface to ArpEmulator, and
 * transmits user input to the connected emulator.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 2/1/1998:
 *	• Major release, corresponding to ArpTelnet v2.0.
 *
 * 8/16/1997:
 *	• Now positions the paste query display at the current
 *	  cursor location, so that it is hopefully more obvious...
 *	  Now includes style information with copied text, which
 *	  StyledEdit can pick up.  Yay!
 *	• The initial highlight position is now moved during a screen
 *	  scroll.
 *
 * 7/25/1997:
 *	• Was transforming a LF into CR LF, and shouldn't have been.
 *	• Accidently left some debugging output in Copy/Paste.
 *	• SelectAll() was not selecting all.
 *
 * 7/17/1997:
 *	• Split basic terminal functionality off into
 *	  ArpCoreTerminal; this is basically all new code since then.
 *
 */

#ifndef ARPTERMINAL_ARPTERMINAL_H
#define ARPTERMINAL_ARPTERMINAL_H

#ifndef _LIST_H
#include <be/support/List.h>
#endif

#ifndef _MENU_H
#include <be/interface/Menu.h>
#endif

#ifndef ARPKERNEL_ARPMOUSETRACKER_H
#include <ArpKernel/ArpMouseTracker.h>
#endif

#ifndef ARPTERMINAL_ARPCORETERMINAL_H
#include <ArpTerminal/ArpCoreTerminal.h>
#endif

#ifndef ARPTERMINAL_ARPEMULATOR_H
#include <ArpTerminal/ArpEmulator.h>
#endif

#include <Clipboard.h>

// forward regs
class BView;
class BTextView;
class BScrollView;
class BButton;

class ArpTerminal :	public ArpCoreTerminal,
					protected ArpMouseTracker {
private:
	typedef ArpCoreTerminal inherited;
	
public:

	ArpTerminal(BRect frame, const char* name,
			 uint32 resizeMode = B_FOLLOW_ALL_SIDES,
			 uint32 flags = B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE);
	virtual ~ArpTerminal();
	
	/* Set the emulator associated with this terminal.
	   Once passed in, the terminal owns the emulator object
	   until another one is supplied -- any attached emulator
	   will be deleted if the terminal is deleted.
	   
	   SetEmulator() returns the emulator that was last attached
	   to the terminal -- it no longer belongs to the terminal,
	   so you usually want to delete the last emulator thusly:
	   "delete myTerminal.SetEmulator(myNewEmulator);"
	   A NULL can be passed in to selecte the default base
	   ArpEmulator class; a NULL will then be returned when
	   SetTerminal is called next.
	*/
	ArpEmulatorInterface* SetEmulator(ArpEmulatorInterface* emulator);
	
	/* Get the emulator associated with this terminal.
	   Guaranteed to always return a valid value.
	*/
	ArpEmulatorInterface& Emulator(void);
	const ArpEmulatorInterface& Emulator(void) const;

	bool IsDefaultEmulator(void) const;
	
	/* These functions are used to turn paste verification on
	   and off.  When on, any time a paste operation occurs
	   a view is popped up within the terminal that allows the
	   user to see what text is in the clipboard and verify
	   that the operation should proceed.  The hope is that this
	   will be useful when using the terminal over slow remote
	   connections, since accidently pasting a large clipboard
	   of text can be extremely frustrating and an undo
	   capability is usually not provided.
	*/
	void SetPasteVerified(bool state);
	bool PasteVerified(void) const;
	
	/* These functions are used to turn RMB paste on and off.
	   When on, pressing the right mouse button will immediately
	   paste in the currently highlighted text; or, if no text
	   is highlighted, a regular paste will be performed.
	*/
	void SetRMBPaste(bool state);
	bool RMBPaste(void) const;
	
	/* This is a convenience function to update the state
	   of a menu containing the available emulation names.
	   It turns the check mark on the menu with the same
	   name as the current emulation, and turns all others
	   off.  (Well, it's a little more complicated than that,
	   but that's all I'll explain for now... ;)
	*/
	bool CheckEmulationMenu(BMenu* inmenu) const;
	
  /* ------------------------------------------------------------
     ARPTERMINALINTERFACE METHODS WE OVERRIDE.
     ------------------------------------------------------------ */

	// Need to track scrolling when user is highlighting text.
	void TermScrollRegion(int32 top, int32 bottom, int32 amount);
										
	virtual void TermSetHighlight(TermHighlightType type,
									int32 row1, int32 col1,
									int32 row2, int32 col2);

	/* By default, we have these echo text right back out to
	   the emulator.  You will almost definitely want to override
	   this. */
	virtual void TermSendRemote(const ichar * d, size_t len);

  /* ------------------------------------------------------------
     BVIEW METHODS WE OVERRIDE.
     ------------------------------------------------------------ */

	virtual void MessageReceived(BMessage* message);
	
	virtual void TermSizeChanged(int32 rows, int32 cols);
	virtual void TermEmulationChanged(const char* name);
	virtual	void ScrollTo(BPoint where);
	
	virtual void KeyDown(const char *bytes, int32 numBytes);
	virtual void MouseDown(BPoint point);
	virtual void TrackMouseMoved(BPoint where, uint32 buttons);
	virtual void TrackMousePulse(BPoint where, uint32 buttons);
	virtual void TrackMouseUp(BPoint where, uint32 buttons);
	
	virtual void MouseMoved(BPoint point, uint32 transit,
							const BMessage* message);
							
	virtual void MakeFocus(bool focusState = TRUE);

	// A clipboard interface like BTextView
	virtual void Cut(BClipboard *clipboard);
	virtual void Copy(BClipboard *clipboard);
	virtual void Paste(BClipboard *clipboard);
	virtual void QuickPaste(BClipboard *clipboard);
	virtual void SelectAll(void);
	
	virtual void CopyToMessage(BMessage* message,
							   int32 toprow, int32 topcol,
							   int32 botrow, int32 botcol);
	virtual bool PasteFromMessage(const BMessage* message);
	
	/* ------------------------------------------------------------
	   INHERITANCE SECTION
	   ------------------------------------------------------------ */

protected:

	void MakeRefsString(BString* strout, const BMessage* from) const;
	
	void ShowPasteReq(const char* text, ssize_t size);
	
	/* Override this so that text is actually sent out to the emulator.
	   Probably what you want. */
	virtual void EmulateToRemote(const ichar* d, size_t len);
	
	/* ------------------------------------------------------------
	   IMPLEMENTATION SECTION
	   ------------------------------------------------------------ */

private:
	void SetPastePos(void);
	void DoPaste(const char* text, size_t size);
	void DidInput(void);
	
	/* ------------------------------------------------------------
	   INSTANCE DATA SECTION
	   ------------------------------------------------------------ */

	//BList addons;
	
	ArpEmulatorInterface* curEmulator;
	ArpEmulator defEmulator;

	bool pasteVerify, rmbPaste;
		
	bool wasNavigable;
	BView* pasteView;
	BTextView* pasteText;
	BScrollView* pasteScroll;
	BButton* cancelButton;
	BButton* pasteButton;
	
	int32 selHMode;				// Current highlight mode / off
	BPoint hInitPoint;			// Initial place button was pressed
	int32 hInitRow, hInitCol;	// Cursor location corresponding to above
	bool selHShown;				// Set if highlight is occurring.
	
	// This is just for debugging
#ifdef DEBUG
	static const bool outputecho;
#endif
};

#endif
