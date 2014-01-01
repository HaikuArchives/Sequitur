/* SeqManageRosterWindows.cpp
 */
#include <set>
#include <be/experimental/ColorTools.h>
#include <be/experimental/ColumnTypes.h>
#include <be/InterfaceKit.h>
#include <be/support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmDevice.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmTool.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqColoredColumn.h"
#include "Sequitur/SeqManageRosterWindows.h"

static const uint32		NEW_ENTRY_MSG			= 'ineE';
static const uint32		EDIT_ENTRY_MSG			= 'iedE';
static const uint32		DUPLICATE_ENTRY_MSG		= 'iduE';
static const uint32		DELETE_ENTRY_MSG		= 'ideE';
static const uint32		COLUMN_MSG				= 'icol';

static const char*		NEW_STR					= "New";
static const char*		EDIT_STR				= "Edit";
static const char*		DUPLICATE_STR			= "Duplicate";
static const char*		DELETE_STR				= "Delete";
static const char*		COLUMN_NAME_STR			= "column_name";
static const char*		NAME_STR				= "Name";
static const char*		KEY_STR					= "Key";
static const char*		AUTHOR_STR				= "Author";
static const char*		EMAIL_STR				= "Email";
static const char*		PATH_STR				= "Path";
static const char*		TABLE_STR				= "table";
static const char*		DESCRIPTION_STR			= "Description";

static const uint32		NAME_COL_INDEX			= 0;
static const uint32		DESCRIPTION_INDEX		= 1;
static const uint32		KEY_COL_INDEX			= 2;
static const uint32		AUTHOR_COL_INDEX		= 3;
static const uint32		EMAIL_COL_INDEX			= 4;
static const uint32		PATH_COL_INDEX			= 5;
static const uint32		COL_2_INDEX				= 6;

#if 0
static const uint32		NAME_COL_INDEX			= 0;
static const uint32		KEY_COL_INDEX			= 1;
static const uint32		AUTHOR_COL_INDEX		= 2;
static const uint32		EMAIL_COL_INDEX			= 3;
static const uint32		PATH_COL_INDEX			= 4;
static const uint32		COL_1_INDEX				= 5;
static const uint32		COL_2_INDEX				= 6;
#endif

static const int32		ENTRY_MENU_INDEX		= 0;
static const int32		ATTRIBUTES_MENU_INDEX	= 1;

