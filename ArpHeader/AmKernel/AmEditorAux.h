/* AmEditorAux.h
 * Copyright (c)2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This file is just helper classes for the track window.
 * There was getting to be enough that I pulled it into a
 * separate file.
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
 * 2001.04.18		hackborn@angryredplanet.com
 * Mutated this file from its original incarnation.
 */
#ifndef AMKERNEL_AMEDITORWAUX_H
#define AMKERNEL_AMEDITORWAUX_H

#include <vector.h>
#include <be/InterfaceKit.h>
#include "BeExp/ToolTip.h"

/*************************************************************************
 * AM-EDITOR-TOOL
 * This is an abstract definition for a tool.
 *************************************************************************/
class AmEditorTool
{
public:
	AmEditorTool(const BBitmap* icon, const BString& toolTip, uint32 code = 0);

	const BBitmap*		Icon() const;
	BString				ToolTip() const;
	/* It's not necessary for tools to have a code, but since this
	 * is such a simple tool definition, clients can give tools one
	 * as an identifier.
	 */
	uint32				Code() const;

private:
	const BBitmap*		mIcon;
	BString				mToolTip;
	uint32				mCode;
};

/*************************************************************************
 * AM-ACTIVE-TOOL-VIEW
 * This class displays which tools are mapped to the mouse buttons.
 *************************************************************************/
class AmActiveToolView : public BView
{
public:
	AmActiveToolView(	BPoint origin, float leftOverhang = 11,
						float topOverhang = 4, float rightOverhang = 11);

	/* The tool is not copied, make sure it exists for the life
	 * of this view.
	 */
	void			SetActiveTool(int32 button, const AmEditorTool* tool);
	const AmEditorTool* ActiveTool(int32 button) const;
	/* If there's no active tool, this will be 0.
	 */
	uint32			ActiveToolCode(int32 button) const;

	virtual	void	AttachedToWindow();
	virtual void	Draw(BRect clip);
	virtual void	GetPreferredSize(float* width, float* height);

	/* Clear out my references to the tools.
	 */
	void			ClearTools();
	
protected:
	void			DrawOn(BView*, BRect clip);

private:
	typedef BView			inherited;
	int32					mMouseType;
	const BBitmap*			mMouseBitmap;
	const AmEditorTool*		mPrimaryTool;
	const AmEditorTool*		mSecondaryTool;
	const AmEditorTool*		mTertiaryTool;
	rgb_color				mViewC;
	float					mLeftOverhang;
	float					mRightOverhang;
	float					mTopOverhang;
};

/***************************************************************************
 * AM-EDITOR-TOOL-BAR-VIEW
 * This class displays a single toolbar designed for AmEditorTools.
 ***************************************************************************/
class AmEditorToolBarView : public BView,
							public BToolTipable
{
public:
	AmEditorToolBarView(BPoint at, const char* name, float space = 0);
	virtual ~AmEditorToolBarView();

	void			AddTool(AmEditorTool* tool);
	void			SetActiveToolView(AmActiveToolView* atv);
	
	virtual void	AttachedToWindow();
	virtual void	Draw(BRect clip);
	virtual	void	GetPreferredSize(float *width, float *height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage *a_message);
	virtual	void	MouseUp(BPoint where);

	virtual status_t GetToolTipInfo(BPoint where, BRect* out_region,
									BToolTipInfo* out_info = 0);
	/* Clear out my references to the tools.
	 */
	void			ClearTools();

protected:
	void			DrawOn(BView* view, BRect clip);

private:
	typedef BView			inherited;
	vector<AmEditorTool*>	mTools;
	AmEditorTool*			mMouseDownTool;	// Set to whatever tool the user clicks on
											// in the MouseDown()
	uint32					mMouseDownButton;
	AmEditorTool*			mMouseOverTool;
	AmActiveToolView*		mActiveToolView;
	
	void			PostMouseUp();
	
	rgb_color		mViewC;
	float			mSpace;				// Number of pixels between tools

	/* My preferred cached width and height, just a convenience.
	 */
	float			mPrefW, mPrefH;

	float				DrawToolOn(	BView* view, BRect clip, AmEditorTool* tool,
									float atX, BPoint mousePt);
	AmEditorTool*		ToolAt(BPoint where, BRect* frame = NULL);
};

#endif
