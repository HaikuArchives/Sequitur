/*
	
	ArpAboutWindow.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#ifndef _SCREEN_H
#include <be/interface/Screen.h>
#endif

#ifndef ARPKERNEL_ARPABOUTWINDOW_H
#include "ArpKernel/ArpAboutWindow.h"
#endif
#ifndef ARPKERNEL_ARPABOUTVIEW_H
#include "ArpKernel/ArpAboutView.h"
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include "ArpKernel/ArpDebug.h"
#endif

ArpMOD();

ArpAboutWindow::ArpAboutWindow(BWindow* inwin,
							const char* appname, const char* verstr,
							const char* build, const char* text)
	: BWindow(inwin ? inwin->Frame() : BScreen().Frame(), appname,
			  B_MODAL_WINDOW_LOOK,
			  B_NORMAL_WINDOW_FEEL,
			  //inwin ? B_FLOATING_SUBSET_WINDOW_FEEL : B_FLOATING_APP_WINDOW_FEEL,
			  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS )
{
	if( inwin ) AddToSubset(inwin);
	
	ArpAboutView	*aView;
	// set up a rectangle and instantiate a new view
	BRect aRect( Bounds() );
	aView = new ArpAboutView(aRect, "AnimView", text,
							appname, verstr, text);
	// add view to window
	AddChild(aView);
	
	BRect frm = Frame();
	float w=0, h=0;
	aView->GetPreferredSize(&w, &h);
	ResizeTo(w, h);
	BRect cfrm = Frame();
	ArpD(cdb << ADH << "Resized frame = " << cfrm << endl);
	MoveTo( frm.left
			+ (frm.Width()-cfrm.Width())/2,
		 	frm.top
		 	+ (frm.Height()-cfrm.Height())/2);
	ArpD(cdb << ADH << "Moved frame = " << Frame() << endl);
}

bool ArpAboutWindow::QuitRequested()
{
	return(true);
}