static void read_only_warning(const char* entryType, const BString& rhythmName)
{
	BString			warning(entryType);
	warning << " \'" << rhythmName << "\' is read only";
	BAlert*	alert = new BAlert(	"Warning", warning.String(),
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
}

/********************************************************
 * _ENTRY-LIST-VIEW
 ********************************************************/
class _EntryListView : public BColumnListView
{
public:
	_EntryListView(	BRect rect, const char *name,
					SeqManageRosterWindow* window);

#if B_BEOS_VERSION_DANO
	virtual bool InitiateDrag(BPoint where, bool wasSelected);
#else
	virtual void InitiateDrag(BPoint where, bool wasSelected);
#endif
	virtual void DrawLatch(BView *view, BRect rect, LatchType pos, BRow *row);
	virtual void ItemInvoked();
	
private:
	typedef BColumnListView	inherited;
	SeqManageRosterWindow* mWindow;
};

/********************************************************
 * _ENTRY-ROW
 ********************************************************/
class _EntryRow : public BRow
{
public:
	_EntryRow(	float height,
				const BString& label,
				const BString& key,
				bool readOnly,
				bool isValid,
				const BString& description,
				const BString& author,
				const BString& email,
				file_entry_id entryId,
				BBitmap* icon = NULL,
				const char* filePath = NULL,
				const char* col2 = NULL);
	virtual ~_EntryRow();
		
	virtual bool		HasLatch() const		{ return false; }
	BString				Label() const;
	BString				Key() const;
	BString				FilePath() const;
	file_entry_id		EntryId() const;
	bool				IsReadOnly() const;
	bool				IsValid() const;
	const BBitmap*		Icon() const;

protected:
	BString				mLabel;
	BString				mKey;
	bool				mReadOnly;
	bool				mIsValid;
	BString				mDescription;
	BString				mAuthor;
	BString				mEmail;
	file_entry_id		mEntryId;
	BBitmap*			mIcon;
	BString				mFilePath;
	BString				mCol2;
};

/*************************************************************************
 * SEQ-MANAGE-ROSTER-WINDOW
 *************************************************************************/
SeqManageRosterWindow::SeqManageRosterWindow(	BRect frame,
												const BMessage* config)
		: inherited(frame, "Manage",
					B_DOCUMENT_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS)
{
}

SeqManageRosterWindow::~SeqManageRosterWindow()
{
}

static BColumn* column_named(const char* name, BColumnListView* fromTable)
{
	if (!fromTable) return NULL;
	BColumn*		col;
	for (uint32 k = 0; (col = fromTable->ColumnAt(k)); k++) {
		BString		n;
		col->GetColumnName(&n);
		if (strcmp( n.String(), name ) == 0) return col;
	}
	return NULL;
}

void SeqManageRosterWindow::MenusBeginning()
{
	inherited::MenusBeginning();
	BMenuBar*			bar = KeyMenuBar();
	if (!bar) return;
	BColumnListView*	table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!table) return;
	BMenu*				menu;
	BMenuItem*			item;

	// Entry menu
	bool		canEdit = false, canDuplicate = false;
	BString		key, filePath;
	bool		readOnly;
	if (GetSelectionInfo(key, filePath, &readOnly) == B_OK) {
		canEdit = !readOnly;
		canDuplicate = true;
	}
	if ( (menu = bar->SubmenuAt(ENTRY_MENU_INDEX)) != NULL) {
		if ( (item = menu->FindItem(EDIT_ENTRY_MSG)) != NULL) item->SetEnabled(canEdit);
		if ( (item = menu->FindItem(DUPLICATE_ENTRY_MSG)) != NULL) item->SetEnabled(canDuplicate);
		if ( (item = menu->FindItem(DELETE_ENTRY_MSG)) != NULL) item->SetEnabled(canEdit);
	}

	// Attributes menu
	if ( (menu = bar->SubmenuAt(ATTRIBUTES_MENU_INDEX)) != NULL) {
		for (int32 k = 0; (item = menu->ItemAt(k)) != NULL; k++) {
			const char*		n;
			if (item->Message() && item->Message()->FindString(COLUMN_NAME_STR, &n) == B_OK) {
				BColumn*	col = column_named(n, table);
				if (col && col->IsVisible() ) {
					if (!item->IsMarked() ) item->SetMarked(true);
				} else {
					if (item->IsMarked() ) item->SetMarked(false);
				}
			}
		}
	}
}

void SeqManageRosterWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case NEW_ENTRY_MSG: {
			BString		key;
			ShowEditWindow(key, BString() );
		} break;
		case EDIT_ENTRY_MSG: {
			BString		key, filePath;
			bool		readOnly;
			if (GetSelectionInfo(key, filePath, &readOnly) == B_OK) {
				if (readOnly) read_only_warning(EntryMenuType(), key);
				else ShowEditWindow(key, filePath);
			}
		} break;
		case DUPLICATE_ENTRY_MSG: {
			BString			key, filePath;
			AmFileRoster*	roster = Roster();
			if (roster && GetSelectionInfo(key, filePath) == B_OK) {
				roster->DuplicateEntry(key, filePath.String() );
			}
		} break;
		case DELETE_ENTRY_MSG: {
			BString			key, filePath;
			AmFileRoster*	roster = Roster();
			if (roster && GetSelectionInfo(key, filePath) == B_OK) {
				roster->DeleteEntry(key);
			}
		} break;
		case AM_FILE_ROSTER_CHANGED: {
			BuildList();
		} break;
		case COLUMN_MSG: {
			const char*		n;
			if (msg->FindString(COLUMN_NAME_STR, &n) == B_OK)
				ToggleColumn(n);
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqManageRosterWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(WindowSettingsIndex(), config);
	}
	AmFileRoster*	roster = Roster();
	if (roster) roster->RemoveObserver(this);

	return true;
}

bool SeqManageRosterWindow::IsSignificant() const
{
	return false;
}

