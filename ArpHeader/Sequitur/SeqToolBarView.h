/* SeqToolBarView.h
 * Copyright (c)2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.03.30		hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef SEQUITUR_SEQTOOLBARVIEW_H
#define SEQUITUR_SEQTOOLBARVIEW_H

#include <vector.h>
#include <interface/Bitmap.h>
#include <interface/View.h>
#include "AmPublic/AmToolBarRef.h"
#include "AmKernel/AmKernelDefs.h"
#include "BeExp/ToolTip.h"
class _SeqCachedTool;

typedef vector<_SeqCachedTool>	cachedtool_vec;

/***************************************************************************
 * SEQ-TOOL-BAR-VIEW
 * This class displays a single toolbar.
 * NOTE:  The way this class is written, whenever it wants to access the
 * tools, it creates a copy of all the tool refs in the tool bar and
 * then accesses those.  This is so that I don't have both the tool bar
 * and its tools locked at the same time, which strikes me as a bad idea.
 * Hopefully there's a better way to do this and we can clean it up later.
 ***************************************************************************/
class SeqToolBarView : public BView,
					   public BToolTipable
{
public:
	SeqToolBarView(	BPoint at, const char* name,
					const BString& toolBarName, float space = 0);
	SeqToolBarView(	BPoint at, const char* name,
					AmToolBarRef toolBarRef, float space = 0);
	virtual ~SeqToolBarView();

	virtual void	AttachedToWindow();
	virtual void	Draw(BRect clip);
	virtual	void	GetPreferredSize(float *width, float *height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage *a_message);
	virtual	void	MouseUp(BPoint where);

	virtual	void	SetViewColor(rgb_color c);

	virtual status_t GetToolTipInfo(BPoint where, BRect* out_region,
									BToolTipInfo* out_info = 0);

protected:
	void			SetToolBar(AmToolBarRef toolBarRef);

	void			DrawOn(BView* view, BRect clip);

private:
	typedef BView	inherited;
	AmToolBarRef	mToolBarRef;
	mutable BString	mToolBarName;
	_SeqCachedTool*	mMouseDownTool;		// Set to whatever tool the user clicks on
										// in the MouseDown()
	/* This is because the WACOM driver seems to report
	 * that no buttons are held down during the mouse move.
	 * This is used as a fallback -- if we hit a mouse up,
	 * and the normal button state has been cleared, use
	 * this one.
	 */
	uint			mMouseDownButtonHack;
	int32			mOverToolIndex;		// Keeps track of the current tool index
										// that's been told to draw the Over icon.
	
	void			PostMouseUp();
	
	rgb_color		mViewC;
	bool			mEmpty;
	float			mSpace;				// Number of pixels between tools

	BRect			mDraggingRect;		// The insertion area if the user is dragging a tool
	/* Cached info so drawing doesn't have much overhead.
	 */
	float			mPrefW, mPrefH;
	cachedtool_vec	mCachedData;
	void			CacheDrawingInfo();

	void			FillVec(toolref_vec& toolRefs) const;
	void			FreeCachedInfo();

	/* If the caller supplies a bool for hitPropertyMenu, then
	 * it will be set to true if the mouse point lies inside of
	 * the tool's property menu area.
	 */
	_SeqCachedTool*	CachedToolAt(BPoint pt, bool* hitPropertyMenu = NULL, BRect* frame = NULL);
	BRect			DraggingRectFor(BPoint pt, int32* lIndex = NULL, int32* rIndex = NULL);
	void			HandleToolDrop(const char* toolName);
	void			ShowPropertyMenu(const AmTool* tool, BPoint where);
};

#endif
