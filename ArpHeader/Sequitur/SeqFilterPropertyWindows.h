/* SeqEditRosterWindow.h
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
 * 2001.05.30		hackborn@angryredplanet.com
 * Extracted from SeqFilterConfigWindow.
 */
#ifndef SEQUITUR_SEQEDITROSTERWINDOW_H
#define SEQUITUR_SEQEDITROSTERWINDOW_H

#include <interface/ColumnTypes.h>
#include <InterfaceKit.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "AmPublic/AmPipelineMatrixRef.h"
#include "BeExp/ToolTip.h"
class SeqSplitterView;
class _FilterPageRow;

enum {
	// Send to window to active/bring to front.
	SEQ_ACTIVATE = '.sac'
};

/******************************************************************
 * SEQ-ABSTRACT-FILTER-PROPERTY-WINDOW
 ******************************************************************/
class SeqAbstractFilterPropertyWindow : public BWindow,
										public ArpBitmapCache,
										public BToolTipFilter
{
public:
	SeqAbstractFilterPropertyWindow(BWindow* parent = NULL,
									window_look look = B_DOCUMENT_WINDOW_LOOK,
									window_feel feel = B_NORMAL_WINDOW_FEEL,
									uint32 flags = B_ASYNCHRONOUS_CONTROLS | B_FRAME_EVENTS);
	~SeqAbstractFilterPropertyWindow();
	
	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	void				SetPage(BView* view);

protected:
	BRect				CurrentPageFrame() const;
	status_t			SetFirstPage();
	
	virtual void		AddPages(BColumnListView* list, BPoint& pt) = 0;

	void				Init();
	void				AddFilterPage(	AmFilterI* filter,
										BColumnListView* list, BRow* parent,
										BPoint& pt, BWindow* win);
	void				AddMultiFilterPage(	AmMultiFilter* filter,
											BColumnListView* list, BRow* parent,
											BPoint& pt, BWindow* win);

private:
	typedef BWindow 	inherited;

	BView*				mBg;
	BColumnListView*	mListView;
	SeqSplitterView*	mSplitter;
	BView*				mPageView;
	BView*				mBlankPageView;

	void				AddViews(BRect frame, BPoint& pt);
	_FilterPageRow*		AddRow(	const BString& label, BView* view,
								BColumnListView* list, BRow* parent,
								BPoint& pt, BWindow* win);
};

/******************************************************************
 * SEQ-PIPELINE-PROPERTY-WINDOW
 ******************************************************************/
class SeqPipelinePropertyWindow : public SeqAbstractFilterPropertyWindow
{
public:
	SeqPipelinePropertyWindow(	const AmPipelineMatrixRef& matrixRef,
								AmPipelineType pipelineType,
								const char* pipelineName,		// Like the name of the tool or filter
								BWindow* parent = NULL);	
protected:
	virtual void		AddPages(BColumnListView* list, BPoint& pt);

private:
	typedef SeqAbstractFilterPropertyWindow inherited;

	AmPipelineMatrixRef	mMatrixRef;
	AmPipelineType		mPipelineType;
};

/******************************************************************
 * SEQ-FILTER-PROPERTY-WINDOW
 ******************************************************************/
class SeqFilterPropertyWindow : public SeqAbstractFilterPropertyWindow
{
public:
	SeqFilterPropertyWindow(const AmPipelineMatrixRef& matrixRef,
							AmPipelineType pipelineType,
							AmFilterHolderI* filter,
							BWindow* parent);
	~SeqFilterPropertyWindow();

	virtual void		MessageReceived(BMessage* msg);

protected:
	virtual void		AddPages(BColumnListView* list, BPoint& pt);

private:
	typedef SeqAbstractFilterPropertyWindow inherited;

	AmPipelineMatrixRef	mMatrixRef;
	AmFilterHolderI*	mFilter;
};

#endif
