/* SeqEditToolWindow.cpp
 */
#include <be/experimental/ColumnListView.h>
#include <be/experimental/ColumnTypes.h>
#include <be/support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmGraphicEffect.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmToolSeedI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolControls.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqBitmapEditor.h"
#include "Sequitur/SeqEditToolWindow.h"
#include "Sequitur/SeqPipelineMatrixView.h"

static const uint32		NAME_MSG			= '#nam';
static const uint32		CLASS_NAME_MSG		= '#cna';
static const uint32		AUTHOR_MSG			= '#aut';
static const uint32		EMAIL_MSG			= '#eml';
static const uint32		SEED_MSG			= '#sed';
static const uint32		ADD_PIPELINE_MSG	= 'iAdP';
static const uint32		DELETE_PIPELINE_MSG	= 'iDlP';
static const uint32		BITMAP_CHANGE_MSG	= 'bmch';
static const uint32		EFFECT_MSG			= '#eff';
//static const uint32		SEED_ROW_SELECTED	= '#srs';
static const uint32		ADD_CONTROL_MSG		= 'iAdC';
static const uint32		DELETE_CONTROL_MSG	= 'iDlC';
static const uint32		ADD_TARGET_MSG		= 'iAdT';
static const uint32		DELETE_TARGET_MSG	= 'iDlT';
static const uint32		DESCRIPTION_MOD_MSG			= 'iDsM';
static const uint32		COPY_ICON_MSG				= 'icic';
static const uint32		PASTE_ICON_MSG				= 'ipic';
static const uint32		FLIP_VERTICALLY_ICON_MSG	= 'ifvi';
static const uint32		FLIP_HORIZONTALLY_ICON_MSG	= 'ifhi';
static const uint32		FILL_WITH_ALPHA_MSG			= 'ibfa';

static const char*		GENERAL_STR			= "General";
static const char*		SEED_LIST_STR		= "seed_list";
static const char*		SEED_STR			= "Seed";
static const char*		PIPELINE_STR		= "Pipeline";
static const char*		DESCRIPTION_STR		= "Description";
static const char*		ICON_STR			= "Icon";
static const char*		EFFECTS_STR			= "Effects";
static const char*		EFFECT_STR			= "Effect";
static const char*		GRAPHIC_LIST_STR	= "graphic_list";
static const char*		VIEW_STR			= "View";
static const char*		NONE_STR			= "None";
static const char*		CONTROL_STR			= "Controls";
static const char*		TOOL_CONTROL_LIST_STR = "tool_control_list";
static const char*		ADD_CONTROL_STR		= "add_control";
static const char*		DELETE_CONTROL_STR	= "delete_control";
static const char*		CONTROL_TARGET_LIST_STR = "control_target_list";
static const char*		DELETE_TARGET_STR	= "delete_target";

static const int32		NAME_FIELD			= 0;
static const int32		KEY_FIELD			= 1;
static const int32		TC_INDEX_FIELD		= 0;
static const int32		TC_NAME_FIELD		= 1;

static const int32		CT_PIPELINE_FIELD	= 0;
static const int32		CT_FILTER_FIELD		= 1;
static const int32		CT_PROPERTY_FIELD	= 2;

static AmGlobalsImpl* gobals_impl()
{
	AmGlobalsI*		globalsI = &(AmGlobals());
	if (!globalsI) printf("COULDN'T GET GLOBALS AT ALL\n");
	return dynamic_cast<AmGlobalsImpl*>(globalsI);
}

#if 0
static BView* new_seed_prop_function(const BString& key)
{
	printf("In the new seed prop function\n");
	return NULL;
}
#endif

/********************************************************
 * _VIEW-LIST
 ********************************************************/
class _ViewRow;

class _ViewList : public BColumnListView
{
public:
	_ViewList(BRect rect, const char* name);

	virtual void	SelectionChanged();

	void			SetPropertyFrame(BRect propertyFrame);
	virtual void	SetTool(const AmTool* tool);
	virtual void	SetKeyForCurrent(const char* key, AmTool* tool);
	void			BuildRows();

protected:
	AmToolRef		mToolRef;
	BRect			mPropertyFrame;
		
	virtual _ViewRow*	NewRow(bool root = false) const = 0;
	virtual _ViewRow*	NewRow(const BString& factoryKey) const = 0;
	virtual _ViewRow*	NewRow(const BString& factoryKey, const BString& viewKey) const = 0;

	virtual status_t	GetToolKey(	uint32 index,
									BString& outFactoryKey,
									BString& outViewKey,
									BString& outKey,
									const AmTool* tool) const = 0;

	virtual BView*		NewViewForKey(	const BString& factoryKey,
										const BString& viewKey,
										const BString& key) const = 0;

	void			ClearRowKeys(BRow* parent = NULL);
	_ViewRow*		ViewRow(const BString& factoryKey,
							const BString& viewKey,
							BRow* parent = NULL);

private:
	typedef BColumnListView		inherited;
	
	bool			HasRow(	const BString& facKey,
							const BString& viewKey) const;
};

/********************************************************
 * _SEED-LIST
 ********************************************************/
class _SeedList : public _ViewList
{
public:
	_SeedList(BRect rect);

	virtual void		SelectionChanged();

	void				SetSeedControl(BMenuField* seedCtrl);

protected:
	virtual _ViewRow*	NewRow(bool root = false) const;
	virtual _ViewRow*	NewRow(const BString& factoryKey) const;
	virtual _ViewRow*	NewRow(const BString& factoryKey, const BString& viewKey) const;

	virtual status_t	GetToolKey(	uint32 index,
									BString& outFactoryKey,
									BString& outViewKey,
									BString& outKey,
									const AmTool* tool) const;

	virtual BView*		NewViewForKey(	const BString& factoryKey,
										const BString& viewKey,
										const BString& key) const;

private:
	typedef _ViewList		inherited;
	BMenuField*				mSeedCtrl;
};

/********************************************************
 * _GRAPHIC-LIST
 ********************************************************/
class _GraphicList : public _ViewList
{
public:
	_GraphicList(BRect rect);

	virtual void		SelectionChanged();
	void				SetMenuControl(BMenuField* menuCtrl);
	
protected:
	virtual _ViewRow*	NewRow(bool root = false) const;
	virtual _ViewRow*	NewRow(const BString& factoryKey) const;
	virtual _ViewRow*	NewRow(const BString& factoryKey, const BString& viewKey) const;

	virtual status_t	GetToolKey(	uint32 index,
									BString& outFactoryKey,
									BString& outViewKey,
									BString& outKey,
									const AmTool* tool) const;

	virtual BView*		NewViewForKey(	const BString& factoryKey,
										const BString& viewKey,
										const BString& key) const;

private:
	typedef _ViewList	inherited;

	BMenuField*			mMenuCtrl;
};

/********************************************************
 * _VIEW-ROW
 ********************************************************/
class _ViewRow : public BRow
{
public:
	_ViewRow(bool root = false);
	_ViewRow(const BString& factoryKey);
	_ViewRow(const BString& factoryKey, const BString& viewKey);
	
	virtual bool		HasLatch() const;
	bool				Matches(const BString& factoryKey,
								const BString& viewKey) const;

	void				SetKey(const char* key);
	virtual void		SetToolKey(const char* key, AmTool* tool) = 0;
	void				GetKeyInfo(BString& factoryKey, BString& viewKey, BString& key) const;
	
	void				Print() const;

protected:
	bool				mRoot;
	BString				mFactoryKey;
	BString				mViewKey;
	BString				mKey;
	BString				mLabel;

