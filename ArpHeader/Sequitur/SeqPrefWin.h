/* SeqPrefWin.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 08.30.00		hackborn
 * Created this file
 */
#ifndef SEQUITUR_SEQPREFWIN_H
#define SEQUITUR_SEQPREFWIN_H

#include <interface/CheckBox.h>
#include <interface/Window.h>
#include "ArpViews/ArpIntControl.h"
#include "Sequitur/SeqWindowStateI.h"
class BFilePanel;
class SeqFactoryListView;
class _OwqList;

/***************************************************************************
 * SEQ-PREF-WIN
 * Allow users to change system-wide preferences.
 ****************************************************************************/
class SeqPrefWin : public BWindow,
				   public SeqWindowStateI
{
public:
	SeqPrefWin(BRect frame, const BMessage* config = 0); 
	~SeqPrefWin();
	
	virtual void		MessageReceived(BMessage* msg);
	virtual	bool		QuitRequested();

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool		IsSignificant() const;
	virtual status_t	GetConfiguration(BMessage* config);
	status_t			SetConfiguration(const BMessage* config);

private:
	typedef BWindow		inherited;
	BMessage			mPreferences;
	mutable _OwqList*	mOwqTable;
	BFilePanel*			mFilePanel;
	ArpIntControl*		mUndoLevelCtrl;
	BCheckBox*			mTrackWinFollowCtrl;
	BCheckBox*			mTrackWinPlayToEndCtrl;
	ArpIntControl*		mTrackHeightCtrl;
	ArpIntControl*		mLabelHeightCtrl;
	uint32				mRefreshWindows;
	BView*				mFactoryView;
	SeqFactoryListView*	mFactoryList;
	BView*				mFactoryInspector;
	
	status_t			SetStringPref(const char* name, const char* a_string, int32 n = 0);
	status_t			SetInt32Pref(const char* name, int32 an_int32, int32 n = 0);
	status_t			SetBoolPref(const char* name, bool a_boolean, int32 n = 0);
	status_t			SetRefPref(const char* name, const entry_ref* ref, int32 n = 0);

	void				SetOpenNewFromFileRef(const entry_ref* ref);
	void				RefreshWindows();

	void				FactoryRowSelected();
	
	void				AddViews(const BMessage& prefs);
	BView*				NewFileView(BRect bounds, const BMessage& prefs) const;
	BView*				NewEditView(BRect bounds, const BMessage& prefs);
	BView*				NewTrackView(BRect bounds, const BMessage& prefs);
	BView*				NewFactoriesView(BRect bounds, const BMessage& prefs);
};

#endif
