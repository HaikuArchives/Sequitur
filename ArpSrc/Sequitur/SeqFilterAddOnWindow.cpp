/* SeqFilterAddOnWindow.cpp
 */
#include <assert.h>
#include <stdio.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <InterfaceKit.h>
#include <support/Autolock.h>
#include "ArpViewsPublic/ArpPrefsI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmFilterRoster.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqColoredColumn.h"
#include "Sequitur/SeqFilterAddOnWindow.h"
#include <ArpKernel/ArpDebug.h>

#define B_BEOS_VERSION_DANO 1

#include <set>

static const float	_ROWSIZE				= 24;
static const char*	SZ_SOURCE				= "Input";
static const char*	SZ_SOURCE_DEST			= "Input/Output";
static const char*	SZ_DESTINATION			= "Output";
static const char*	SZ_THROUGH				= "Through";

static const char*	NEW_STR					= "New";
static const char*	EDIT_STR				= "Edit";
static const char*	DUPLICATE_STR			= "Duplicate";
static const char*	DELETE_STR				= "Delete";
static const char*	NAME_STR				= "Name";
static const char*	TYPE_STR				= "Type";
static const char*	DESCRIPTION_STR			= "Description";
static const char*	MAX_CONNECTIONS_STR		= "Max Connections";
static const char*	VERSION_STR				= "Version";
static const char*	KEY_STR					= "Key";
static const char*	AUTHOR_STR				= "Author";
static const char*	EMAIL_STR				= "Email";
static const char*	PATH_STR				= "Path";

static const char*	TABLE_STR				= "table";

static const uint32	NEW_MULTI_MSG			= 'inwM';
static const uint32	EDIT_MULTI_MSG			= 'iedM';
static const uint32	DUPLICATE_MULTI_MSG		= 'iduM';
static const uint32	DELETE_MULTI_MSG		= 'ideM';
static const uint32	NAME_MSG				= 'inam';
static const uint32	TYPE_MSG				= 'ityp';
static const uint32	DESCRIPTION_MSG			= 'ides';
static const uint32 MAX_CONNECTIONS_MSG		= 'imax';
static const uint32	VERSION_MSG				= 'ivrs';
static const uint32	KEY_MSG					= 'ikey';
static const uint32	AUTHOR_MSG				= 'iaut';
static const uint32	EMAIL_MSG				= 'ieml';
static const uint32	PATH_MSG				= 'ipth';

static const int32	NAME_INDEX				= 0;
static const int32	TYPE_INDEX				= 1;
static const int32	DESCRIPTION_INDEX		= 2;
static const int32	MAX_CONNECTIONS_INDEX	= 3;
static const int32	VERSION_INDEX			= 4;
static const int32	KEY_INDEX				= 5;
static const int32	AUTHOR_INDEX			= 6;
static const int32	EMAIL_INDEX				= 7;
static const int32	PATH_INDEX				= 8;

static const int32	FILTER_MENU_INDEX		= 0;
static const int32	ATTRIBUTE_MENU_INDEX	= 1;

/********************************************************
 * _ADDON-LIST
 ********************************************************/
class _AddOnList : public BColumnListView
{
public:
	_AddOnList(BRect rect);

#if B_BEOS_VERSION_DANO
	virtual bool InitiateDrag(BPoint p, bool wasSelected);
#else
	virtual void InitiateDrag(BPoint p, bool wasSelected);
#endif
	virtual void DrawLatch(BView *view, BRect rect, LatchType pos, BRow *row);
	virtual void ItemInvoked();

private:
	typedef BColumnListView		inherited;
};

/********************************************************
 * _ADD-ON-ROW
 ********************************************************/
class _AddOnRow : public BRow
{
public:
	_AddOnRow(float height);
	virtual ~_AddOnRow();

	virtual bool			HasLatch() const		{ return false; }

	virtual status_t		GetInfo(BString& key, bool* readOnly = NULL, BString* outPath = NULL) const
													{ return B_ERROR; }
	virtual status_t		BuildDragMessage(BMessage* into) const = 0;
	const BBitmap*			Image() const;

protected:
	BBitmap*				mImage;
};

/********************************************************
 * _HANDLE-ROW
 ********************************************************/
class _HandleRow : public _AddOnRow
{
public:
	_HandleRow(AmFilterAddOnHandle* addon, float height);
	virtual ~_HandleRow();

	virtual status_t		BuildDragMessage(BMessage* into) const;
	AmFilterAddOnHandle*	AddOn() const;

	bool					SetMatching(AmFilterAddOnHandle* second);
	AmFilterAddOnHandle*	Matching() const;

private:
	AmFilterAddOnHandle*	mAddOn;
	AmFilterAddOnHandle*	mMatching;
	BStringField*			mTypeField;
};

