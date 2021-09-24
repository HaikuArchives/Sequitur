#include <cstdio>
#include <experimental/ColumnListView.h>
#include <experimental/ColumnTypes.h>
#include <StorageKit.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ViewTools.h>
#include <ArpInterface/ArpPrefs.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlRootNode.h>
#include "GlPublic/GlStrainedParamList.h"
#include <GlKernel/GlDefs.h>
#include "Glasslike/GlApp.h"
#include "Glasslike/GlPrefWin.h"
#include <Glasslike/GlDefs.h>
#include <Glasslike/SZ.h>
class GlMidiBindingRow;

static const uint32		OK_MSG						= 'iok_';
static const uint32		CANCEL_MSG					= 'icnc';
// General
static const uint32		_CREATOR_MSG				= 'iGCr';
static const uint32		_KEY_MSG					= 'iGKy';
// Midi
static const uint32		LEARN_ONE_MSG				= 'ilr1';
static const uint32		LEARN_MANY_MSG				= 'ilrM';
static const uint32		DELETE_MSG					= 'ideM';
static const uint32		MIDI_RT_MSG					= 'iMRt';
static const uint32		MIDI_FILE_MSG				= 'iMFm';
static const uint32		MIDI_ROOT_MSG				= 'iMRo';
static const uint32		MIDI_PARAM_MSG				= 'iMPm';
#if 0
static const uint32		MIDI_RANGE_MSG				= 'iMrn';
#endif

static const int32		PORT_COL					= 0;
static const int32		CHANNEL_COL					= 1;
static const int32		EVENT_COL					= 2;
static const int32		CONTROLLER_COL				= 3;

static const char*		WIN_NAME					= "Preferences";
static const char*		GENERAL_STR					= "General";
static const char*		MIDI_STR					= "MIDI";
static const char*		TABVIEW_STR					= "tabview";

/********************************************************
 * GL-MIDI-BINDINGS-LIST-VIEW
 ********************************************************/
class GlMidiBindingsListView : public BColumnListView
{
public:
	GlMidiBindingsListView(BRect frame, GlPrefWin* win);

	virtual void			SelectionChanged();

	bool					HasEvent(GlMidiEvent event);
	void					SelectEvent(GlMidiEvent event,
										const GlMidiBinding* binding = 0,
										bool assignLetter = false);

	void					AddBinding(const GlMidiBinding* binding);

private:
	typedef BColumnListView inherited;
	GlPrefWin*				mPrefWin;

	GlMidiBindingRow*		GetEventRow(GlMidiEvent event);
};

/********************************************************
 * GL-MIDI-BINDING-ROW
 ********************************************************/
class GlMidiBindingRow : public BRow
{
public:
	GlMidiBindingRow(	GlMidiEvent event,
						const GlMidiBinding* binding);
	virtual ~GlMidiBindingRow();

	GlMidiBinding		binding;
	std::vector<BString16>	rootNames;
	std::vector<BString16>	paramNames;

	virtual bool		HasLatch() const		{ return false; }

	bool				IsDirty() const;
	void				SetDirty();
	
	GlMidiEvent			Event() const;
	bool				Matches(const GlMidiEvent& event) const;

	int32				Rt() const;
	void				SetRt(int32 midi);

	status_t			SetFileName(const BString16& fn);
	BString16			FileName() const;
	
	status_t			LoadRoot(int32 index);
	GlRootNode*			Root();
	int32				RootIndex() const;
		
	void				SetParam(int32 index);
	int32				Param() const;

	void				SetRange(float min, float max);
	void				GetRange(float* min, float* max) const;

private:
	bool				mDirty;
};

/*************************************************************************
 * GL-PREF-WIN
 *************************************************************************/
