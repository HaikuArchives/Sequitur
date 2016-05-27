/* SeqStudioWindow.cpp
 */
#include <algo.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <interface/Button.h>
#include <interface/MenuBar.h>
#include <interface/MenuItem.h>
#include <interface/StringView.h>
#include <interface/View.h>
#include <midi2/MidiConsumer.h>
#include <midi2/MidiProducer.h>
#include <midi2/MidiRoster.h>
#include <support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmDevice.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "AmKernel/AmPhraseEvent.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqColoredColumn.h"
#include "Sequitur/SeqStudioWindow.h"

static const float		MIN_WIDTH = 50;
static const float		MIN_HEIGHT = 50;
static const float		INSET = 3;

static const char*		ENDPOINT_LIST_STR	= "endpoint_list";
static const char*		MIDI_PORT_STR		= "MIDI Port";
static const char*		TYPE_STR		 	= "Type";
static const char*		DEVICE_STR			= "Device";
static const char*		LABEL_STR			= "Label";
static const char*		DEV_MFG_STR			= "dev_mfg";
static const char*		DEV_NAME_STR		= "dev_name";
static const char*		NONE_STR			= "None";
static const char*		VARIOUS_STR			= "Various";
static const char*		DELETE_STR			= "Delete";
static const char*		COLUMN_NAME_STR		= "col_name";

static const uint32		LABEL_MSG			= '#lbl';
static const uint32		DEVICE_MSG			= '#dev';
static const uint32		DELETE_PORT_MSG		= 'iDlP';
static const uint32		COLUMN_MSG			= 'iCoT';

static const int32		ENTRY_MENU_INDEX		= 0;
static const int32		ATTRIBUTES_MENU_INDEX	= 1;

/********************************************************
 * _ENDPOINT-LIST
 ********************************************************/
class _EndpointRow;

class _EndpointList : public BColumnListView
{
public:
	_EndpointList(BRect rect);

	BTextControl*	mLabelTarget;
	BMenuField*		mDeviceTarget;

	void			SetTargets(BTextControl* labelTarget, BMenuField* deviceTarget);

	virtual void	SelectionChanged();

	void			HandleLabelChange(const char* label);
	void			HandleDeviceChange(const char* devMfg, const char* devName);

	status_t		MidiRegistered(int32 id, AmEndpointType type);
	status_t		MidiUnregistered(int32 id, AmEndpointType type);

	void			DeleteCurrentSelection();

private:
	typedef BColumnListView		inherited;

	void			AddEndpoint(BMidiEndpoint* endpoint);
	void			DeleteRow(_EndpointRow* row);

	status_t		AddChildRow(am_studio_endpoint& endpoint, BString* label,
								BString* devMfg, BString* devName);
	_EndpointRow*	ParentNamed(const BString& name, AmEndpointType type);
};

/********************************************************
 * _ENDPOINT-ROW
 ********************************************************/
class _EndpointRow : public BRow
{
public:
	_EndpointRow(BMidiEndpoint* endpoint, int32 channel, _EndpointList* list);

	_EndpointRow(	const am_studio_endpoint& endpoint, int32 channel, _EndpointList* list,
					bool isValid);

	/* Answer true if the user has modified my studio entry -- either given
	 * it a label or assigned a device.
	 */
	bool	HasChanges();
	void	SelectionSetup(_EndpointList* target);
	void	SetIsValid(bool isValid);
	void	HandleLabelChange(const char* label);
	void	HandleDeviceChange(const char* devMfg, const char* devName);

	void	UpdateDeviceColumn();

	void	UpdateLabelColumn();

	am_studio_endpoint	mEndpoint;
	bool				mReadOnly, mIsValid;

private:
	_EndpointList*		mList;

	void	GetLabellingInfo(BString& outDeviceLabel, BString& outLabel) const;
	// Answer:  0 = None, 1 = Various, 2 = devMfg and devName are valid.
	int32	GetAllChannelDeviceInfo(BString& label, BString& devMfg, BString& devName) const;
	void	Init();
};

/*************************************************************************
 * SEQ-STUDIO-WINDOW
 *************************************************************************/