status_t SeqManageRosterWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT(config);
	config->what = ConfigWhat();
	status_t	err = GetDimensions(config, this);
	if (err != B_OK) return err;
	/* Add the columns
	 */
	BColumnListView* table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (table) {
		BColumn*	col;
		for( int32 k = 0; (col = table->ColumnAt(k)); k++ ) {
			BMessage	colMsg;
			BString		colName;
			col->GetColumnName(&colName);
			if( colMsg.AddString("name", colName.String() ) == B_OK
					&& colMsg.AddFloat("width", col->Width() ) == B_OK
					&& colMsg.AddBool("visible", col->IsVisible() ) == B_OK ) {
				config->AddMessage("column", &colMsg);
			}
		}
	}
	return B_OK;
}

static BColumn* col_named(const char* name, BColumnListView* table)
{
	BColumn*	col;
	for (int32 k = 0; (col = table->ColumnAt(k)); k++) {
		BString		colName;
		col->GetColumnName( &colName );
		if (strcmp( name, colName.String() ) == 0) return col;
	}
	return NULL;
}

status_t SeqManageRosterWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	status_t	err = SetDimensions(config, this);
	if (err != B_OK) return err;
	/* Set the columns
	 */
	BColumnListView* table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (table) {
		BMessage	colMsg;
		for (int32 k = 0; config->FindMessage("column", k, &colMsg) == B_OK; k++) {
			const char*		colName;
			float			colW;
			bool			colVis;
			if (colMsg.FindString("name", &colName) == B_OK
					&& colMsg.FindFloat("width", &colW) == B_OK
					&& colMsg.FindBool("visible", &colVis) == B_OK) {
				BColumn*	col = col_named(colName, table);
				if (col) {
					col->SetWidth(colW);
					col->SetVisible(colVis);
				}
			}
		}
	}
	return B_OK;
}

void SeqManageRosterWindow::InitiateDrag(	BPoint where, bool wasSelected,
											BColumnListView* dragView)
{
}

struct entry_row
{
	entry_row()						: row(NULL), id(0)			{ }
	entry_row(_EntryRow* inRow, file_entry_id inId)
									: row(inRow), id(inId)		{ }
	entry_row(file_entry_id inId)	: row(NULL), id(inId)		{ }
	entry_row(const entry_row& r)	: row(r.row), id(r.id)		{ }
	
	_EntryRow*		row;
	file_entry_id	id;
	
	#define COMPARE(OP)										\
		bool operator OP(const entry_row& o) const {		\
			if (id && o.id) return id OP o.id;				\
			return row OP o.row;							\
		}
	
	COMPARE(==);
	COMPARE(!=);
	COMPARE(<=);
	COMPARE(<);
	COMPARE(>);
	COMPARE(>=);
	#undef COMPARE
};

void SeqManageRosterWindow::BuildList()
{
	BColumnListView*	listView = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!listView) return;
	AmFileRoster*		roster = Roster();
	if (!roster) return;
	set<entry_row>		rows;
	
	for (int32 k = 0; k < listView->CountRows(); k++) {
		_EntryRow*		row = dynamic_cast<_EntryRow*>(listView->RowAt(k));
		if (row) rows.insert(entry_row(row, row->EntryId() ));
	}

	for (uint32 k = 0; k < roster->CountEntries(); k++) {
		file_entry_id	id = roster->EntryIdAt(k);
		if (id) {
			set<entry_row>::iterator it = rows.find(entry_row(id));
			if (it == rows.end()) {
				_EntryRow*	row = NewEntryRow(k, id);
				if (row) listView->AddRow(row);
			} else rows.erase(it);
		}
	}
	
	for (set<entry_row>::iterator it = rows.begin(); it != rows.end(); it++) {
		if (it->row) {
			listView->RemoveRow(it->row);
			delete it->row;
		}
	}
}

float SeqManageRosterWindow::RowHeight() const
{
	return 16;
}

float SeqManageRosterWindow::LatchWidth() const
{
	return 0;
}

status_t SeqManageRosterWindow::GetSelectionInfo(	BString& key, BString& filePath,
													bool* readOnly) const
{
	BColumnListView* listView = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!listView) return B_ERROR;
	_EntryRow*		row = dynamic_cast<_EntryRow*>( listView->CurrentSelection() );
	if (!row) return B_ERROR;
	key = row->Key();
	filePath = row->FilePath();	
	if (readOnly) *readOnly = row->IsReadOnly();
	return B_OK;
}

