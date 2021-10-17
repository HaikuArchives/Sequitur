/*
	
	LayoutTest.cpp
	
	Copyright (C) 1997 Dianne Hackborn
	e-mail: hackbod@cs.orst.edu
*/

#ifndef PROFILE_H
#include "Profile.h"
#endif

#include <Button.h>
#include <ColorControl.h>
#include <Autolock.h>

#define DEBUG 1

#ifndef ARPCOMMON_ARPDEBUG_H
#include "ArpCommon/ArpDebug.h"
#endif

/* ---------------------------------------------------------------
 *
 * class TestWindow: Out main application window.
 *
 */
 
TestWindow::TestWindow(BApplication* myApp)
	: BWindow(BRect(20,20,100,100),
	  "Code Profile", B_TITLED_WINDOW, 0)
{
	BRect frm = Bounds();

	BView* myview = new BView(BRect(),"testView",0,0);
	
	BOutlineListView* olist =
		new BOutlineListView(BRect(),"MyList",
							B_SINGLE_SELECTION_LIST,B_FOLLOW_NONE);
	if( myview && olist ) {
		myview->AddChild(olist);
		BView* vw = olist;
		vw->SetViewColor(0xc0,0xc0,0xc0);
		vw->Invalidate();
		vw->SetLowColor(0xc0,0xc0,0xc0);
		vw->Invalidate();
		vw->SetHighColor(0x00,0x00,0x00);
		vw->Invalidate();
		vw->SetFont(be_bold_font);
		this->AddChild(myview);
		BRect frm = vw->Frame();
		vw->ResizeTo(1,1);
		vw->Draw(vw->Bounds());
		vw->ResizeToPreferred();
		float w=0,h=0;
		vw->GetPreferredSize(&w,&h);
		printf("Preferred size = %f x %f\n",w,h);
	}
	
	string = new BStringView(BRect(0,0,100,20),"String",
								"Ready to profile...");	
	
	if( string ) {
		string->SetViewColor(0xc0,0xc0,0xc0);
		this->AddChild(string);
		float w=0, h=0;
		string->GetPreferredSize(&w,&h);
		MoveTo(30,30);
		ResizeTo(w,h);
	}
	
	BMenuBar* menu = new BMenuBar(BRect(),"MainMenu",B_FOLLOW_NONE);
	if( menu ) {
		this->AddChild(menu);
		float w=0, h=0;
		menu->GetPreferredSize(&w,&h);
		printf("Preferred Size = (%f,%f)\n",w,h);
		menu->SetFont(be_plain_font);
		menu->GetPreferredSize(&w,&h);
		printf("Preferred Size = (%f,%f)\n",w,h);
		menu->SetFont(be_bold_font);
		menu->GetPreferredSize(&w,&h);
		printf("Preferred Size = (%f,%f)\n",w,h);
		menu->SetFont(be_fixed_font);
		menu->GetPreferredSize(&w,&h);
		printf("Preferred Size = (%f,%f)\n",w,h);
	}
}

TestWindow::~TestWindow()
{
}

void TestWindow::FrameResized(float width,float height)
{
}

bool TestWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

TestWindow* TestWindow::TestFunc(const char* param, int val)
{
	if( this ) do_test(param,val);
	return this;
}

void TestWindow::do_test(const char* param, int val)
{
	if( strcmp(param,"test string") == 0 ) root = NULL;
	return;
}

TestWindow* TestWindow::TestMsg(BMessage& params)
{
	root = NULL;
	return;
}

TestApplication::TestApplication()
	: BApplication("application/x-vnd.ARP-layoutest")
{
	TestWindow		*aWindow;

	// Instantiate the test window, and make it visible.	
	aWindow = new TestWindow(this);
	aWindow->Show();
	
	fflush(stdout);
	fflush(stderr);
	
	{
		DB(DBALL,cdb << "Testing messages..." << std::endl);
		BMessage testMsg;
		status_t res = testMsg.AddInt32("A param...",65);
		DB(DBALL,cdb << "Added an int32; result=" << res << std::endl);
		res = testMsg.AddFloat("A param...",103.4);
		DB(DBALL,cdb << "Added a float; result=" << res << std::endl);
		DB(DBALL,cdb << "Final message = " << testMsg << std::endl);
		DB(DBALL,cdb.flush());
	}
	
	{
		BStopWatch watch("FuncCall StopWatch");
		watch.Reset();
	
		printf("Testing virtual function calls...\n");
		int64 i;
		watch.Resume();
		const int fcount = 10000000;
		for( i=0; i<fcount; i++ ) {
			aWindow->TestFunc("a string",23);
		}
		watch.Suspend();
	
		bigtime_t el = watch.ElapsedTime();
		printf("Operations per second: %f\n",
			(((float)fcount)*1000000.0)/(float)el);
	}

	{
		BStopWatch watch("BMessage StopWatch");
		watch.Reset();
	
		printf("Testing BMessage invocation...\n");
		int64 i;
		watch.Resume();
		const int fcount = 10000000;
		for( i=0; i<fcount; i++ ) {
			BMessage testMsg;
			aWindow->TestMsg(testMsg);
		}
		watch.Suspend();
	
		bigtime_t el = watch.ElapsedTime();
		printf("Operations per second: %f\n",
			(((float)fcount)*1000000.0)/(float)el);
	}

	{
		BStopWatch watch("BMessage init StopWatch");
		watch.Reset();
	
		printf("Testing BMessage initialization...\n");
		int64 i;
		watch.Resume();
		const int fcount = 10000;
		for( i=0; i<fcount; i++ ) {
			BMessage testMsg;
			testMsg.AddInt32("A param...",65);
			aWindow->TestMsg(testMsg);
		}
		watch.Suspend();
	
		bigtime_t el = watch.ElapsedTime();
		printf("Operations per second: %f\n",
			(((float)fcount)*1000000.0)/(float)el);
	}
}

main()
{	
	TestApplication *myApplication;

	myApplication = new TestApplication();
	myApplication->Run();
	
	delete(myApplication);
	return(0);
}
