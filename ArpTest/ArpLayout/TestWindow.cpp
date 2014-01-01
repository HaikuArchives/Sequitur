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
 */

#ifndef LAYOUTWINDOW_H
#include "TestWindow.h"
#endif

#include <Button.h>
#include <ColorControl.h>
#include <TextControl.h>
#include <Autolock.h>
#include <float.h>

//#define DEBUG 1

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

LayoutWindow::LayoutWindow(void)
	: BWindow(BRect(20,20,100,600),
	  		  "Test", B_TITLED_WINDOW, 0)
{
	BView* root = new BView(Bounds(),"Test",B_FOLLOW_ALL,0);
	AddChild(root);
	
	BRect bnd = Bounds();
	for( int i=0; i<30; i++ ) {
		int top = int( i*(bnd.Height()/30) + bnd.top );
		BRect rect(bnd.left,top,bnd.right,top+(bnd.Height()/60));
		BButton* but = new BButton(rect, "Button", "Button",
								NULL,B_FOLLOW_TOP_BOTTOM|B_FOLLOW_LEFT_RIGHT,
								B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE);
		root->AddChild(but);
		but->ResizeTo(rect.Width(),rect.Height());
	}
}

LayoutWindow::~LayoutWindow()
{
}

void LayoutWindow::MessageReceived(BMessage *message)
{
	if( message ) {
	}
	inherited::MessageReceived(message);
}

void LayoutWindow::FrameResized(float,float)
{
#if 0
	if( root ) {
		DisableUpdates();
		root->SetLayout(Bounds());
		EnableUpdates();
	}
#endif
}

bool LayoutWindow::QuitRequested()
{
	//target.PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}
