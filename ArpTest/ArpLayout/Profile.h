/*
	
	LayoutTest.h
	
	Copyright (C) 1997 Dianne Hackborn
	e-mail: hackbod@cs.orst.edu
*/

#ifndef PROFILE_H
#define PROFILE_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

#ifndef _VIEW_H
#include "View.h"
#endif

class TestWindow : public BWindow {

  public:
	TestWindow(BApplication* myApp);
	~TestWindow();
	
	virtual	bool	QuitRequested();
	virtual	void	FrameResized(float width,float height);
	
	TestWindow*		TestFunc(const char* param, int val);
	virtual void	do_test(const char* param, int val);

	TestWindow*		TestMsg(BMessage& params);
		
  private:
	BView*	root;
	BStringView* string;
};

class TestApplication : public BApplication {

  public:
	TestApplication();
};

#endif