GlPrefWin::GlPrefWin(BRect frame, BMessage* config)
		: inherited(frame, WIN_NAME, B_TITLED_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS),
		  mCreator(0), mKey(0), mMidiLearn(0), mMidiListView(0),
		  mMidiLearnOne(0), mMidiLearnMany(0), mMidiDelete(0),
		  mMidiRtMenu(0), mMidiFileName(0), mMidiRootMenu(0),
		  mMidiParamMenu(0)
#if 0
		  mMidiMinCtrl(0), mMidiMaxCtrl(0)
#endif
{
	BMessage			preferences;
	AddViews(preferences);
}

GlPrefWin::~GlPrefWin()
{
}

void GlPrefWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case OK_MSG:
			OkGeneral();
			OkMidi();
			PostMessage(B_QUIT_REQUESTED);
			break;
		case CANCEL_MSG:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case _CREATOR_MSG:
			break;
		case _KEY_MSG:
			break;

		case LEARN_ONE_MSG:
		case LEARN_MANY_MSG:
			LearnMidi(msg->what);
			break;
		case DELETE_MSG:
			DeleteMidi();
			break;
		case GL_MIDI_EVENT_MSG:
			ReceiveMidi(msg);
			break;

		case MIDI_RT_MSG:
			MidiRtChanged();
			break;
		case MIDI_FILE_MSG:
			MidiFileChanged();
			break;
		case MIDI_ROOT_MSG:
			MidiRootChanged();
			break;
		case MIDI_PARAM_MSG:
			MidiParamChanged();
			break;
#if 0
		case MIDI_RANGE_MSG:
			MidiRangeChanged();
			break;
#endif
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

bool GlPrefWin::QuitRequested()
{
	gl_app->UnsetMidiTarget(BMessenger(this));
	return true;
}

void GlPrefWin::WindowActivated(bool state)
{
	if (!state) {
		gl_app->UnsetMidiTarget(BMessenger(this));
		if (mMidiLearnOne) mMidiLearnOne->SetEnabled(true);
		if (mMidiLearnMany) mMidiLearnMany->SetEnabled(true);
	}
}

void GlPrefWin::MidiSelectionChanged()
{
	ArpVALIDATE(mMidiListView, return);
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());

	if (mMidiRtMenu) {
		if (!row) mMidiRtMenu->SetCurrentIndex(-1);
		else mMidiRtMenu->SetCurrentIndex(row->Rt());
	}

	if (mMidiFileName) {
		BString16			fn;
		if (row) fn = row->FileName();
		mMidiFileName->SetFileNameNoInvoke(&fn);
		mMidiFileName->Invalidate();
	}
	if (mMidiRootMenu) {
		BMessage			items;
		if (row) {
			for (uint32 k = 0; k < row->rootNames.size(); k++)
				items.AddString("item", row->rootNames[k].String());
		}
		mMidiRootMenu->ReplaceItems(items);
		int32				index = 0;
		if (row) index = row->RootIndex();
		mMidiRootMenu->SetCurrentIndex(index);
	}
	if (mMidiParamMenu) {
		BMessage			items;
		if (row) {
			for (uint32 k = 0; k < row->paramNames.size(); k++)
				items.AddString("item", row->paramNames[k].String());
		}
		mMidiParamMenu->ReplaceItems(items);
		int32				index = 0;
		if (row) index = row->Param();
		mMidiParamMenu->SetCurrentIndex(index);
	}
#if 0
	float					min = 0, max = 1;
	if (row) row->GetRange(&min, &max);

	if (mMidiMinCtrl) mMidiMinCtrl->SetValue(min);
	if (mMidiMaxCtrl) mMidiMaxCtrl->SetValue(max);
#endif
	if (mMidiDelete) {
		if (row && mMidiDelete->IsEnabled() == false) mMidiDelete->SetEnabled(true);
		else if (!row && mMidiDelete->IsEnabled()) mMidiDelete->SetEnabled(false);
	}
}

void GlPrefWin::OkGeneral()
{
	if (!mCreator || !mKey) return;
	GlKeyTracker&		tracker = gl_app->KeyTracker();
	const BString16		t = mCreator->Text();
	tracker.SetCurrent(t, mKey->Value());
}