status_t SeqManageRosterWindow::AddColumn(	const char* name, uint32 index, float width,
											float minWidth, float maxWidth, uint32 truncate)
{
	ArpASSERT(name);
	BColumnListView*	table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!table) return B_ERROR;
	BMenuBar*			bar = KeyMenuBar();
	if (!bar) return B_ERROR;
	BMenu*		menu = bar->SubmenuAt(ATTRIBUTES_MENU_INDEX);
	if (!menu) return B_ERROR;

	BColumn*		col = new SeqColoredColumn(name, width, minWidth, maxWidth, truncate);
	if (!col) return B_NO_MEMORY;
	BMessage*		msg = new BMessage(COLUMN_MSG);
	if (!msg) {
		delete col;
		return B_NO_MEMORY;
	}
	msg->AddString(COLUMN_NAME_STR, name);
	BMenuItem*		item = new BMenuItem(name, msg);
	if (!item) {
		delete col;
		delete msg;
		return B_NO_MEMORY;
	}
	
	table->AddColumn(col, index);
	menu->AddItem(item);
	return B_OK;	
}

void SeqManageRosterWindow::Initialize()
{
	BRect		b = Bounds();
	BRect		f = b;
	f.bottom = f.top + Prefs().Size(MAINMENU_Y);
	AddMainMenu(f);
	f.top = f.bottom + 1;
	f.bottom = b.bottom;	
	AddViews(f);

	BuildList();

	AmFileRoster*	roster = Roster();
	if (roster) {
		roster->AddObserver(this);
		BString		name("Manage ");
		name << roster->Name();
		SetTitle( name.String() );
	}
}

void SeqManageRosterWindow::ShowEditWindow(const BString& key, const BString& filePath)
{
	BMessage		msg;
	SetShowEditWindowMsg(msg, key);
	if (filePath.Length() > 0) msg.AddString("path", filePath);
	be_app->PostMessage(&msg);
}

void SeqManageRosterWindow::ToggleColumn(const char* name)
{
	BColumnListView*	table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!table) return;
	BColumn*			col = column_named(name, table);
	if (!col) return;
	if (col->IsVisible() ) col->SetVisible(false);
	else col->SetVisible(true);
}

void SeqManageRosterWindow::AddMainMenu(BRect frame)
{
	BMenuBar*	menuBar;
	BMenu*		menu;
	BMenuItem*	item;
	menuBar = new BMenuBar(	frame, NULL,
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_ITEMS_IN_ROW, FALSE);
	/* Entry Menu
	 */
	menu = new BMenu(EntryMenuType(), B_ITEMS_IN_COLUMN);
	add_menu_item(menu,	NEW_STR,		NEW_ENTRY_MSG,			'N');
	add_menu_item(menu,	EDIT_STR,		EDIT_ENTRY_MSG,			'E');
	add_menu_item(menu,	DUPLICATE_STR,	DUPLICATE_ENTRY_MSG,	'D');
	add_menu_item(menu,	DELETE_STR,		DELETE_ENTRY_MSG,		'T');
	item = new BMenuItem(menu);
	menuBar->AddItem(item, ENTRY_MENU_INDEX);

	/* Attributes Menu
	 */
	menu = new BMenu("Attributes", B_ITEMS_IN_COLUMN);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, ATTRIBUTES_MENU_INDEX);

	AddChild(menuBar);
	SetKeyMenuBar(menuBar);
}

void SeqManageRosterWindow::AddViews(BRect frame)
{	
	frame.right++;
	frame.bottom++;
	BColumnListView*	listView;
	listView = new _EntryListView(frame, TABLE_STR, this);
	if (!listView) return;
	if (LatchWidth() > 0) listView->SetLatchWidth(LatchWidth() );
	AddChild(listView);
	AddColumn(NAME_STR, NAME_COL_INDEX);
	AddColumn(DESCRIPTION_STR, DESCRIPTION_INDEX);
	AddColumn(KEY_STR, KEY_COL_INDEX);
	AddColumn(AUTHOR_STR, AUTHOR_COL_INDEX);
	AddColumn(EMAIL_STR, EMAIL_COL_INDEX);
	AddColumn(PATH_STR, PATH_COL_INDEX, 200, 20, 550);
//	listView->SetSortColumn(listView->ColumnAt(1), false, true);
	listView->SetSortColumn(listView->ColumnAt(NAME_COL_INDEX), true, true);
	listView->SetColumnVisible(KEY_COL_INDEX, false);
	listView->SetColumnVisible(AUTHOR_COL_INDEX, false);
	listView->SetColumnVisible(EMAIL_COL_INDEX, false);
	listView->SetColumnVisible(PATH_COL_INDEX, false);
	listView->MakeFocus();
}

