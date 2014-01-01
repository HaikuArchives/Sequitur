/*
 * Copyright (c)1998 by Angry Red Planet.
 *
 * This code is distributed under a modified form of the
 * Artistic License.  A copy of this license should have
 * been included with it; if this wasn't the case, the
 * entire package can be obtained at
 * <URL:http://www.angryredplanet.com/>.
 *
 * ----------------------------------------------------------------------
 *
 * ArpMouseTracker.h
 *
 * NOTE: This should go away with the new B_ASYNCHRONOUS_CONTROLS
 * option in R4.
 *
 * A class that encapsulates all of the garbage needed to
 * track the mouse pointer when doing drag operations.  This
 * means creating a thread, watching for changes in mouse
 * position, and sleeping so we don't chew many cycles.
 * It is intended to be used as a mix-in class: you combine
 * it with the class that is derived from BView, and override
 * the TrackMouse*() functions as you would BView's Mouse*()
 * functions.
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
 *	• Should really move "inowner" to be the last argument to
 *	  StartMouseTracker().
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 0.1: (7/8/1997) Created this file.
 *
 */

#ifndef ARPKERNEL_ARPMOUSETRACKER_H
#define ARPKERNEL_ARPMOUSETRACKER_H

#ifndef _VIEW_H
#include <View.h>
#endif

class ArpMouseTracker {

public:

	ArpMouseTracker();
	virtual ~ArpMouseTracker();

	// If you are a subclass that has inherited from both
	// ArpMouseTracker and BView, you don't need to pass in
	// any arguments -- the associated BView will automatically
	// be found for you.  [Ain't RTTI grand!].
	// Arguments:
	//  inowner: optional target BView to watch mouse in.
	//  inpoll: rate it which to poll mouse, in microseconds.
	//  pulse: number of polls between calls to TrackMousePulse().
	//
	// Errors:
	//	B_BAD_VALUE if associated BView can't be found.
	//	B_NO_ERROR if all is okay.
	//	Otherwise, standard spawn_thread and resume_thread
	//	error codes.
	
	// NOTE -- you MUST have the view's window locked before
	// calling StartMouseTracker().
	status_t StartMouseTracker(BView* inowner=NULL,
								bigtime_t inpoll=20000,
								int32 pulse=2);
								
	// Terminate any running track thread, and clean up.
	// TrackMouseUp() will NOT be called.
	void StopMouseTracker(void);
	
	// These are called as the mouse information changes
	// accordingly.  The window belonging to BView is
	// guaranteed to be locked when control enters either of
	// these functions.
	virtual void TrackMouseMoved(BPoint point, uint32 buttons);
	virtual void TrackMouseUp(BPoint point, uint32 buttons);
	virtual void TrackMousePulse(BPoint point, uint32 buttons);
	
private:

	thread_id trackThread;
	BView* ownerView;
	bigtime_t pollTime;
	int32 pollPerPulse;
	
	BPoint initPos;
	uint32 initButtons;
	
	static int32 trackThreadEntry(void* arg);
	int32 trackThreadFunc(void);
};

#endif