void GlPrefWin::OkMidi()
{
	if (!mMidiListView) return;
	GlMidiBindingList&		bindings = gl_app->MidiBindings();
	/* Remove all bindings in the delete list (although we don't have
	 * to remove any that have a binding row with the same event, as it
	 * would just get re-added.
	 */
	for (uint32 k = 0; k < mDeletedBindings.size(); k++) {
		if (!(mMidiListView->HasEvent(mDeletedBindings[k])))
			bindings.Delete(mDeletedBindings[k]);
	}
	/* Update any bindings that are new or have changed.
	 */
	int32					count = mMidiListView->CountRows();
	for (int32 k = 0; k < count; k++) {
		BRow*				r = mMidiListView->RowAt(k);
		if (r) {
			GlMidiBindingRow*	br = (GlMidiBindingRow*)r;
			if (br->IsDirty()) bindings.Update(&(br->binding));
		}
	}
}

status_t GlPrefWin::LearnMidi(uint32 what)
{
	mMidiLearn = what;
	if (mMidiLearnOne) mMidiLearnOne->SetEnabled(false);
	if (mMidiLearn != LEARN_MANY_MSG) mMidiLearnMany->SetEnabled(false);
	gl_app->SetMidiTarget(BMessenger(this));
	return B_OK;
}

status_t GlPrefWin::ReceiveMidi(BMessage* msg)
{
	ArpASSERT(msg);
	GlMidiEvent		event;
	status_t		err = event.ReadFrom(*msg);
	if (mMidiListView && err == B_OK) mMidiListView->SelectEvent(event, 0, true);

	if (mMidiLearn != LEARN_MANY_MSG) {
		gl_app->SetMidiTarget(BMessenger());
		if (mMidiLearnOne) mMidiLearnOne->SetEnabled(true);
		if (mMidiLearnMany) mMidiLearnMany->SetEnabled(true);
		mMidiLearn = 0;
	}
	return err;
}

status_t GlPrefWin::DeleteMidi()
{
	if (!mMidiListView) return B_ERROR;
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (!row) return B_ERROR;
	
	mDeletedBindings.push_back(row->Event());
	mMidiListView->RemoveRow(row);
	delete row;

	return B_OK;
}

status_t GlPrefWin::MidiRtChanged()
{
	if (!mMidiListView || !mMidiRtMenu) return B_ERROR;
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (!row) return B_ERROR;

	row->SetRt(mMidiRtMenu->CurrentIndex());

	mMidiListView->Invalidate();

	return B_OK;
}

status_t GlPrefWin::MidiFileChanged()
{
	if (!mMidiListView || !mMidiFileName) return B_ERROR;
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (!row) return B_ERROR;
	row->SetFileName(mMidiFileName->FileName());

	BuildRootMenu();
	BuildParamMenu();

	return B_OK;
}

status_t GlPrefWin::MidiRootChanged()
{
	if (!mMidiListView || !mMidiRootMenu) return B_ERROR;
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (!row) return B_ERROR;

	row->LoadRoot(mMidiRootMenu->CurrentIndex());
	BuildParamMenu();

	return B_OK;
}

status_t GlPrefWin::MidiParamChanged()
{
	if (!mMidiListView || !mMidiParamMenu) return B_ERROR;
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (!row) return B_ERROR;

	int32					id;
	if (mMidiParamMenu->GetCurrentId(&id) == B_OK)
		row->SetParam(id);

	return B_OK;
}

#if 0
status_t GlPrefWin::MidiRangeChanged()
{
	if (!mMidiListView || !mMidiMinCtrl || !mMidiMaxCtrl) return B_ERROR;
	GlMidiBindingRow*		row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (!row) return B_ERROR;

	float					min = mMidiMinCtrl->Value(),
							max = mMidiMaxCtrl->Value();
	if (min < 0.0) min = 0.0;
	if (max > 1.0) max = 1.0;
	row->SetRange(min, max);

	return B_OK;
}
#endif

