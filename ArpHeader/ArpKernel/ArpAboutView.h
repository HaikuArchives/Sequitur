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
 * ArpAboutView.h
 *
 * This is Angry Red Planet's developing standard "About" box.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 * This is still in development, so the code inside is rather ugly...
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * Dec 6, 1998:
 *	First public release.
 *
 */

#ifndef ARPKERNEL_ARPABOUTVIEW_H
#define ARPKERNEL_ARPABOUTVIEW_H

#ifndef _VIEW_H
#include <be/interface/View.h>
#endif

#ifndef _BITMAP_H
#include <be/interface/Bitmap.h>
#endif

#ifndef __BSTRING__
#include <be/support/String.h>
#endif

#ifndef ARPCOLLECTIONS_ARPVECTORI_H
#include <ArpCollections/ArpVectorI.h>
#endif

class BButton;

class ArpAboutView : public BView 
{
public:
	ArpAboutView(BRect frame, const char *name, const char *text,
				const char* appname, const char* verstr, const char* build); 
	~ArpAboutView();
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	virtual void GetPreferredSize(float* width, float* height);
	
	virtual	void FrameResized(float new_width, float new_height);
	virtual void Draw(BRect rect);
	virtual void MouseDown(BPoint point);
	virtual void MessageReceived(BMessage *message);
	
	virtual void StepAnim();
	
private:
	typedef BView inherited;
	
	void FreeMemory();
	float FindMaxWidth(const BFont& font);
	void ComputeDimens();
	
	void DrawLine(const char* text, float pos, float center, BFont& font);
	
	ArpVectorI<BString>* mText;
	size_t N;
	uint32 mDrawMode;
	
	BString mAppName;
	BString mVersion;
	BString mBuild;
	
	BBitmap* mAppPic;
	BBitmap* mArpPic;
	BButton* mDoneBut;
	BButton* mWebBut;
	BFont mBaseFont;
	
	// Global layout
	int32 mOutliers;			// number of lines above and below center
	float mFlatPart;			// part of total (0.0-1.0) that is not rotated
	
	// Values computed by ComputeDimens()
	float mBaseSize;			// size of mBaseFont
	float mRotPart;				// 1-mFlatPart
	float mFlatPos;				// mFlatPart*mOutliers
	float mFlatPixel;			// mFlatPos*mBaseSize
	BPoint mTextCenter;
	float mYRot;				// Height of area that we rotate through
	
	bool mUsedChars[128];
	
	// Rendering
	BBitmap* mBitmap;
	BView* mDrawView;
	float mStep;
	bigtime_t mAnimBaseTime;
	bool mFirstTime;
	
	// Animation thread
	thread_id mAnimThread;
	static int32 animEntry(void* arg);
	int32 animFunc(void);
};

#endif
