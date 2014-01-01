/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include <Window.h>

//#define USE_LIFEVIEW

#ifdef USE_LIFEVIEW
#include "lifeView.h"
#else
#include "learnView.h"
#endif

class lifeWin : public BWindow
{
 public:
 	lifeWin();
 	~lifeWin();
 	bool QuitRequested();
 private:
#ifdef USE_LIFEVIEW
 	lifeView *mv;
#else
	learnView*	mLearnView;
#endif
};