struct row_entry {
	row_entry()							: row(NULL), addon(NULL)			{ }
	row_entry(_HandleRow* inRow, AmFilterAddOnHandle* inH)
										: row(inRow), addon(inH)			{ }
	row_entry(AmFilterAddOnHandle* inH)	: row(NULL), addon(inH)				{ }
	row_entry(const row_entry& r)		: row(r.row), addon(r.addon)		{ }

	_HandleRow* row;
	AmFilterAddOnHandle* addon;

	#define COMPARE(OP)										\
		bool operator OP(const row_entry& o) const {		\
			if( addon && o.addon ) return addon OP o.addon;	\
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

struct assoc_entry {
	assoc_entry(AmFilterAddOnHandle* inH, bool associated=false, _HandleRow* inRow=NULL)
	{
		row = inRow;
		assoc_type = inH->Type();
		assoc_name = inH->Name();
		if (associated) {
			switch(assoc_type) {
				case AmFilterAddOn::SOURCE_FILTER:
					assoc_type = AmFilterAddOn::DESTINATION_FILTER;
					assoc_class = inH->KeyForType(assoc_type);
					break;
				case AmFilterAddOn::DESTINATION_FILTER:
					assoc_type = AmFilterAddOn::SOURCE_FILTER;
					assoc_class = inH->KeyForType(assoc_type);
					break;
				default:
					break;
			}
		} else {
			assoc_class = inH->Key();
		}
	}

	bool can_associate() const
	{
		return (assoc_class.Length() > 0);
	}

	AmFilterAddOn::type assoc_type;
	BString assoc_class;
	BString assoc_name;

	_HandleRow* row;

	#define COMPARE(OP)															\
		bool operator OP(const assoc_entry& o) const {							\
			if (assoc_type != o.assoc_type) return assoc_type OP o.assoc_type;	\
			if (assoc_name != o.assoc_name) return assoc_name OP o.assoc_name;	\
			return assoc_class OP o.assoc_class;								\
		}

	COMPARE(==);
	COMPARE(!=);
	COMPARE(<=);
	COMPARE(<);
	COMPARE(>);
	COMPARE(>=);
	#undef COMPARE
};

/********************************************************
 * _MULTI-ROW
 ********************************************************/
class _MultiRow : public _AddOnRow
{
public:
	_MultiRow(ArpRef<AmMultiFilterAddOn> addon, float height, bool readOnly, bool isValid);

	virtual status_t		GetInfo(BString& key, bool* readOnly = NULL, BString* outPath = NULL) const;
	virtual status_t		BuildDragMessage(BMessage* into) const;
	ArpRef<AmMultiFilterAddOn>	AddOn() const;

	bool					SetMatching(ArpRef<AmMultiFilterAddOn> second);
	ArpRef<AmMultiFilterAddOn>	Matching() const;

private:
	ArpRef<AmMultiFilterAddOn>	mAddOn;
	ArpRef<AmMultiFilterAddOn>	mMatching;
	BStringField*			mTypeField;
};

struct multi_row_entry {
	multi_row_entry()							: row(NULL), addon(NULL)			{ }
	multi_row_entry(_MultiRow* inRow, ArpRef<AmMultiFilterAddOn> inA)
												: row(inRow), addon(inA)			{ }
	multi_row_entry(ArpRef<AmMultiFilterAddOn> inA)	: row(NULL), addon(inA)				{ }
	multi_row_entry(const multi_row_entry& r)	: row(r.row), addon(r.addon)		{ }

	_MultiRow* 				row;
	ArpRef<AmMultiFilterAddOn> 	addon;

	#define COMPARE(OP)										\
		bool operator OP(const multi_row_entry& o) const {	\
			if (addon && o.addon) return addon OP o.addon;	\
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

struct multi_assoc_entry {
	multi_assoc_entry(ArpRef<AmMultiFilterAddOn> inA, bool associated=false, _MultiRow* inRow=NULL)
	{
		row = inRow;
		assoc_type = inA->Type();
		assoc_name = inA->Name();
		if (associated) {
			switch(assoc_type) {
				case AmFilterAddOn::SOURCE_FILTER:
					assoc_type = AmFilterAddOn::DESTINATION_FILTER;
					assoc_class = inA->KeyForType(assoc_type);
					break;
				case AmFilterAddOn::DESTINATION_FILTER:
					assoc_type = AmFilterAddOn::SOURCE_FILTER;
					assoc_class = inA->KeyForType(assoc_type);
					break;
				default:
					break;
			}
		} else {
			assoc_class = inA->Key();
		}
	}

	bool can_associate() const
	{
		return (assoc_class.Length() > 0);
	}

	AmFilterAddOn::type assoc_type;
	BString assoc_class;
	BString assoc_name;

	_MultiRow* row;

	#define COMPARE(OP)															\
		bool operator OP(const multi_assoc_entry& o) const {					\
			if (assoc_type != o.assoc_type) return assoc_type OP o.assoc_type;	\
			if (assoc_name != o.assoc_name) return assoc_name OP o.assoc_name;	\
			return assoc_class OP o.assoc_class;								\
		}

	COMPARE(==);
	COMPARE(!=);
	COMPARE(<=);
	COMPARE(<);
	COMPARE(>);
	COMPARE(>=);
	#undef COMPARE
};

/********************************************************
 * SEQ-FILTER-ADDON-WINDOW
 ********************************************************/
SeqFilterAddOnWindow::SeqFilterAddOnWindow(	BRect frame,
											const BMessage* config,
											AmFilterRoster* roster)
		: inherited(frame, "Manage Filters", B_DOCUMENT_WINDOW,
					B_NOT_ZOOMABLE|B_WILL_ACCEPT_FIRST_CLICK),
		mRowHeight(_ROWSIZE)
{
	AddMainMenu();
	AddViews();
	if (!roster) roster = AmFilterRoster::Default();
	mRoster = roster;
	AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
	if (roster) roster->AddObserver(this);

	if (config) SetConfiguration(config);
}

thread_id SeqFilterAddOnWindow::Run()
{
	thread_id tid = BWindow::Run();

	if (tid >= B_OK && mRoster) {
		BAutolock _l(this);
		mRoster->StartWatching(BMessenger(this));
		UpdateList();
	}

	return tid;
}

void SeqFilterAddOnWindow::MenusBeginning()
{
	inherited::MenusBeginning();

	BMenuBar*	bar = KeyMenuBar();
	if (!bar) return;
	BMenuItem*	atts = bar->FindItem( "Attributes" );
	if (!atts) return;
	BMenu*		menu = atts->Submenu();
	if (!menu) return;
	BMenuItem*	item;
	for (int32 k = 0; (item = menu->ItemAt(k)); k++)
		item->SetMarked(false);
	BColumnListView* table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!table) return;

	BColumn*		col;
	for (int32 k = 0; (col = table->ColumnAt( k )); k++) {
		BString		n;
		col->GetColumnName(&n);
		if( (item = menu->FindItem( n.String() )) ) {
			if (col->IsVisible() ) item->SetMarked(true);
		}
	}

	bool		readOnly, canEdit = false, canDuplicate = false;
	BString		key;
	if (GetSelectionInfo(key, &readOnly) == B_OK) {
		canEdit = !readOnly;
		canDuplicate = true;
	}
	if ( (menu = bar->SubmenuAt(FILTER_MENU_INDEX)) != NULL) {
		if ( (item = menu->FindItem(EDIT_MULTI_MSG)) != NULL) item->SetEnabled(canEdit);
		if ( (item = menu->FindItem(DUPLICATE_MULTI_MSG)) != NULL) item->SetEnabled(canDuplicate);
		if ( (item = menu->FindItem(DELETE_MULTI_MSG)) != NULL) item->SetEnabled(canEdit);
	}
}

void SeqFilterAddOnWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	if (message->what == B_MOUSE_UP) {
		BPoint where;
		if (message->FindPoint("where", &where) == B_OK &&
				Bounds().Contains(where)) {
			Activate(true);
		}
	}
	inherited::DispatchMessage(message, handler);
}

static void read_only_warning(const BString& key)
{
	BString			warning("Multi filter \'");
	warning << key << "\' is read only.\nTo change it, it needs to be duplicated first.";
	BAlert*	alert = new BAlert(	"Warning", warning.String(),
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
}

void SeqFilterAddOnWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			{
				int32 what;
				if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what) == B_OK
						&& what == B_ADD_ONS_CHANGED) {
					UpdateHandleList();
					return;
				}
			}
			break;
		case AM_FILE_ROSTER_CHANGED: {
			UpdateMultiList();
		} break;

