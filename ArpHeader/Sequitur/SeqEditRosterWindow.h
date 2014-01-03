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
#include "BeExp/ToolTip.h"
#include "Sequitur/SeqWindowStateI.h"
class SeqDumbTextView;
class SeqSplitterView;

/******************************************************************
 * SEQ-EDIT-ROSTER-WINDOW
 ******************************************************************/
class SeqEditRosterWindow : public BWindow,
							public ArpBitmapCache,
						    public BToolTipFilter,
							public SeqWindowStateI
{
public:
	SeqEditRosterWindow(BRect frame, const char* name,
						window_look look = B_DOCUMENT_WINDOW_LOOK,
						window_feel feel = B_NORMAL_WINDOW_FEEL,
						uint32 flags = B_ASYNCHRONOUS_CONTROLS | B_FRAME_EVENTS);
	~SeqEditRosterWindow();
	
	virtual void		MessageReceived(BMessage *message);
	virtual	void		Quit();
	virtual	bool		QuitRequested();

	void				SetPage(BView* view);

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool		IsSignificant() const;
	virtual status_t	GetConfiguration(BMessage* config);
	status_t			SetConfiguration(const BMessage* config);

protected:
	BString				mInitialKey;
	BString				mInitialAuthor;
	BString				mInitialEmail;
	BTextControl*		mAuthorCtrl;
	BTextControl*		mEmailCtrl;
	SeqDumbTextView*	mShortDescriptionCtrl;

	virtual uint32		ConfigWhat() const = 0;
	virtual	bool		Validate() = 0;
	virtual	void		SaveChanges() = 0;
	virtual const char*	EntryName() const = 0;
	/* This utility should be called before a subclass sets its
	 * main data object.  If it returns false, then it shouldn't set
	 * the object.
	 */
	bool				SetEntryCheck();

	/* Some fields get placed in the preferences if there aren't
	 * already preferences.  This should be called from every
	 * SaveChanges() implementation.
	 */
	void				SetHiddenPrefs();
	
	bool				HasChanges() const;
	void				SetHasChanges(bool hasChanges);
	status_t			AddPage(BView* page);

	BRect				CurrentPageFrame() const;
	status_t			SetFirstPage();
	
	/* Raise an alert with the error and answer false.
	 */
	bool				ReportError(const char* error = "Unknown error");
	/* A convenience for the text controls.  All text controls should
	 * have a modification message, but sometimes that message does not
	 * get delivered as expected, causing the window to believe it has
	 * changes when it doesn't.  This method detaches the modification
	 * message before setting the control.
	 */
	void				SetTextControl(BTextControl* ctrl, const char* text, uint32 what);
	
private:
	typedef BWindow 	inherited;

	BView*				mBg;
	BColumnListView*	mListView;
	SeqSplitterView*	mSplitter;
	BView*				mPageView;
	BView*				mBlankPageView;

	bool				mHasChanges;
	bool				mForceClose;
	
	void				AddViews(BRect frame);
	void				BuildViews(BColumnListView* list);
};

/******************************************************************
 * SEQ-DUMB-TEST-VIEW
 * The text view doesn't resize its text rect when resizing.  This
 * class fixes that.  Dumb, dumb text view.
 ******************************************************************/
class SeqDumbTextView : public BTextView
{
public:
	SeqDumbTextView(BRect frame, const char* name, BRect textRect,
					uint32 resizeMask, uint32 flags = B_WILL_DRAW | B_PULSE_NEEDED);
	virtual ~SeqDumbTextView();
	
	virtual	void	DeleteText(int32 fromOffset, int32 toOffset);
	virtual	void	FrameResized(float width, float height);
	virtual	void	InsertText(	const char *inText, int32 inLength, 
								int32 inOffset, const text_run_array *inRuns);

	void			SetModificationMessage(BMessage* msg);

private:
	typedef BTextView	inherited;
	BMessage*			mModificationMessage;
};

#endif