	virtual void		GetKeyLabel(const char* key, BString& outLabel) = 0;

private:
	typedef BRow	inherited;
};

/********************************************************
 * _SEED-ROW
 ********************************************************/
class _SeedRow : public _ViewRow
{
public:
	_SeedRow(bool root = false);
	_SeedRow(const BString& factoryKey);
	_SeedRow(const BString& factoryKey, const BString& viewKey);
	
	virtual void		SetToolKey(const char* key, AmTool* tool);

protected:
	virtual void		GetKeyLabel(const char* key, BString& outLabel);

private:
	typedef _ViewRow	inherited;
};

/********************************************************
 * _GRAPHIC-ROW
 ********************************************************/
class _GraphicRow : public _ViewRow
{
public:
	_GraphicRow(bool root = false);
	_GraphicRow(const BString& factoryKey);
	_GraphicRow(const BString& factoryKey, const BString& viewKey);
	
	virtual void		SetToolKey(const char* key, AmTool* tool);

protected:
	virtual void		GetKeyLabel(const char* key, BString& outLabel);

private:
	typedef _ViewRow	inherited;
};

/********************************************************
 * _TOOL-CONTROL-LIST
 ********************************************************/
class _ToolControlList : public BColumnListView
{
public:
	_ToolControlList(	BRect rect, const char* name,
						AmToolRef toolRef);

	virtual void	SelectionChanged();
	void			SetTool(const AmTool* tool);
	void			SetTargets(_ControlTargetList* targets);
	void			BuildRows();

private:
	typedef BColumnListView		inherited;
	AmToolRef		mToolRef;
	_ControlTargetList* mTargets;

	void			BuildRows(const AmTool* tool);
};

/********************************************************
 * _TOOL-CONTROL-ROW
 ********************************************************/
class _ToolControlRow : public BRow
{
public:
	_ToolControlRow(const AmToolControl* control, uint32 index);
	
	virtual bool		HasLatch() const;
	uint32				Type() const;
	uint32				Index() const;
	
private:
	typedef BRow	inherited;
	void*			mToolControlId;
	uint32			mIndex;
	uint32			mType;
};

/********************************************************
 * _CONTROL-TARGET-LIST
 ********************************************************/
class _ControlTargetList : public BColumnListView
{
public:
	_ControlTargetList(	BRect rect, const char* name,
						AmToolRef toolRef,
						_ToolControlList* controls);

	virtual void	SelectionChanged();
	void			SetTool(const AmTool* tool);
	void			BuildRows();

private:
	typedef BColumnListView		inherited;
	AmToolRef			mToolRef;
	_ToolControlList*	mControls;
	
	void			BuildRows(_ToolControlRow* row, const AmTool* tool);
};

/********************************************************
 * _CONTROL-TARGET-ROW
 ********************************************************/
class _ControlTargetRow : public BRow
{
public:
	_ControlTargetRow(	uint32 index, int32 pipeline,
						int32 filter, const BString& name);
	
	virtual bool		HasLatch() const;
	uint32				Index() const;
	int32				Pipeline() const;
	int32				Filter() const;
	
private:
	typedef BRow	inherited;
	uint32			mIndex;
	int32			mPipeline;
	int32			mFilter;
	BString			mName;
};

/*************************************************************************
 * SEQ-EDIT-TOOL-WINDOW
 *************************************************************************/
SeqEditToolWindow::SeqEditToolWindow(	BRect frame,
										const BMessage* config,
										const BMessage* toolMsg)
		: inherited(frame, "Edit tool"),
		  mNameCtrl(NULL), mKeyCtrl(NULL), mSeedList(NULL),
		  mSeedCtrl(NULL), mPipelineView(NULL), mPipelineScrollView(NULL),
		  mLongDescriptionCtrl(NULL), mIconCtrl(NULL),
		  mIconEditor(NULL), mToolControlList(NULL), mDeleteControlButton(NULL),
		  mControlTargetList(NULL), mAddTargetMenu(NULL),
		  mEffectCtrl(NULL), mGraphicList(NULL)
{
//	mSeedPropFactory.SetNewFunction(new_seed_prop_function);
	
	BRect		targetF(CurrentPageFrame() );
	BView*		view = NULL;
	if ((view = NewGeneralView(targetF))) AddPage(view);
	if ((view = NewSeedView(targetF))) AddPage(view);
	if ((view = NewPipelineView(targetF))) AddPage(view);
	if ((view = NewDescriptionView(targetF))) AddPage(view);
	if ((view = NewIconView(targetF))) AddPage(view);
//	if ((view = NewControlView(targetF))) AddPage(view);
	if ((view = NewEffectsView(targetF))) AddPage(view);

	if (config) SetConfiguration(config);
	else SetFirstPage();
	if (toolMsg) SetTool(toolMsg);
}

SeqEditToolWindow::~SeqEditToolWindow()
{
	if (mToolRef.IsValid() ) {
		AmGlobalsImpl*	globals = gobals_impl();
		if (globals) globals->UnregisterTemporaryMatrix( mToolRef.ToolId() );
	}
}

void SeqEditToolWindow::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	SetPipelineScrollBars();
}

static status_t add_bool_items(	BMenu** menu, const BMessage& props, const char* label,
								int32 pipeline, int32 filter)
{
	if (!label) return B_ERROR;
#if B_BEOS_VERSION_DANO
	char*			name;
#else
	char*			name;
#endif
	type_code		typeAnswer;
	BMenu*			submenu = NULL;
	for (int32 k = 0; props.GetInfo(B_BOOL_TYPE, k, &name, &typeAnswer) == B_OK; k++) {
		if (!*menu) *menu = new BMenu("Pipeline");
		if (!*menu) return B_NO_MEMORY;
		if (!submenu) submenu = new BMenu(label);
		if (!submenu) return B_NO_MEMORY;
		BMessage*	msg = new BMessage(ADD_TARGET_MSG);
		if (msg && msg->AddInt32("pipeline", pipeline) == B_OK
				&& msg->AddInt32("filter", filter) == B_OK
				&& msg->AddString("name", name) == B_OK) {
			BMenuItem*	item = new BMenuItem(name, msg);
			if (item) submenu->AddItem(item);
		}
	}
	if (submenu && *menu) {
		BMenuItem*	item = new BMenuItem(submenu);
		if (item) (*menu)->AddItem(item);
	}
	return B_OK;
}

static void add_targets_to(BMenu* menu, const AmTool* tool, uint32 type)
{
	menu->RemoveItems(0, menu->CountItems(), true);
	pipeline_id		pid;
	for (uint32 k = 0; (pid = tool->PipelineId(k)) != 0; k++) {
		AmFilterHolderI*	holder = tool->Filter(pid, THROUGH_PIPELINE);
		BMenu*			pMenu = NULL;
		int32			fIndex = 0;
		while (holder && holder->Filter() ) {
			BMessage		msg;
			if (holder->Filter()->GetProperties(&msg) == B_OK) {
				BString		fLabel;
				fLabel << fIndex + 1 << " " << holder->Filter()->Label();
				if (type == AmToolControl::BOOL_TYPE) {
					add_bool_items(&pMenu, msg, fLabel.String(), k, fIndex); 
				}
			}
			msg.MakeEmpty();
			fIndex++;
			holder = holder->NextInLine();
		}
		if (pMenu) {
			BString		name("Pipeline ");
			name << k + 1;
			pMenu->SetName(name.String() );
			BMenuItem*	item = new BMenuItem(pMenu);
			if (item) menu->AddItem(item);
		}
	}
}