// #pragma mark -

/********************************************************
 * _DEVICE-ROW
 ********************************************************/
static BString device_label_for(const BString& mfg, const BString& name)
{
	BString		n(mfg);
	if (n.Length() > 0 && name.Length() > 0) n << " ";
	n << name;
	if (n.Length() < 1) n = "Unknown device";
	return n;
}

class _DeviceRow : public _EntryRow
{
public:
	_DeviceRow(	float height, const BString& mfg, const BString& name,
				const BString& key,
				bool readOnly,
				bool isValid,
				const BString& descr,
				const BString& author,
				const BString& email,
				file_entry_id entryId,
				BBitmap* icon = NULL,
				const char* filePath = NULL)
			: _EntryRow(height, device_label_for(mfg, name), key, readOnly, isValid,
						descr, author, email, entryId, icon, filePath)
	{
	}
};

/*************************************************************************
 * SEQ-MANAGE-DEVICES-WINDOW
 *************************************************************************/
SeqManageDevicesWindow::SeqManageDevicesWindow(	BRect frame,
												const BMessage* config)
		: inherited(frame, config)
{
	Initialize();
	if (config) SetConfiguration(config);
}

_EntryRow* SeqManageDevicesWindow::NewEntryRow(uint32 index, file_entry_id entryId) const
{
	AmDeviceRoster*	roster = AmDeviceRoster::Default();
	if (!roster) return NULL;
	BString			mfg, name, key, descr, author, email, filePath;
	bool			readOnly, isValid;
	BBitmap*		icon;
	if (roster->GetDeviceInfo(	index, mfg, name, key, &readOnly, &isValid, &descr, &author,
								&email, &icon, &filePath) != B_OK) return NULL;
	if (icon) {
		BBitmap*		realIcon = ArpMakeFilterBitmap(icon, BPoint(20, 20), -1);
		if (realIcon && realIcon != icon) {
			delete icon;
			icon = realIcon;
		}
	}
	return new _DeviceRow(	24, mfg, name, key, readOnly, isValid, descr, author, email,
							entryId, icon, filePath.String() );
}

float SeqManageDevicesWindow::RowHeight() const
{
	return 21;
}

float SeqManageDevicesWindow::LatchWidth() const
{
	return 21;
}

status_t SeqManageDevicesWindow::SetShowEditWindowMsg(BMessage& msg, const BString& key) const
{
	msg.what = SHOW_EDIT_DEVICE_MSG;
	return msg.AddString("device_unique_name", key.String() );
}

AmFileRoster* SeqManageDevicesWindow::Roster() const
{
	return AmDeviceRoster::Default();
}

uint32 SeqManageDevicesWindow::ConfigWhat() const
{
	return MANAGE_DEVICES_WINDOW_SETTING_MSG;
}

uint32 SeqManageDevicesWindow::WindowSettingsIndex() const
{
	return SeqApplication::MANAGE_DEVICES_WIN_INDEX;
}

const char* SeqManageDevicesWindow::EntryMenuType() const
{
	return "Device";
}

// #pragma mark -

/*************************************************************************
 * SEQ-MANAGE-MOTIONS-WINDOW
 *************************************************************************/
SeqManageMotionsWindow::SeqManageMotionsWindow(	BRect frame,
												const BMessage* config)
		: inherited(frame, config)
{
	Initialize();
	if (config) SetConfiguration(config);
}

