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
 * LayoutWindow.h
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

#ifndef LAYOUTWINDOW_H
#define LAYOUTWINDOW_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

class LayoutWindow : public BWindow {
  private:
	typedef BWindow inherited;
	
  public:
	LayoutWindow(void);
	~LayoutWindow();
	
	virtual	bool	QuitRequested();
	virtual	void	FrameResized(float width,float height);
	virtual void	MessageReceived(BMessage *message);
	
  private:
};

#endif