void SeqEditToolWindow::MenusBeginning()
{
	inherited::MenusBeginning();
	if (mToolControlList && mAddTargetMenu) {
		_ToolControlRow*	row = dynamic_cast<_ToolControlRow*>( mToolControlList->CurrentSelection() );
		if (row) {
			// READ TOOL BLOCK
			const AmTool*	tool = mToolRef.ReadLock();
			if (tool) {
				add_targets_to(mAddTargetMenu, tool, row->Type() );
			}
			mToolRef.ReadUnlock(tool);
			// END READ TOOL BLOCK
		}
	}
}

void SeqEditToolWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case ADD_TARGET_MSG: {
			_ToolControlRow*		row = NULL;
			if (mToolControlList && (row = dynamic_cast<_ToolControlRow*>( mToolControlList->CurrentSelection() )) != NULL) {
				int32				pIndex;
				int32				fIndex;
				const char*			name;
				if (msg->FindInt32("pipeline", &pIndex) == B_OK
						&& msg->FindInt32("filter", &fIndex) == B_OK
						&& msg->FindString("name", &name) == B_OK) {
					// WRITE TOOL BLOCK
					AmTool*		tool = mToolRef.WriteLock();
					if (tool) {
						AmToolControlList*	list = tool->ControlList();
						if (list) {
							AmToolControl*	control;
							if (list->GetControl(row->Index(), &control) == B_OK) {
								control->AddTarget(pIndex, fIndex, name);
								SetHasChanges(true);
							}
						}
					}
					mToolRef.WriteUnlock(tool);
					// END WRITE TOOL BLOCK
				}
				if (mControlTargetList) mControlTargetList->BuildRows();
			}
		} break;
		case SHOW_EDIT_TOOL_MSG:
			SetTool(msg);
			break;
		case NAME_MSG: {
			if (mToolRef.IsValid() && mNameCtrl) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					tool->SetLabel( mNameCtrl->Text() );
					SetWindowTitle();
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
			}
		} break;
		case CLASS_NAME_MSG: {
			if (mToolRef.IsValid() && mKeyCtrl) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					tool->SetKey( mKeyCtrl->Text() );
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
			}
		} break;
		case AUTHOR_MSG: {
			if (mToolRef.IsValid() && mAuthorCtrl) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					tool->SetAuthor(mAuthorCtrl->Text() );
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
			}
		} break;
		case EMAIL_MSG: {
			if (mToolRef.IsValid() && mEmailCtrl) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					tool->SetEmail(mEmailCtrl->Text() );
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
			}
		} break;
		case SEED_MSG: {
			const char*		key;
			if (msg->FindString(SEED_STR, &key) != B_OK) key = NULL;		
			if (mSeedList) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					mSeedList->SetKeyForCurrent(key, tool);
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
				mSeedList->SelectionChanged();
			}
		} break;
		case ADD_PIPELINE_MSG: {
			if (mToolRef.IsValid() ) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					tool->PushPipeline();
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
				if (mPipelineView) {
					mPipelineView->FillMetrics();
					mPipelineView->Invalidate();
				}
				SetPipelineScrollBars();
			}
		} break;
		case DELETE_PIPELINE_MSG: {
			if (mToolRef.IsValid() ) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					tool->PopPipeline();
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
				if (mPipelineView) {
					mPipelineView->FillMetrics();
					mPipelineView->Invalidate();
				}
				SetPipelineScrollBars();
			}
		} break;
		case ADD_CONTROL_MSG: {
			if (mToolRef.IsValid() ) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					AmToolControlList*	list = tool->ControlList();
					if (list) {
						list->AddControl(new AmToolControl() );
						SetHasChanges(true);
					}
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
				if (mToolControlList) mToolControlList->BuildRows();
			}
		} break;
		case DELETE_CONTROL_MSG: {
			_ToolControlRow*		row = NULL;
			if (mToolControlList && (row = dynamic_cast<_ToolControlRow*>( mToolControlList->CurrentSelection() )) != NULL) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					AmToolControlList*	list = tool->ControlList();
					if (list) {
						AmToolControl*		ctrl = list->RemoveControl( row->Index() );
						if (ctrl) delete ctrl;
						SetHasChanges(true);
					}
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
				mToolControlList->BuildRows();
			}
		} break;
		case AmNotifier::PIPELINE_CHANGE_OBS: {
			SetHasChanges(true);
			SetPipelineScrollBars();
		} break;
		case AmNotifier::FILTER_CHANGE_OBS: {
			SetHasChanges(true);
			SetPipelineScrollBars();
		} break;
#if 0
		case SEED_ROW_SELECTED: {
			if (mSeedList) {
				BString		seedKey;
				BMessage	config;
//				if (mSeedList->GetCurrentKey(seedKey, config) == B_OK) {
//					BView*	v = mSeedPropFactory.ViewFor(seedKey, &config);
//				}
			}
		} break;
#endif

		case DESCRIPTION_MOD_MSG: {
			SetHasChanges(true);
		} break;
		case BITMAP_CHANGE_MSG: {
			bool	b;
			if (msg->FindBool("bitmap changes", &b) == B_OK && b)
				SetHasChanges(true);
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
		case EFFECT_MSG: {
			const char*		key;
			if (msg->FindString(EFFECT_STR, &key) != B_OK) key = NULL;		
			if (mGraphicList) {
				// WRITE TOOL BLOCK
				AmTool*		tool = mToolRef.WriteLock();
				if (tool) {
					mGraphicList->SetKeyForCurrent(key, tool);
					SetHasChanges(true);
				}
				mToolRef.WriteUnlock(tool);
				// END WRITE TOOL BLOCK
			}
		} break;
		case AM_TOOL_SEED_CHANGED_MSG:
			SetHasChanges(true);
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqEditToolWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::EDIT_TOOL_WIN_INDEX, config);
	}
	return true;
}

void SeqEditToolWindow::SetTool(const BMessage* toolMsg)
{
	if (!toolMsg) return;
	const char*		path;
	if (toolMsg->FindString("path", &path) != B_OK) path = NULL;
	const char*		toolName;
	if (toolMsg->FindString("tool_key", &toolName) == B_OK)
		SetTool(toolName, path);
}