		case NEW_MULTI_MSG: {
			BString		key;
			ShowEditMultiWin(key, BString() );
		} break;
		case EDIT_MULTI_MSG: {
			BString		key, path;
			bool		readOnly;
			if (GetSelectionInfo(key, &readOnly, &path) == B_OK) {
				if (readOnly) read_only_warning(key);
				else ShowEditMultiWin(key, path);
			}
		} break;
		case DUPLICATE_MULTI_MSG: {
			BString					key, path;
			AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
			if (roster && GetSelectionInfo(key, NULL, &path) == B_OK) {
				roster->DuplicateEntry(key, path.String() );
			}
		} break;
		case DELETE_MULTI_MSG: {
			BString					key;
			AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
			if (roster && GetSelectionInfo(key) == B_OK) {
				roster->DeleteEntry(key);
			}
		} break;

		case NAME_MSG:
			ToggleColumn(NAME_STR);
			break;
		case TYPE_MSG:
			ToggleColumn(TYPE_STR);
			break;
		case DESCRIPTION_MSG:
			ToggleColumn(DESCRIPTION_STR);
			break;
		case MAX_CONNECTIONS_MSG:
			ToggleColumn(MAX_CONNECTIONS_STR);
			break;
		case KEY_MSG:
			ToggleColumn(KEY_STR);
			break;
		case VERSION_MSG:
			ToggleColumn(VERSION_STR);
			break;
		case AUTHOR_MSG:
			ToggleColumn(AUTHOR_STR);
			break;
		case EMAIL_MSG:
			ToggleColumn(EMAIL_STR);
			break;
		case PATH_MSG:
			ToggleColumn(PATH_STR);
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

bool SeqFilterAddOnWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting() ) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::MANAGE_FILTERS_WIN_INDEX, config);
	}
	AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
	if (roster) roster->RemoveObserver(this);

	return true;
}

