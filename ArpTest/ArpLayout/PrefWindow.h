/*
 * Copyright (c)1997 by Dianne Hackborn.
 * All rights reserved.
 *
 * A logical GUI layout engine: the programmer describes
 * high-level relationships between the different user interface
 * object through formal container classes, which then take care
 * of their physical placement.  The system is completely
 * font-sensitive and resizeable.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@enteract.com>.
 *
 * ----------------------------------------------------------------------
 *
 * PrefWindow.h
 *
 * A complete window that provides an interface for editing
 * global color and font settings.  Just pass in the global
 * BMessage object, and BMessenger to which updates should be
 * sent, and away you go!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * • The BColorControl seems to have problems -- when being
 *   sized by the library, it very often gets its child text
 *   entry controls screwed up.  It should also be turned into
 *   a class in ViewStubs so that the background color of the
 *   text views can be set, but I don't see what I can do to
 *   avoid the sizing problems...  [Well, I guess I could
 *   override things to never let its size be set.  How
 *   desperate.]
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
 * 0.1: Created this file.
 *
 */

#ifndef PREFWINDOW_H
#define PREFWINDOW_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#include <ArpLayout/ArpRootLayout.h>
#endif

enum {
	ARP_PREF_MSG = ARP_GLOBALS_MSG,
};

// forward refs
class ArpButton;
class ArpMenuBar;
class ArpMenuField;
class BPopUpMenu;
class BColorControl;
class ArpTextControl;
class ArpListView;
class BCheckBox;

class PrefWindow : public BWindow {
  private:
	typedef BWindow inherited;
	
  public:
	PrefWindow(const BMessenger& target, const BMessage* initglobs);
	~PrefWindow();
	
	virtual	bool	QuitRequested();
	virtual	void	FrameResized(float width,float height);
	virtual void	MessageReceived(BMessage *message);
	
  private:
  	BStringItem* find_string_item(BListView* lv, const char* match);
  	void show_color(void);
  	void show_font(void);
  	void show_style(font_style* name);
  	
	ArpRootLayout*	root;

	BMessenger	dest;
	ArpMessage globals;
	ArpMessage reversion;
	
	ArpMenuBar* mainmenu;
	BColorControl* color;
	ArpListView* color_vars;
	BPopUpMenu* color_pop;
	ArpListView* font_vars;
	BPopUpMenu* font_pop;
	ArpListView* font_names;
	ArpListView* font_styles;
	ArpTextControl* font_size;
	ArpButton* revert_but;
};

#endif
