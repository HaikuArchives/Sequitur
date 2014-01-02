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

#define ArpDEBUG 1

#ifndef _APPLICATION_H
#include <app/Application.h>
#endif

#ifndef _RECT_H
#include <interface/Rect.h>
#endif

#ifndef _ALERT_H
#include <interface/Alert.h>
#endif

#include <translation/TranslationUtils.h >

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#ifndef ARPKERNEL_ARPMULTIDIR_H
#include <ArpKernel/ArpMultiDir.h>
#endif

#ifndef ARPKERNEL_ARPSTRING_H
#include <ArpKernel/ArpString.h>
#endif

#ifndef MAIN_H
#include "Main.h"
#endif

#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <vector>

main()
{	
	MainApp *myApplication;

	myApplication = new MainApp();

	myApplication->Run();
	int result = myApplication->Result();
	
	delete(myApplication);
	return(result);
}

MainApp::MainApp()
	: BApplication("application/x-vnd.ARP-makearpicons"),
	  mResult(0)
{
}

void MainApp::AboutRequested(void)
{
	BAlert* alert =
		new BAlert("About makearpicons",
					"makearpicons version " PROGRAM_VERSION
					" / " __DATE__ "\n"
					B_UTF8_COPYRIGHT "1998 Angry Red Planet, Inc.\n\n",
					"Fantastic");
	if( alert ) {
		alert->Go(NULL);
	}
}

void MainApp::SendQuitMsg(int result)
{
	mResult = result;
	PostMessage(B_QUIT_REQUESTED);
}

void MainApp::ArgvReceived(int32 argc, char** argv)
{
	if( argc < 3 ) {
		cout << "Usage: makearpicons outresfile bitmap namefile" << endl;
		SendQuitMsg(1);
		return;
	}
	
	BBitmap* bitmap = 0;
	vector<ArpString> iconnames;
	
	try {
		BFile resfile(argv[1]);
		if( resfile.InitCheck() ) {
			cerr << "*** Unable to open resources file " << argv[1] << "." << endl;
			SendQuitMsg(1);
			return;
		}
	
		BResources resources(&resfile, true);
	
		ifstream readnames(argv[3]);
		if( !readnames.good() ) {
			cerr << "*** Unable to open description file " << argv[3] << "." << endl;
			SendQuitMsg(1);
			return;
		}
		
		while( !readnames.eof() ) {
			char buff[1024];
			readnames.getline(buff, sizeof(buff)-1);
			if( buff[0] == '+' ) {
				iconnames.push_back(ArpString(&buff[1]));
			}
		}
		
		bitmap = GetBitmapFile(argv[2]);
		if( !bitmap ) {
			cerr << "*** Unable to open bitmap file " << argv[2] << "." << endl;
			SendQuitMsg(1);
			return;
		}
	
	// Process command line arguments:  Connect to given host
	// and optional port.
	if( argc >= 2 ) {
		BMessage connmsg(ARPTELNET_CONNECT_MSG);
		if( strchr(argv[1],':') != NULL ) {
			if( !mMadeWindow ) {
				mMadeWindow = MakeTermWindow(argv[1]);
			} else {
				// Send a raw url.
				connmsg.AddString("url", argv[1]);
				termmsg.SendMessage(&connmsg);
			}
		} else {
			ArpString url;
			url = ArpLIT("telnet://") + argv[1];
			if( argc >= 3 ) url += ArpLIT(":") + argv[2] + ":";
			if( !mMadeWindow ) {
				mMadeWindow = MakeTermWindow(url);
			} else {
				connmsg.AddString("url", url);
				termmsg.SendMessage(&connmsg);
			}
		}
		cmdline = true;
	}
}

void MainApp::ReadyToRun()
{
	if( !mMadeWindow ) mMadeWindow = MakeTermWindow("shell:");
}