bool SeqFilterAddOnWindow::IsSignificant() const
{
	return false;
}

status_t SeqFilterAddOnWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT( config );
	config->what = FILTER_WINDOW_SETTING_MSG;
	BScreen 	screen(this);
	BRect		f = Frame(), sf = screen.Frame();

	bool		minimized = 0;
	if( IsMinimized() ) minimized = 1;

	status_t	err;
	if( (err = config->AddBool( "f_minimized", IsMinimized() )) != B_OK ) return err;
	if( (err = config->AddFloat( "f_left", f.left / sf.Width() )) != B_OK ) return err;
	if( (err = config->AddFloat( "f_top", f.top / sf.Height() )) != B_OK ) return err;
	if( (err = config->AddFloat( "f_right", f.right / sf.Width() )) != B_OK ) return err;
	if( (err = config->AddFloat( "f_bottom", f.bottom / sf.Height() )) != B_OK ) return err;
	/* Get the columns
	 */
	BColumnListView* table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (table) {
		BColumn*	col;
		for( int32 k = 0; (col = table->ColumnAt(k)); k++ ) {
			BMessage	colMsg;
			BString		colName;
			col->GetColumnName(&colName);
			if( colMsg.AddString( "name", colName.String() ) == B_OK
					&& colMsg.AddFloat( "width", col->Width() ) == B_OK
					&& colMsg.AddBool( "visible", col->IsVisible() ) == B_OK ) {
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

status_t SeqFilterAddOnWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT( config );
	bool		minimized;
	if (config->FindBool("f_minimized", &minimized) != B_OK) minimized = false;
	Minimize(minimized);

	BScreen		s(this);
	if (!s.IsValid() ) return B_ERROR;
	BRect		sf = s.Frame();
	float		l, t, r, b;
	if (config->FindFloat( "f_left", &l ) == B_OK
			&& config->FindFloat( "f_top", &t ) == B_OK
			&& config->FindFloat( "f_right", &r ) == B_OK
			&& config->FindFloat( "f_bottom", &b ) == B_OK ) {
		BRect	f( l * sf.Width(), t * sf.Height(), r * sf.Width(), b * sf.Height() );
		MoveTo(f.LeftTop() );
		ResizeTo(f.Width(), f.Height() );
	}
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

void SeqFilterAddOnWindow::UpdateList()
{
	UpdateHandleList();
	UpdateMultiList();
}

void SeqFilterAddOnWindow::UpdateHandleList()
{
	// For each item in the addon list, add it to my list and
	// make a view for it.
	BAutolock _l(mRoster->Locker());

	BColumnListView*	listView = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!listView) return;

	set<row_entry>		rows;
	set<assoc_entry>	assoc;
	int32				i;

	for (i=0; i<listView->CountRows(); i++) {
		_HandleRow*		row = dynamic_cast<_HandleRow*>(listView->RowAt(i));
		if (row) {
			rows.insert(row_entry(row, row->AddOn()));
			if (row->Matching()) rows.insert(row_entry(row, row->Matching()));
		}
	}

	for (i=0; i<mRoster->CountAddOns(); i++) {
		BAddOnHandle* h = mRoster->AddOnAt(i);
		AmFilterAddOnHandle* fh = dynamic_cast<AmFilterAddOnHandle*>(h);
		if (fh) {
			set<row_entry>::iterator r = rows.find(row_entry(fh));
			assoc_entry ae(fh, true);
			if (ae.can_associate()) {
				set<assoc_entry>::iterator a = assoc.find(ae);
				if (a != assoc.end() && a->row && a->row->SetMatching(fh)) {
					if (r != rows.end()) {
						rows.erase(r);
					}
					continue;
				}
			}

			_HandleRow* row = NULL;
			if (r == rows.end()) {
				row = new _HandleRow( fh, mRowHeight );
				if( row )  listView->AddRow( row );
			} else {
				row = r->row;
				rows.erase(r);
			}

			if (ae.can_associate()) {
				assoc.insert(assoc_entry(fh, false, row));
			}
		}
	}

	for (set<row_entry>::iterator j=rows.begin(); j != rows.end(); j++) {
		if (j->row) {
			listView->RemoveRow(j->row);
			delete j->row;
		}
	}
}

void SeqFilterAddOnWindow::UpdateMultiList()
{
	// For each item in the addon list, add it to my list and
	// make a view for it.
	BAutolock _l(mRoster->Locker());

	BColumnListView*		listView = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!listView) return;

	AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
	if (!roster) return;

	set<multi_row_entry>	rows;
	set<multi_assoc_entry>	assoc;
	int32					i;

	for (i = 0; i < listView->CountRows(); i++) {
		_MultiRow*		row = dynamic_cast<_MultiRow*>(listView->RowAt(i));
		if (row) {
			rows.insert(multi_row_entry(row, row->AddOn()));
			if (row->Matching()) rows.insert(multi_row_entry(row, row->Matching()));
		}
	}

	ArpRef<AmMultiFilterAddOn>	h;
	for (i = 0; (h = roster->FilterAt(i)) != NULL; i++) {
		if (h) {
			set<multi_row_entry>::iterator r = rows.find(multi_row_entry(h));
			multi_assoc_entry ae(h, true);
			if (ae.can_associate()) {
				set<multi_assoc_entry>::iterator a = assoc.find(ae);
				if (a != assoc.end() && a->row && a->row->SetMatching(h)) {
					if (r != rows.end()) {
						rows.erase(r);
					}
					continue;
				}
			}

			_MultiRow* row = NULL;
			if (r == rows.end()) {
				row = new _MultiRow(h, mRowHeight, h->IsReadOnly(), h->IsValid() );
				if (row)  listView->AddRow(row);
			} else {
				row = r->row;
				rows.erase(r);
			}

			if (ae.can_associate()) {
				assoc.insert(multi_assoc_entry(h, false, row));
			}
		}
	}

	for (set<multi_row_entry>::iterator j=rows.begin(); j != rows.end(); j++) {
		if (j->row) {
			listView->RemoveRow(j->row);
			delete j->row;
		}
	}
}

status_t SeqFilterAddOnWindow::GetSelectionInfo(BString& key, bool* readOnly, BString* outPath) const
{
	BColumnListView* listView = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!listView) return B_ERROR;
	_AddOnRow*		row = dynamic_cast<_AddOnRow*>( listView->CurrentSelection() );
	if (!row) return B_ERROR;
	return row->GetInfo(key, readOnly, outPath);
}

