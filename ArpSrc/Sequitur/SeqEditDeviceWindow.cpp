#include <algo.h>
#include <app/Clipboard.h>
#include <experimental/ColumnListView.h>
#include <experimental/ColumnTypes.h>
#include <support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpInlineTextView.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmDevice.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmPhraseEvent.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqEditDeviceWindow.h"
#include "Sequitur/SeqBitmapEditor.h"

static const float		MIN_WIDTH = 50;
static const float		MIN_HEIGHT = 50;
static const float		INSET = 3;

static const uint32		MFG_MSG						= '#mfm';
static const uint32		NAME_MSG					= '#nam';
static const uint32		AUTHOR_MSG					= '#aut';
static const uint32		EMAIL_MSG					= '#eml';

static const uint32		BS_0_32_MSG					= 'b032';
static const uint32		BS_0_MSG					= 'b0__';
static const uint32		BS_32_MSG					= 'b32_';
static const uint32		BS_NONE_MSG					= 'bnon';
static const uint32		NEW_BANK_MSG				= 'nbnk';
static const uint32		DELETE_BANK_MSG				= 'iDBk';
static const uint32		MERGE_BANK_MSG				= 'iMBk';
static const uint32		PASTE_PATCH_LIST_MSG		= 'iPsP';
static const uint32		NEXT_BANK_VALUE_MSG			= 'iBkN';
static const uint32		PREV_BANK_VALUE_MSG			= 'iBkP';
static const uint32		MOD_BANK_VALUE_MSG			= 'iBkM';
static const uint32		CONTROL_VALUE_MSG			= 'iCcR';
static const uint32		MOD_CONTROL_VALUE_MSG		= 'iCcM';
static const uint32		DESCRIPTION_MOD_MSG			= 'iDsM';
static const uint32		BITMAP_CHANGE_MSG			= 'bmch';
static const uint32		NEW_ICON_MSG				= 'inic';
static const uint32		DELETE_ICON_MSG				= 'idic';
static const uint32		COPY_ICON_MSG				= 'icic';
static const uint32		PASTE_ICON_MSG				= 'ipic';
static const uint32		FLIP_VERTICALLY_ICON_MSG	= 'ifvi';
static const uint32		FLIP_HORIZONTALLY_ICON_MSG	= 'ifhi';
static const uint32		FILL_WITH_ALPHA_MSG			= 'ibfa';
static const uint32		INPUT_FILTER_MSG			= 'iInF';

static const char*		BANK_LIST_STR				= "bank_list";
static const char*		BANKS_STR					= "Banks";
static const char*		CONTROLLER_LIST_STR			= "controller_list";
static const char*		CONTROLLERS_STR				= "Controllers";
static const char*		DESCRIPTION_STR				= "Description";
static const char*		GENERAL_STR					= "General";
static const char*		ICON_STR					= "Icon";
static const char*		FILTER_STR					= "Filter";
static const char*		MERGE_FROM_STR				= "Merge From";
static const char*		PASTE_PATCH_LIST_STR		= "Paste Patch List";
static const char*		NAME_STR					= "Name";
static const char*		NUMBER_STR					= "Number";
static const char*		NONE_STR					= "None";

static void _set_device_for_input_filter(BMenu* m, const BString& key);

/********************************************************
 * _CONTROLLER-LIST
 ********************************************************/
class _ControllerList : public BColumnListView
{
public:
	_ControllerList(BRect frame);

	BTextControl*				mValueCtrl;

	void			SetDevice(ArpRef<AmDevice> device);
	void			SetTarget(BTextControl* valueCtrl);
	virtual void	SelectionChanged();
	/* Take the currently selected row's value and save
	 * it into the device.
	 */
	void			SaveSelection();
	
private:
	typedef BColumnListView		inherited;
	ArpRef<AmDevice>			mDevice;
};

/********************************************************
 * _CONTROLLER-ROW
 ********************************************************/
class _ControllerRow : public BRow
{
public:
	_ControllerRow(ArpRef<AmDevice> device, uint32 number);
	virtual ~_ControllerRow();

	virtual bool		HasLatch() const		{ return false; }
	virtual void		SelectionSetup(BTextControl* valueCtrl);
	status_t			SaveSelection(BTextControl* valueCtrl);

private:
	ArpRef<AmDevice>	mDevice;
	uint32				mNumber;
};

/********************************************************
 * _BANK-LIST
 ********************************************************/
class _BankList : public BColumnListView
{
public:
	_BankList(BRect frame);

	BTextControl*				mValueCtrl;

	void			SetDevice(ArpRef<AmDevice> device);
	void			SetTarget(BTextControl* valueCtrl);
	virtual void	SelectionChanged();
	/* Take the currently selected row's value and save
	 * it into the device.
	 */
	void			SaveSelection();
	/* Find the currently selected row and select the
	 * next visible row.
	 */
	void			SelectNext();
	void			SelectPrev();

private:
	typedef BColumnListView		inherited;
	ArpRef<AmDevice>			mDevice;

	void			UpdateAll(ArpRef<AmBank> bank, BRow* parent = NULL);
};

/********************************************************
 * _ABSTRACT-BANK-ROW
 ********************************************************/
class _AbstractBankRow : public BRow
{
public:
	ArpRef<AmBank>		Bank() const;

	virtual void		SelectionSetup(BTextControl* valueCtrl);
	/* If subclasses supply 'true' to updateAll, then all rows
	 * with my bank will be updated.
	 */
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll) = 0;
	virtual void		SetNameField()		{ }
		
protected:
	_AbstractBankRow(ArpRef<AmBank> bank);

	ArpRef<AmBank>		mBank;
};

ArpRef<AmBank>
_AbstractBankRow::Bank() const
{
	return mBank;
}
/********************************************************
 * _BANK-ROW
 ********************************************************/
class _BankRow : public _AbstractBankRow
{
public:
	_BankRow(	ArpRef<AmBank> bank,
				BColumnListView* owner);

	virtual bool		HasLatch() const		{ return true; }
	virtual void		SelectionSetup(BTextControl* valueCtrl);
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll);
	virtual void		SetNameField();

private:
	typedef _AbstractBankRow inherited;
};

/********************************************************
 * _BANK-SELECTION-ROW
 ********************************************************/
class _BankSelectionRow : public _AbstractBankRow
{
public:
	_BankSelectionRow(	ArpRef<AmBank> bank,
						BColumnListView* owner, BRow* parent);

	virtual bool		HasLatch() const		{ return true; }
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll);

private:
	typedef _AbstractBankRow inherited;
};

/********************************************************
 * _BANK-FILTER
 ********************************************************/
class _BankFilter : public BMessageFilter
{
public:
	_BankFilter();

	virtual filter_result Filter(BMessage *message, BHandler **target);
};

/********************************************************
 * _CONTROL-CHANGE-ROW
 ********************************************************/
class _ControlChangeRow : public _AbstractBankRow
{
public:
	_ControlChangeRow(	AmControlChange* event,
						BColumnListView* owner, BRow* parent);
	virtual ~_ControlChangeRow();
	
	virtual bool		HasLatch() const		{ return false; }
	virtual void		SelectionSetup(BTextControl* valueCtrl);
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll);
	virtual void		SetNameField();

private:
	typedef _AbstractBankRow inherited;

	AmControlChange*	mEvent;
};

/********************************************************
 * _BANK-PATCHES-ROW
 ********************************************************/
class _BankPatchesRow : public _AbstractBankRow
{
public:
	_BankPatchesRow(ArpRef<AmBank> bank,
					BColumnListView* owner, BRow* parent);

	virtual bool		HasLatch() const		{ return true; }
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll);

private:
	typedef _AbstractBankRow inherited;
};

/********************************************************
 * _PATCH-ROW
 ********************************************************/
class _PatchRow : public _AbstractBankRow
{
public:
	_PatchRow(	ArpRef<AmBank> bank, uint32 number,
				BColumnListView* owner, BRow* parent);

