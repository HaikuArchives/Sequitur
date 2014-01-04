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
 * LayoutTest.h
 *
 * A test application for the ArpLayoutable classes.  Creates
 * a simple [and currently pretty ugly] GUI showing some of
 * the classes, then attaches a PrefWindow to it that can be
 * used to modify its global rendering attributes.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * • Does not see when the pref window is closed, so it can
 *   never be created again.
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

#ifndef LAYOUTTEST_H
#define LAYOUTTEST_H

#ifndef _APPLICATION_H
#include <app/Application.h>
#endif

#ifndef _MESSENGER_H
#include <app/Messenger.h>
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef ARPLAYOUT_ARPROOTLAYOUT_H
#include <ArpLayout/ArpRootLayout.h>
#endif

#ifndef ARPKERNEL_ARPMESSAGE_H
#include <ArpKernel/ArpMessage.h>
#endif

class TestWindow : public BWindow {
  private:
	typedef BWindow inherited;
	
  public:
	TestWindow(BApplication* myApp);
	~TestWindow();
	
	virtual	bool	QuitRequested();
	virtual	void	FrameResized(float width,float height);
	virtual void	MessageReceived(BMessage *message);
	
	const ArpGlobalSetI* Globals() { return root ? root->Globals() : 0; }
	
  private:
	void MakeTermSettings();
	
	ArpRootLayout* root;
	ArpBaseLayout* addpos;
	
	BMessenger mPrefWin;
	
	BMessenger mTermWin, mTermSet;
};

class TestApplication : public BApplication {

  public:
	TestApplication();

	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void AboutRequested(void);
	
  private:
	
	BMessenger mAboutWin;
};

#endif