void SeqFilterAddOnWindow::ShowEditMultiWin(const BString& key, const BString& path)
{
	BMessage		msg(SHOW_EDIT_MULTIFILTER_MSG);
	msg.AddString("key", key);
	if (path.Length() > 0) msg.AddString("path", path);
	be_app->PostMessage(&msg);
}

static BColumn* column_named(const char* name, BColumnListView* fromTable)
{
	if( !fromTable ) return 0;
	BColumn*		col;
	for( uint32 k = 0; (col = fromTable->ColumnAt( k )); k++ ) {
		BString		n;
		col->GetColumnName( &n );
		if( strcmp( n.String(), name ) == 0 ) return col;
	}
	return 0;
}

void SeqFilterAddOnWindow::ToggleColumn(const char* name)
{
	BColumnListView*	table = dynamic_cast<BColumnListView*>( FindView(TABLE_STR) );
	if (!table) return;
	BColumn*			col = column_named(name, table);
	if (!col) return;
	if (col->IsVisible() ) col->SetVisible(false);
	else col->SetVisible(true);
}

#if 0
static bool add_menu_item(	const char* label,
							int32 what,
							BMenu *toMenu)
{
	BMessage*	msg = new BMessage(what);
	if( !msg ) return false;
	BMenuItem*	item = new BMenuItem(label, msg, 0, 0);
	if( !item ) {
		delete msg;
		return false;
	}
	toMenu->AddItem(item);
	return true;
}
#endif

