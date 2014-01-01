/* SeqEditMultiFilterWindow.h
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

#ifndef SEQUITUR_SEQEDITMULTIFILTERWINDOW_H
#define SEQUITUR_SEQEDITMULTIFILTERWINDOW_H

#include "AmPublic/AmToolRef.h"
#include "Sequitur/SeqEditRosterWindow.h"
class AmMultiFilter;
class AmMultiFilterAddOn;
class SeqBitmapEditor;
class SeqPipelineMatrixView;

/*************************************************************************
 * SEQ-EDIT-MULTI-FILTER-WINDOW
 * This window allows users to edit all the properties for a single
 * multi filter.
 *************************************************************************/
class SeqEditMultiFilterWindow : public SeqEditRosterWindow
{
public:
	SeqEditMultiFilterWindow(	BRect frame,
								const BMessage* config,
								const BMessage* multiMsg);
	virtual ~SeqEditMultiFilterWindow();

	virtual void		MessageReceived(BMessage* msg);
	virtual	void		Quit();
	virtual	bool		QuitRequested();

	void				SetMultiFilter(const BMessage* multiMsg);
	void				SetMultiFilter(const BString& key, const BString& path);

protected:
	virtual uint32		ConfigWhat() const;
	virtual	bool		Validate();
	void				SaveChanges();
	virtual const char*	EntryName() const;
		
private:
	typedef SeqEditRosterWindow inherited;
	AmMultiFilterAddOn*	mMultiFilterAddOn;
	AmMultiFilter*		mMultiFilter;
	BTextControl*		mNameCtrl;
	BTextControl*		mKeyCtrl;
	SeqDumbTextView*	mLongDescriptionCtrl;
	SeqPipelineMatrixView* mPipelineView;
	BScrollView*		mPipelineScrollView;
	BMenuField*			mIconCtrl;
	SeqBitmapEditor*	mIconEditor;

	void				SetWindowTitle();
	void				SetPipelineScrollBars();

	BView*				NewGeneralView(BRect frame);
	BView*				NewPipelineView(BRect frame);
	BView*				NewDescriptionView(BRect frame);
	BView*				NewIconView(BRect frame);
};

#endif