/* THIS IS A TOTAL HACK!  If the way labels are stored changes,
 * this breaks.  Not sure what to do about that now, because for
 * all other purposes label storing is so much better now.
 */
static status_t	_get_label(const BMessage& msg, BString16& lbl)
{
	BMessage			params;
	if (msg.FindMessage("node_params", &params) != B_OK) return B_ERROR;
//printf("ALL PARAMS\n");
//params.PrintToStream();

	BMessage			p;
	for (int32 i = 0; params.FindMessage("p", i, &p) == B_OK; i++) {
//printf("PARAM\n");
//p.PrintToStream();
		int32			i32;
		if (p.FindInt32("ky", &i32) == B_OK && i32 == GL_PARAM_ROOT_LABEL) {
			const char*	str;
			if (p.FindString("val", &str) == B_OK && str) {
				lbl = str;
				return B_OK;
			}
		}
	}
	return B_ERROR;
}

status_t GlPrefWin::BuildRootMenu()
{
	if (!mMidiListView || !mMidiRootMenu) return B_ERROR;
	BMessage					items;
	GlMidiBindingRow*			row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	if (row) {
		row->rootNames.resize(0);
		row->paramNames.resize(0);
		BMessage				projectMsg;
		BFile					file;
		BEntry					entry(row->FileName().String());
		if (entry.InitCheck() == B_OK && entry.IsFile()
				&& file.SetTo(&entry, B_READ_ONLY) == B_OK
				&& projectMsg.Unflatten(&file) == B_OK) {
			/* The file format has a series of BMessages storing matrices.
			 */
			int32				msgIndex = 0;
			BMessage			rootMsg;
			while (projectMsg.FindMessage(GL_ROOT_STR, msgIndex, &rootMsg) == B_OK) {
//rootMsg.PrintToStream();
				if (rootMsg.what == GL_ROOT_MSG) {
					BString16	lbl;
					if (_get_label(rootMsg, lbl) == B_OK) {
						row->rootNames.push_back(lbl);
						items.AddString("item", lbl);
					}
				}
				rootMsg.MakeEmpty();
				msgIndex++;
			}
		}
	}
	mMidiRootMenu->ReplaceItems(items);
	mMidiRootMenu->SetCurrentIndex(0);
	if (row) row->LoadRoot(0);

	return B_OK;
}

status_t GlPrefWin::BuildParamMenu()
{
	if (!mMidiListView || !mMidiParamMenu) return B_ERROR;

	BMessage					items;
	int32						index = -1, count = 0;
	GlMidiBindingRow*			row = (GlMidiBindingRow*)(mMidiListView->CurrentSelection());
	GlRootNode*					root;
	if (row && (root = row->Root()) != 0) {
		row->paramNames.resize(0);
		GlStrainedParamList		params;
		root->GetParams(params);
		GlStrainedParam			p;
		uint32					size = params.Size();
		for (uint32 k = 0; k < size; k++) {
			if (params.At(k, p) == B_OK) {
				ArpASSERT(p.nid && p.pt && p.label);
				uint32			flags = p.pt->StateFlags();
				if (!(flags&GL_ROOT_INFO_F)) {
					row->paramNames.push_back(*p.label);
					items.AddString(ARP_MENU_ITEM_STR, *p.label);
					items.AddInt32(ARP_MENU_ID_STR, count);
					if (index < 0) index = count;
				}
			}
			count++;
		}
	}

	mMidiParamMenu->ReplaceItems(items);
	mMidiParamMenu->SetCurrentIndex(0);
	if (index >= 0) row->SetParam(index);

//	if (mMidiMinCtrl) mMidiMinCtrl->SetValue(0);
//	if (mMidiMaxCtrl) mMidiMaxCtrl->SetValue(1);

	return B_OK;
}