void SeqFilterAddOnWindow::AddMainMenu()
{
	BMenuBar*	menuBar;
	BMenu*		menu;
	BMenuItem*	item;
	BRect		f = Bounds();

	f.bottom = f.top + Prefs().Size( MAINMENU_Y );
	menuBar = new BMenuBar(	f,
							NULL,
							B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_ITEMS_IN_ROW,
							FALSE);

	/* Filter Menu
	 */
	menu = new BMenu("Filter", B_ITEMS_IN_COLUMN);
	add_menu_item(menu,	NEW_STR,				NEW_MULTI_MSG,			'N');
	add_menu_item(menu,	EDIT_STR,				EDIT_MULTI_MSG,			'E');
	add_menu_item(menu,	DUPLICATE_STR,			DUPLICATE_MULTI_MSG,	'D');
	add_menu_item(menu,	DELETE_STR,				DELETE_MULTI_MSG,		'T');
	item = new BMenuItem(menu);
	menuBar->AddItem(item, FILTER_MENU_INDEX);
	/* Attributes Menu
	 */
	menu = new BMenu("Attributes", B_ITEMS_IN_COLUMN);
	add_menu_item(menu,	NAME_STR,				NAME_MSG,				0);
	add_menu_item(menu,	TYPE_STR,				TYPE_MSG,				0);
	add_menu_item(menu,	DESCRIPTION_STR,		DESCRIPTION_MSG,		0);
	add_menu_item(menu,	MAX_CONNECTIONS_STR,	MAX_CONNECTIONS_MSG,	0);
	add_menu_item(menu,	VERSION_STR,			VERSION_MSG,			0);
	add_menu_item(menu,	KEY_STR,				KEY_MSG,				0);
	add_menu_item(menu,	AUTHOR_STR,				AUTHOR_MSG,				0);
	add_menu_item(menu,	EMAIL_STR,				EMAIL_MSG,				0);
	add_menu_item(menu,	PATH_STR,				PATH_MSG,				0);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, ATTRIBUTE_MENU_INDEX);

	AddChild(menuBar);
	SetKeyMenuBar(menuBar);
}

void SeqFilterAddOnWindow::AddViews()
{
	_AddOnList*		listView;
	BRect			f = Bounds();
	f.top += (Prefs().Size(MAINMENU_Y) + 1);
	f.right++;
	f.bottom++;
	listView = new _AddOnList(f);
	if (!listView) return;
	listView->SetLatchWidth(24);
	AddChild(listView);

	listView->AddColumn( new SeqColoredColumn(NAME_STR,				110, 20, 350, B_TRUNCATE_MIDDLE), NAME_INDEX );
	listView->AddColumn( new SeqColoredColumn(TYPE_STR,				100, 20, 150, B_TRUNCATE_END), TYPE_INDEX );
	listView->AddColumn( new SeqColoredColumn(DESCRIPTION_STR,		250, 20, 450, B_TRUNCATE_END), DESCRIPTION_INDEX );
	listView->AddColumn( new SeqColoredColumn(MAX_CONNECTIONS_STR,	100, 20, 250, B_TRUNCATE_END), MAX_CONNECTIONS_INDEX );
	listView->AddColumn( new SeqColoredColumn(VERSION_STR,			50, 20, 450, B_TRUNCATE_END), VERSION_INDEX );
	listView->AddColumn( new SeqColoredColumn(KEY_STR,				150, 20, 450, B_TRUNCATE_END), KEY_INDEX );
	listView->AddColumn( new SeqColoredColumn(AUTHOR_STR,			150, 20, 450, B_TRUNCATE_END), AUTHOR_INDEX );
	listView->AddColumn( new SeqColoredColumn(EMAIL_STR,			150, 20, 450, B_TRUNCATE_END), EMAIL_INDEX );
	listView->AddColumn( new SeqColoredColumn(PATH_STR,				250, 20, 450, B_TRUNCATE_END), PATH_INDEX );

//	BColumn*		col = new BStringColumn(INFO_STR, 250, 20, 450, B_TRUNCATE_END);
//	if( col ) {
//		listView->AddColumn( col , 3 );
//		col->SetVisible( false );
//	}
	listView->SetSortColumn(listView->ColumnAt(1), false, true);
	listView->SetSortColumn(listView->ColumnAt(0), true, true);
	listView->SetColumnVisible(3, false);
	listView->MakeFocus();
}

/********************************************************
 * _ADDON-LIST
 ********************************************************/
_AddOnList::_AddOnList(BRect frame)
		: inherited( frame, TABLE_STR, B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER )
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
}

