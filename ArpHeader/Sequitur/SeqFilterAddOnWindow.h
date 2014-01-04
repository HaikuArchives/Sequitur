/* SeqFilterAddOnWindow
 * Copyright (c)1998 by Angry Red Planet.
 * All rights reserved.
 *
 * Author: Eric Hackborn
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Angry Red Planet,
 * at <hackborn@angryredplanet.com> or <hackbod@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 12.06.98		hackborn
 * Created this file
 */

#ifndef SEQUITUR_SEQFILTERADDONWINDOW_H
#define SEQUITUR_SEQFILTERADDONWINDOW_H

#include <interface/Window.h>
#include <interface/Bitmap.h>
#include <support/List.h>
#include "Sequitur/SeqWindowStateI.h"

class AmFilterRoster;

/*****************************************************************************
 * SEQ-FILTER-ADDON-WINDOW
 * This window displays a list of all filters it has been supplied.  It allows
 * basic drag-drop of each of the addons.
 *****************************************************************************/
class SeqFilterAddOnWindow : public BWindow,
							 public SeqWindowStateI
{
public:
	SeqFilterAddOnWindow(	BRect frame,
							const BMessage* config = NULL,
							AmFilterRoster* roster = NULL);

	virtual thread_id	Run();
	virtual	void		MenusBeginning();
	virtual	void		DispatchMessage(BMessage *message, BHandler *handler);
	virtual void		MessageReceived(BMessage *message);
	virtual	bool		QuitRequested();

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool		IsSignificant() const;
	virtual status_t	GetConfiguration(BMessage* config);
	status_t			SetConfiguration(const BMessage* config);

private:
	typedef BWindow		inherited;
	// The pixel height of one row in the window. 
	float				mRowHeight;
	AmFilterRoster*		mRoster;
	
	void				UpdateList();
	void				UpdateHandleList();
	void				UpdateMultiList();

	status_t			GetSelectionInfo(BString& key, bool* readOnly = NULL, BString* outPath = NULL) const;

	void				ShowEditMultiWin(const BString& key, const BString& path);
		
	void				ToggleColumn(const char* name);

	void				AddMainMenu();
	void				AddViews();
};

#endif
