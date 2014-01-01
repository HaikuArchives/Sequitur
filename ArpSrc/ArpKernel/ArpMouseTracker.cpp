/*
	
	ArpMouseTracker.cpp
	
	Copyright (c)1998 by Angry Red Planet.

	This code is distributed under a modified form of the
	Artistic License.  A copy of this license should have
	been included with it; if this wasn't the case, the
	entire package can be obtained at
	<URL:http://www.angryredplanet.com/>.
*/

#ifndef APRKERNEL_ARPMOUSETRACKER_H
#include "ArpKernel/ArpMouseTracker.h"
#endif

#ifndef _LOCKER_H
#include <Locker.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

ArpMouseTracker::ArpMouseTracker()
	: trackThread(-1), ownerView(NULL),
	  pollTime(0), pollPerPulse(0)
{
}

ArpMouseTracker::~ArpMouseTracker()
{
	StopMouseTracker();
}

status_t ArpMouseTracker::StartMouseTracker(BView* inowner,
											bigtime_t inpoll,
											int32 pulse)
{
	StopMouseTracker();
	
	// Set up thread information
	if( !inowner ) inowner = dynamic_cast<BView*>(this);
	if( !inowner ) return B_BAD_VALUE;
	ownerView = inowner;
	pollTime = inpoll;
	pollPerPulse = pulse;
	
	// Retrieve initial mouse position
	ownerView->GetMouse(&initPos, &initButtons);
	
	// Create & start tracking thread
	trackThread = spawn_thread(trackThreadEntry,
								"ARP™ Mouse Tracker",
								B_DISPLAY_PRIORITY,
								this);
	if( trackThread < 0 ) return (status_t)trackThread;
	status_t res = resume_thread(trackThread);
	
	// Kill the thread if unable to start it.
	if( res ) StopMouseTracker();
	
	return res;
}

void ArpMouseTracker::StopMouseTracker(void)
{
	if( trackThread >= 0 ) {
		status_t ret;
		kill_thread(trackThread);
		// Apparently wait_for_thread() does block...
		// Oh well, it -should- be dead by now, right?
#if 1
		// Wait for this thread to exit.
		// XXX Are we SURE the thread won't ever block here?
		wait_for_thread(trackThread,&ret);
#endif
	}
	ownerView = NULL;
	pollTime = 0;
	pollPerPulse = 0;
}

void ArpMouseTracker::TrackMouseMoved(BPoint, uint32)
{
}

void ArpMouseTracker::TrackMousePulse(BPoint, uint32)
{
}

void ArpMouseTracker::TrackMouseUp(BPoint, uint32)
{
}

int32 ArpMouseTracker::trackThreadEntry(void *arg) 
{ 
	ArpMouseTracker *obj = (ArpMouseTracker *)arg; 
	return (obj->trackThreadFunc()); 
}

int32 ArpMouseTracker::trackThreadFunc(void)
{
	int32 count=0;
	BPoint last_pos = initPos;
	uint32 last_but = initButtons;
	uint32 buttons;
	
	do {
		// Copy ownerView to be sure it doesn't change under us.
		BView* view = ownerView;
		BWindow* win = NULL;
		if( view ) win = view->Window();
		if( !win || !win->Lock() ) break;
		
		BPoint where;
		view->GetMouse(&where, &buttons);
		
		if( buttons && (where!=last_pos || buttons!=last_but) ) {
			TrackMouseMoved(where,buttons);
			last_pos = where;
			last_but = buttons;
		}
		
		if( !buttons ) {
			TrackMouseUp(last_pos,buttons);
		} else if( pollPerPulse > 0 ) {
			count = (count+1)%pollPerPulse;
			if( count == 0 ) TrackMousePulse(where,buttons);
		}
		
		if( win ) win->Unlock();
		snooze(pollTime);
	} while(buttons);

	ownerView = NULL;
	trackThread = -1;
	return 0;
}