#if B_BEOS_VERSION_DANO
bool
#else
void
#endif
_AddOnList::InitiateDrag(BPoint where, bool wasSelected)
{
	inherited::InitiateDrag( where, wasSelected );
	const _AddOnRow*	row = dynamic_cast<_AddOnRow*>( RowAt(where) );
#if B_BEOS_VERSION_DANO
	if (!row) return false;
#else
	if (!row) return;
#endif

	const BBitmap*	image = row->Image();
	BMessage		msg;
#if B_BEOS_VERSION_DANO
	if (row->BuildDragMessage(&msg) != B_OK) return false;
#else
	if (row->BuildDragMessage(&msg) != B_OK) return;
#endif

	if (image) {
		BBitmap*	bmp = new BBitmap(image);
		BPoint		pt(bmp->Bounds().Width() / 2, bmp->Bounds().Height() / 2);
		DragMessage( &msg, bmp, B_OP_BLEND, pt, Window() );
	} else {
		DragMessage( &msg, BRect(0, 0, 20, 20), Window() );
	}
#if B_BEOS_VERSION_DANO
	return true;
#endif
}

void _AddOnList::DrawLatch(BView *view, BRect rect, LatchType pos, BRow *row)
{
	BRect oldRect(rect);
//	oldRect.right = oldRect.left + 15.0;
	inherited::DrawLatch(view, oldRect, pos, row);
	_AddOnRow*	r = dynamic_cast<_AddOnRow*>(row);
	if (r && r->Image() ) {
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap(r->Image(), BPoint(oldRect.left + 2, oldRect.top + 1) );
	}
}

void _AddOnList::ItemInvoked()
{
	inherited::ItemInvoked();
	BWindow*	win = dynamic_cast<BWindow*>(Window() );
	if (win) win->PostMessage(EDIT_MULTI_MSG);
}

/********************************************************
 * _ADD-ON-ROW
 ********************************************************/
_AddOnRow::_AddOnRow(float height)
		: BRow(height), mImage(NULL)
{
}

_AddOnRow::~_AddOnRow()
{
	delete mImage;
	mImage = NULL;
}

const BBitmap* _AddOnRow::Image() const
{
	return mImage;
}

/********************************************************
 * _HANDLE-ROW
 ********************************************************/
_HandleRow::_HandleRow(AmFilterAddOnHandle* addon, float height)
		: _AddOnRow(height), mAddOn(addon), mMatching(NULL), mTypeField(0)
{
	mAddOn->Acquire();

	BString			type;
	if (mAddOn->Subtype() == AmFilterAddOn::TOOL_SUBTYPE) type = "Tool ";
	else if (mAddOn->Subtype() == AmFilterAddOn::MULTI_SUBTYPE) type = "Multi ";
	if (mAddOn->Type() == AmFilterAddOn::DESTINATION_FILTER) type << SZ_DESTINATION;
	else if (mAddOn->Type() == AmFilterAddOn::SOURCE_FILTER) type << SZ_SOURCE;
	else type << SZ_THROUGH;

	SetField( new BStringField(mAddOn->Name().String() ), NAME_INDEX);
	SetField( (mTypeField = new BStringField(type.String() )), TYPE_INDEX);
	SetField( new BStringField(mAddOn->ShortDescription().String() ), DESCRIPTION_INDEX);
	BString		max;
	if (mAddOn->MaxConnections() < 1) max = "Unlimited";
	else max << mAddOn->MaxConnections();
	SetField( new BStringField(max.String() ), MAX_CONNECTIONS_INDEX);

	BString		version;
	int32		maj, min;
	mAddOn->GetVersion(&maj, &min);
	version << maj << "." << min;
	SetField( new BStringField(version.String() ), VERSION_INDEX);

	SetField( new BStringField(mAddOn->Key().String() ), KEY_INDEX);

	SetField( new BStringField(mAddOn->Author().String() ), AUTHOR_INDEX);

	SetField( new BStringField(mAddOn->Email().String() ), EMAIL_INDEX);

	mImage = mAddOn->FinalImage(BPoint(20, 20));
	if (!mImage) {
		const BBitmap*	bm = seq_app->DefaultFilterImage();
		if( bm ) mImage = new BBitmap(bm);
	}
}

_HandleRow::~_HandleRow()
{
	if (mAddOn) mAddOn->Release();
	mAddOn = NULL;
	if (mMatching) mMatching->Release();
	mMatching = NULL;
}

status_t _HandleRow::BuildDragMessage(BMessage* into) const
{
	BMessage filter;
	status_t err = mAddOn->GetArchiveTemplate(&filter);
	if (err == B_OK) err = into->AddMessage(SZ_FILTER_ARCHIVE, &filter);
	if (mMatching) {
		filter.MakeEmpty();
		status_t err = mMatching->GetArchiveTemplate(&filter);
		if (err == B_OK) err = into->AddMessage(SZ_FILTER_ARCHIVE, &filter);
	}
	into->what = ARPMSG_FILTERADDONDRAG;
	return err;
}

AmFilterAddOnHandle* _HandleRow::AddOn() const
{
	return mAddOn;
}