_EntryRow* SeqManageMotionsWindow::NewEntryRow(uint32 index, file_entry_id entryId) const
{
	AmMotionRoster*	roster = AmMotionRoster::Default();
	if (!roster) return NULL;
	BString			label, key, descr, author, email, path;
	bool			readOnly, isValid;
	if (roster->GetMotionInfo(	index, label, key, &readOnly, &isValid,
								&descr, &author, &email, &path) != B_OK) return NULL;
	return new _EntryRow(16, label, key, readOnly, isValid, descr, author, email, entryId, NULL, path.String() );
}

status_t SeqManageMotionsWindow::SetShowEditWindowMsg(BMessage& msg, const BString& key) const
{
	msg.what = SHOW_EDIT_MOTION_MSG;
	return msg.AddString("motion_unique_name", key.String() );
}

AmFileRoster* SeqManageMotionsWindow::Roster() const
{
	return AmMotionRoster::Default();
}

uint32 SeqManageMotionsWindow::ConfigWhat() const
{
	return MANAGE_MOTIONS_WINDOW_SETTING_MSG;
}

uint32 SeqManageMotionsWindow::WindowSettingsIndex() const
{
	return SeqApplication::MANAGE_MOTIONS_WIN_INDEX;
}

const char* SeqManageMotionsWindow::EntryMenuType() const
{
	return "Motion";
}

// #pragma mark -

/*************************************************************************
 * SEQ-MANAGE-TOOLS-WINDOW
 *************************************************************************/
SeqManageToolsWindow::SeqManageToolsWindow(	BRect frame,
											const BMessage* config)
		: inherited(frame, config)
{
	Initialize();
//	AddColumn(DESCRIPTION_STR, COL_1_INDEX);
	if (config) SetConfiguration(config);
}

void SeqManageToolsWindow::MessageReceived(BMessage* msg)
{
	/* If this is just a message to set the button for a
	 * tool, ignore it.
	 */
	if (msg->what == AM_FILE_ROSTER_CHANGED) {
		if (msg->HasInt32("button") ) return;
	}
	inherited::MessageReceived(msg);
}

void SeqManageToolsWindow::InitiateDrag(BPoint where, bool wasSelected,
										BColumnListView* dragView)
{
	ArpASSERT(dragView);
	const _EntryRow*	row = dynamic_cast<_EntryRow*>( dragView->RowAt(where) );
	if (!row) return;
	if (row->Key().Length() < 1) return;
	if (!row->IsValid() ) return;
	
	BBitmap*		bm = NULL;
	AmToolRoster*	roster = AmToolRoster::Default();
	if (roster) {
		AmToolRef		toolRef = roster->FindTool( row->Key().String() );
		// READ TOOL BLOCK
		const AmTool*	tool = toolRef.ReadLock();
		if (tool && tool->Icon() ) bm = new BBitmap( tool->Icon() );
		toolRef.ReadUnlock(tool);
		// END READ TOOL BLOCK
	}
	BMessage		msg(AM_DRAG_TOOL_MSG);
	msg.AddString("tool_name", row->Key().String() );
	if (bm) {
		BPoint		pt(bm->Bounds().Width() / 2, bm->Bounds().Height() / 2);
		dragView->DragMessage(&msg, bm, B_OP_ALPHA, pt, this);
	} else {
		dragView->DragMessage(&msg, BRect(0, 0, 23, 23), this);
	}
}

_EntryRow* SeqManageToolsWindow::NewEntryRow(uint32 index, file_entry_id entryId) const
{
	AmToolRoster*	roster = AmToolRoster::Default();
	if (!roster) return NULL;
	BString			label, key, author, email, shortDesc, filePath;
	bool			readOnly, isValid;
	BBitmap*		icon;
	if (roster->GetToolInfo(index, label, key, &readOnly, &isValid,
							&author, &email, &shortDesc, NULL, &icon,
							&filePath) != B_OK) return NULL;
	return new _EntryRow(	RowHeight(), label, key, readOnly, isValid, shortDesc, author,
							email, entryId, icon, filePath.String() );
}

float SeqManageToolsWindow::RowHeight() const
{
	return 25;
}

float SeqManageToolsWindow::LatchWidth() const
{
	return 25;
}

status_t SeqManageToolsWindow::SetShowEditWindowMsg(BMessage& msg, const BString& key) const
{
	msg.what = SHOW_EDIT_TOOL_MSG;
	return msg.AddString("tool_key", key.String() );
}