SeqStudioWindow::SeqStudioWindow(	BRect frame,
									const BMessage* config = NULL)
		: inherited(frame,
					"Studio",
					B_DOCUMENT_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS),
		  mBg(NULL), mPortMenu(NULL),
		  mLabelCtrl(NULL), mDeviceCtrl(NULL)
{
	BRect		b(Bounds() );
	BRect		r(b);
	r.bottom  = Prefs().Size(MAINMENU_Y);
	AddMainMenu(r);
	r.top = r.bottom + 1;
	/* This accounts for the document window tab.  That tab does
	 * not play nicely with a background that goes completely behind
	 * it -- it leaves trails -- so we have to chunk the background
	 * into two parts.
	 */
	r.bottom = b.bottom - B_H_SCROLL_BAR_HEIGHT;
	AddViews(r);
	r.top = r.bottom + 1;
	r.bottom = b.bottom;
	r.right = b.right - B_V_SCROLL_BAR_WIDTH;
	BView*		v = new BView(r, "filler", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, 0);
	if (v) {
		v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		AddChild(v);
	}

	if (config) SetConfiguration(config);
}

SeqStudioWindow::~SeqStudioWindow()
{
}

static bool sort_items(BMenuItem* item1, BMenuItem* item2)
{
	if (item1->Label() == NULL) return false;
	if (item2->Label() == NULL) return true;
	BString		str(item1->Label() );
	if (str.ICompare(item2->Label() ) <= 0) return true;
	return false;
}

