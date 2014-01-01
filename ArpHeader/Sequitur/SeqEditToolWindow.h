/* SeqEditToolWindow.h
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
 * 2001.01.31		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQEDITTOOLWINDOW_H
#define SEQUITUR_SEQEDITTOOLWINDOW_H

#include "AmPublic/AmToolRef.h"
#include "Sequitur/SeqEditRosterWindow.h"
class SeqBitmapEditor;
class SeqPipelineMatrixView;
class _ControlTargetList;
class _GraphicList;
class _ToolControlList;
class _ViewList;

/*************************************************************************
 * SEQ-EDIT-TOOL-WINDOW
 * This window allows users to edit all the properties for a single
 * tool.
 *************************************************************************/
class SeqEditToolWindow : public SeqEditRosterWindow
{
public:
	SeqEditToolWindow(	BRect frame,
						const BMessage* config,
						const BMessage* toolMsg);
	virtual ~SeqEditToolWindow();

	virtual	void		FrameResized(float new_width, float new_height);
	virtual	void		MenusBeginning();
	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	void				SetTool(const BMessage* toolMsg);
	void				SetTool(const BString& uniqueName,
								const BString& path);

protected:
	virtual uint32		ConfigWhat() const;
	virtual	bool		Validate();
	virtual	void		SaveChanges();
	virtual const char*	EntryName() const;

private:
	typedef SeqEditRosterWindow inherited;
	AmToolRef			mToolRef;
	BTextControl*		mNameCtrl;
	BTextControl*		mKeyCtrl;
	_ViewList*			mSeedList;
	BMenuField*			mSeedCtrl;
	SeqPipelineMatrixView* mPipelineView;
	BScrollView*		mPipelineScrollView;
	SeqDumbTextView*	mLongDescriptionCtrl;
	BMenuField*			mIconCtrl;
	SeqBitmapEditor*	mIconEditor;
	_ToolControlList*	mToolControlList;
	BButton*			mDeleteControlButton;
	_ControlTargetList*	mControlTargetList;
	BMenu*				mAddTargetMenu;
	BMenuField*			mEffectCtrl;
	_GraphicList*		mGraphicList;

	void				SetWindowTitle();
	void				SetPipelineScrollBars();
	
	BView*				NewGeneralView(BRect frame);
	BView*				NewSeedView(BRect frame);
	BView*				NewPipelineView(BRect frame);
	BView*				NewDescriptionView(BRect frame);
	BView*				NewIconView(BRect frame);
	BView*				NewControlView(BRect frame);
	BView*				NewEffectsView(BRect frame);
};

#endif
