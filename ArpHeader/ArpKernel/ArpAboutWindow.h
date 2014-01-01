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
 * ArpAboutWindow.h
 *
 * A small wrapper around ArpAboutView to take care of the
 * common details of creating the about box window, etc.
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

#ifndef ARPKERNEL_ARPABOUTWINDOW_H
#define ARPKERNEL_ARPABOUTWINDOW_H

#ifndef _WINDOW_H
#include <Window.h>
#endif

class ArpAboutWindow : public BWindow 
{
public:
	ArpAboutWindow(BWindow* inwin, const char* appname, const char* verstr,
							const char* build, const char* text);
	virtual bool QuitRequested();
};

#endif