static void add_device_menu_items(BMenu* menu)
{
	if (!menu) return;
	BString		markedLabel;
	BMenuItem*	item = menu->FindMarked();
	if (item) markedLabel = item->Label();
	menu->RemoveItems(0, menu->CountItems(), true);

	AmDeviceRoster*				roster = AmDeviceRoster::Default();
	if (roster) {
		vector<BMenuItem*>		items;
		BString					mfg, name, key;
		bool					isValid;
		for (uint32 k = 0; roster->GetDeviceInfo(k, mfg, name, key, NULL, &isValid) == B_OK; k++) {
			if (isValid) {
				BString			label = AmDeviceI::MakeLabel(mfg, name);
				BMessage*		msg = new BMessage(DEVICE_MSG);
				if (msg) {
					if (mfg.Length() > 0) msg->AddString(DEV_MFG_STR, mfg);
					if (name.Length() > 0) msg->AddString(DEV_NAME_STR, name);

					item = new BMenuItem(label.String(), msg);
					if (item) items.push_back(item);
				}
			}
		}
		sort(items.begin(), items.end(), sort_items);
		for (uint32 k = 0; k < items.size(); k++) menu->AddItem(items[k]);
		items.resize(0);
	}
	/* Set the marked to whatever it previously was.  It's important to try
	 * to find the marked BEFORE adding the separator item -- otherwise, if
	 * there is no marked label, it will find the separator item.
	 */
	BMenuItem*	marked = menu->FindItem(markedLabel.String() );

	if (menu->CountItems() > 0) menu->AddSeparatorItem();
	BMenuItem*	noneItem = new BMenuItem(NONE_STR, new BMessage(DEVICE_MSG));
	if (noneItem) menu->AddItem(noneItem);
	item = new BMenuItem(VARIOUS_STR, NULL);
	if (item) {
		menu->AddItem(item);
		item->SetEnabled(false);
	}
	/* Now set the actual marked item.
	 */
	if (!marked) marked = noneItem;
	if (!marked) marked = menu->ItemAt(0);
	if (marked) marked->SetMarked(true);
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

void SeqStudioWindow::MenusBeginning()
{
	inherited::MenusBeginning();
	BMenuBar*			bar = KeyMenuBar();
	if (!bar) return;
	BColumnListView*	table = dynamic_cast<BColumnListView*>( FindView(ENDPOINT_LIST_STR) );
	if (!table) return;

	if (mDeviceCtrl && mDeviceCtrl->Menu() ) add_device_menu_items(mDeviceCtrl->Menu() );

	// MIDI Port menu
	if (mPortMenu) {
		bool				deleteEnabled = false;
		_EndpointRow*	r = dynamic_cast<_EndpointRow*>(table->CurrentSelection() );
		if (r && !r->mIsValid && r->mEndpoint.channel < 0) deleteEnabled = true;

		BMenuItem*			deleteItem = mPortMenu->FindItem(DELETE_STR);
		if (deleteItem && deleteItem->IsEnabled() != deleteEnabled) deleteItem->SetEnabled(deleteEnabled);
	}

	// Attributes menu
	BMenu*					menu;
	BMenuItem*				item;
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

void SeqStudioWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case LABEL_MSG: {
			_EndpointList* listView = dynamic_cast<_EndpointList*>( FindView(ENDPOINT_LIST_STR) );
			if (listView && mLabelCtrl) {
				listView->HandleLabelChange( mLabelCtrl->Text() );
			}
		} break;
		case DEVICE_MSG: {
			const char*		devMfg;
			const char*		devName;
			if (msg->FindString(DEV_MFG_STR, &devMfg) != B_OK) devMfg = NULL;
			if (msg->FindString(DEV_NAME_STR, &devName) != B_OK) devName = NULL;
			_EndpointList* listView = dynamic_cast<_EndpointList*>( FindView(ENDPOINT_LIST_STR) );
			if (listView) {
				listView->HandleDeviceChange(devMfg, devName);
			}
		} break;
		case DELETE_PORT_MSG: {
			_EndpointList* listView = dynamic_cast<_EndpointList*>( FindView(ENDPOINT_LIST_STR) );
			if (listView) listView->DeleteCurrentSelection();
		} break;
		case COLUMN_MSG: {
			const char*		n;
			if (msg->FindString(COLUMN_NAME_STR, &n) == B_OK)
				ToggleColumn(n);
		} break;
		case B_MIDI_EVENT: {
			int32			op, id;
			const char*		type;
			if (msg->FindInt32("be:op", &op) == B_OK && msg->FindString("be:type", &type) == B_OK
					&& msg->FindInt32("be:id", &id) == B_OK
					&& (op == B_MIDI_REGISTERED || op == B_MIDI_UNREGISTERED) ) {
				BString		producer("producer"), consumer("consumer");
				AmEndpointType	et = AM_PRODUCER_TYPE;
				if (producer == type) et = AM_PRODUCER_TYPE;
				else if (consumer == type) et = AM_CONSUMER_TYPE;
				_EndpointList* listView = dynamic_cast<_EndpointList*>( FindView(ENDPOINT_LIST_STR) );
				if (listView && (et == AM_PRODUCER_TYPE || et == AM_CONSUMER_TYPE) ) {
					if (op == B_MIDI_REGISTERED) listView->MidiRegistered(id, et);
					else if (op == B_MIDI_UNREGISTERED) listView->MidiUnregistered(id, et);
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqStudioWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::STUDIO_WIN_INDEX, config);
	}
	return true;
}

thread_id SeqStudioWindow::Run()
{
	thread_id	tid = inherited::Run();
	BMessenger	m(this);
	BMidiRoster::StartWatching(&m);
	return tid;
}

bool SeqStudioWindow::IsSignificant() const
{
	return false;
}

status_t SeqStudioWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT(config);
	config->what = STUDIO_WINDOW_SETTING_MSG;
	status_t	err = GetDimensions(config, this);
	if (err != B_OK) return err;
	/* Add the columns
	 */
	BColumnListView* table = dynamic_cast<BColumnListView*>( FindView(ENDPOINT_LIST_STR) );
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
		col->GetColumnName(&colName);
		if (strcmp(name, colName.String() ) == 0) return col;
	}
	return NULL;
}

status_t SeqStudioWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	status_t	err = SetDimensions(config, this);
	if (err != B_OK) return err;
	/* Set the columns
	 */
	BColumnListView* table = dynamic_cast<BColumnListView*>( FindView(ENDPOINT_LIST_STR) );
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

void SeqStudioWindow::ToggleColumn(const char* name)
{
	BColumnListView*	table = dynamic_cast<BColumnListView*>( FindView(ENDPOINT_LIST_STR) );
	if (!table) return;
	BColumn*			col = column_named(name, table);
	if (!col) return;
	if (col->IsVisible() ) col->SetVisible(false);
	else col->SetVisible(true);
}