void SeqEditToolWindow::SetTool(const BString& key, const BString& path)
{
	if (!SetEntryCheck() ) return;
	if (!Lock()) return;
	if (mToolRef.IsValid() ) {
		AmGlobalsImpl*	globals = gobals_impl();
		if (globals) globals->UnregisterTemporaryMatrix( mToolRef.ToolId() );
	}
	mInitialAuthor = (const char*)NULL;
	mInitialEmail = (const char*)NULL;
	mToolRef.RemoveMatrixObserver(0, this);
	mToolRef.SetTo(NULL);

	const char*			s = NULL;
	if (path.Length() > 0) s = path.String();
	AmToolRoster*		roster = AmToolRoster::Default();
	if (roster) mToolRef = roster->NewTool(key, s);
	if (!mToolRef.IsValid() ) {
		const char*		auth;
		const char*		email;
		if (seq_get_string_preference(AUTHOR_PREF, &auth) != B_OK) auth = NULL;
		if (seq_get_string_preference(EMAIL_PREF, &email) != B_OK) email = NULL;
		mToolRef = new AmTool("New tool", NULL, NULL, auth, email);
	}
	SetHasChanges(false);
	mInitialKey = mToolRef.ToolKey();
	// WRITE TOOL BLOCK
	AmTool*		tool = mToolRef.WriteLock();
	if (tool) {
		AmGlobalsImpl*	globals = gobals_impl();
		if (globals) globals->RegisterTemporaryMatrix(tool);

		mInitialAuthor = tool->Author();
		mInitialEmail = tool->Email();

		SetTextControl(mNameCtrl, tool->Label().String(), NAME_MSG);
		SetTextControl(mKeyCtrl, tool->Key().String(), CLASS_NAME_MSG);
		SetTextControl(mAuthorCtrl, tool->Author().String(), AUTHOR_MSG);
		SetTextControl(mEmailCtrl, tool->Email().String(), EMAIL_MSG);
		if (mShortDescriptionCtrl) mShortDescriptionCtrl->SetText(tool->ShortDescription().String() );
		if (mLongDescriptionCtrl) {
			BString		ld;
			tool->LongDescription(ld);
			mLongDescriptionCtrl->SetText(ld.String() );
		}
		if (mIconEditor) mIconEditor->SetBitmap( tool->Icon() );

		if (mSeedList) mSeedList->SetTool(tool);
		if (mGraphicList) mGraphicList->SetTool(tool);
		if (mToolControlList) mToolControlList->SetTool(tool);
		if (mControlTargetList) mControlTargetList->SetTool(tool);
		
		BString		title("Edit ");
		if (tool->Label().Length() > 0) title << tool->Label();
		else title << "Unnamed tool";
		SetTitle( title.String() );
	}
	mToolRef.WriteUnlock(tool);
	// END WRITE TOOL BLOCK
	mToolRef.AddMatrixPipelineObserver(0, this);
	mToolRef.AddMatrixFilterObserver(0, this);
	if (mPipelineView) mPipelineView->SetMatrixRef( AmPipelineMatrixRef(mToolRef) );
	SetWindowTitle();
	SetPipelineScrollBars();
	Unlock();
}

uint32 SeqEditToolWindow::ConfigWhat() const
{
	return EDIT_TOOL_WINDOW_SETTING_MSG;
}