AmFileRoster* SeqManageToolsWindow::Roster() const
{
	return AmToolRoster::Default();
}

uint32 SeqManageToolsWindow::ConfigWhat() const
{
	return MANAGE_TOOLS_WINDOW_SETTING_MSG;
}

uint32 SeqManageToolsWindow::WindowSettingsIndex() const
{
	return SeqApplication::MANAGE_TOOLS_WIN_INDEX;
}

const char* SeqManageToolsWindow::EntryMenuType() const
{
	return "Tool";
}

// #pragma mark -

/********************************************************
 * _ENTRY-LIST-VIEW
 ********************************************************/
_EntryListView::_EntryListView(	BRect rect, const char *name,
								SeqManageRosterWindow* window)
		: inherited(rect, name, B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER),
		  mWindow(window)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
}

void _EntryListView::DrawLatch(BView *view, BRect rect, LatchType pos, BRow *row)
{
	inherited::DrawLatch(view, rect, pos, row);
	_EntryRow*	r = dynamic_cast<_EntryRow*>(row);
	if (r && r->Icon() ) {
		drawing_mode	mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->DrawBitmap(r->Icon(), BPoint(rect.left + 2, rect.top + 1) );
		view->SetDrawingMode(mode);
	}
}

#if B_BEOS_VERSION_DANO
bool
#else
void
#endif
_EntryListView::InitiateDrag(BPoint where, bool wasSelected)
{
	inherited::InitiateDrag(where, wasSelected);
	SeqManageRosterWindow*	win = dynamic_cast<SeqManageRosterWindow*>( Window() );
	if (win) win->InitiateDrag(where, wasSelected, this);
#if B_BEOS_VERSION_DANO
	return true;
#endif
}

void _EntryListView::ItemInvoked()
{
	inherited::ItemInvoked();
	SeqManageRosterWindow*	win = dynamic_cast<SeqManageRosterWindow*>( Window() );
	if (win) win->PostMessage(EDIT_ENTRY_MSG);
}

// #pragma mark -

/********************************************************
 * _ENTRY-ROW
 ********************************************************/
_EntryRow::_EntryRow(	float height,
						const BString& label,
						const BString& key,
						bool readOnly,
						bool isValid,
						const BString& description,
						const BString& author,
						const BString& email,
						file_entry_id entryId,
						BBitmap* icon,
						const char* filePath,
						const char* col2)
		: BRow(height), mIsValid(isValid), mIcon(icon)
{
	mReadOnly = readOnly;
	mLabel = label;
	SetField(new SeqColoredField(mLabel.String(), readOnly, isValid), NAME_COL_INDEX);

	mDescription = description;
	SetField(new SeqColoredField(mDescription.String(), readOnly, isValid), DESCRIPTION_INDEX);

	mKey = key;
	SetField(new SeqColoredField(mKey.String(), readOnly, isValid), KEY_COL_INDEX);
	mAuthor = author;
	SetField(new SeqColoredField(mAuthor.String(), readOnly, isValid), AUTHOR_COL_INDEX);
	mEmail = email;
	SetField(new SeqColoredField(mEmail.String(), readOnly, isValid), EMAIL_COL_INDEX);
	mEntryId = entryId;
	if (filePath) {
		mFilePath = filePath;
		SetField(new SeqColoredField(mFilePath.String(), readOnly, isValid), PATH_COL_INDEX);
	}
	if (col2) {
		mCol2 = col2;
		SetField(new SeqColoredField(mCol2.String(), readOnly, isValid), COL_2_INDEX);
	}
}

_EntryRow::~_EntryRow()
{
	delete mIcon;
}

BString _EntryRow::Label() const
{
	return mLabel;
}

BString _EntryRow::Key() const
{
	return mKey;
}

BString _EntryRow::FilePath() const
{
	return mFilePath;
}

file_entry_id _EntryRow::EntryId() const
{
	return mEntryId;
}

bool _EntryRow::IsReadOnly() const
{
	return mReadOnly;
}

bool _EntryRow::IsValid() const
{
	return mIsValid;
}

const BBitmap* _EntryRow::Icon() const
{
	return mIcon;
}