static void add_attribute_item(BMenu* toMenu, const char* label)
{
	BMessage*		msg = new BMessage(COLUMN_MSG);
	if (!msg) return;
	if (msg->AddString(COLUMN_NAME_STR, label) != B_OK) return;
	BMenuItem*		item = new BMenuItem(label, msg);
	if (!item) {
		delete msg;
		return;
	}
	toMenu->AddItem(item);
}

void SeqStudioWindow::AddMainMenu(BRect f)
{
	BMenuBar*		menuBar;
	BMenu*			menu;
	BMenuItem*		item;
	menuBar = new BMenuBar(	f, NULL, B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
							B_ITEMS_IN_ROW, FALSE);
	/* Entry Menu
	 */
	mPortMenu = new BMenu("MIDI Port", B_ITEMS_IN_COLUMN);
	add_menu_item(mPortMenu,	DELETE_STR,		DELETE_PORT_MSG,		'T');
	item = new BMenuItem(mPortMenu);
	menuBar->AddItem(mPortMenu, ENTRY_MENU_INDEX);

	/* Attributes Menu
	 */
	menu = new BMenu("Attributes", B_ITEMS_IN_COLUMN);
	item = new BMenuItem(menu);
	menuBar->AddItem(item, ATTRIBUTES_MENU_INDEX);
	add_attribute_item(menu, MIDI_PORT_STR);
	add_attribute_item(menu, TYPE_STR);
	add_attribute_item(menu, DEVICE_STR);
	add_attribute_item(menu, LABEL_STR);

	AddChild(menuBar);
	SetKeyMenuBar(menuBar);
}