	virtual bool		HasLatch() const		{ return false; }
	virtual void		SelectionSetup(BTextControl* valueCtrl);
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll);
	virtual void		SetNameField();

	uint32				Number() const;
	
private:
	typedef _AbstractBankRow inherited;
	uint32				mNumber;
};

/********************************************************
 * _NUMBER-PATCHES-ROW
 ********************************************************/
class _NumberPatchesRow : public _AbstractBankRow
{
public:
	_NumberPatchesRow(	ArpRef<AmBank> bank,
						BColumnListView* owner, BRow* parent);

	virtual bool		HasLatch() const		{ return false; }
	virtual void		SelectionSetup(BTextControl* valueCtrl);
	virtual status_t	SaveSelection(BTextControl* valueCtrl, bool* updateAll);
	virtual void		SetNameField();

private:
	typedef _AbstractBankRow inherited;
};

/*************************************************************************
 * SEQ-EDIT-DEVICE-WINDOW
 *************************************************************************/
SeqEditDeviceWindow::SeqEditDeviceWindow(	BRect frame,
											const BMessage* config,
											const BMessage* deviceMsg)
		: inherited(frame, "Edit device"),
		  mDevice(NULL),
		  mMfgCtrl(NULL), mNameCtrl(NULL),
		  mBankSelectionCtrl(NULL), mBankMenu(NULL),
		  mBankList(0), mControllerList(0), mIconEditor(0),
		  mInputFilterMenu(0)
{
	BRect		targetF(CurrentPageFrame() );
	BView*		view = NULL;
	if ((view = NewGeneralView(targetF))) AddPage(view);
	if ((view = NewBankView(targetF))) AddPage(view);
	if ((view = NewControllersView(targetF))) AddPage(view);
	if ((view = NewDescriptionView(targetF))) AddPage(view);
	if ((view = NewIconView(targetF))) AddPage(view);
	if ((view = NewFilterView(targetF))) AddPage(view);

	if (config) SetConfiguration(config);
	else SetFirstPage();
	if (deviceMsg) SetDevice(deviceMsg);
	
	AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO), (BView *)NULL);
}

SeqEditDeviceWindow::~SeqEditDeviceWindow()
{
}

static AmBankChange* new_0_32_bank()
{
	AmControlChange*	cc1 = new AmControlChange(0, 0, 0);
	AmControlChange*	cc2 = new AmControlChange(32, 0, 1);
	AmProgramChange*	pc = new AmProgramChange(0, 2);
	AmBankChange*		be = new AmBankChange();
	if (!cc1 || !cc2 || !pc || !be) {
		if (cc1) cc1->Delete();
		if (cc2) cc2->Delete();
		if (pc) pc->Delete();
		if (be) be->Delete();
		return NULL;
	}
	be->Add(cc1);
	be->Add(cc2);
	be->Add(pc);
	return be;
}

static AmBankChange* new_0_bank()
{
	AmControlChange*	cc = new AmControlChange(0, 0, 0);
	AmProgramChange*	pc = new AmProgramChange(0, 1);
	AmBankChange*		be = new AmBankChange();
	if (!cc || !pc || !be) {
		if (cc) cc->Delete();
		if (pc) pc->Delete();
		if (be) be->Delete();
		return NULL;
	}
	be->Add(cc);
	be->Add(pc);
	return be;
}

static AmBankChange* new_32_bank()
{
	AmControlChange*	cc = new AmControlChange(32, 0, 0);
	AmProgramChange*	pc = new AmProgramChange(0, 1);
	AmBankChange*		be = new AmBankChange();
	if (!cc || !pc || !be) {
		if (cc) cc->Delete();
		if (pc) pc->Delete();
		if (be) be->Delete();
		return NULL;
	}
	be->Add(cc);
	be->Add(pc);
	return be;
}

static bool sort_items(BMenuItem* item1, BMenuItem* item2)
{
	if (item1->Label() == NULL) return false;
	if (item2->Label() == NULL) return true;
	BString		str(item1->Label() );
	if (str.ICompare(item2->Label() ) <= 0) return true;
	return false;
}

void SeqEditDeviceWindow::MenusBeginning()
{
	inherited::MenusBeginning();
	if (!mBankMenu || !mBankMenu->Menu() ) return;

	bool		enabled = false;
	BString		deviceKey;
	if (mDevice && mBankList) {
		deviceKey = mDevice->Key();
		_BankRow*	r = dynamic_cast<_BankRow*>(mBankList->CurrentSelection() );
		if (r) enabled = true;
	}
	BMenuItem*	item = mBankMenu->Menu()->FindItem(DELETE_BANK_MSG);
	if (item && item->IsEnabled() != enabled) item->SetEnabled(enabled);

	item = mBankMenu->Menu()->FindItem(MERGE_FROM_STR);
	if (item && item->Submenu() ) {
		item->Submenu()->RemoveItems(0, item->Submenu()->CountItems(), true);
		AmDeviceRoster*				roster = AmDeviceRoster::Default();
		if (roster) {
			vector<BMenuItem*>		items;
			BString					mfg, name, key;
			bool					isValid;
			for (uint32 k = 0; roster->GetDeviceInfo(k, mfg, name, key, NULL, &isValid) == B_OK; k++) {
				if (isValid && deviceKey != key) {
					BString			label = AmDeviceI::MakeLabel(mfg, name);
					BMessage*		msg = new BMessage(MERGE_BANK_MSG);
					if (msg && msg->AddString("key", key) == B_OK) {
						BMenuItem*	newItem = new BMenuItem(label.String(), msg);
						if (newItem) items.push_back(newItem);
					}
				}
			}
			sort(items.begin(), items.end(), sort_items);
			for (uint32 k = 0; k < items.size(); k++) item->Submenu()->AddItem(items[k]);
			items.resize(0);
		}
	}

	item = mBankMenu->Menu()->FindItem(PASTE_PATCH_LIST_MSG);
	if (item && mBankList) {
		_PatchRow*	r = dynamic_cast<_PatchRow*>(mBankList->CurrentSelection() );
		if (r && item->IsEnabled() == false) item->SetEnabled(true);
		else if (!r && item->IsEnabled() ) item->SetEnabled(false);
	}
}

void SeqEditDeviceWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SHOW_EDIT_DEVICE_MSG:
			SetDevice(msg);
			break;
		case NAME_MSG: {
			if (mDevice && mNameCtrl && mDevice->Product() != mNameCtrl->Text() ) {
				mDevice->SetName(mNameCtrl->Text() );
				SetWindowTitle();
				SetHasChanges(true);
			}
		} break;
		case MFG_MSG: {
			if (mDevice && mMfgCtrl && mDevice->Manufacturer() != mMfgCtrl->Text() ) {
				mDevice->SetManufacturer(mMfgCtrl->Text() );
				SetWindowTitle();
				SetHasChanges(true);
			}
		} break;
		case AUTHOR_MSG: {
			if (mDevice && mAuthorCtrl) {
				mDevice->SetAuthor(mAuthorCtrl->Text() );
				SetHasChanges(true);
			}
		} break;
		case EMAIL_MSG: {
			if (mDevice && mEmailCtrl) {
				mDevice->SetEmail(mEmailCtrl->Text() );
				SetHasChanges(true);
			}
		} break;
		case BS_0_32_MSG: {
			AmBankChange*	be = new_0_32_bank();
			SetBankSelection(be);
			if (be) be->Delete();
		} break;
		case BS_0_MSG: {
			AmBankChange*	be = new_0_bank();
			SetBankSelection(be);
			if (be) be->Delete();
		} break;
		case BS_32_MSG: {
			AmBankChange*	be = new_32_bank();
			SetBankSelection(be);
			if (be) be->Delete();
		} break;
		case BS_NONE_MSG: {
			SetBankSelection(NULL);
		} break;
		case NEW_BANK_MSG:
			NewBank();
			break;
		case DELETE_BANK_MSG: {
			if (mDevice && mBankList) {
				_BankRow*	selection = dynamic_cast<_BankRow*>(mBankList->CurrentSelection() );
				if (selection && selection->Bank() ) {
					if (mDevice->RemoveBank(selection->Bank()) == B_OK) {
						mBankList->SetDevice(mDevice);
						SetHasChanges(true);
					}
				}
			}
		} break;
		case MERGE_BANK_MSG: {
			const char*		s;
			AmDeviceRoster*	roster = AmDeviceRoster::Default();
			if (roster && msg->FindString("key", &s) == B_OK) {
				BString		key(s);
				ArpRef<AmDevice>	d = roster->NewDevice(key);
				if (d) {
					uint32	count = d->CountBanks();
					for (uint32 k = 0; k < count; k++) {
						ArpRef<AmBank>	b = d->Bank(k);
						AmBank*			newBank = NULL;
						if (b && (newBank = b->Copy()) ) mDevice->AddBank(newBank);
					}
					mBankList->SetDevice(mDevice);
					SetHasChanges(true);
				}
			}
		} break;
		case PASTE_PATCH_LIST_MSG: {
			if (mDevice && mBankList) {
				_PatchRow*		selection = dynamic_cast<_PatchRow*>(mBankList->CurrentSelection() );
				if (selection) {
					bool					isVisible;
					BRow*					row;
					_AbstractBankRow*		ab;
					if (mBankList->FindParent(selection, &row, &isVisible) == true
							&& (ab = dynamic_cast<_AbstractBankRow*>(row)) != NULL) {
						PastePatchList(ab, selection->Number() );
					}
				}
			}
		} break;
		case NEXT_BANK_VALUE_MSG: {
			if (mBankList) mBankList->SelectNext();
		} break;
		case PREV_BANK_VALUE_MSG: {
			if (mBankList) mBankList->SelectPrev();
		} break;
		case MOD_BANK_VALUE_MSG: {
			if (mBankList) {
				mBankList->SaveSelection();
				SetHasChanges(true);
			}
		} break;
		case CONTROL_VALUE_MSG: {
			if (mControllerList) {
				BRow*	selection = mControllerList->CurrentSelection();
				int32	next = 0;
				if (selection) next = mControllerList->IndexOf(selection) + 1;
				selection = mControllerList->RowAt(next);
				if (!selection) selection = mControllerList->RowAt(int32(0) );
				if (selection) mControllerList->AddToSelection(selection);
				mControllerList->SelectionChanged();
			}
		} break;
		case MOD_CONTROL_VALUE_MSG: {
			if (mControllerList) {
				mControllerList->SaveSelection();
				SetHasChanges(true);
			}
		} break;
		case DESCRIPTION_MOD_MSG: {
			SetHasChanges(true);
		} break;
		case NEW_ICON_MSG: {
			if (mDevice && mIconEditor) {
				BBitmap*	bm = new BBitmap(mDevice->IconBounds(), B_RGBA32);
				if (bm) {
					mDevice->SetIcon(bm);
					mIconEditor->SetBitmap( mDevice->Icon(BPoint(20, 20)) );
					delete bm;
					SetHasChanges(true);
				}
			}
		} break;
		case DELETE_ICON_MSG: {
			if (mDevice && mIconEditor) {
				mDevice->SetIcon(NULL);
				mIconEditor->SetBitmap(NULL);
				SetHasChanges(true);
			}
		} break;
		case COPY_ICON_MSG: {
			if (mIconEditor) mIconEditor->Copy();
		} break;
		case PASTE_ICON_MSG: {
			if (mIconEditor) mIconEditor->Paste();
		} break;
		case FLIP_VERTICALLY_ICON_MSG: {
			if (mIconEditor) mIconEditor->FlipVertically();
		} break;
		case FLIP_HORIZONTALLY_ICON_MSG: {
			if (mIconEditor) mIconEditor->FlipHorizontally();
		} break;
		case FILL_WITH_ALPHA_MSG: {
			if (mIconEditor) mIconEditor->FillAlpha();
		} break;
		case BITMAP_CHANGE_MSG: {
			bool	b;
			if (msg->FindBool("bitmap changes", &b) == B_OK && b)
				SetHasChanges(true);
		} break;
		case INPUT_FILTER_MSG: {
			if (mDevice) {
				const char*		key = 0;
				if (msg->FindString(SZ_FILTER_KEY, &key) != B_OK) key = 0;
				mDevice->SetInputFilterKey(key);
				SetHasChanges(true);
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqEditDeviceWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::EDIT_DEVICE_WIN_INDEX, config);
	}
	return true;
}

void SeqEditDeviceWindow::SetDevice(const BMessage* deviceMsg)
{
	if (!deviceMsg) return;
	const char*		path;
	if (deviceMsg->FindString("path", &path) != B_OK) path = NULL;
	const char*		deviceName;
	if (deviceMsg->FindString("device_unique_name", &deviceName) == B_OK)
		SetDevice(deviceName, path);
}

static uint32 what_for_bank_change(const AmBankChange* be)
{
	if (!be || !be->Phrase() ) return BS_NONE_MSG;
	const AmNode*	e1 = be->Phrase()->HeadNode();
	if (!e1) return BS_NONE_MSG;
	const AmNode*	e2 = e1->next;
	if (!e2 || e2->Event()->Type() == AmEvent::PROGRAMCHANGE_TYPE) {
		if (e1->Event()->Type() == AmEvent::CONTROLCHANGE_TYPE) {
			AmControlChange*	cc = dynamic_cast<AmControlChange*>(e1->Event() );
			if (cc && cc->ControlNumber() == 0) return BS_0_MSG;
			else if (cc && cc->ControlNumber() == 32) return BS_32_MSG;
		}
		return BS_NONE_MSG;
	}
	if (e2->next && e2->next->Event()->Type() != AmEvent::PROGRAMCHANGE_TYPE)
		return BS_NONE_MSG;
	if (e1->Event()->Type() == AmEvent::CONTROLCHANGE_TYPE
			&& e2->Event()->Type() == AmEvent::CONTROLCHANGE_TYPE) {
		AmControlChange*	cc1 = dynamic_cast<AmControlChange*>(e1->Event() );
		AmControlChange*	cc2 = dynamic_cast<AmControlChange*>(e2->Event() );
		if (cc1 && cc1->ControlNumber() == 0 && cc2 && cc2->ControlNumber() == 32)
			return BS_0_32_MSG;
	}
	return BS_NONE_MSG;
}

void SeqEditDeviceWindow::SetDevice(const BString& key, const BString& path)
{
	if (!SetEntryCheck() ) return;
	if (!Lock()) return;

	mInitialKey = (const char*)NULL;
	mInitialAuthor = (const char*)NULL;
	mInitialEmail = (const char*)NULL;

	const char*		s = NULL;
	if (path.Length() > 0) s = path.String();

	mDevice = NULL;
	AmDeviceRoster*		roster = AmDeviceRoster::Default();
	if (roster) mDevice = roster->NewDevice(key, s);
	if (mDevice) mInitialKey = mDevice->Key();
	else {
		const char*		auth;
		const char*		email;
		if (seq_get_string_preference(AUTHOR_PREF, &auth) != B_OK) auth = NULL;
		if (seq_get_string_preference(EMAIL_PREF, &email) != B_OK) email = NULL;
		mDevice = new AmDevice(NULL, auth, email);
		if (mDevice) {
			AmBankChange*	be = NewBankSelection();
			mDevice->SetBankSelection(be);
			if (be) be->Delete();
		}
	}
	SetHasChanges(false);

	if (mIconEditor) mIconEditor->SetBitmap(NULL);
	
	if (mDevice) {
		mInitialAuthor = mDevice->Author();
		mInitialEmail = mDevice->Email();

		SetTextControl(mMfgCtrl, mDevice->Manufacturer().String(), MFG_MSG);
		SetTextControl(mNameCtrl, mDevice->Product().String(), NAME_MSG);
		SetTextControl(mAuthorCtrl, mDevice->Author().String(), AUTHOR_MSG);
		SetTextControl(mEmailCtrl, mDevice->Email().String(), EMAIL_MSG);
		if (mShortDescriptionCtrl) mShortDescriptionCtrl->SetText(mDevice->ShortDescription().String() );
		if (mIconEditor) mIconEditor->SetBitmap( mDevice->Icon(BPoint(20, 20)) );
		if (mBankSelectionCtrl && mBankSelectionCtrl->Menu() ) {
			BMenuItem*	item = mBankSelectionCtrl->Menu()->FindItem(what_for_bank_change(mDevice->BankSelection() ));
			if (item) item->SetMarked(true);
		}
		if (mBankList) mBankList->SetDevice(mDevice);
		if (mControllerList) mControllerList->SetDevice(mDevice);

		if (mInputFilterMenu) {
			BString			inputFilterKey = mDevice->InputFilterKey();
			_set_device_for_input_filter(mInputFilterMenu, inputFilterKey);
		}
	}
	SetWindowTitle();
	Unlock();
}