void GlPrefWin::AddViews(const BMessage& prefs)
{
	BRect		b = Bounds();
	/* Add the bottom panel with the Cancel OK buttons.  When this
	 * block is done, it will adjust the bounds accordingly (i.e.,
	 * without the space used by this view).
	 */
	{
		float		buttonW = 60, buttonH = 24;
		float		edgeR = 8, edgeB = 8, edgeT = 8;
		BRect		f(b);
		f.top = f.bottom - edgeB - buttonH - edgeT;
		BView*		v = new BView(f, "button_panel", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, 0);
		if (v) {
			BRect		f( b.right - edgeR - buttonW, edgeT, b.right - edgeR, edgeT + buttonH );
			BButton*	button = new BButton( f, "ok_button", SZ(SZ_OK)->String(), new BMessage( OK_MSG ), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
			if (button) {
				v->AddChild( button );
				button->MakeDefault( true );
			}
			f.OffsetBy( 0-(buttonW + 10), 0 );	
			button = new BButton( f, "cancel_button", SZ(SZ_Cancel)->String(), new BMessage( CANCEL_MSG ), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
			if (button) v->AddChild(button);

//			v->SetViewColor(Prefs().Color(AM_AUX_WINDOW_BG_C));
			AddChild(v);
			b.bottom = b.bottom - edgeB - buttonH - edgeT - 1;
		}
	}
	BTabView*	tv = new BTabView(b, TABVIEW_STR);
	if (!tv) return;
	BView*		generalView = NewGeneralView(b, prefs);
	if (generalView) tv->AddTab(generalView);
	BView*		midiView = NewMidiView(b, prefs);
	if (midiView) tv->AddTab(midiView);

	AddChild(tv);
	/* NOTE:  Have to do this after the tab view's been added to the window or else
	 * you get a crash.  It's a bug in the tab view, nothing to be done about it.
	 */
	tv->SetTabWidth(B_WIDTH_FROM_WIDEST);
}

BView* GlPrefWin::NewGeneralView(BRect frame, const BMessage& prefs)
{
	BView*					v = new BView(frame, GENERAL_STR, B_FOLLOW_ALL, 0);
	if (!v) return v;
	v->SetViewColor( Prefs().GetColor(ARP_WIN_BG_C) );
	float					padX = Prefs().GetFloat(ARP_PADX_F),
							padY = Prefs().GetFloat(ARP_PADY_F);
	float					div = v->StringWidth(SZ(SZ_Creator)->String()) + 5;
	float					w = div + 150;
	BRect					f(padX, padY, padX + w, padY + Prefs().GetInt32(ARP_TEXTCTRL_Y));
	f.right = ARP_MIN(f.right, frame.Width() - padX);
	BString16				creatorInit;
	int32					keyInit = 0;
	
	GlKeyTracker&			tracker = gl_app->KeyTracker();
	tracker.GetCurrent(creatorInit, &keyInit);
		
	mCreator = new BTextControl(f, NULL, SZ(SZ_Creator)->String(), creatorInit.String(), new BMessage(_CREATOR_MSG));
	if (mCreator) {
		mCreator->SetDivider(div);
		v->AddChild(mCreator);
	}

	f.top = f.bottom + padY;
	f.bottom = f.top + Prefs().GetInt32(ARP_INTCTRL_Y);
	mKey = new ArpIntControl(f, NULL, SZ(SZ_Key), new BMessage(_KEY_MSG));
	if (mKey) {
		mKey->SetDivider(div);
		mKey->SetValue(keyInit);
		v->AddChild(mKey);
	}
	
	return v;
}

BView* GlPrefWin::NewMidiView(BRect frame, const BMessage& prefs)
{
	BView*					v = new BView(frame, MIDI_STR, B_FOLLOW_ALL, 0);
	if (!v) return v;
	v->SetViewColor( Prefs().GetColor(ARP_WIN_BG_C) );
	float					padX = Prefs().GetFloat(ARP_PADX_F),
							padY = Prefs().GetFloat(ARP_PADY_F);
	float					captureW = v->StringWidth(SZ(SZ_Learn_One)->String()),
							learnManyW = v->StringWidth(SZ(SZ_Learn_Many)->String()),
							deleteW = v->StringWidth(SZ(SZ_Delete)->String());
	float					buttonW = ARP_MAX(captureW, learnManyW), buttonH = 24;
	buttonW = ARP_MAX(buttonW, deleteW) + 10;
	float					col1R = (frame.Width() * 0.5f) - padX;
	BRect					r(padX, padY, col1R, frame.Height() - (padY * 2) - buttonH - 24);
	mMidiListView = new GlMidiBindingsListView(r, this);
	if (mMidiListView) {
		GlMidiBindingList&			bindings = gl_app->MidiBindings();
		uint32				size = bindings.Size();
		for (uint32 k = 0; k < size; k++) {
			const GlMidiBinding*	binding = bindings.At(k);
			if (binding) mMidiListView->AddBinding(binding);
		}
		v->AddChild(mMidiListView);
	}

	// Learn one
	r.right = r.left + buttonW;
	r.top = r.bottom + padY;
	r.bottom = r.top + buttonH;
	mMidiLearnOne = new BButton(r, NULL, SZ(SZ_Learn_One)->String(), new BMessage(LEARN_ONE_MSG), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	if (mMidiLearnOne) v->AddChild(mMidiLearnOne);
	// Learn many
	r.left = r.right + padX;
	r.right = r.left + buttonW;
	mMidiLearnMany = new BButton(r, NULL, SZ(SZ_Learn_Many)->String(), new BMessage(LEARN_MANY_MSG), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM );
	if (mMidiLearnMany) v->AddChild(mMidiLearnMany);
	// Delete
	r.left = r.right + padX;
	r.right = r.left + buttonW;
	mMidiDelete = new BButton(r, NULL, SZ(SZ_Delete)->String(), new BMessage(DELETE_MSG), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM );
	if (mMidiDelete) {
//		if (!mMidiListView || !(mMidiListView->CurrentSelection()))
//			mMidiDelete->SetEnabled(false);
		v->AddChild(mMidiDelete);
	}
	
	/* Second column: File name control, menu for grid, menu for parameter
	 */
	float					rtW = v->StringWidth(SZ(SZ_Control)->String()),
							rootW = v->StringWidth(SZ(SZ_Root)->String()),
							paramW = v->StringWidth(SZ(SZ_Parameter)->String());
	float					div = ARP_MAX(rtW, rootW);
	div = ARP_MAX(div, paramW) + 5;
	r.Set(col1R + padX, padY, frame.Width() - padX, padY + Prefs().GetInt32(ARP_MENUCTRL_Y));
	BMessage				items;
	items.AddString(ARP_MENU_ITEM_STR, "File");
	items.AddInt32(ARP_MENU_I_STR, -2);
	items.AddString(ARP_MENU_ITEM_STR, "sep");
	items.AddInt32(ARP_MENU_I_STR, -1000);
	
	for (int32 k = -1; k < 26; k++) {
		const BString16*	str = gl_midi_label(k);
		if (str) {
			items.AddString(ARP_MENU_ITEM_STR, *str);
			items.AddInt32(ARP_MENU_I_STR, k);
		}
	}
	mMidiRtMenu = new ArpMenuControl(r, NULL, SZ(SZ_Control), MIDI_RT_MSG, items);
	if (mMidiRtMenu) {
		mMidiRtMenu->SetDivider(div);
		mMidiRtMenu->SetCurrentIndex(-1);
		v->AddChild(mMidiRtMenu);
	}

	r.top = r.bottom + padY;
	r.bottom = r.top + Prefs().GetInt32(ARP_MENUCTRL_Y);
	mMidiFileName = new ArpFileNameControl(r, NULL, B_FOLLOW_TOP | B_FOLLOW_LEFT);
	if (mMidiFileName) {
		mMidiFileName->SetTarget(BMessenger(this));
		mMidiFileName->SetMessage(new BMessage(MIDI_FILE_MSG));
		v->AddChild(mMidiFileName);
	}
	
	r.top = r.bottom + padY;
	r.bottom = r.top + Prefs().GetInt32(ARP_MENUCTRL_Y);
	items.MakeEmpty();
	mMidiRootMenu = new ArpMenuControl(r, NULL, SZ(SZ_Root), MIDI_ROOT_MSG, items);
	if (mMidiRootMenu) {
		mMidiRootMenu->SetDivider(div);
		v->AddChild(mMidiRootMenu);
	}
	
	r.top = r.bottom + padY;
	r.bottom = r.top + Prefs().GetInt32(ARP_MENUCTRL_Y);
	mMidiParamMenu = new ArpMenuControl(r, NULL, SZ(SZ_Parameter), MIDI_PARAM_MSG, items);
	if (mMidiParamMenu) {
		mMidiParamMenu->SetDivider(div);
		v->AddChild(mMidiParamMenu);
	}

	MidiSelectionChanged();
	
	return v;
}

// #pragma mark -

/********************************************************
 * GL-MIDI-BINDINGS-LIST-VIEW
 ********************************************************/
GlMidiBindingsListView::GlMidiBindingsListView(	BRect frame,
												GlPrefWin* win)
		: inherited(frame, "lv", B_FOLLOW_LEFT | B_FOLLOW_TOP,
					B_WILL_DRAW, B_PLAIN_BORDER),
		  mPrefWin(win)
{
	AddColumn(new BStringColumn(SZ(SZ_Port)->String(), 100, 20, 250, B_TRUNCATE_END), PORT_COL);
	AddColumn(new BStringColumn(SZ(SZ_Channel)->String(), 50, 20, 100, B_TRUNCATE_END), CHANNEL_COL);
	AddColumn(new BStringColumn(SZ(SZ_Event)->String(), 100, 20, 250, B_TRUNCATE_END), EVENT_COL);
	AddColumn(new BStringColumn(SZ(SZ_Controller)->String(), 100, 20, 250, B_TRUNCATE_END), CONTROLLER_COL);
	SetSortingEnabled(false);
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
}

void GlMidiBindingsListView::SelectionChanged()
{
	inherited::SelectionChanged();
	if (mPrefWin) mPrefWin->MidiSelectionChanged();
}

bool GlMidiBindingsListView::HasEvent(GlMidiEvent event)
{
	return GetEventRow(event) != 0;
}

void GlMidiBindingsListView::SelectEvent(	GlMidiEvent event,
											const GlMidiBinding* binding,
											bool assignLetter)
{
	GlMidiBindingRow*		row = GetEventRow(event);
	if (row) {
		AddToSelection(row);
		return;
	}

	row = new GlMidiBindingRow(event, binding);
	if (!row) return;
	row->SetDirty();
	/* If it's a new row, by default it assign it the first available
	 * realtime controller, if any.
	 */
	if (assignLetter) {
		uint8				rt[GL_MIDI_SIZE];
		memset(rt, 0, GL_MIDI_SIZE);
		int32				k, count = CountRows();
		for (k = 0; k < count; k++) {
			GlMidiBindingRow*	r = (GlMidiBindingRow*)RowAt(k);
			if (r) {
				int32		 letter = r->Rt();
				if (letter >= 0 && letter < GL_MIDI_SIZE) rt[letter] = 1;
			}
		}
		for (k = 0; k < GL_MIDI_SIZE; k++) {
			if (rt[k] == 0) {
				row->SetRt(k);
				break;
			}
		}
	}
	AddRow(row);
	AddToSelection(row);
}

void GlMidiBindingsListView::AddBinding(const GlMidiBinding* binding)
{
	ArpVALIDATE(binding, return);
	SelectEvent(binding->event, binding);
	GlMidiBindingRow*		row = GetEventRow(binding->event);
	if (!row) return;
	float					min, max;
	binding->GetValueRange(&min, &max);
	row->binding.SetValueRange(min, max);
}

GlMidiBindingRow* GlMidiBindingsListView::GetEventRow(GlMidiEvent event)
{
	int32						count = CountRows();
	for (int32 k = 0; k < count; k++) {
		BRow*					r = RowAt(k);
		if (r) {
			GlMidiBindingRow*	br = (GlMidiBindingRow*)r;
			if (br->Matches(event)) return br;
		}
	}
	return 0;
}

// #pragma mark -

/********************************************************
 * GL-MIDI-BINDING-ROW
 ********************************************************/
GlMidiBindingRow::GlMidiBindingRow(	GlMidiEvent event,
									const GlMidiBinding* b)
		: mDirty(false)
{
	if (b) binding = *b;
	binding.event = event;
	const BString16*	portName = binding.PortName();
	if (portName) SetField(new BStringField(portName->String()), PORT_COL);
	else SetField(new BStringField(SZ(SZ_error)->String()), PORT_COL);
	BString16			str;
	str << binding.event.channel;
	SetField(new BStringField(str.String()), CHANNEL_COL);

	if (binding.event.type == binding.event.CONTROL_CHANGE) {
		str = "Controller ";
		str << binding.event.value1;
		SetField(new BStringField(str.String()), EVENT_COL);
	} else SetField(new BStringField(SZ(SZ_Unknown_event)->String()), EVENT_COL);

	SetField(new BStringField(gl_midi_label(binding.rt)->String()), CONTROLLER_COL);
}

GlMidiBindingRow::~GlMidiBindingRow()
{
}

bool GlMidiBindingRow::IsDirty() const
{
	return mDirty;
}

void GlMidiBindingRow::SetDirty()
{
	mDirty = true;
}

GlMidiEvent GlMidiBindingRow::Event() const
{
	return binding.event;
}

bool GlMidiBindingRow::Matches(const GlMidiEvent& event) const
{
	return binding.event == event;
}

int32 GlMidiBindingRow::Rt() const
{
	return binding.rt;
}

void GlMidiBindingRow::SetRt(int32 midi)
{
	binding.rt = midi;
	SetField(new BStringField(gl_midi_label(midi)->String()), CONTROLLER_COL);
	mDirty = true;
}

status_t GlMidiBindingRow::SetFileName(const BString16& fn)
{
	binding.filename = fn;
	rootNames.resize(0);
	paramNames.resize(0);
	mDirty = true;
	return B_OK;
}

BString16 GlMidiBindingRow::FileName() const
{
	return binding.filename;
}

status_t GlMidiBindingRow::LoadRoot(int32 index)
{
	if (binding.root) binding.root->DecRefs();
	binding.root = 0;
	if (index < 0) return B_ERROR;

	binding.rootIndex = index;
	status_t		err = binding.Load();
	if (err != B_OK) binding.rootIndex = 0;
	return err;
}

GlRootNode* GlMidiBindingRow::Root()
{
	return binding.root;
}

int32 GlMidiBindingRow::RootIndex() const
{
	return binding.rootIndex;
}

void GlMidiBindingRow::SetParam(int32 index)
{
	binding.paramIndex = index;
	mDirty = true;
}

int32 GlMidiBindingRow::Param() const
{
	return binding.paramIndex;
}

void GlMidiBindingRow::SetRange(float min, float max)
{
	binding.SetValueRange(min, max);
	mDirty = true;
}

void GlMidiBindingRow::GetRange(float* min, float* max) const
{
	ArpASSERT(min && max);
	binding.GetValueRange(min, max);
}