static BMenuField* new_device_control(BRect f)
{
	BMenu*						menu = new BMenu("device_menu");
	if (!menu) return NULL;
	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);
	add_device_menu_items(menu);
	BMenuField*	field = new BMenuField(f, "device_field", "Device:", menu, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

void SeqStudioWindow::AddViews(BRect frame)
{
	mBg = new BView(frame, "BG", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!mBg) return;
	mBg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	/* Layout the frames.
	 */
	float		spaceX = 5, spaceY = 5;
	float		divider = mBg->StringWidth("Device:") + 10;
	BRect		labelF(spaceX, frame.Height() - spaceY - Prefs().Size(TEXTCTRL_Y), frame.Width() - spaceX, frame.Height() - spaceY);
	BRect		devicesF(labelF);
	devicesF.bottom = devicesF.top - spaceY;
	devicesF.top = devicesF.bottom - Prefs().Size(MENUFIELD_Y);
	BRect		studioF(devicesF);
	studioF.bottom = devicesF.top - spaceY;
	studioF.top = spaceY;

	/* The consumer list view.
	 */
	_EndpointList*		consumerList = new _EndpointList(studioF);
	if (consumerList) {
		mBg->AddChild(consumerList);
	}
	/* The Device field.
	 */
	mDeviceCtrl = new_device_control(devicesF);
	if (mDeviceCtrl) {
		mDeviceCtrl->SetDivider(divider);
		mDeviceCtrl->SetEnabled(false);
		mBg->AddChild(mDeviceCtrl);
	}
	/* The Label field.
	 */
	mLabelCtrl = new BTextControl(labelF, "label_ctrl", "Label:", NULL, new BMessage(LABEL_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	if (mLabelCtrl) {
		mLabelCtrl->SetDivider(divider);
		mLabelCtrl->MakeFocus(true);
		mLabelCtrl->SetEnabled(false);
		mBg->AddChild(mLabelCtrl);
	}

	if (consumerList) consumerList->SetTargets(mLabelCtrl, mDeviceCtrl);

	AddChild(mBg);
}

// #pragma mark -

/********************************************************
 * _DEVICE-COMPARE
 ********************************************************/
class _DeviceCompare
{
public:
	_DeviceCompare()
			: mValid(false), mCannotHaveLabel(false),
			  mIsNone(true), mIsVarious(false)
	{
	}

	bool IsValid() const
	{
		return mValid;
	}

	void GetInfo(AmStudio& studio, am_studio_endpoint& endpoint)
	{
		if (studio.GetDeviceInfo(endpoint, mLabel, mDevMfg, mDevName) == B_OK) {
			mValid = true;
			if (mDevMfg.Length() > 0 || mDevName.Length() > 0) mIsNone = false;
		} else
			mValid = false;
	}

	void AddCompare(_DeviceCompare& o)
	{
		ArpASSERT(mValid && o.mValid);
		if (!mCannotHaveLabel) {
			if (mLabel != o.mLabel) {
				mLabel = (const char*)NULL;
				mCannotHaveLabel = true;
			}
		}
		if (mIsNone) {
			if (o.mDevMfg.Length() > 0 || o.mDevName.Length() > 0)
				mIsNone = false;
		}
		if (mIsVarious == false) {
			if (mDevMfg.Length() > 0 || o.mDevMfg.Length() > 0
					|| mDevName.Length() > 0 || o.mDevName.Length() > 0) {
				if (mDevMfg != o.mDevMfg || mDevName != o.mDevName)
					mIsVarious = true;
			}
		}
	}

	void Fill(BString& label, BString& devMfg, BString& devName) const
	{
		label = mLabel;
		devMfg = mDevMfg;
		devName = mDevName;
	}

	int32 Status() const
	{
		if (mIsNone) return 0;
		if (mIsVarious) return 1;
		return 2;
	}

private:
	bool			mValid;

	BString			mLabel, mDevMfg, mDevName;
	bool			mCannotHaveLabel;
	bool			mIsNone, mIsVarious;
};

/********************************************************
 * _ENDPOINT-LIST
 ********************************************************/
_EndpointList::_EndpointList(BRect frame)
		: inherited(frame, ENDPOINT_LIST_STR, B_FOLLOW_ALL,
					B_WILL_DRAW, B_NO_BORDER),
		  mLabelTarget(NULL), mDeviceTarget(NULL)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortingEnabled(false);

	AddColumn( new SeqColoredColumn(MIDI_PORT_STR, 150, 20, 350, B_TRUNCATE_MIDDLE), 0);
	AddColumn( new SeqColoredColumn(TYPE_STR, 50, 20, 350, B_TRUNCATE_MIDDLE), 1);
	AddColumn( new SeqColoredColumn(DEVICE_STR, 150, 20, 350, B_TRUNCATE_MIDDLE), 2);
	AddColumn( new SeqColoredColumn(LABEL_STR, 50, 20, 350, B_TRUNCATE_MIDDLE), 3);

	int32 id = 0;
	BMidiProducer* prod;
	while ((prod=BMidiRoster::NextProducer(&id)) != NULL) {
		if (prod->IsValid() ) AddEndpoint(prod);
	}
	id = 0;
	BMidiConsumer* cons;
	while ((cons=BMidiRoster::NextConsumer(&id)) != NULL) {
		if (cons->IsValid() ) AddEndpoint(cons);
	}

	am_studio_endpoint		endpt;
	BString					l, m, n;
	for (uint32 k = 0; AmGlobals().GetStudioInfoAt(k, endpt, &l, &m, &n) == B_OK; k++) {
		AddChildRow(endpt, &l, &m, &n);
	}
}

void _EndpointList::SetTargets(BTextControl* labelTarget, BMenuField* deviceTarget)
{
	mLabelTarget = labelTarget;
	mDeviceTarget = deviceTarget;
}

void _EndpointList::SelectionChanged()
{
	inherited::SelectionChanged();
	_EndpointRow*	row = dynamic_cast<_EndpointRow*>( CurrentSelection() );
	if (row && row->mIsValid) {
		row->SelectionSetup(this);
		if (mDeviceTarget && !mDeviceTarget->IsEnabled() ) mDeviceTarget->SetEnabled(true);
		if (mLabelTarget && !mLabelTarget->IsEnabled() ) mLabelTarget->SetEnabled(true);
	} else {
		if (mDeviceTarget && mDeviceTarget->IsEnabled() ) mDeviceTarget->SetEnabled(false);
		if (mLabelTarget && mLabelTarget->IsEnabled() ) mLabelTarget->SetEnabled(false);
	}
}

void _EndpointList::HandleLabelChange(const char* label)
{
	_EndpointRow*	row = dynamic_cast<_EndpointRow*>( CurrentSelection() );
	if (!row) return;
	row->HandleLabelChange(label);
}

void _EndpointList::HandleDeviceChange(const char* devMfg, const char* devName)
{
	_EndpointRow*	row = dynamic_cast<_EndpointRow*>( CurrentSelection() );
	if (!row) return;
	row->HandleDeviceChange(devMfg, devName);
}

status_t _EndpointList::MidiRegistered(int32 id, AmEndpointType type)
{
	BMidiEndpoint*	endpoint = BMidiRoster::FindEndpoint(id);
	if (!endpoint) {
		ArpASSERT(false);
		return B_ERROR;
	}
	if (!endpoint->IsValid() ) return B_ERROR;
	BString			name = endpoint->Name();
	_EndpointRow*	p = ParentNamed(name, type);
	if (p) return B_OK;
	AddEndpoint(endpoint);
	return B_OK;
}

status_t _EndpointList::MidiUnregistered(int32 id, AmEndpointType type)
{
	BMidiEndpoint*	endpoint = BMidiRoster::FindEndpoint(id);
	if (!endpoint) {
		ArpASSERT(false);
		return B_ERROR;
	}
	BString			name = endpoint->Name();
	_EndpointRow*	p = ParentNamed(name, type);
	if (!p) return B_ERROR;
	if (!p->HasChanges() ) {
		DeleteRow(p);
	} else {
		p->SetIsValid(false);
		UpdateRow(p);
	}
	return B_OK;
}

void _EndpointList::DeleteCurrentSelection()
{
	_EndpointRow*	row = dynamic_cast<_EndpointRow*>(CurrentSelection() );
	ArpVALIDATE(row && !row->mIsValid, return);
	DeleteRow(row);
}

void _EndpointList::AddEndpoint(BMidiEndpoint* endpoint)
{
	ArpASSERT(endpoint);
	BRow*			row = new _EndpointRow(endpoint, -1, this);
	if (row) {
		AddRow(row);
		for (int32 k = 0; k < 16; k++) {
			BRow*	crow = new _EndpointRow(endpoint, k, this);
			if (crow) AddRow(crow, row);
		}
	}
}

void _EndpointList::DeleteRow(_EndpointRow* row)
{
	ArpVALIDATE(row && row->mEndpoint.channel < 0, return);
	AmGlobalsImpl*	globals = dynamic_cast<AmGlobalsImpl*>( &(AmGlobals()) );
	if (!globals) return;
	AmStudio&		studio = globals->Studio();
	studio.DeleteEndpoint(row->mEndpoint);
	RemoveRow(row);
	delete row;
}

static _EndpointRow* row_for_channel(BColumnListView* lv, BRow* parent, int32 channel)
{
	_EndpointRow*		r = NULL;
	for (int32 k = 0; (r = dynamic_cast<_EndpointRow*>(lv->RowAt(k, parent))) != NULL; k++) {
		if (r->mEndpoint.channel == channel) return r;
	}
	return NULL;
}

status_t _EndpointList::AddChildRow(am_studio_endpoint& endpoint, BString* label,
									BString* devMfg, BString* devName)
{
	/* If the parent doesn't already exist, create it.
	 */
	bool			create = false;
	_EndpointRow*	p = ParentNamed(endpoint.name, endpoint.type);
	if (!p) {
		p = new _EndpointRow(endpoint, -1, this, false);
		if (!p) return B_NO_MEMORY;
		AddRow(p);
		create = true;
	}
	/* If the child doesn't already exist, create it.
	 * This is hacked a bit because of a big in the CLV --
	 * you can't ask or count children or do ANYTHING with
	 * children on a row that doesn't have any.  It will crash.
	 */
	_EndpointRow*	r = NULL;
	if (!create) r = row_for_channel(this, p, endpoint.channel);
	if (!r) {
		BRow*		r = new _EndpointRow(endpoint, endpoint.channel, this, false);
		if (!r) return B_NO_MEMORY;
		AddRow(r, p);
	}
	return B_OK;
}

_EndpointRow* _EndpointList::ParentNamed(const BString& name, AmEndpointType type)
{
	_EndpointRow*	r = NULL;
	for (int32 k = 0; (r = dynamic_cast<_EndpointRow*>(RowAt(k))) != NULL; k++) {
		if (r->mEndpoint.name == name && r->mEndpoint.type == type) return r;
	}
	return NULL;
}

// #pragma mark -

/********************************************************
 * _ENDPOINT-ROW
 ********************************************************/
_EndpointRow::_EndpointRow(BMidiEndpoint* endpoint, int32 channel, _EndpointList* list)
			: mReadOnly(false), mIsValid(true), mList(list)
{
	ArpASSERT(endpoint);
	mEndpoint.name = endpoint->Name();
	if (endpoint->IsProducer() ) mEndpoint.type = AM_PRODUCER_TYPE;
	else mEndpoint.type = AM_CONSUMER_TYPE;
	mEndpoint.id = endpoint->ID();
	mEndpoint.channel = channel;
	Init();
}

_EndpointRow::_EndpointRow(	const am_studio_endpoint& endpoint, int32 channel, _EndpointList* list,
							bool isValid)
			: mReadOnly(false), mIsValid(isValid), mList(list)
{
	mEndpoint.name = endpoint.name;
	mEndpoint.type = endpoint.type;
	mEndpoint.id = endpoint.id;
	mEndpoint.channel = channel;
	Init();
}

bool _EndpointRow::HasChanges()
{
	BString		device;
	BString		label;
	GetLabellingInfo(device, label);
	if (device != NONE_STR) return true;
	if (label.Length() > 0) return true;
	return false;
}

void _EndpointRow::SelectionSetup(_EndpointList* target)
{
	ArpASSERT(target);
	BString		devLabel;
	BString		label;
	GetLabellingInfo(devLabel, label);
	if (target->mLabelTarget) target->mLabelTarget->SetText( label.String() );
	if (target->mDeviceTarget)
		arp_mark_item(devLabel.String(), target->mDeviceTarget->Menu() );
}

void _EndpointRow::SetIsValid(bool isValid)
{
	mIsValid = isValid;
	int32		count = CountFields();
	for (int32 k = 0; k < count; k++) {
		SeqColoredField*	field = dynamic_cast<SeqColoredField*>(GetField(k) );
		if (field) field->SetIsValid(isValid);
	}
}

void _EndpointRow::HandleLabelChange(const char* label)
{
	AmGlobalsImpl*	globals = dynamic_cast<AmGlobalsImpl*>( &(AmGlobals()) );
	if (!globals) return;
	AmStudio&		studio = globals->Studio();
	if (mEndpoint.channel < 0) {
		for (uchar k = 0; k < 16; k++) {
			am_studio_endpoint		endpoint(mEndpoint);
			endpoint.channel = k;
			studio.SetLabel(endpoint, label);
		}
	} else {
		studio.SetLabel(mEndpoint, label);
	}
	UpdateLabelColumn();
	if (mList) {
		BRow*		r;
		bool		is_visible;
		if (mList->FindParent(this, &r, &is_visible) == true) {
			_EndpointRow*	er = dynamic_cast<_EndpointRow*>(r);
			if (er) er->UpdateLabelColumn();
		}
		/* There's a bug in the column list view -- you can't ask for
		 * RowAt() with a parent if the parent has no children.  So I hack
		 * it: I know I have children only if my channel is -1.
		 */
		if (mEndpoint.channel < 0) {
			int32	count = mList->CountRows(this);
			for (int32 k = 0; k < count; k++) {
				_EndpointRow*	er = dynamic_cast<_EndpointRow*>(mList->RowAt(k, this));
				if (er) er->UpdateLabelColumn();
			}
		}
	}
}

void _EndpointRow::HandleDeviceChange(const char* devMfg, const char* devName)
{
	AmGlobalsImpl*	globals = dynamic_cast<AmGlobalsImpl*>( &(AmGlobals()) );
	if (!globals) return;
	AmStudio&		studio = globals->Studio();
	if (mEndpoint.channel < 0) {
		for (uchar k = 0; k < 16; k++) {
			am_studio_endpoint		endpoint(mEndpoint);
			endpoint.channel = k;
			studio.SetDevice(endpoint, devMfg, devName);
		}
	} else {
		studio.SetDevice(mEndpoint, devMfg, devName);
	}
	UpdateDeviceColumn();
	if (mList) {
		BRow*		r;
		bool		is_visible;
		if (mList->FindParent(this, &r, &is_visible) == true) {
			_EndpointRow*	er = dynamic_cast<_EndpointRow*>(r);
			if (er) er->UpdateDeviceColumn();
		}
		/* There's a bug in the column list view -- you can't ask for
		 * RowAt() with a parent if the parent has no children.  So I hack
		 * it: I know I have children only if my channel is -1.
		 */
		if (mEndpoint.channel < 0) {
			int32	count = mList->CountRows(this);
			for (int32 k = 0; k < count; k++) {
				_EndpointRow*	er = dynamic_cast<_EndpointRow*>(mList->RowAt(k, this));
				if (er) er->UpdateDeviceColumn();
			}
		}
	}
}

void _EndpointRow::UpdateDeviceColumn()
{
	BString		devLabel;
	BString		colLabel;
	GetLabellingInfo(devLabel, colLabel);
	SetField(new SeqColoredField(devLabel.String(), mReadOnly, mIsValid), 2);
	if (mList) mList->UpdateRow(this);
}

void _EndpointRow::UpdateLabelColumn()
{
	BString		devLabel;
	BString		colLabel;
	GetLabellingInfo(devLabel, colLabel);
	SetField(new SeqColoredField(colLabel.String(), mReadOnly, mIsValid), 3);
	if (mList) mList->UpdateRow(this);
}

void _EndpointRow::GetLabellingInfo(BString& outDeviceLabel, BString& outLabel) const
{
	BString			label;
	BString			devMfg;
	BString			devName;
	BString			devLabel;
	if (mEndpoint.channel >= 0) {
		if (AmGlobals().GetDeviceInfo(mEndpoint, devMfg, devName, label) == B_OK) {
			if (devMfg.Length() < 1 && devName.Length() < 1) devLabel = NONE_STR;
			else devLabel = AmDeviceI::MakeLabel(devMfg, devName);
		} else
			devLabel = NONE_STR;
	} else {
		int32		answer = GetAllChannelDeviceInfo(label, devMfg, devName);
		if (answer == 0) devLabel = NONE_STR;
		else if (answer == 1) devLabel = VARIOUS_STR;
		else devLabel = AmDeviceI::MakeLabel(devMfg, devName);
	}
	outDeviceLabel = devLabel;
	outLabel = label;
}

int32 _EndpointRow::GetAllChannelDeviceInfo(BString& label, BString& devMfg, BString& devName) const
{
	AmGlobalsImpl*		globals = dynamic_cast<AmGlobalsImpl*>( &(AmGlobals()) );
	if (!globals) return 0;
	AmStudio&			studio = globals->Studio();
	am_studio_endpoint	endpoint(mEndpoint);
	_DeviceCompare		first, second;

	for (int32 k = 0; k < 16; k++) {
		endpoint.channel = k;
		if ( !first.IsValid() ) first.GetInfo(studio, endpoint);
		else {
			second.GetInfo(studio, endpoint);
			if ( second.IsValid() ) {
				first.AddCompare(second);
			}
		}
	}
	if ( !first.IsValid() ) return 0;
	first.Fill(label, devMfg, devName);
	return first.Status();
}

void _EndpointRow::Init()
{
	if (mEndpoint.channel >= 0) {
		BString		str("Channel ");
		str << mEndpoint.channel + 1;
		SetField(new SeqColoredField(str.String(), mReadOnly, mIsValid), 0);
	} else {
		SetField(new SeqColoredField(mEndpoint.name.String(), mReadOnly, mIsValid), 0);
	}
	SetField(new SeqColoredField( (mEndpoint.type == AM_PRODUCER_TYPE) ? "In" : "Out", mReadOnly, mIsValid), 1);

	BString		devLabel;
	BString		label;
	GetLabellingInfo(devLabel, label);
	if (devLabel.Length() > 0)
		SetField(new SeqColoredField(devLabel.String(), mReadOnly, mIsValid), 2);
	if (label.Length() > 0)
		SetField(new SeqColoredField(label.String(), mReadOnly, mIsValid), 3);
}