uint32 SeqEditDeviceWindow::ConfigWhat() const
{
	return EDIT_DEVICE_WINDOW_SETTING_MSG;
}

bool SeqEditDeviceWindow::Validate()
{
	if (!mDevice) return ReportError("There is no device");

	BString			key = mDevice->Key();	
	if (key.Length() < 1) return ReportError("This device must have a name or manufacturer");
	if (mInitialKey.Length() < 1 || mInitialKey != key) {
		AmDeviceRoster*	roster = AmDeviceRoster::Default();
		if (!roster) return ReportError();
		if (roster && roster->KeyExists(key.String() ) ) {
			BString	error = "There is already a device with the key \'";
			error << key.String() << "\'";
			return ReportError(error.String() );
		}
	}	
	return true;
}

void SeqEditDeviceWindow::SaveChanges()
{
	if (!HasChanges() || !mDevice) return;

	SetHiddenPrefs();

	if (mShortDescriptionCtrl) mDevice->SetShortDescription(mShortDescriptionCtrl->Text() );

	AmDeviceRoster*		roster = AmDeviceRoster::Default();
	if (roster) roster->CreateEntry(mDevice);
	SetHasChanges(false);
}

const char* SeqEditDeviceWindow::EntryName() const
{
	return "device";
}

void SeqEditDeviceWindow::SetBankSelection(const AmBankChange* bankEvent)
{
	if (!mDevice) return;
	mDevice->SetBankSelection(bankEvent);
	SetHasChanges(true);
	if (mBankList) mBankList->SetDevice(mDevice);
}

void SeqEditDeviceWindow::NewBank()
{
	if (!mDevice) return;
	AmBank*		bank = new AmBank();
	if (bank) {
		mDevice->AddBank(bank);
		if (mBankList) mBankList->SetDevice(mDevice);
		SetHasChanges(true);
	}
}

AmBankChange* SeqEditDeviceWindow::NewBankSelection()
{
	BMenuItem*		item = NULL;
	if (!mBankSelectionCtrl || !mBankSelectionCtrl->Menu()
				|| ((item = mBankSelectionCtrl->Menu()->FindMarked()) == NULL)
				|| !item->Message() )
		return NULL;
	uint32			what = item->Message()->what;

	if (what == BS_0_32_MSG) return new_0_32_bank();
	else if (what == BS_0_MSG) return new_0_bank();
	else if (what == BS_32_MSG) return new_32_bank();
	else return NULL;
}

void SeqEditDeviceWindow::SetWindowTitle()
{
	BString		title("Edit ");
	if (mDevice) {
		BString	label = AmDeviceI::MakeLabel(mDevice->Manufacturer(), mDevice->Product() );
		title << label;
		if (label.Length() > 0) title << " ";
	}
	title << "Device";
	SetTitle( title.String() );
}

static int32 next_token(const char* text, int32 pos, int32 length)
{
	if (pos >= length) return -1;
	int32		nl = 0;
	for (int32 k = pos; k < length && text[k] != '\n' && text[k] != 13; k++) nl++;
	return nl;
}

status_t SeqEditDeviceWindow::PastePatchList(_AbstractBankRow* parent, uint32 firstPatch)
{
	ArpASSERT(parent && mBankList);
	ArpRef<AmBank>		bank = parent->Bank();
	if (!bank) return B_ERROR;
	if (!be_clipboard->Lock() ) return B_ERROR;
	BMessage*			clip = be_clipboard->Data();
	if (!clip) {
		be_clipboard->Unlock();
		return B_ERROR;
	}

	uint32				patches = bank->CountPatches();
	const char*			text = NULL;
	int32				len; 
	if (clip->FindData("text/plain", B_MIME_TYPE, (const void **)&text, &len) == B_OK) {
		int32			pos = 0;
		int32			length = -1;
		while ( (length = next_token(text, pos, len)) > 0) {
			if (firstPatch < patches) {
				BString		str;
				str.Insert(text, pos, length, 0);
				bank->SetPatchName(firstPatch, str.String() );
				_AbstractBankRow*	row = dynamic_cast<_AbstractBankRow*>(mBankList->RowAt(firstPatch, parent) );
				if (row) {
					row->SetNameField();
					mBankList->UpdateRow(row);
				}
				firstPatch++;
			}
			pos += length + 1;
			while (pos < len && (text[pos] == '\n' || text[pos] == 13) ) pos++;
		}
	}

	be_clipboard->Unlock();
	return B_OK;

}

