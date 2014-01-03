/* SeqBitmapEditor.h
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
 * 2001.02.07		hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef SEQUITUR_SEQBITMAPEDITOR_H
#define SEQUITUR_SEQBITMAPEDITOR_H

#include <InterfaceKit.h>
class AmActiveToolView;
class AmEditorTool;
class AmEditorToolBarView;

void seq_write_pixel(const BBitmap* bm, float x, float y, rgb_color c);

/***************************************************************************
 * SEQ-BITMAP-EDITOR
 ***************************************************************************/
class SeqBitmapEditor : public BView
{
public:
	/* The editor does not own the bitmap it's supplied, instead
	 * it reads and writes directly to whatever is passed in.
	 * If an action menu is supplied, I will add an Action button
	 * to the left of my controls, and I become the owner of this
	 * menu.  Most clients should, if nothing else, include an
	 * action menu with the Paste and Copy commands.
	 */
	SeqBitmapEditor(BRect frame, const char* name,
					BBitmap* bitmap, uint32 resizeMask,
					BMenu* actionMenu = NULL);
	virtual ~SeqBitmapEditor();

	virtual void	AttachedToWindow();
	virtual	void	FrameResized(float new_width, float new_height);
	virtual	void	MessageReceived(BMessage* msg);
	
	void			SetBitmap(BBitmap* bitmap);
	/* I will send this message out the first time the
	 * bitmap changes.
	 */
	void			SetBitmapChangeMessage(BMessage* msg);	

	/* Manipulations.
	 */
	status_t		Copy();
	status_t		Paste();
	status_t		FlipVertically();
	status_t		FlipHorizontally();
	status_t		FillAlpha();
	
private:
	typedef BView			inherited;
	AmActiveToolView*		mActiveTools;
	AmEditorToolBarView*	mToolBar;
	AmEditorTool*			mPencilTool;
	AmEditorTool*			mDropperTool;
	BScrollView*			mScrollView;
		
	void					HandleZoom(BMessage* msg);
	void					SetupScrollBars(BView* target);
	void					AddViews(BRect frame, BMenu* actionMenu);
};

#endif
