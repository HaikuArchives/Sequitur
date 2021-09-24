/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <GLView.h>
#include <View.h>

#ifndef _common_h
#include "common.h"
#endif

#include <cstdio>
#include <MenuItem.h>
#include <cstdlib>
#include "MDust/MSpace.h"
#include "MDust/MultiLocker.h"

const int BOARD_SIZE = 60;

class learnView : public BGLView
{
 public:
	learnView(BRect R);
	~learnView();

	MSpace* Space() const;
	uint32 Frame() const;
	void IncrementFrame(uint32 delta);
	
	void AttachedToWindow();
	void MessageReceived(BMessage *msg);
	
 	void SpinCalc();
	void Display();
	void ExitThreads();

	void Start();
	
	inline void SetAllAngles(float ang); 
	inline bool spinning(); 
	inline bool continuousMode();
	inline void continuousMode(bool val);
	inline bool singleStepMode();
	inline void singleStepMode(bool val);
	inline bool QuitPending();
	inline void QuitPending(bool val);	

	/* A lock for reading and writing my MSpace.
	 */
	MultiLocker		mSpaceLock;
	
 private:
 	/* The data representing this space
 	 */
 	MSpace*		mSpace;
	/* The current position in the song.  0 is the start.
	 */
	uint32		mFrame;
	 	
	void DrawFrame();
	
	char genStr[50];

	bool continuous;
	bool singleStep;
	bool _QuitRequested;
		
	thread_id	drawTID; 
	thread_id	lifeTID;
	thread_id	frameTID;

	float xspin, yspin, zspin;
	int xangle, yangle, zangle;
};

// inline functions
void learnView::SetAllAngles(float ang) {xangle = yangle = zangle = (int)ang; }

bool learnView::spinning() { return (xspin || yspin || zspin); }

bool learnView::continuousMode() { return continuous; }
void learnView::continuousMode(bool val) { continuous = val; }
bool learnView::singleStepMode() { return singleStep; }
void learnView::singleStepMode(bool val) { singleStep = val; }
bool learnView::QuitPending() { return _QuitRequested; }
void learnView::QuitPending(bool val) { _QuitRequested = val; }