BView* SeqEditDeviceWindow::NewGeneralView(BRect frame)
{
	BView*		v = new BView(frame, GENERAL_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	float		divider = v->StringWidth("Manufacturer:") + 10;
	/* The manufacturer and name fields.
	 */
	BRect		f(spaceX, 0, frame.Width(), fh);
	mNameCtrl = new BTextControl(f, "name_ctrl", "Name:", NULL, new BMessage(NAME_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mNameCtrl) {
		f.top = mNameCtrl->Frame().bottom;
		mNameCtrl->SetDivider(divider);
		v->AddChild(mNameCtrl);
	}
	f.top += spaceY;
	f.bottom = f.top + fh;
	mMfgCtrl = new BTextControl(f, "mfg_ctrl", "Manufacturer:", NULL, new BMessage(MFG_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mMfgCtrl) {
		f.top = mMfgCtrl->Frame().bottom;
		mMfgCtrl->SetDivider(divider);
		mMfgCtrl->MakeFocus(true);
		mMfgCtrl->SetModificationMessage(new BMessage(MFG_MSG));
		v->AddChild(mMfgCtrl);
	}
	/* The Author field.
	 */
	f.top += spaceY;
	f.bottom = f.top + fh;
	mAuthorCtrl = new BTextControl(f, "author_ctrl", "Author:", NULL, new BMessage(AUTHOR_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mAuthorCtrl) {
		f.top = mAuthorCtrl->Frame().bottom;
		mAuthorCtrl->SetDivider(divider);
		v->AddChild(mAuthorCtrl);
	}
	/* The Email field.
	 */
	f.top += spaceY;
	f.bottom = f.top + fh;
	mEmailCtrl = new BTextControl(f, "author_ctrl", "Email:", NULL, new BMessage(EMAIL_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mEmailCtrl) {
		f.top = mEmailCtrl->Frame().bottom;
		mEmailCtrl->SetDivider(divider);
		v->AddChild(mEmailCtrl);
	}
	return v;
}

static BMenuField* new_bank_selection_control(BRect f)
{
	f.right += 300;
	BMenu*		menu = new BMenu("bank_selection_menu");
	BMenuItem*	item1 = new BMenuItem("Controllers 0, 32", new BMessage(BS_0_32_MSG));
	BMenuItem*	item2 = new BMenuItem("Controller 0", new BMessage(BS_0_MSG));
	BMenuItem*	item3 = new BMenuItem("Controller 32", new BMessage(BS_32_MSG));
	BMenuItem*	item4 = new BMenuItem("None", new BMessage(BS_NONE_MSG));
	if (!menu || !item1 || !item2 || !item3 || !item4) {
		delete menu;
		delete item1;
		delete item2;
		delete item3;
		delete item4;
		return NULL;
	}
	menu->AddItem(item1);
	menu->AddItem(item2);
	menu->AddItem(item3);
	menu->AddItem(item4);
	item1->SetMarked(true);
	menu->SetLabelFromMarked(true);
	
	BMenuField*	field = new BMenuField(f, "bank_selection_field", "Select banks with:", menu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

static BMenuField* new_bank_menu(BRect f)
{
	BMenu*		menu = new BMenu("Bank");
	BMenuItem*	item1 = new BMenuItem("New", new BMessage(NEW_BANK_MSG) );
	BMenuItem*	item2 = new BMenuItem("Delete", new BMessage(DELETE_BANK_MSG) );
	BMenuItem*	item3 = new BMenuItem(new BMenu(MERGE_FROM_STR), new BMessage(MERGE_BANK_MSG) );
	BMenuItem*	item4 = new BMenuItem(PASTE_PATCH_LIST_STR, new BMessage(PASTE_PATCH_LIST_MSG) );
	if (!menu || !item1 || !item2 || !item3 || !item4) {
		delete menu;
		delete item1;
		delete item2;
		delete item3;
		delete item4;
		return NULL;
	}
	menu->AddItem(item1);
	menu->AddItem(item2);
	menu->AddItem(item3);
	menu->AddSeparatorItem();
	menu->AddItem(item4);
	menu->SetLabelFromMarked(false);
	
	BMenuField*	field = new BMenuField(f, "bank_field", "", menu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

BView* SeqEditDeviceWindow::NewBankView(BRect frame)
{
	BView*		v = new BView(frame, BANKS_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	const char*	label = "Select banks with:";
	float		divider = v->StringWidth(label) + 10;
	/* The Bank selection field.
	 */
	BRect		f(spaceX, 0, frame.Width(), Prefs().Size(MENUFIELD_Y) );
	mBankSelectionCtrl = new_bank_selection_control(f);
	if (mBankSelectionCtrl) {
		mBankSelectionCtrl->SetDivider(divider);
		v->AddChild(mBankSelectionCtrl);
		f.top = mBankSelectionCtrl->Frame().bottom;
	}
	/* The Bank control.
	 */
	f.top += spaceY + spaceY;
	f.bottom = f.top + fh;
	mBankMenu = new_bank_menu(f);
	if (mBankMenu) {
		mBankMenu->SetDivider(0);
		v->AddChild(mBankMenu);
		f.top = mBankMenu->Frame().bottom;
	}
	/* The bank list view.
	 */
	f.top += spaceY + spaceY + spaceY;
	f.bottom = frame.Height() - spaceY - fh - spaceY - spaceY;
	f.right = frame.Width() - spaceX;
	mBankList = new _BankList(f);
	if (mBankList) v->AddChild(mBankList);

	/* The bank text control.
	 */
	f.top = f.bottom + spaceY;
	f.bottom = frame.Height() - spaceY;
	label = "Value:";
	BTextControl*		valueCtrl = new BTextControl(	f, "bank_value_ctrl", label, NULL,
														NULL,
														B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	if (valueCtrl) {
		valueCtrl->TextView()->AddFilter(new _BankFilter);
		valueCtrl->TextView()->SetDoesUndo(true);
		valueCtrl->SetDivider(v->StringWidth(label) + 10);
		valueCtrl->SetModificationMessage(new BMessage(MOD_BANK_VALUE_MSG) );
		valueCtrl->SetEnabled(false);
		v->AddChild(valueCtrl);
	}

	if (mBankList && valueCtrl)
		mBankList->SetTarget(valueCtrl);

	return v;
}

BView* SeqEditDeviceWindow::NewControllersView(BRect frame)
{
	BView*				v = new BView(frame, CONTROLLERS_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float				fh = arp_get_font_height(v);
	float				spaceX = 5, spaceY = 5;
	/* The controller list view.
	 */
	BRect				f(spaceX, 0, frame.Width(), frame.Height() - spaceY - fh - spaceY - spaceY);
	mControllerList = new _ControllerList(f);
	if (mControllerList) v->AddChild(mControllerList);
	/* The controller value control.
	 */
	f.top = f.bottom + spaceY;
	f.bottom = frame.Height() - spaceY;
	const char*			label = "Name:";
	BTextControl*		ccValueCtrl = new BTextControl(	f, "cc_value_ctrl", label, NULL,
														new BMessage(CONTROL_VALUE_MSG),
														B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	if (ccValueCtrl) {
		ccValueCtrl->SetDivider(v->StringWidth(label) + 10);
		ccValueCtrl->SetModificationMessage(new BMessage(MOD_CONTROL_VALUE_MSG) );
		ccValueCtrl->SetEnabled(false);
		v->AddChild(ccValueCtrl);
	}

	if (mControllerList && ccValueCtrl)
		mControllerList->SetTarget(ccValueCtrl);

	return v;
}

BView* SeqEditDeviceWindow::NewDescriptionView(BRect frame)
{
	BView*			v = new BView(frame, DESCRIPTION_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	float			fh = arp_get_font_height(v);
	float			spaceX = 5, spaceY = 5;
	BRect			shortLabelR(spaceX, 0, frame.Width(), spaceY + fh);
	BRect			shortR(spaceX, shortLabelR.bottom + spaceY, shortLabelR.right - Prefs().Size(V_SCROLLBAR_X) - 4,
							frame.Height() - 2);

	BStringView*	sv = new BStringView(shortLabelR, "short_label", "Short description:");
	if (sv) v->AddChild(sv);

	mShortDescriptionCtrl = new SeqDumbTextView(shortR, "short_descr", BRect(5, 5, shortR.Width() - 10, 0), B_FOLLOW_ALL);
	if (mShortDescriptionCtrl) {
		mShortDescriptionCtrl->SetModificationMessage(new BMessage(DESCRIPTION_MOD_MSG) );
		BScrollView*		sv = new BScrollView("short_scroll", mShortDescriptionCtrl,
												 B_FOLLOW_ALL, 0, false, true);
		if (sv) v->AddChild(sv);
		else v->AddChild(mShortDescriptionCtrl);
	}
	return v;
}

static BMenu* new_icon_editor_menu()
{
	BMenu*		menu = new BMenu("Icon");
	if (!menu) return NULL;
	BMenuItem*	item = new BMenuItem("New", new BMessage(NEW_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);
	item = new BMenuItem("Delete", new BMessage(DELETE_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem("Copy", new BMessage(COPY_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);	
	item = new BMenuItem("Paste", new BMessage(PASTE_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);	
	menu->AddSeparatorItem();
	item = new BMenuItem("Flip Vertically", new BMessage(FLIP_VERTICALLY_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);	
	item = new BMenuItem("Flip Horizontally", new BMessage(FLIP_HORIZONTALLY_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);	
	menu->AddSeparatorItem();
	item = new BMenuItem("Fill with Alpha", new BMessage(FILL_WITH_ALPHA_MSG), 0, 0);
	if (item) menu->AddItem(item);	
	return menu;
}

BView* SeqEditDeviceWindow::NewIconView(BRect frame)
{
	BView*		v = new BView(frame, ICON_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	BRect	f(5, 0, frame.Width(), frame.Height() );
	mIconEditor = new SeqBitmapEditor(f, "bitmap_editor", NULL, B_FOLLOW_ALL, new_icon_editor_menu() );
	if (mIconEditor) {
		mIconEditor->SetBitmapChangeMessage( new BMessage(BITMAP_CHANGE_MSG) );
		mIconEditor->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		v->AddChild(mIconEditor);
	}
	return v;
}

static inline bool accept_for_input(AmFilterAddOnHandle* handle)
{
	ArpASSERT(handle);
	if (handle->Type() != AmFilterAddOn::THROUGH_FILTER) return false;
	if (handle->Subtype() == AmFilterAddOn::TOOL_SUBTYPE) return false;
	return true;
}

static bool accept_for_input(AmMultiFilterAddOn* addon)
{
	ArpASSERT(addon);
	if (addon->Type() != AmFilterAddOn::THROUGH_FILTER) return false;
	if (addon->Subtype() == AmFilterAddOn::TOOL_SUBTYPE) return false;
	return true;
}

static BMenu* new_filter_menu(	AmPipelineType pipelineType,
								BHandler* target)
{
	AmFilterRoster*				roster = AmFilterRoster::Default();
	BMenu*						menu = new BMenu("Filters");
	if (!menu) return 0;
	vector<BMenuItem*>			items;
	BMenuItem*					item;

	roster->Locker()->Lock();
	const int32					N = roster->CountAddOns();
	for (int32 k = 0; k < N; k++) {
		AmFilterAddOnHandle*	handle = dynamic_cast<AmFilterAddOnHandle*>(roster->AddOnAt(k));
		if (handle && accept_for_input(handle)) {
			BString				key, label;
			if (handle->AddOn() ) {
				key = handle->AddOn()->Key();
				handle->AddOn()->GetLabel(label, true);
			} else {
				key = handle->Key();
				label = handle->Name();
			}
			if (key.Length() > 0 && label.Length() > 0) {
				BMessage*		msg = new BMessage(INPUT_FILTER_MSG);
				if (msg && label.Length() > 0 && (item = new BMenuItem(label.String(), msg)) ) {
					msg->AddString(SZ_FILTER_KEY, key);
					items.push_back(item);
				}
			}
		}
	}
	roster->Locker()->Unlock();
	
	AmMultiFilterRoster*			roster2 = AmMultiFilterRoster::Default();
	if (roster2) {
		ArpRef<AmMultiFilterAddOn>	addon = NULL;
		for (uint32 k = 0; (addon = roster2->FilterAt(k)) != NULL; k++) {
			if (accept_for_input(addon)) {
				BString				key, label;
				key = addon->Key();
				addon->GetLabel(label, true);
				if (key.Length() > 0 && label.Length() > 0) {
					BMessage*			msg = new BMessage(INPUT_FILTER_MSG);
					if (msg && label.Length() > 0 && (item = new BMenuItem(label.String(), msg)) ) {
						msg->AddString(SZ_FILTER_KEY, key);
						items.push_back(item);
					}
				}
			}
		}
	}
		
	sort(items.begin(), items.end(), sort_items);
	if ((item = new BMenuItem(NONE_STR, new BMessage(INPUT_FILTER_MSG))) != 0) {
		menu->AddItem(item);
		if (items.size() > 0) menu->AddSeparatorItem();
	}
	for (uint32 k = 0; k < items.size(); k++) menu->AddItem(items[k]);
	items.resize(0);

	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	menu->SetTargetForItems(target);
	return menu;
}

BView* SeqEditDeviceWindow::NewFilterView(BRect frame)
{
	BView*			v = new BView(frame, FILTER_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor(Prefs().Color(AM_AUX_WINDOW_BG_C));

	BRect			f(5, 0, frame.Width(), Prefs().Size(MENUFIELD_Y));
	BMenu*			m = new_filter_menu(INPUT_PIPELINE, this);
	if (!m) return v;
	BMenuField*		mf = new BMenuField(f, "Filters", "Input filter:", m);
	if (!mf) {
		delete m;
		return v;
	}
	mInputFilterMenu = m;
	v->AddChild(mf);
	
	return v;
}

// #pragma mark -

/********************************************************
 * _CONTROLLER-LIST
 ********************************************************/
_ControllerList::_ControllerList(BRect frame)
		: inherited(frame, CONTROLLER_LIST_STR, B_FOLLOW_ALL,
					B_WILL_DRAW),
		  mValueCtrl(NULL), mDevice(NULL)
{
	AddColumn(new BIntegerColumn(NUMBER_STR, 50, 20, 100), 0);
	AddColumn(new BStringColumn(NAME_STR, 150, 20, 250, B_TRUNCATE_END), 1);
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortColumn(ColumnAt(0), false, true);
	SetSortingEnabled(false);
}

void _ControllerList::SetDevice(ArpRef<AmDevice> device)
{
	BRow*			rw;
	while ( (rw = RowAt(0)) != NULL) {
		RemoveRow(rw);
		delete rw;
	}

	mDevice = device;
	if (mDevice) {
		uint32		count = mDevice->CountControls();
		for (uint32 k = 0; k < count; k++) {
			_ControllerRow*		r = new _ControllerRow(mDevice, k);
			if (r) AddRow(r);
		}
	}
}

void _ControllerList::SetTarget(BTextControl* valueCtrl)
{
	mValueCtrl = valueCtrl;
}

void _ControllerList::SelectionChanged()
{
	inherited::SelectionChanged();
	if (!mValueCtrl) return;

	_ControllerRow*	selection = dynamic_cast<_ControllerRow*>(CurrentSelection() );
	if (!selection) {
		if (mValueCtrl->IsEnabled() ) mValueCtrl->SetEnabled(false);
		mValueCtrl->SetText(NULL);
	} else {
		if (!mValueCtrl->IsEnabled() ) mValueCtrl->SetEnabled(true);
		selection->SelectionSetup(mValueCtrl);
	}
}

void _ControllerList::SaveSelection()
{
	if (!mDevice || !mValueCtrl) return;
	_ControllerRow*	selection = dynamic_cast<_ControllerRow*>(CurrentSelection() );
	if (selection) {
		selection->SaveSelection(mValueCtrl);
		UpdateRow(selection);
	}
}

// #pragma mark -

/********************************************************
 * _CONTROLLER-ROW
 ********************************************************/
_ControllerRow::_ControllerRow(	ArpRef<AmDevice> device,
								uint32 number)
		: mDevice(device), mNumber(number)
{
	SetField(new BIntegerField(number), 0);
	BString		n;
	if (mDevice) n = mDevice->ControlName(mNumber, false);
	SetField(new BStringField(n.String() ), 1);
}

_ControllerRow::~_ControllerRow()
{
}

void _ControllerRow::SelectionSetup(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	if (!mDevice) {
		valueCtrl->SetText(NULL);
		return;
	}
	valueCtrl->SetText(mDevice->ControlName(mNumber, false).String() );
	valueCtrl->MakeFocus(true);
	if (valueCtrl->TextView() ) valueCtrl->TextView()->SelectAll();
}

status_t _ControllerRow::SaveSelection(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	if (!mDevice) return B_ERROR;
	mDevice->SetControlName(mNumber, valueCtrl->Text() );
	SetField(new BStringField(valueCtrl->Text() ), 1);
	return B_OK;
}

// #pragma mark -

/********************************************************
 * _BANK-LIST
 ********************************************************/
_BankList::_BankList(BRect frame)
		: inherited(frame, BANK_LIST_STR, B_FOLLOW_ALL,
					B_WILL_DRAW),
		  mValueCtrl(NULL), mDevice(NULL)
{
	AddColumn(new BStringColumn(NAME_STR, 300, 20, 550, B_TRUNCATE_MIDDLE), 0);
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortColumn(ColumnAt(0), false, true);
	SetSortingEnabled(false);
}

void _BankList::SetDevice(ArpRef<AmDevice> device)
{
	BRow*			rw;
	while ( (rw = RowAt(0)) != NULL) {
		RemoveRow(rw);
		delete rw;
	}

	mDevice = device;
	if (mDevice) {
		ArpRef<AmBank>		bank;
		for (uint32 k = 0; (bank = mDevice->Bank(k)) != NULL; k++) {
			_BankRow*		brow = new _BankRow(bank, this);
			if (brow) ;		// Shut the compiler up
		}
	}

}

void _BankList::SetTarget(BTextControl* valueCtrl)
{
	mValueCtrl = valueCtrl;
}

void _BankList::SelectionChanged()
{
	inherited::SelectionChanged();
	if (!mValueCtrl) return;

	_AbstractBankRow*	selection = dynamic_cast<_AbstractBankRow*>(CurrentSelection() );
	if (!selection) {
		if (mValueCtrl->IsEnabled() ) mValueCtrl->SetEnabled(false);
		mValueCtrl->SetText(NULL);
	} else {
		if (!mValueCtrl->IsEnabled() ) mValueCtrl->SetEnabled(true);
		selection->SelectionSetup(mValueCtrl);
	}
}

void _BankList::SaveSelection()
{
	if (!mDevice || !mValueCtrl) return;
	_AbstractBankRow*	selection = dynamic_cast<_AbstractBankRow*>(CurrentSelection() );
	if (selection) {
		bool		updateAll = false;
		selection->SaveSelection(mValueCtrl, &updateAll);
		if (!updateAll) UpdateRow(selection);
		else UpdateAll(selection->Bank() );
	}
}

void _BankList::SelectNext()
{
#if 0
	BRow*	selection = CurrentSelection();
	int32	next = 0;
	if (selection) next = IndexOf(selection) + 1;
	BRow*	parent;
	bool	isVisible;
	if (FindParent(selection, &parent, &isVisible) == true && isVisible == true) {
		selection = RowAt(next, parent);		
	} else selection = RowAt(next);
	if (!selection) selection = RowAt(int32(0) );
	if (selection) AddToSelection(selection);
	SelectionChanged();
#endif
#if 1
	BRow*	selection = CurrentSelection();
	int32	next = 0;
	if (selection) next = IndexOf(selection) + 1;
	BRow*	parent;
	bool	isVisible;
	if (FindParent(selection, &parent, &isVisible) == true && isVisible == true) {
		selection = RowAt(next, parent);		
	} else selection = RowAt(next);
	if (!selection) selection = RowAt(int32(0) );
	if (selection) {
		AddToSelection(selection);
		BRect rect;
		GetRowRect(selection, &rect);
// For EXP
//		BView*		scrollView = ScrollView();
		BView*		scrollView = ChildAt(0);
		if (scrollView) {
			BRect frame = scrollView->Bounds();
			if (rect.top < frame.top) scrollView->ScrollBy(0, rect.top-frame.top);
			else if (rect.bottom > frame.bottom) scrollView->ScrollBy(0, rect.bottom-frame.bottom);
		}
	}
	SelectionChanged();
#endif
}

void _BankList::SelectPrev()
{
#if 1
	BRow*	selection = CurrentSelection();
	int32	next = 0;
	if (selection) {
		next = IndexOf(selection) - 1;
		if (next < 0) next = 0;
	}
	BRow*	parent;
	bool	isVisible;
	if (FindParent(selection, &parent, &isVisible) == true && isVisible == true) {
		selection = RowAt(next, parent);		
	} else selection = RowAt(next);
	if (!selection) selection = RowAt(int32(0) );
	if (selection) {
		AddToSelection(selection);
		BRect rect;
		GetRowRect(selection, &rect);
// For EXP
//		BView*		scrollView = ScrollView();
		BView*		scrollView = ChildAt(0);
		if (scrollView) {
			BRect frame = scrollView->Bounds();
			if (rect.top < frame.top) scrollView->ScrollBy(0, rect.top-frame.top);
			else if (rect.bottom > frame.bottom) scrollView->ScrollBy(0, rect.bottom-frame.bottom);
		}
	}
	SelectionChanged();
#endif
}

void _BankList::UpdateAll(ArpRef<AmBank> bank, BRow* parent)
{
	if (!bank) return;
	BRow*		r;
	for (int32 k = 0; (r = RowAt(k, parent)) != NULL; k++) {
		_AbstractBankRow*	row = dynamic_cast<_AbstractBankRow*>(r);
		if (row && bank == row->Bank() ) {
			if (row->HasLatch() ) UpdateAll(bank, row);
			row->SetNameField();
			UpdateRow(row);
		}
	}
}

// #pragma mark -

/********************************************************
 * _ABSTRACT-BANK-ROW
 ********************************************************/
_AbstractBankRow::_AbstractBankRow(ArpRef<AmBank> bank)
		: mBank(bank)
{
}

void _AbstractBankRow::SelectionSetup(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	valueCtrl->SetText(NULL);
	if (valueCtrl->IsEnabled() ) valueCtrl->SetEnabled(false);
}

// #pragma mark -

/********************************************************
 * _BANK-ROW
 ********************************************************/
_BankRow::_BankRow(	ArpRef<AmBank> bank,
					BColumnListView* owner)
		: inherited(bank)
{
	if (owner) {
		owner->AddRow(this);
		if (bank->BankSelection() ) {
			BRow*	row = new _BankSelectionRow(bank, owner, this);
			if (row) ;		// Shut the compiler up
		}
		BRow*	row = new _BankPatchesRow(bank, owner, this);
		if (row) ;		// Shut the compiler up
		row = new _NumberPatchesRow(bank, owner, this);
		if (row) ;		// Shut the compiler up
	}
	SetNameField();
}

void _BankRow::SelectionSetup(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	if (!mBank) {
		valueCtrl->SetText(NULL);
		return;
	}
	valueCtrl->SetText(mBank->Name().String() );
	valueCtrl->MakeFocus(true);
	const char*		label = "Bank name:";
	valueCtrl->SetLabel(label);
	valueCtrl->SetDivider(valueCtrl->StringWidth(label) + 10);
	if (valueCtrl->TextView() ) valueCtrl->TextView()->SelectAll();
}

status_t _BankRow::SaveSelection(BTextControl* valueCtrl, bool* update)
{
	ArpASSERT(valueCtrl);
	if (!mBank) return B_ERROR;
	mBank->SetName(valueCtrl->Text() );
	SetNameField();
	return B_OK;
}

void _BankRow::SetNameField()
{
	if (!mBank) {
		SetField(new BStringField("No bank"), 0);
	} else {
		BString		n("Bank ");
		n << mBank->BankNumber();
		if (mBank->Name().Length() > 0) n << " - " << mBank->Name();
		SetField(new BStringField(n.String() ), 0);
	}
}

// #pragma mark -

/********************************************************
 * _BANK-SELECTION-ROW
 ********************************************************/
_BankSelectionRow::_BankSelectionRow(	ArpRef<AmBank> bank,
										BColumnListView* owner,
										BRow* parent)
		: inherited(bank)
{
	if (owner) {
		owner->AddRow(this, parent);
		AmBankChange*	be = mBank->BankSelection();
		if (be && be->Phrase() ) {
			AmNode*		n = be->Phrase()->HeadNode();
			while (n) {
				if (n->Event()->Type() == n->Event()->CONTROLCHANGE_TYPE) {
					AmControlChange*	cc = dynamic_cast<AmControlChange*>( n->Event() );
					if (cc) {
						_ControlChangeRow*		row = new _ControlChangeRow(cc, owner, this);
						if (row) ;		// Shut the compiler up
					}
				}
				n = n->next;
			}
		}
	}
	SetField(new BStringField("Selection"), 0);
}

status_t _BankSelectionRow::SaveSelection(BTextControl* valueCtrl, bool* update)
{
	return B_ERROR;
}

// #pragma mark -

/********************************************************
 * _BANK-FILTER
 ********************************************************/
_BankFilter::_BankFilter()
	:	BMessageFilter(B_KEY_DOWN)
{
}

filter_result _BankFilter::Filter(BMessage *message, BHandler **target)
{
	int32 mods;
	int8 byte;
	if (message->FindInt32("modifiers", &mods) != B_OK) mods = 0;
	if (message->FindInt8("byte", &byte) != B_OK) byte = 0;
	
	BMessage cmd;
	bool eat = false;
	if ((mods&(B_SHIFT_KEY|B_CONTROL_KEY|B_COMMAND_KEY|B_OPTION_KEY)) == 0 && byte == B_UP_ARROW) {
		cmd.what = PREV_BANK_VALUE_MSG;
		eat = true;
	} else if ((mods&(B_SHIFT_KEY|B_CONTROL_KEY|B_COMMAND_KEY|B_OPTION_KEY)) == 0 && byte == B_DOWN_ARROW) {
		cmd.what = NEXT_BANK_VALUE_MSG;
		eat = true;
	} else if ((mods&(B_SHIFT_KEY|B_CONTROL_KEY|B_COMMAND_KEY|B_OPTION_KEY)) == 0 && byte == B_ENTER) {
		cmd.what = NEXT_BANK_VALUE_MSG;
	}
	
	if (cmd.what) {
		BView* v = dynamic_cast<BView*>(*target);
		if (v) {
			BInvoker* invk = dynamic_cast<BInvoker*>(v->Parent());
			if (invk) invk->Messenger().SendMessage(&cmd);
		}
	}
	
	return eat ? B_SKIP_MESSAGE : B_DISPATCH_MESSAGE;
}

// #pragma mark -

/********************************************************
 * _CONTROL-CHANGE-ROW
 ********************************************************/
_ControlChangeRow::_ControlChangeRow(	AmControlChange* event,
										BColumnListView* owner,
										BRow* parent)
		: inherited(NULL), mEvent(event)
{
	mEvent->IncRefs();
	if (owner) owner->AddRow(this, parent);
	SetNameField();
}

_ControlChangeRow::~_ControlChangeRow()
{
	mEvent->DecRefs();
}

void _ControlChangeRow::SelectionSetup(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	if (!mEvent) {
		valueCtrl->SetText(NULL);
		return;
	}
	BString		str;
	str << int32(mEvent->ControlValue() );
	valueCtrl->SetText(str.String() );
	valueCtrl->MakeFocus(true);
	const char*		label = "Controller value:";
	valueCtrl->SetLabel(label);
	valueCtrl->SetDivider(valueCtrl->StringWidth(label) + 10);
	if (valueCtrl->TextView() ) valueCtrl->TextView()->SelectAll();
}

status_t _ControlChangeRow::SaveSelection(BTextControl* valueCtrl, bool* update)
{
	ArpASSERT(valueCtrl);
	if (!mEvent) return B_ERROR;
	int32		value = atol(valueCtrl->Text() );
	if (value < 0 || value > 127) return B_ERROR;
	mEvent->SetControlValue(value);
	SetNameField();
	return B_OK;
}

void _ControlChangeRow::SetNameField()
{
	BString		n("Controller ");
	n << int32(mEvent->ControlNumber() ) << " - " << int32(mEvent->ControlValue() );
	SetField(new BStringField(n.String() ), 0);
}

// #pragma mark -

/********************************************************
 * _BANK-PATCHES-ROW
 ********************************************************/
_BankPatchesRow::_BankPatchesRow(	ArpRef<AmBank> bank,
									BColumnListView* owner,
									BRow* parent)
		: inherited(bank)
{
	if (owner) {
		owner->AddRow(this, parent);
		uint32			count = mBank->CountPatches();
		for (uint32 k = 0; k < count; k++) {
			BRow*	row = new _PatchRow(bank, k, owner, this);
			if (row) ;		// Shut the compiler up
		}
	}

	SetField(new BStringField("Patches"), 0);
}

status_t _BankPatchesRow::SaveSelection(BTextControl* valueCtrl, bool* update)
{
	return B_ERROR;
}

// #pragma mark -

/********************************************************
 * _PATCH-ROW
 ********************************************************/
_PatchRow::_PatchRow(	ArpRef<AmBank> bank, uint32 number,
						BColumnListView* owner,
						BRow* parent)
		: inherited(bank), mNumber(number)
{
	if (owner) owner->AddRow(this, parent);
	SetNameField();
}

void _PatchRow::SelectionSetup(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	if (!mBank) {
		valueCtrl->SetText(NULL);
		return;
	}
	valueCtrl->SetText(mBank->PatchName(mNumber).String() );
	valueCtrl->MakeFocus(true);
	const char*		label = "Patch name:";
	valueCtrl->SetLabel(label);
	valueCtrl->SetDivider(valueCtrl->StringWidth(label) + 10);
	if (valueCtrl->TextView() ) valueCtrl->TextView()->SelectAll();
}

status_t _PatchRow::SaveSelection(BTextControl* valueCtrl, bool* update)
{
	ArpASSERT(valueCtrl);
	if (!mBank) return B_ERROR;
	mBank->SetPatchName(mNumber, valueCtrl->Text() );
	SetNameField();
	return B_OK;
}

void _PatchRow::SetNameField()
{
	BString		n;
	if (mBank) {
		n << mNumber + mBank->FirstPatchNumber();
		BString		patch = mBank->PatchName(mNumber).String();
		if (patch.Length() > 0) n << " - " << patch.String();
	} else n << mNumber;
	SetField(new BStringField(n.String() ), 0);
}

uint32 _PatchRow::Number() const
{
	return mNumber;
}

// #pragma mark -

/********************************************************
 * _NUMBER-PATCHES-ROW
 ********************************************************/
_NumberPatchesRow::_NumberPatchesRow(	ArpRef<AmBank> bank,
										BColumnListView* owner,
										BRow* parent)
		: inherited(bank)
{
	if (owner) owner->AddRow(this, parent);
	SetNameField();
}

void _NumberPatchesRow::SelectionSetup(BTextControl* valueCtrl)
{
	ArpASSERT(valueCtrl);
	if (!mBank) {
		valueCtrl->SetText(NULL);
		return;
	}
	BString			n;
	n << mBank->FirstPatchNumber();
	valueCtrl->SetText(n.String() );
	valueCtrl->MakeFocus(true);
	const char*		label = "First number:";
	valueCtrl->SetLabel(label);
	valueCtrl->SetDivider(valueCtrl->StringWidth(label) + 10);
	if (valueCtrl->TextView() ) valueCtrl->TextView()->SelectAll();
}

status_t _NumberPatchesRow::SaveSelection(BTextControl* valueCtrl, bool* update)
{
	ArpASSERT(valueCtrl);
	if (!mBank) return B_ERROR;
	int32		value = atol(valueCtrl->Text() );
	if (value < 0) return B_ERROR;
	mBank->SetFirstPatchNumber(value);
	SetNameField();
	if (update) *update = true;
	return B_OK;
}

void _NumberPatchesRow::SetNameField()
{
	BString		n("Number patches from ");
	if (!mBank) n << "<no bank>";
	else n << mBank->FirstPatchNumber();
	SetField(new BStringField(n.String() ), 0);
}

// #pragma mark -

/********************************************************
 * Misc
 ********************************************************/
static void _set_device_for_input_filter(BMenu* m,
								const BString& key)
{
	ArpVALIDATE(m, return);
	BMenuItem*			item;
	if (key.Length() < 1) {
		item = m->FindItem(NONE_STR);
		if (item) item->SetMarked(true);
		return;
	}

	for (int32 k = 0; (item = m->ItemAt(k)) != 0; k++) {
		const char*		itemKey = 0;
		if (item->Message() && item->Message()->FindString(SZ_FILTER_KEY, &itemKey) == B_OK) {
			if (itemKey && key == itemKey) {
				item->SetMarked(true);
				return;
			}
		}
	}
}