bool SeqEditToolWindow::Validate()
{
	if (!mKeyCtrl) {
		low_memory_warning();
		return false;
	}
	BString				error;
	size_t				len = 0;
	if (mKeyCtrl->Text() ) len = strlen(mKeyCtrl->Text() );
	
	if (len < 1) error = "This tool must have a key";
	else if (mInitialKey.Length() < 1 || mInitialKey != mKeyCtrl->Text() ) {
		AmToolRoster*	roster = AmToolRoster::Default();
		if (roster && roster->KeyExists(mKeyCtrl->Text() ) ) {
			error = "There is already a tool with the key \'";
			error << mKeyCtrl->Text() << "\'";
		}
	}
	if (error.Length() < 1) return true;
	BAlert*	alert = new BAlert(	"Warning", error.String(),
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
	return false;
}

void SeqEditToolWindow::SaveChanges()
{
	if (!HasChanges() || !mToolRef.IsValid() ) return;

	SetHiddenPrefs();
	/* UGH!  BTextControls have a problem getting their
	 * changes out.  I'm sure there's a better way to
	 * do this.
	 */
	// WRITE TOOL BLOCK
	AmTool*		tool = mToolRef.WriteLock();
	if (tool) {
		if (mNameCtrl) tool->SetLabel(mNameCtrl->Text() );
		if (mKeyCtrl) tool->SetKey(mKeyCtrl->Text() );
		if (mAuthorCtrl) tool->SetAuthor(mAuthorCtrl->Text() );
		if (mEmailCtrl) tool->SetEmail(mEmailCtrl->Text() );
		if (mShortDescriptionCtrl) tool->SetShortDescription(mShortDescriptionCtrl->Text() );
		if (mLongDescriptionCtrl) tool->SetLongDescription(mLongDescriptionCtrl->Text() );
	}
	mToolRef.WriteUnlock(tool);
	// END WRITE TOOL BLOCK

	AmToolRoster*		roster = AmToolRoster::Default();
	if (roster) {
		// READ TOOL BLOCK
		const AmTool*		tool = mToolRef.ReadLock();
		if (tool) roster->CreateEntry(tool);
		mToolRef.ReadUnlock(tool);
		// END READ TOOL BLOCK
	}
	SetHasChanges(false);
}

const char* SeqEditToolWindow::EntryName() const
{
	return "tool";
}

void SeqEditToolWindow::SetWindowTitle()
{
	BString		title("Edit ");
	if (mNameCtrl) title << mNameCtrl->Text() << " ";
	title << "Tool";
	SetTitle( title.String() );
}

void SeqEditToolWindow::SetPipelineScrollBars()
{
	if (!mPipelineView || !mPipelineScrollView) return;
	BScrollBar*		b = mPipelineScrollView->ScrollBar(B_HORIZONTAL);
	if (b) arp_setup_horizontal_scroll_bar(b, mPipelineView);
	b = mPipelineScrollView->ScrollBar(B_VERTICAL);
	if (b) arp_setup_vertical_scroll_bar(b, mPipelineView);
}

BView* SeqEditToolWindow::NewGeneralView(BRect frame)
{
	BView*		v = new BView(frame, GENERAL_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	float		divider = v->StringWidth("Author:") + 10;
	BRect		f(spaceX, 0, frame.Width(), fh);
	/* The Name field.
	 */
	mNameCtrl = new BTextControl(f, "name_ctrl", "Name:", NULL, new BMessage(NAME_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mNameCtrl) {
		f.top = mNameCtrl->Frame().bottom;
		mNameCtrl->SetDivider(divider);
		mNameCtrl->MakeFocus(true);
		v->AddChild(mNameCtrl);
	}
	/* The Class Name field.
	 */
	f.top += spaceY;
	f.bottom = f.top + fh;
	mKeyCtrl = new BTextControl(f, "class_name_ctrl", "Key:", NULL, new BMessage(CLASS_NAME_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mKeyCtrl) {
		f.top = mKeyCtrl->Frame().bottom;
		mKeyCtrl->SetDivider(divider);
		v->AddChild(mKeyCtrl);
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

static BMenuField* new_seed_control(BRect f)
{
	BMenu*		menu = new BMenu("seed_menu");
	if (!menu) return NULL;
	BString		name, key;
	for (uint32 k = 0; AmToolSeedI::GetSeedInfo(k, name, key) == B_OK; k++) {
		BMessage*		msg = new BMessage(SEED_MSG);
		if (msg) {
			msg->AddString(SEED_STR, key);
			BMenuItem*	item = new BMenuItem(name.String(), msg);
			if (!item) delete msg;
			else menu->AddItem(item);
		}
	}
	if (menu->CountItems() > 0) menu->AddSeparatorItem();
	BMenuItem*	item = new BMenuItem(NONE_STR, new BMessage(SEED_MSG));
	if (item) {
		item->SetMarked(true);
		menu->AddItem(item);
	}
	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);	
	BMenuField*	field = new BMenuField(f, "seed_field", "Seed:", menu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

BView* SeqEditToolWindow::NewSeedView(BRect frame)
{
	BView*		v = new BView(frame, SEED_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		spaceX = 5, spaceY = 5;

	/* The factory list.
	 */
	BRect		f(spaceX, 0, frame.Width(), frame.Height()  / 2);
	mSeedList = new _SeedList(f);
	if (mSeedList) v->AddChild(mSeedList);
	/* The Seed field.
	 */
	f.top = f.bottom + spaceY;
	f.bottom = f.top + Prefs().Size(MENUFIELD_Y);
	mSeedCtrl = new_seed_control(f);
	if (mSeedCtrl) {
		mSeedCtrl->SetDivider( v->StringWidth("Seed:") + 10 );
		mSeedCtrl->SetEnabled(false);
		v->AddChild(mSeedCtrl);
		f.bottom = mSeedCtrl->Frame().bottom + spaceY;
		if (mSeedList) (dynamic_cast<_SeedList*>(mSeedList))->SetSeedControl(mSeedCtrl);
	}
	/* The target frame to place the seed property views.
	 */
	if (mSeedList) {
		mSeedList->SetPropertyFrame( BRect(f.left, f.bottom, f.right, frame.Height() ) );
	}
	return v;
}

BView* SeqEditToolWindow::NewPipelineView(BRect frame)
{
	BView*		v = new BView(frame, PIPELINE_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float			x = 5, y = 5;
	const char*		addL = "Add";
	const char*		deleteL = "Delete";
	float			buttonW = v->StringWidth(deleteL) + 30, buttonH = 24;
	/* Lay out the views.
	 */
	BRect			addF(x, 0, x + buttonW, buttonH);
	BRect			deleteF(addF.right + x, addF.top, addF.right + x + buttonW, addF.bottom);
	float			sbW = Prefs().Size(V_SCROLLBAR_X) + 3, sbH = Prefs().Size(H_SCROLLBAR_Y) + 3;
	BRect			pipelineF(x, addF.bottom + y, frame.Width() - sbW, frame.Height() - sbH);
	/* Create and add the views.
	 */
	BButton*	button = new BButton(addF, "add_btn", addL, new BMessage(ADD_PIPELINE_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (button) v->AddChild(button);
	button = new BButton(deleteF, "del_btn", deleteL, new BMessage(DELETE_PIPELINE_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (button) v->AddChild(button);
	mPipelineView = new SeqPipelineMatrixView(pipelineF, "pipeline_matrix", mToolRef, NULLINPUTOUTPUT_PIPELINE, SEQ_SUPPRESS_BACKGROUND);
	if (mPipelineView) {
		mPipelineView->SetResizingMode(B_FOLLOW_ALL);
		mPipelineView->SetShowProperties(true);
		mPipelineView->ForceViewColor( tint_color(Prefs().Color(AM_AUX_WINDOW_BG_C), B_DARKEN_1_TINT) );
		mPipelineScrollView = new BScrollView("pipeline_scroll", mPipelineView, B_FOLLOW_ALL, 0, true, true);	
		if (mPipelineScrollView) {
			v->AddChild(mPipelineScrollView);
			mPipelineScrollView->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		} else v->AddChild(mPipelineView);
	}
	return v;
}

BView* SeqEditToolWindow::NewDescriptionView(BRect frame)
{
	BView*			v = new BView(frame, DESCRIPTION_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	float			fh = arp_get_font_height(v);
	float			spaceX = 5, spaceY = 5;
	BRect			shortLabelR(spaceX, 0, frame.Width(), fh);
	BRect			shortR(spaceX, shortLabelR.bottom + spaceY, shortLabelR.right - Prefs().Size(V_SCROLLBAR_X) - 4, shortLabelR.bottom + spaceY + (fh * 3) + spaceY);
	BRect			longLabelR(spaceX, shortR.bottom + spaceY, shortLabelR.right, shortR.bottom + spaceY + fh);
	BRect			longR(spaceX, longLabelR.bottom + spaceY, shortR.right, frame.Height() - 2);

	BStringView*	sv = new BStringView(shortLabelR, "short_label", "Short description:");
	if (sv) v->AddChild(sv);
	sv = new BStringView(longLabelR, "long_label", "Long description:");
	if (sv) v->AddChild(sv);

	mShortDescriptionCtrl = new SeqDumbTextView(shortR, "short_descr", BRect(5, 5, shortR.Width() - 10, 0), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mShortDescriptionCtrl) {
		mShortDescriptionCtrl->SetModificationMessage(new BMessage(DESCRIPTION_MOD_MSG) );
		BScrollView*		sv = new BScrollView("short_scroll", mShortDescriptionCtrl,
												 B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 0, false, true);
		if (sv) v->AddChild(sv);
		else v->AddChild(mShortDescriptionCtrl);
	}
	mLongDescriptionCtrl = new SeqDumbTextView(longR, "long_descr", BRect(5, 5, shortR.Width() - 10, 0), B_FOLLOW_ALL);
	if (mLongDescriptionCtrl) {
		mLongDescriptionCtrl->SetModificationMessage(new BMessage(DESCRIPTION_MOD_MSG) );
		BScrollView*		sv = new BScrollView("long_scroll", mLongDescriptionCtrl,
												 B_FOLLOW_ALL, 0, false, true);
		if (sv) v->AddChild(sv);
		else v->AddChild(mLongDescriptionCtrl);
	}
	return v;
}

static BMenu* new_icon_editor_menu()
{
	BMenu*		menu = new BMenu("Icon");
	if (!menu) return NULL;
#if 0
	BMenuItem*	item = new BMenuItem("New", new BMessage(NEW_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);
	item = new BMenuItem("Delete", new BMessage(DELETE_ICON_MSG), 0, 0);
	if (item) menu->AddItem(item);
	menu->AddSeparatorItem();
#endif
	BMenuItem*	item = new BMenuItem("Copy", new BMessage(COPY_ICON_MSG), 0, 0);
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

BView* SeqEditToolWindow::NewIconView(BRect frame)
{
	BView*		v = new BView(frame, ICON_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	/* The icon editor.
	 */
	BRect		f(5, 0, frame.Width(), frame.Height() );
	mIconEditor = new SeqBitmapEditor(f, ICON_STR, NULL, B_FOLLOW_ALL, new_icon_editor_menu() );
	if (mIconEditor) {
		mIconEditor->SetBitmapChangeMessage( new BMessage(BITMAP_CHANGE_MSG) );
		mIconEditor->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		v->AddChild(mIconEditor);
	}

	return v;
}

BView* SeqEditToolWindow::NewControlView(BRect frame)
{
	BView*		v = new BView(frame, CONTROL_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	/* Layout the controls.
	 */
//	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	float		half = frame.top + (frame.Height() / 2);	
	BRect		controlF(spaceX, 0, frame.Width(), frame.Height() + half);
	float		deleteW = v->StringWidth("Delete") + 10;
	float		buttonH = 24;
	BRect		addControlF(controlF.left, controlF.bottom + spaceY, controlF.left + deleteW, controlF.bottom + spaceY + buttonH);
	BRect		deleteControlF(addControlF);
	deleteControlF.OffsetBy(deleteW + spaceX, 0);

	BRect		addTargetF(controlF.left, frame.bottom - spaceY - buttonH, controlF.left + deleteW, frame.bottom - spaceY);
	BRect		deleteTargetF(addTargetF);
	deleteTargetF.OffsetBy(deleteW + spaceX, 0);
	BRect		targetF(controlF.left, addControlF.bottom + spaceY, controlF.right, addTargetF.top - spaceY);
	/* Add the controls.
	 */
	mToolControlList = new _ToolControlList(controlF, TOOL_CONTROL_LIST_STR, mToolRef);
	if (mToolControlList) {
		v->AddChild(mToolControlList);
	}
	BButton*	button = new BButton(addControlF, ADD_CONTROL_STR, "Add", new BMessage(ADD_CONTROL_MSG), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	if (button) v->AddChild(button);
	mDeleteControlButton = new BButton(deleteControlF, DELETE_CONTROL_STR, "Delete", new BMessage(DELETE_CONTROL_MSG), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	if (mDeleteControlButton) v->AddChild(mDeleteControlButton);

	mControlTargetList = new _ControlTargetList(targetF, CONTROL_TARGET_LIST_STR, mToolRef, mToolControlList);
	if (mControlTargetList) {
		v->AddChild(mControlTargetList);
		if (mToolControlList) mToolControlList->SetTargets(mControlTargetList);
	}
	mAddTargetMenu = new BMenu("Add");
	if (mAddTargetMenu) {
		BMenuField*		field = new BMenuField(addTargetF, "add_target_field", NULL, mAddTargetMenu, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		if (field) v->AddChild(field);
	}
	button = new BButton(deleteTargetF, DELETE_TARGET_STR, "Delete", new BMessage(DELETE_TARGET_MSG), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	if (button) v->AddChild(button);
	return v;
}

static BMenuField* new_effect_control(BRect f)
{
	BMenu*		menu = new BMenu("effect_menu");
	if (!menu) return NULL;
	BString		name, key;
	for (uint32 k = 0; AmGraphicEffect::GetEffectInfo(k, name, key) == B_OK; k++) {
		BMessage*		msg = new BMessage(EFFECT_MSG);
		if (msg) {
			msg->AddString(EFFECT_STR, key);
			BMenuItem*	item = new BMenuItem(name.String(), msg);
			if (!item) delete msg;
			else menu->AddItem(item);
		}
	}
	if (menu->CountItems() > 0) menu->AddSeparatorItem();
	BMenuItem*	item = new BMenuItem(NONE_STR, new BMessage(EFFECT_MSG));
	if (item) {
		item->SetMarked(true);
		menu->AddItem(item);
	}
	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);	
	BMenuField*	field = new BMenuField(f, "effect_field", "Effect:", menu, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	if (!field) {
		delete menu;
		return NULL;
	}
	field->SetEnabled(false);
	return field;
}

BView* SeqEditToolWindow::NewEffectsView(BRect frame)
{
	BView*		v = new BView(frame, EFFECTS_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	/* The Effect field.
	 */
	BRect		f(spaceX, frame.Height() - fh - 10, frame.Width(), frame.Height() );
	mEffectCtrl = new_effect_control(f);
	if (mEffectCtrl) {
		mEffectCtrl->SetDivider( v->StringWidth("Effect:") + 10 );
		v->AddChild(mEffectCtrl);
		f.bottom = mEffectCtrl->Frame().top - spaceY;
	}
	/* The factory list.
	 */
	f.bottom = f.top - spaceY;
	f.top = 0;
	mGraphicList = new _GraphicList(f);
	if (mGraphicList) {
		mGraphicList->SetMenuControl(mEffectCtrl);
		v->AddChild(mGraphicList);
	}
	return v;
}

/********************************************************
 * _VIEW-LIST
 ********************************************************/
_ViewList::_ViewList(BRect frame, const char* name)
		: inherited(frame, name, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
					B_WILL_DRAW, B_NO_BORDER)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	AddColumn( new BStringColumn(VIEW_STR, 170, 20, 350, B_TRUNCATE_END), NAME_FIELD);
}

void _ViewList::SelectionChanged()
{
	inherited::SelectionChanged();
//	if (!Window()->Lock() ) return;
	if (!Parent() ) return;
	BView*			v = Parent()->FindView("key_properties");
	if (v && Parent()->RemoveChild(v) ) delete v;

	_ViewRow*		selection = dynamic_cast<_ViewRow*>( CurrentSelection() );
	if (!selection) return;
	BString			factoryKey, viewKey, key;
	selection->GetKeyInfo(factoryKey, viewKey, key);
	v = NewViewForKey(factoryKey, viewKey, key);
	if (!v) return;
	v->SetName("key_properties");
	BRect		f = Frame();
	v->MoveTo(f.left, mPropertyFrame.top);
	v->ResizeTo(f.Width(), mPropertyFrame.Height() );
//	v->MoveTo( mPropertyFrame.LeftTop() );
//	v->ResizeTo( mPropertyFrame.Width(), mPropertyFrame.Height() );
	Parent()->AddChild(v);
}

void _ViewList::SetPropertyFrame(BRect propertyFrame)
{
	mPropertyFrame = propertyFrame;
}

void _ViewList::SetTool(const AmTool* tool)
{
	ArpASSERT(tool);
	ClearRowKeys();

	BString		factoryKey, viewKey, key;
	for (uint32 k = 0; GetToolKey(k, factoryKey, viewKey, key, tool) == B_OK; k++) {
		if (key.Length() > 0) {
			_ViewRow*	row = ViewRow(factoryKey, viewKey);
			if (row) row->SetKey(key.String() );
		}
		factoryKey = viewKey = key = (const char*)NULL;
	}
	mToolRef.SetTo(tool);
}

void _ViewList::SetKeyForCurrent(const char* key, AmTool* tool)
{
	_ViewRow*		selection = dynamic_cast<_ViewRow*>( CurrentSelection() );
	if (!selection) return;
	selection->SetToolKey(key, tool);
	UpdateRow(selection);
}

void _ViewList::BuildRows()
{
	_ViewRow*	root = NewRow(true);
	if (!root) return;
	AddRow(root);

	AmViewFactory*		factory;
	for (uint32 k = 0; (factory = AmGlobals().FactoryAt(k)) != NULL; k++) {
		BString			facKey = factory->Signature();
		_ViewRow*		facrow = NewRow(facKey);
		if (facrow) {
			AddRow(facrow, root);
			BString		viewKey;
			for (uint32 k = 0; factory->DataNameAt(k, PRI_VIEW, viewKey) == B_OK; k++) {
				_ViewRow*	viewrow = NewRow(facKey, viewKey);
				if (viewrow) AddRow(viewrow, facrow);
				viewKey = (const char*)NULL;
			}
			for (uint32 k = 0; factory->DataNameAt(k, SEC_VIEW, viewKey) == B_OK; k++) {
				if (!HasRow(facKey, viewKey)) {
					_ViewRow*	viewrow = NewRow(facKey, viewKey);
					if (viewrow) AddRow(viewrow, facrow);
				}
				viewKey = (const char*)NULL;
			}
		}
	}
}

bool _ViewList::HasRow(const BString& facKey, const BString& viewKey) const
{
	return false;
}

void _ViewList::ClearRowKeys(BRow* parent)
{
	/* This is a work around -- currently there's
	 * a bug in the column list view that causes
	 * a crash if you try to access the child information
	 * of a child with no children.  If yaknowwhatImean.
	 */
	if (parent && !parent->HasLatch() ) return;
	BRow*	row;
	int32	count = CountRows(parent);
	for (int32 k = 0; k < count; k++) {
		if ((row = RowAt(k, parent)) != NULL) {
			_ViewRow*	vr = dynamic_cast<_ViewRow*>(row);
			if (vr) {
				vr->SetKey(NULL);
				ClearRowKeys(row);
			}
		}
	}
}

_ViewRow* _ViewList::ViewRow(	const BString& factoryKey,
								const BString& viewKey,
								BRow* parent)
{
	if (parent && !parent->HasLatch() ) return NULL;
	BRow*	row;
	int32	count = CountRows(parent);
	for (int32 k = 0; k < count; k++) {
		if ((row = RowAt(k, parent)) != NULL) {
			_ViewRow*	vr = dynamic_cast<_ViewRow*>(row);
			if (vr && vr->Matches(factoryKey, viewKey) ) return vr;
			vr = ViewRow(factoryKey, viewKey, row);
			if (vr) return vr;
		}
	}
	return NULL;
}

/********************************************************
 * _SEED-LIST
 ********************************************************/
_SeedList::_SeedList(BRect frame)
		: inherited(frame, SEED_LIST_STR), mSeedCtrl(NULL)
{
	BuildRows();
	AddColumn( new BStringColumn(SEED_STR, 70, 20, 350, B_TRUNCATE_END), KEY_FIELD);
//	SetSelectionMessage( new BMessage(SEED_ROW_SELECTED) );
}

void _SeedList::SelectionChanged()
{
	inherited::SelectionChanged();
	if (mSeedCtrl) {
		_ViewRow*	r = dynamic_cast<_ViewRow*>(CurrentSelection() );
		if (r) {
			if (!mSeedCtrl->IsEnabled() ) mSeedCtrl->SetEnabled(true);
			BString		fact, viewKey, key, label;
			r->GetKeyInfo(fact, viewKey, key);
			AmToolSeedI::GetSeedInfo(key, label);
			BMenu*		m = mSeedCtrl->Menu();
			if (m) {
				BMenuItem*	item;
				if (key.Length() > 0 && (item = m->FindItem(label.String() )) ) item->SetMarked(true);
				else if ( (item = m->FindItem(NONE_STR)) ) item->SetMarked(true);
			}
		} else {
			if (mSeedCtrl->IsEnabled() ) mSeedCtrl->SetEnabled(false);
		}
	}
}

void _SeedList::SetSeedControl(BMenuField* seedCtrl)
{
	mSeedCtrl = seedCtrl;
}

_ViewRow* _SeedList::NewRow(bool root) const
{
	return new _SeedRow(root);
}

_ViewRow* _SeedList::NewRow(const BString& factoryKey) const
{
	return new _SeedRow(factoryKey);
}

_ViewRow* _SeedList::NewRow(const BString& factoryKey, const BString& viewKey) const
{
	return new _SeedRow(factoryKey, viewKey);
}

status_t _SeedList::GetToolKey(	uint32 index,
								BString& outFactoryKey,
								BString& outViewKey,
								BString& outKey,
								const AmTool* tool) const
{
	return tool->GetSeed(index, outFactoryKey, outViewKey, outKey);
}

BView* _SeedList::NewViewForKey(const BString& factoryKey,
								const BString& viewKey,
								const BString& key) const
{
	BView*		v = AmToolSeedI::NewView(key);
	if (v) AmToolSeedI::ConfigureView(v, mToolRef, factoryKey, viewKey, key);
	return v;
}

/********************************************************
 * _GRAPHIC-LIST
 ********************************************************/
_GraphicList::_GraphicList(BRect frame)
		: inherited(frame, GRAPHIC_LIST_STR), mMenuCtrl(NULL)
{
	SetResizingMode(B_FOLLOW_ALL);
	BuildRows();
	AddColumn( new BStringColumn(EFFECT_STR, 70, 20, 350, B_TRUNCATE_END), KEY_FIELD);
}

void _GraphicList::SelectionChanged()
{
	inherited::SelectionChanged();
	if (mMenuCtrl && mMenuCtrl->Menu() ) {
		_ViewRow*		r = dynamic_cast<_ViewRow*>(CurrentSelection() );
		if (!r) mMenuCtrl->SetEnabled(false);
		else {
			if (!mMenuCtrl->IsEnabled() ) mMenuCtrl->SetEnabled(true);
			BString		fact, view, key;
			r->GetKeyInfo(fact, view, key);
			BString		label;
			AmGraphicEffect::GetEffectInfo(key, label);
			if (label.Length() < 1) label = NONE_STR;
			BMenuItem*	item = mMenuCtrl->Menu()->FindItem(label.String() );
			if (item) item->SetMarked(true);
		}
	}
}

void _GraphicList::SetMenuControl(BMenuField* menuCtrl)
{
	mMenuCtrl = menuCtrl;
}

_ViewRow* _GraphicList::NewRow(bool root) const
{
	return new _GraphicRow(root);
}

_ViewRow* _GraphicList::NewRow(const BString& factoryKey) const
{
	return new _GraphicRow(factoryKey);
}

_ViewRow* _GraphicList::NewRow(const BString& factoryKey, const BString& viewKey) const
{
	return new _GraphicRow(factoryKey, viewKey);
}

status_t _GraphicList::GetToolKey(	uint32 index,
									BString& outFactoryKey,
									BString& outViewKey,
									BString& outKey,
									const AmTool* tool) const
{
	return tool->Graphic(index, outFactoryKey, outViewKey, outKey);
}

BView* _GraphicList::NewViewForKey(	const BString& factoryKey,
									const BString& viewKey,
									const BString& key) const
{
	return NULL;
}

/********************************************************
 * _VIEW-ROW
 ********************************************************/
_ViewRow::_ViewRow(bool root)
		: mRoot(root)
{
	if (mRoot) SetField(new BStringField("All Views"), NAME_FIELD);
}

_ViewRow::_ViewRow(const BString& factoryKey)
		: mRoot(false), mFactoryKey(factoryKey)
{
	SetField(new BStringField( mFactoryKey.String() ), NAME_FIELD);
}

_ViewRow::_ViewRow(const BString& factoryKey, const BString& viewKey)
		: mRoot(false), mFactoryKey(factoryKey), mViewKey(viewKey)
{
	SetField(new BStringField( mViewKey.String() ), NAME_FIELD);
}

bool _ViewRow::HasLatch() const
{
	if (mViewKey.Length() > 0) return false;
	return true;
}

bool _ViewRow::Matches(	const BString& factoryKey,
						const BString& viewKey) const
{
	return mFactoryKey == factoryKey && mViewKey == viewKey;
}

void _ViewRow::SetKey(const char* key)
{
	mKey = key;
	GetKeyLabel(key, mLabel);
	SetField(new BStringField( mLabel.String() ), KEY_FIELD);
}

void _ViewRow::GetKeyInfo(BString& factoryKey, BString& viewKey, BString& key) const
{
	factoryKey = mFactoryKey;
	viewKey = mViewKey;
	key = mKey;
}

void _ViewRow::Print() const
{
	if (mRoot) printf("root, key: %s\n", mKey.String());
	else {
		printf("factory: %s, view: %s, key: %s\n",
				mFactoryKey.String(),  mViewKey.String(), mKey.String() );
	}
}

/********************************************************
 * _SEED-ROW
 ********************************************************/
_SeedRow::_SeedRow(bool root)
		: inherited(root)
{
}

_SeedRow::_SeedRow(const BString& factoryKey)
		: inherited(factoryKey)
{
}

_SeedRow::_SeedRow(const BString& factoryKey, const BString& viewKey)
		: inherited(factoryKey, viewKey)
{
}

void _SeedRow::SetToolKey(const char* key, AmTool* tool)
{
	SetKey(key);
	BString		seedKey;
	if (key) seedKey = key;
	tool->SetSeed(mFactoryKey, mViewKey, seedKey);
}

void _SeedRow::GetKeyLabel(const char* key, BString& outLabel)
{
	if (!key) {
		outLabel = NONE_STR;
		return;
	}
	BString		label;
	if (AmToolSeedI::GetSeedInfo(BString(key), label) != B_OK)
		outLabel = key;
	else
		outLabel = label;	
}

/********************************************************
 * _GRAPHIC-ROW
 ********************************************************/
_GraphicRow::_GraphicRow(bool root)
		: inherited(root)
{
}

_GraphicRow::_GraphicRow(const BString& factoryKey)
		: inherited(factoryKey)
{
}

_GraphicRow::_GraphicRow(const BString& factoryKey, const BString& viewKey)
		: inherited(factoryKey, viewKey)
{
}

void _GraphicRow::SetToolKey(const char* key, AmTool* tool)
{
	SetKey(key);
	BString		graphicKey;
	if (key) graphicKey = key;
	tool->SetGraphic(mFactoryKey, mViewKey, graphicKey);
}

void _GraphicRow::GetKeyLabel(const char* key, BString& outLabel)
{
	if (!key) {
		outLabel = NONE_STR;
		return;
	}
	BString		label;
	if (AmGraphicEffect::GetEffectInfo(BString(key), label) != B_OK)
		outLabel = key;
	else
		outLabel = label;	
}

/********************************************************
 * _TOOL-CONTROL-LIST
 ********************************************************/
_ToolControlList::_ToolControlList(	BRect frame, const char* name,
									AmToolRef toolRef)
		: inherited(frame, name, B_FOLLOW_ALL,
					B_WILL_DRAW, B_NO_BORDER),
		  mToolRef(toolRef), mTargets(NULL)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortingEnabled(false);
	AddColumn( new BStringColumn("", 50, 20, 150, B_TRUNCATE_END), TC_INDEX_FIELD);
	AddColumn( new BStringColumn("Name", 200, 20, 350, B_TRUNCATE_END), TC_NAME_FIELD);
}

void _ToolControlList::SelectionChanged()
{
	inherited::SelectionChanged();
	if (mTargets) mTargets->BuildRows();
}

void _ToolControlList::SetTool(const AmTool* tool)
{
	mToolRef.SetTo(tool);
	if (tool) BuildRows(tool);
}

void _ToolControlList::SetTargets(_ControlTargetList* targets)
{
	mTargets = targets;
}

void _ToolControlList::BuildRows()
{
	/* All this selection nonsense is because building the rows might
	 * cause the control target to rebuild its rows, which gets another
	 * lock, which would nest them.
	 */
	BRow*				selection = CurrentSelection();
	int32				selectionIndex = -1;
	if (selection) selectionIndex = IndexOf(selection);
	DeselectAll();
	// READ TOOL BLOCK
	const AmTool*	tool = mToolRef.ReadLock();
	if (tool) BuildRows(tool);
	mToolRef.ReadUnlock(tool);
	// END READ TOOL BLOCK
	if (selectionIndex >= 0) {
		selection = RowAt(selectionIndex);
		if (selection) AddToSelection(selection);
	}
}

void _ToolControlList::BuildRows(const AmTool* tool)
{
	/* FIX: This is temporary until I figure out why it's not working to
	 * just do the set and remove.
	 */
	BRow*		r;
	while ( (r = RowAt(0)) != NULL ) {
		RemoveRow(r);
		delete r;
	}

	ArpASSERT(tool);
	if (!tool) return;

	AmToolControlList*	list = tool->ControlList();
	if (!list) return;

	AmToolControl*	control;
	for (uint32 k = 0; list->GetControl(k, &control) == B_OK; k++) {
		_ToolControlRow*	row = new _ToolControlRow(control, k);
		if (row) AddRow(row);
	}
}

/********************************************************
 * _TOOL-CONTROL-ROW
 ********************************************************/
_ToolControlRow::_ToolControlRow(	const AmToolControl* control,
									uint32 index)
		: mIndex(index), mType(control->Type() )
{
	ArpASSERT(control);
	mToolControlId = (void*)control;

	BString		n;
	n << mIndex;
	SetField(new BStringField(n.String() ), TC_INDEX_FIELD);
	SetField(new BStringField("Tool Control"), TC_NAME_FIELD);
}
	
bool _ToolControlRow::HasLatch() const
{
	return false;
}

uint32 _ToolControlRow::Type() const
{
	return mType;
}

uint32 _ToolControlRow::Index() const
{
	return mIndex;
}

/********************************************************
 * _CONTROL-TARGET-LIST
 ********************************************************/
_ControlTargetList::_ControlTargetList(	BRect frame, const char* name,
										AmToolRef toolRef,
										_ToolControlList* controls)
		: inherited(frame, name, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
					B_WILL_DRAW, B_NO_BORDER),
		  mToolRef(toolRef), mControls(controls)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortingEnabled(false);
	AddColumn( new BStringColumn("Pipeline", 50, 20, 150, B_TRUNCATE_END), CT_PIPELINE_FIELD);
	AddColumn( new BStringColumn("Filter", 50, 20, 350, B_TRUNCATE_END), CT_FILTER_FIELD);
	AddColumn( new BStringColumn("Property", 100, 20, 350, B_TRUNCATE_END), CT_PROPERTY_FIELD);
}

void _ControlTargetList::SelectionChanged()
{
	inherited::SelectionChanged();
}

void _ControlTargetList::SetTool(const AmTool* tool)
{
	mToolRef.SetTo(tool);
	if (tool) {
		_ToolControlRow*	row = NULL;
		if (mControls) row = dynamic_cast<_ToolControlRow*>( mControls->CurrentSelection() );
		BuildRows(row, tool);
	}
}

void _ControlTargetList::BuildRows()
{
	_ToolControlRow*	row = NULL;
	if (mControls) row = dynamic_cast<_ToolControlRow*>( mControls->CurrentSelection() );
	
	// READ TOOL BLOCK
	const AmTool*	tool = mToolRef.ReadLock();
	if (tool) BuildRows(row, tool);
	mToolRef.ReadUnlock(tool);
	// END READ TOOL BLOCK
}

void _ControlTargetList::BuildRows(_ToolControlRow* controlRow, const AmTool* tool)
{
	/* FIX: This is temporary until I figure out why it's not working to
	 * just do the set and remove.
	 */
	BRow*		r;
	while ( (r = RowAt(0)) != NULL ) {
		RemoveRow(r);
		delete r;
	}

	ArpASSERT(tool);
	if (!controlRow || !tool) return;
	AmToolControlList*	list = tool->ControlList();
	if (!list) return;
	AmToolControl*	control;
	if (list->GetControl(controlRow->Index(), &control) != B_OK) return;

	int32		pipeline, filter;
	BString		name;
	for (uint32 k = 0; control->GetTarget(k, &pipeline, &filter, &name) == B_OK; k++) {
		_ControlTargetRow*	row = new _ControlTargetRow(k, pipeline, filter, name);
		if (row) AddRow(row);
	}
}

/********************************************************
 * _CONTROL-TARGET-ROW
 ********************************************************/
_ControlTargetRow::_ControlTargetRow(	uint32 index, int32 pipeline,
										int32 filter, const BString& name)
		: mIndex(index), mPipeline(pipeline), mFilter(filter), mName(name)
{
	BString		n1;
	n1 << mPipeline + 1;
	SetField(new BStringField(n1.String() ), CT_PIPELINE_FIELD);
	BString		n2;
	n2 << mFilter + 1;
	SetField(new BStringField(n2.String() ), CT_FILTER_FIELD);
	SetField(new BStringField(mName.String()), CT_PROPERTY_FIELD);
}
	
bool _ControlTargetRow::HasLatch() const
{
	return false;
}

uint32 _ControlTargetRow::Index() const
{
	return mIndex;
}

int32 _ControlTargetRow::Pipeline() const
{
	return mPipeline;
}

int32 _ControlTargetRow::Filter() const
{
	return mFilter;
}




#if 0
/*************************************************************************
 * _SEQ-KEY-FACTORY
 *************************************************************************/
_SeqKeyFactory::_SeqKeyFactory()
		: mNewFunction(NULL)
{
}

_SeqKeyFactory::~_SeqKeyFactory()
{
}

void _SeqKeyFactory::SetNewFunction(new_function)
{
	mNewFunction = new_function;
}
	
BView* _SeqKeyFactory::ViewFor(	const BString& key,
								const BMessage* config)
{
	printf("In the factory\n");
	if (!mNewFunction) return NULL;
	BView*		v = mNewFunction(key);
	if (!v) return NULL;
	return v;
}
#endif