bool _HandleRow::SetMatching(AmFilterAddOnHandle* addon)
{
	if (!mMatching) {
		mMatching = addon;
		mMatching->Acquire();
#if B_BEOS_VERSION_DANO
		if (mTypeField) mTypeField->SetString(SZ_SOURCE_DEST);
#else
		if (mTypeField) mTypeField->SetTo(SZ_SOURCE_DEST);
#endif
		return true;
	}
	return false;
}

AmFilterAddOnHandle* _HandleRow::Matching() const
{
	return mMatching;
}

/********************************************************
 * _MULTI-ROW
 ********************************************************/
_MultiRow::_MultiRow(	ArpRef<AmMultiFilterAddOn> addon, float height,
						bool readOnly, bool isValid)
		: _AddOnRow(height), mAddOn(addon), mMatching(NULL), mTypeField(0)
{
	BString		type("Multi ");
	if (mAddOn->Type() == AmFilterAddOn::DESTINATION_FILTER) type << SZ_DESTINATION;
	else if (mAddOn->Type() == AmFilterAddOn::SOURCE_FILTER) type << SZ_SOURCE;
	else type << SZ_THROUGH;

	SetField( new SeqColoredField(mAddOn->Name().String(), readOnly, isValid), NAME_INDEX);
	SetField( (mTypeField = new SeqColoredField(type.String(), readOnly, isValid)), TYPE_INDEX);
	SetField( new SeqColoredField(mAddOn->ShortDescription().String(), readOnly, isValid), DESCRIPTION_INDEX);
	BString		max;
	if (mAddOn->MaxConnections() < 1) max = "Unlimited";
	else max << mAddOn->MaxConnections();
	SetField( new SeqColoredField(max.String(), readOnly, isValid), MAX_CONNECTIONS_INDEX);

	BString		version;
	int32		maj, min;
	mAddOn->GetVersion(&maj, &min);
	version << maj << "." << min;
	SetField( new SeqColoredField(version.String(), readOnly, isValid), VERSION_INDEX);

	SetField( new SeqColoredField(mAddOn->Key().String(), readOnly, isValid), KEY_INDEX);

	SetField( new SeqColoredField(mAddOn->Author().String(), readOnly, isValid), AUTHOR_INDEX);

	SetField( new SeqColoredField(mAddOn->Email().String(), readOnly, isValid), EMAIL_INDEX);

	SetField( new SeqColoredField(mAddOn->LocalFilePath().String(), readOnly, isValid), PATH_INDEX);

	BPoint		requestedSize(20, 20);
	mImage = mAddOn->FinalImage(requestedSize);
	if (mImage) {
		mImage = ArpMakeFilterBitmap(mImage, requestedSize);
	} else if (!mImage) {
		const BBitmap*	bm = seq_app->DefaultFilterImage();
		if (bm) mImage = new BBitmap(bm);
	}
}

status_t _MultiRow::GetInfo(BString& key, bool* readOnly, BString* outPath) const
{
	if (!mAddOn) return B_ERROR;
	key = mAddOn->UniqueName();
	if (readOnly) *readOnly = mAddOn->IsReadOnly();
	if (outPath) *outPath = mAddOn->LocalFilePath();
	return B_OK;
}

status_t _MultiRow::BuildDragMessage(BMessage* into) const
{
	if (!mAddOn) return B_ERROR;
	uint32		flags = AM_ARCHIVE_ALL & ~AM_ARCHIVE_DESCRIPTION;
	flags &= ~AM_ARCHIVE_IMAGE;
	BMessage	filter;
	status_t	err = mAddOn->GetArchiveTemplate(&filter, flags);
	if (err == B_OK) err = into->AddMessage(SZ_FILTER_ARCHIVE, &filter);
	if (mMatching) {
		filter.MakeEmpty();
		status_t err = mMatching->GetArchiveTemplate(&filter, flags);
		if (err == B_OK) err = into->AddMessage(SZ_FILTER_ARCHIVE, &filter);
	}
	into->what = ARPMSG_FILTERADDONDRAG;
	return err;
}

ArpRef<AmMultiFilterAddOn> _MultiRow::AddOn() const
{
	return mAddOn;
}

bool _MultiRow::SetMatching(ArpRef<AmMultiFilterAddOn> addon)
{
	if (!mMatching) {
		mMatching = addon;
#if B_BEOS_VERSION_DANO
		if (mTypeField) mTypeField->SetString(SZ_SOURCE_DEST);
#else
		if (mTypeField) mTypeField->SetTo(SZ_SOURCE_DEST);
#endif
		return true;
	}
	return false;
}

ArpRef<AmMultiFilterAddOn> _MultiRow::Matching() const
{
	return mMatching;
}
