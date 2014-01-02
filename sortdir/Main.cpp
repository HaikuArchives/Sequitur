/*
 * ArpTelnet Copyright (c)1997-98 by Dianne Hackborn.
 * All rights reserved.
 *
 * Based (increasingly loosly) on WebTerm, a Web-based terminal
 * applet written in Java, which is
 * Copyright (C)1996 by the National Alliance for Computational
 * Science and Engineering (NACSE).
 * See <URL:http://www.nacse.org/> for more information.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Dianne Hackborn,
 * at <hackbod@lucent.com> or <hackbod@angryredplanet.com>.
 *
 */

#define USE_STREAMS 1

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef _MESSAGE_H
#include <app/Message.h>
#endif

#ifndef _FILE_H
#include <storage/File.h>
#endif

#ifndef _WINDOW_H
#include <interface/Window.h>
#endif

#ifndef _VIEW_H
#include <include/View.h>
#endif

#ifndef _FILE_PANEL_H
#include <storage/FilePanel.h>
#endif

#ifndef MAIN_H
#include "Main.h"
#endif

#ifndef ARPKERNEL_ARPABOUTWINDOW_H
#include <ArpKernel/ArpAboutWindow.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#include <stdlib.h>
#include <getopt.h>
#include <vector>
#include <algorithm>

const char* ArpAppSig = "application/x-vnd.ARP.sortdir";

static void Usage(int /*argc*/, char **argv)
{
	printf("Usage: %s <dir>\n", argv[0]);
}

bool ArpString_CaseNumLt(const ArpString& s1, const ArpString& s2)
{
	return s1.INumCompare(s2) < 0;
}

int main(int argc, char **argv)
{
	int c;
	bool useGUI = false;
	
	// Parse options
	while(1) {
		int option_index = 0;
		static struct option long_options[] = {
			{"help",	no_argument,	0,	'h'},
			{"gui",		no_argument,	0,	'g'},
			{0,0,0,0}
		};
		c = getopt_long (argc, argv, "h", long_options, &option_index);
		if (c == -1)
			break;
		switch(c) {
			case 'h':
			case '?':
				Usage(argc,argv);
				return 0;
			case 'g':
				useGUI = true;
				break;
			default:
				printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}
	
	if( useGUI ) {
		MainApp *myApplication;
	
		myApplication = new MainApp();
	
		myApplication->Run();
		
		delete(myApplication);
		return(0);
	}
	
	if( optind >= argc ) {
		Usage(argc,argv);
		return 5;
	}
	
	while( optind < argc ) {
		const char* dirname = argv[optind];
		optind++;
		
		BDirectory dir(dirname);
		status_t err = dir.InitCheck();
		if( err ) {
			cerr << "*** Unable to open directory " << dirname << ":" << endl
				<< strerror(err) << endl;
			continue;
		}
		
		vector<ArpString> names;
		
		entry_ref ref;
		while( dir.GetNextRef(&ref) == B_OK ) {
			names.push_back(ArpString(ref.name));
		}
		
		sort(names.begin(), names.end(), ArpString_CaseNumLt);
		
		cerr << "Sorted " << names.size() << " Files:" << endl;
		for( size_t i=0; i<names.size(); i++ ) {
			cerr << names[i] << endl;
		}
	}
	
	return 0;
}

MainApp::MainApp()
	: BApplication(ArpAppSig),
	  mCmdLine(false), mOpenPanel(0), mSavePanel(0)
{
}

MainApp::~MainApp()
{
	delete mOpenPanel;
	delete mSavePanel;
}

bool MainApp::QuitRequested(void)
{
	if( mMainWin.IsValid() ) {
		mMainWin.SendMessage(B_QUIT_REQUESTED);
		if( mAboutWin.IsValid() ) mAboutWin.SendMessage(B_QUIT_REQUESTED);
		return true;
	} else {
		if( mAboutWin.IsValid() ) mAboutWin.SendMessage(B_QUIT_REQUESTED);
		return true;
	}
}

void MainApp::AboutRequested(void)
{
	if( !mAboutWin.IsValid() ) {
		ArpAboutWindow* win = 
			new ArpAboutWindow(0, "printmsg", 0, __DATE__,
					"printmsg version " PROGRAM_VERSION
					" / " __DATE__ "\n"
					B_UTF8_COPYRIGHT "1999 Angry Red Planet Software.\n\n"
					
					"Print BMessage objects.  Wow.\n\n"
					
					"For more info and the latest version, see\n"
					"<URL:http://www.angryredplanet.com/>.\n\n"
					
					"Written by Dianne Hackborn\n"
					"(email: hackbod@angryredplanet.com)\n\n");

		mAboutWin = BMessenger(win);
		win->Show();
	}
}

void MainApp::DispatchMessage(BMessage *message,
								  BHandler *handler)
{
	ArpD(cdb << ADH << "MainApp::DispatchMessage: " <<
				*message << endl);
	inherited::DispatchMessage(message,handler);
}

void MainApp::MessageReceived(BMessage* message)
{
	if( !message ) return;
	
	switch( message->what ) {
		default:
			inherited::MessageReceived(message);
	}
}

class PrintView : public BView
{
public:
	PrintView(	BRect frame,
				const char *name,
				uint32 resizeMask = B_FOLLOW_ALL,
				uint32 flags = B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
		: BView(frame, name, resizeMask, flags)
	{
		SetFont(be_bold_font);
	}
	
	virtual	void MessageReceived(BMessage *msg)
	{
		if( !msg ) return;
		if( msg->WasDropped() ) {
			cout << "Message dropped on window:" << endl
				<< *msg << endl;
		}
		
		BView::MessageReceived(msg);
	}
	
	virtual	void Draw(BRect updateRect)
	{
		MoveTo(BPoint(0, Bounds().bottom/2));
		DrawString("Drop Me, Baby!");
	}
};

bool MainApp::MakeMainWindow(const entry_ref file)
{
	BWindow* win = new BWindow(BRect(30,30,400,200), "Drop Message",
							   B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
							   B_ASYNCHRONOUS_CONTROLS);
	if( win ) {
		win->AddChild( new PrintView(win->Bounds(), "PrintView") );
		win->Show();
		mMainWin = BMessenger(win);
		return true;
	}
	return false;
}

void MainApp::ArgvReceived(int32 argc, char** argv)
{
	ArpParseDBOpts(argc, argv);
}

void MainApp::RefsReceived(BMessage* message)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	
	if( !message ) return;
	
	message->GetInfo("refs", &type, &count); 
	if ( type != B_REF_TYPE ) return; 
	
	if( count > 0 ) {
		if( message->FindRef("refs", 0, &ref) == B_OK ) {
			if( !mMainWin.IsValid() ) {
				MakeMainWindow(ref);
			}
		}
	}
}

void MainApp::ReadyToRun()
{
	if( !mMainWin.IsValid() ) MakeMainWindow();
}
