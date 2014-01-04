/* SeqManageRosterWindows.h
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
 * 2001.03.10		hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef SEQUITUR_SEQMANAGEROSTERWINDOWS_H
#define SEQUITUR_SEQMANAGEROSTERWINDOWS_H

#include <experimental/ColumnListView.h>
#include <interface/Window.h>
#include "AmPublic/AmToolRef.h"
#include "Sequitur/SeqWindowStateI.h"
class AmFileRoster;
class _EntryRow;

/*************************************************************************
 * SEQ-MANAGE-ROSTER-WINDOW
 * This is an abstract window for displaying a list of all the entries
 * in a given roster.  It allows basic behaviour like adding, removing,
 * changing, and duplicating the entries.
 *************************************************************************/
class SeqManageRosterWindow : public BWindow,
							  public SeqWindowStateI
{
public:
	SeqManageRosterWindow(	BRect frame,
							const BMessage* config = NULL); 
	virtual ~SeqManageRosterWindow();

	virtual	void			MenusBeginning();
	virtual void			MessageReceived(BMessage* msg);
	virtual	bool			QuitRequested();

	/*---------------------------------------------------------
	 * SEQ-WINDOW-STATE-I INTERFACE
	 *---------------------------------------------------------*/
	virtual bool			IsSignificant() const;
	virtual status_t		GetConfiguration(BMessage* config);
	status_t				SetConfiguration(const BMessage* config);

	virtual void			InitiateDrag(	BPoint where, bool wasSelected,
											BColumnListView* dragView);

protected:
	virtual void			BuildList();
	virtual _EntryRow*		NewEntryRow(uint32 index, file_entry_id entryId) const = 0;
	virtual float			RowHeight() const;
	virtual float			LatchWidth() const;
	virtual status_t		SetShowEditWindowMsg(BMessage& msg, const BString& key) const = 0;
	virtual AmFileRoster*	Roster() const = 0;
	virtual uint32			ConfigWhat() const = 0;
	virtual uint32			WindowSettingsIndex() const = 0;
	virtual const char*		EntryMenuType() const = 0;
	
	void					Initialize();
	status_t				GetSelectionInfo(	BString& key, BString& filePath,
												bool* readOnly = NULL) const;
	
	/* Always adds string columns.
	 */
	status_t				AddColumn(	const char* name, uint32 index, float width = 110,
										float minWidth = 20, float maxWidth = 350,
										uint32 truncate = B_TRUNCATE_MIDDLE);
	
private:
	typedef BWindow			inherited;

	void					ShowEditWindow(const BString& key, const BString& filePath);
	void					ToggleColumn(const char* name);
	void					AddMainMenu(BRect frame);
	void					AddViews(BRect frame);
};

/*************************************************************************
 * SEQ-MANAGE-DEVICES-WINDOW
 *************************************************************************/
class SeqManageDevicesWindow : public SeqManageRosterWindow
{
public:
	SeqManageDevicesWindow(	BRect frame,
							const BMessage* config = NULL); 

protected:
	virtual _EntryRow*		NewEntryRow(uint32 index, file_entry_id entryId) const;
	virtual float			RowHeight() const;
	virtual float			LatchWidth() const;
	virtual status_t		SetShowEditWindowMsg(BMessage& msg, const BString& key) const;
	virtual AmFileRoster*	Roster() const;
	virtual uint32			ConfigWhat() const;
	virtual uint32			WindowSettingsIndex() const;
	virtual const char*		EntryMenuType() const;
	
private:
	typedef SeqManageRosterWindow inherited;
};

/*************************************************************************
 * SEQ-MANAGE-MOTIONS-WINDOW
 *************************************************************************/
class SeqManageMotionsWindow : public SeqManageRosterWindow
{
public:
	SeqManageMotionsWindow(	BRect frame,
							const BMessage* config = NULL); 

protected:
	virtual _EntryRow*		NewEntryRow(uint32 index, file_entry_id entryId) const;
	virtual status_t		SetShowEditWindowMsg(BMessage& msg, const BString& key) const;
	virtual AmFileRoster*	Roster() const;
	virtual uint32			ConfigWhat() const;
	virtual uint32			WindowSettingsIndex() const;
	virtual const char*		EntryMenuType() const;
	
private:
	typedef SeqManageRosterWindow inherited;
};

/*************************************************************************
 * SEQ-MANAGE-TOOLS-WINDOW
 *************************************************************************/
class SeqManageToolsWindow : public SeqManageRosterWindow
{
public:
	SeqManageToolsWindow(	BRect frame,
							const BMessage* config = NULL); 

	virtual void			MessageReceived(BMessage* msg);
	virtual void			InitiateDrag(	BPoint where, bool wasSelected,
											BColumnListView* dragView);

protected:
	virtual _EntryRow*		NewEntryRow(uint32 index, file_entry_id entryId) const;
	virtual float			RowHeight() const;
	virtual float			LatchWidth() const;
	virtual status_t		SetShowEditWindowMsg(BMessage& msg, const BString& key) const;
	virtual AmFileRoster*	Roster() const;
	virtual uint32			ConfigWhat() const;
	virtual uint32			WindowSettingsIndex() const;
	virtual const char*		EntryMenuType() const;

private:
	typedef SeqManageRosterWindow inherited;
};

#endif
