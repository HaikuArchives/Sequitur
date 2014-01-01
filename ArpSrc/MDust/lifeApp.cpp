/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "MDust/lifeApp.h"
#include "MDust/AttWin.h"
#include <Alert.h>

lifeApp::lifeApp()
	: BApplication("application/x-vnd.ARP-MDust")
{
	mw = new lifeWin();
	mw->Show();
//	AttWin*	win = new AttWin();
//	win->Show();
}

void
lifeApp::AboutRequested()
{
	(new BAlert("","3D Life -- A Small OpenGL Example", "sweet"))->Go();
}
