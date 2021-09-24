#include <cstdio>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <interface/Alert.h>
#include <interface/Box.h>
#include <interface/CheckBox.h>
#include <interface/Button.h>
#include <interface/Menu.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/RadioButton.h>
#include <interface/Screen.h>
#include <interface/StringView.h>
#include <interface/TabView.h>
#include <interface/TextControl.h>
#include <storage/FilePanel.h>
#include <storage/Path.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmViewFactory.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqFactoryListView.h"
#include "Sequitur/SeqPrefWin.h"
//#include "Sequitur/SeqMessageWrapper.h"

static const uint32		OK_MSG						= 'iok_';
static const uint32		CANCEL_MSG					= 'icnc';
static const uint32		REMEMBER_OPEN_MSG			= 'irmo';
static const uint32		CHANGE_SKIN_MSG				= 'icsk';
static const uint32		CHANGE_SKIN_TO_DEFAULT_MSG	= 'icsd';
static const uint32		OPEN_BLANK_MSG				= 'iofb';
static const uint32		OPEN_FOUR_MSG				= 'iof4';
static const uint32		OPEN_FILE_MSG				= 'ioff';
static const uint32		OWQ_INVOKE_ADD				= 'ioqI';
static const uint32		OWQ_ADD						= 'ioqA';
static const uint32		OWQ_CHANGE					= 'ioqC';
static const uint32		OWQ_REMOVE					= 'ioqR';
static const uint32		CHANGE_UNDO_HISTORY_MSG		= 'icuh';
static const uint32		TRACK_WIN_FOLLOW_MSG		= 'itwf';
static const uint32		TRACK_WIN_PLAY_TO_END_MSG	= 'itwp';
static const uint32		TRACK_HEIGHT_MSG			= 'itht';
static const uint32		LABEL_HEIGHT_MSG			= 'itlh';
static const uint32		VIEW_ROW_SELECTED_MSG		= 'iVrS';

static const char*		TABVIEW_STR					= "tabview";
static const char*		FILE_STR					= "File";
static const char*		REMEMBER_OPEN_STR			= "remember_open";
static const char*		OPEN_BLANK_STR				= "open_blank";
static const char*		OPEN_FOUR_STR				= "open_four";
static const char*		OPEN_FILE_STR				= "open_file";

static const char*		ON_STR						= "On";
static const char*		NAME_STR					= "Name";
static const char*		QUERY_STR					= "Query or folder";
//static const char*		SKIP_TOP_LEVEL_STR			= "Skip Top Level";
static const char*		EDIT_STR					= "Edit";
static const char*		TRACK_STR					= "Track";
static const char*		VIEWS_STR					= "Views";
static const char*		UNDO_LEVEL_INT				= "undo level";
static const char*		TRACK_WIN_FOLLOW_STR		= "track_win_follow";
static const char*		TRACK_WIN_PLAY_TO_END_STR	= "track_win_play_to_end";
static const float		INDENT						= 5;
static const float		LABEL_PAD					= 9;
static const int32		UNDO_HISTORY_MAX			= 999;

/********************************************************
 * _OWQ-LIST
 ********************************************************/
class _OwqList : public BColumnListView
{
public:
	_OwqList(BRect rect, const BMessage& prefs);
	virtual ~_OwqList();

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* msg);

	virtual void	ItemInvoked();
	virtual void	MessageDropped(BMessage* msg, BPoint point);
	virtual void	SelectionChanged();

	void			SetButtons(BButton* add, BButton* change, BButton* remove);
	/* Remove all OPEN_FROM_QUERY_PREF preferences in the supplied
	 * message, and build a new list, based on my current state.
	 */
	void			ReplacePreferences(BMessage& prefs);

private:
	typedef BColumnListView		inherited;
	BMessenger		mAddWin;
	BButton			*mAdd, *mChange, *mRemove;

	void			OpenAddWindow(BMessage* row = 0);
	void			Add(BMessage* msg);
	void			Change();
};

/********************************************************
 * _OWQ-ROW
 ********************************************************/
class _OwqRow : public BRow
{
public:
	_OwqRow(const BMessage* config, float height);
	virtual ~_OwqRow();

	virtual bool	HasLatch() const		{ return false; }

	bool			Matches( const BMessage& prefs ) const;
	status_t		GetPreferences(BMessage* prefs);
	status_t		SetPreferences(const BMessage* prefs);
	void			SetRef(const entry_ref& ref);

private:
	bool		mOn, mSkip;
	BString		mName;
	entry_ref*	mRef;
};

/*************************************************************************
 * SEQ-PREF-WIN
 *************************************************************************/
SeqPrefWin::SeqPrefWin(BRect frame, const BMessage* config)
		: inherited(frame, "Preferences", B_TITLED_WINDOW, B_NOT_ZOOMABLE),
		  mOwqTable(0), mFilePanel(0), mUndoLevelCtrl(NULL), mTrackWinFollowCtrl(NULL),
		  mTrackWinPlayToEndCtrl(NULL),
		  mTrackHeightCtrl(NULL), mLabelHeightCtrl(NULL), mRefreshWindows(0),
		  mFactoryView(0), mFactoryList(0), mFactoryInspector(0)
{
	seq_app->GetPreferences(&mPreferences);
	if (config) SetConfiguration(config);
	AddViews(mPreferences);
}

SeqPrefWin::~SeqPrefWin()
{
	delete mFilePanel;
}

static void new_skin_warning()
{
	BAlert*	alert = new BAlert(	"Warning", "To see the new skin, close Preferences,\nthen close and restart Sequitur",
										"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
	if (alert) alert->Go();
}

void SeqPrefWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case OK_MSG:
			if (mOwqTable) mOwqTable->ReplacePreferences( mPreferences );
			seq_app->SetPreferences(&mPreferences);
			if (mUndoLevelCtrl) AmGlobals().SetUndoHistory(mUndoLevelCtrl->Value() );
			if (mRefreshWindows != 0) RefreshWindows();
			PostMessage(B_QUIT_REQUESTED);
			break;
		case CANCEL_MSG:
			PostMessage(B_QUIT_REQUESTED);
			break;
		case CHANGE_SKIN_MSG:
			{
				const char*		skin;
				if (msg->FindString("seq:skin", &skin) == B_OK) {
					SetStringPref(CURRENT_SKIN_PREF, skin);
					new_skin_warning();
				}
			}
			break;
		case CHANGE_SKIN_TO_DEFAULT_MSG:
			{
				if( mPreferences.HasString(CURRENT_SKIN_PREF) ) mPreferences.RemoveData(CURRENT_SKIN_PREF);
				new_skin_warning();
			}
			break;
		case REMEMBER_OPEN_MSG:
			{
				BCheckBox*	cb = dynamic_cast<BCheckBox*>( FindView(REMEMBER_OPEN_STR) );
				if( cb ) SetBoolPref( REMEBER_OPEN_SONGS_PREF, cb->Value() );
			}
			break;
		case OPEN_BLANK_MSG:
			SetStringPref( OPEN_NEW_SONG_PREF, "blank" );
			break;
		case OPEN_FOUR_MSG:
			SetStringPref( OPEN_NEW_SONG_PREF, "channels" );
			SetInt32Pref( OPEN_NEW_SONG_CHANNEL_PREF, 2 );
			break;
		case OPEN_FILE_MSG:
			SetStringPref( OPEN_NEW_SONG_PREF, "file" );
			/* Open a file panel to select a new file.
			 */
			if (!mFilePanel ) mFilePanel = new BFilePanel(B_OPEN_PANEL, 0, 0, 0,
																false);
			if (mFilePanel->IsShowing() ) break;
			if (mFilePanel->Window() ) {
				mFilePanel->Window()->SetTitle("Select file" B_UTF8_ELLIPSIS);
			}

			mFilePanel->SetTarget(BMessenger(this));
			mFilePanel->SetMessage( new BMessage('nfen') );
			mFilePanel->Show();

			break;
		case 'nfen':
			{
				entry_ref	ref;
				if( msg->FindRef( "refs", &ref ) == B_OK ) {
					SetOpenNewFromFileRef( &ref );
				}
			}
			break;
		case CHANGE_UNDO_HISTORY_MSG:
			if (mUndoLevelCtrl) SetInt32Pref(UNDO_HISTORY_PREF, mUndoLevelCtrl->Value() );
			break;
		case TRACK_WIN_FOLLOW_MSG:
			if (mTrackWinFollowCtrl) SetBoolPref(TRACK_WIN_FOLLOW_PREF, mTrackWinFollowCtrl->Value() == B_CONTROL_ON);
			break;
		case TRACK_WIN_PLAY_TO_END_MSG:
			if (mTrackWinPlayToEndCtrl) SetBoolPref(TRACK_WIN_PLAY_TO_END_PREF, mTrackWinPlayToEndCtrl->Value() == B_CONTROL_ON);
			break;
		case TRACK_HEIGHT_MSG:
			if (mTrackHeightCtrl) SetInt32Pref(TRACK_HEIGHT_PREF, mTrackHeightCtrl->Value() );
			break;
		case LABEL_HEIGHT_MSG:
			if (mLabelHeightCtrl) {
				SetInt32Pref(PHRASE_LABEL_HEIGHT_PREF, mLabelHeightCtrl->Value() );
				mRefreshWindows |= SEQ_REFRESH_PHRASES;
			}
			break;
		case VIEW_ROW_SELECTED_MSG:
			FactoryRowSelected();
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

bool SeqPrefWin::QuitRequested()
{
	if( !inherited::QuitRequested() ) return false;
	BMessage	config;
	if( GetConfiguration( &config ) == B_OK ) {
		if( seq_is_quitting() ) seq_app->AddShutdownMessage( "window_settings", &config );
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::PREF_WIN_INDEX, config);
	}
	return true;
}

bool SeqPrefWin::IsSignificant() const
{
	return false;
}

status_t SeqPrefWin::GetConfiguration(BMessage* config)
{
	ArpASSERT( config );
	config->what = PREFERENCE_WINDOW_SETTING_MSG;
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
	return B_OK;
}

status_t SeqPrefWin::SetConfiguration(const BMessage* config)
{
	ArpASSERT( config );
	bool		minimized;
	if( config->FindBool( "f_minimized", &minimized ) != B_OK ) minimized = false;
	Minimize(minimized);

	BScreen		s(this);
	if( !s.IsValid() ) return B_ERROR;
	BRect		sf = s.Frame();
	float		l, t, r, b;
	if( config->FindFloat( "f_left", &l ) == B_OK
			&& config->FindFloat( "f_top", &t ) == B_OK
			&& config->FindFloat( "f_right", &r ) == B_OK
			&& config->FindFloat( "f_bottom", &b ) == B_OK ) {
		BRect	f( l * sf.Width(), t * sf.Height(), r * sf.Width(), b * sf.Height() );
		MoveTo( f.LeftTop() );
		ResizeTo( f.Width(), f.Height() );
	}
	return B_OK;
}

status_t SeqPrefWin::SetStringPref(const char* name, const char* a_string, int32 n)
{
	if (mPreferences.HasString( name, n ) ) return mPreferences.ReplaceString( name, n, a_string );
	else return mPreferences.AddString( name, a_string );
}

status_t SeqPrefWin::SetInt32Pref(const char *name, int32 an_int32, int32 n)
{
	if (mPreferences.HasInt32(name, n) ) return mPreferences.ReplaceInt32( name, n, an_int32 );
	else return mPreferences.AddInt32( name, an_int32 );
}

status_t SeqPrefWin::SetBoolPref(const char *name, bool a_boolean, int32 n)
{
	if (mPreferences.HasBool( name, n ) ) return mPreferences.ReplaceBool( name, n, a_boolean );
	else return mPreferences.AddBool( name, a_boolean );
}

status_t SeqPrefWin::SetRefPref(const char *name, const entry_ref *ref, int32 n)
{
	if (mPreferences.HasRef( name, n ) ) return mPreferences.ReplaceRef( name, n, ref );
	else return mPreferences.AddRef( name, ref );
}

static BString label_for_open_new_from_file(const entry_ref* ref)
{
	BString		str("From file: ");
	BPath		path(ref);
	if( path.InitCheck() == B_OK ) {
		str << path.Path();
	} else {
		str << "<click to select>";
	}
	return str;
}

void SeqPrefWin::SetOpenNewFromFileRef(const entry_ref* ref)
{
	SetRefPref(OPEN_NEW_SONG_FILE_PREF, ref);
	BRadioButton*	button = dynamic_cast<BRadioButton*>( FindView(OPEN_FILE_STR) );
	if( button ) button->SetLabel( label_for_open_new_from_file(ref).String() );
}

void SeqPrefWin::RefreshWindows()
{
	if (!be_app || !be_app->Lock() ) return;
	int32		count = be_app->CountWindows();
	BMessage	msg(SEQ_REFRESH_WINDOW_MSG);
	msg.AddInt32("flags", mRefreshWindows);
	for (int32 k = 0; k < count; k++) {
		BWindow*	win = be_app->WindowAt(k);
		if (win) win->PostMessage(&msg);
	}
	be_app->Unlock();
}

void SeqPrefWin::FactoryRowSelected()
{
	if (mFactoryInspector) {
		if (mFactoryInspector->RemoveSelf()) delete mFactoryInspector;
		mFactoryInspector = 0;
	}

	if (!mFactoryView) return;

	ArpVALIDATE(mFactoryList, return);
	float				sx = Prefs().Size(SPACE_X), sy = Prefs().Size(SPACE_Y);
	BPoint				at = BPoint(sx, mFactoryList->Frame().bottom + sy);

	BString				fac, key;
	if (mFactoryList->GetCurrentKeys(fac, key) != B_OK
			|| fac.Length() < 1 || key.Length() < 1)
		return;

	AmViewFactory*		factory = AmGlobals().FactoryNamed(fac);
	if (!factory) return;

	BRect				f(mFactoryView->Bounds());
	f.left = at.x;
	f.top = at.y;
	mFactoryInspector = factory->NewPrefView(f, &mPreferences, key);
	if (mFactoryInspector) mFactoryView->AddChild(mFactoryInspector);
}

void SeqPrefWin::AddViews(const BMessage& prefs)
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
		BView*		v = new BView( f, "button_panel", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, 0 );
		if( v ) {
			BRect		f( b.right - edgeR - buttonW, edgeT, b.right - edgeR, edgeT + buttonH );
			BButton*	button = new BButton( f, "ok_button", "OK", new BMessage( OK_MSG ), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
			if( button ) {
				v->AddChild( button );
				button->MakeDefault( true );
			}
			f.OffsetBy( 0-(buttonW + 10), 0 );
			button = new BButton( f, "cancel_button", "Cancel", new BMessage( CANCEL_MSG ), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
			if( button ) v->AddChild( button );

			v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
			AddChild( v );
			b.bottom = b.bottom - edgeB - buttonH - edgeT - 1;
		}
	}
	BTabView*	tv = new BTabView(b, TABVIEW_STR);
	if (!tv) return;
	BView*		fileView = NewFileView(b, prefs);
	if (fileView) tv->AddTab(fileView);
	BView*		editView = NewEditView(b, prefs);
	if (editView) tv->AddTab(editView);
	BView*		playbackView = NewTrackView(b, prefs);
	if (playbackView) tv->AddTab(playbackView);
	mFactoryView = NewFactoriesView(b, prefs);
	if (mFactoryView) tv->AddTab(mFactoryView);

	AddChild(tv);
	/* NOTE:  Have to do this after the tab view's been added to the window or else
	 * you get a crash.  It's a bug in the tab view, nothing to be done about it.
	 */
	tv->SetTabWidth(B_WIDTH_FROM_WIDEST);
}

static float view_font_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

static float bold_font_height()
{
	const BFont*	font = be_bold_font;
	if( !font ) return 0;
	font_height		fh;
	font->GetHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

BView* SeqPrefWin::NewFileView(BRect bounds, const BMessage& prefs) const
{
	BView*		v = new BView( bounds, FILE_STR, B_FOLLOW_ALL, 0 );
	if( !v ) return v;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = view_font_height(v);
	float		bfh = bold_font_height();
	float		openH = bfh + 5 + fh + 5 + fh + 5 + fh;
	/* The Remember Open Songs preference.
	 */
	float		w = v->StringWidth("Remember open songs") + 25;
	BRect		f(bounds.left + 5, bounds.top + 5, bounds.left + 5 + w, bounds.top + 5 + fh);
	BCheckBox*	cb = new BCheckBox( f, REMEMBER_OPEN_STR, "Remember open songs", new BMessage(REMEMBER_OPEN_MSG) );
	if( cb ) {
		bool	b;
		if( prefs.FindBool(REMEBER_OPEN_SONGS_PREF, &b) != B_OK ) b = false;
		cb->SetValue( (b) ? B_CONTROL_ON : B_CONTROL_OFF );
		v->AddChild( cb );
	}
	/* The Skin preference.
	 */
	BMenu*		menu = new BMenu("skin_menu");
	BMessage	skinMsg(CHANGE_SKIN_MSG);
	BMenuItem*	item = new BMenuItem( "Default", new BMessage(CHANGE_SKIN_TO_DEFAULT_MSG) );
	item->SetMarked(true);
	menu->AddItem(item);
	menu->AddSeparatorItem();
	menu->SetLabelFromMarked(true);
	if( seq_make_skin_menu(menu, &skinMsg) == B_OK ) {
		const char*	label = "Choose skin:";
		f.Set(f.left, f.bottom + 8, bounds.right - 5, f.bottom + 8 + fh + 10);
		BMenuField*	field = new BMenuField(f, "skin_field", label, menu);
		if (field) {
			field->SetDivider( v->StringWidth(label) + 10 );
			v->AddChild(field);
		} else delete menu;
	} else delete menu;

	/* The Open New Songs preferences.
	 */
	f.Set(bounds.left + 5, f.bottom + 10, bounds.right - 10, f.bottom + 10 + openH + 10);
	BBox*		box = new BBox( f,
								"open_new_songs",
								B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if( box ) {
		box->SetLabel( "Open new songs" );
		BRect		boxB = box->Bounds();
		BRect		sf(boxB.left + 5, boxB.top + 5 + bfh, boxB.right - 5, boxB.top + 5 + bfh + fh);
		const char*	choice;
		if( prefs.FindString(OPEN_NEW_SONG_PREF, &choice) != B_OK ) choice = 0;
		BRadioButton*	button = new BRadioButton( sf, OPEN_BLANK_STR, "Blank", new BMessage(OPEN_BLANK_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
		if( button ) {
			if( choice && strcmp(choice, "blank") == 0 ) button->SetValue( B_CONTROL_ON );
			box->AddChild( button );
		}
		sf.OffsetBy( 0, 5 + fh );
		button = new BRadioButton( sf, OPEN_FOUR_STR, "With two channels of each device", new BMessage(OPEN_FOUR_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
		if( button ) {
			if( choice && strcmp(choice, "channels") == 0 ) button->SetValue( B_CONTROL_ON );
			box->AddChild( button );
		}
		sf.OffsetBy( 0, 5 + fh );
		button = new BRadioButton( sf, OPEN_FILE_STR, "From file: <click to select>", new BMessage(OPEN_FILE_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP );
		if( button ) {
			if( choice && strcmp(choice, "file") == 0 ) button->SetValue( B_CONTROL_ON );
			entry_ref	ref;
			if( prefs.FindRef(OPEN_NEW_SONG_FILE_PREF, &ref) == B_OK )
				button->SetLabel( label_for_open_new_from_file(&ref).String() );
			box->AddChild( button );
		}
		v->AddChild( box );
		f.OffsetBy(0, f.bottom - f.top + 10 );
	}
	/* The Open From Query preferences
	 */
	f.bottom = bounds.bottom - 27;
	box = new BBox( f, "open_from_query", B_FOLLOW_ALL);
	if( box ) {
		box->SetLabel("Open from query");

		BRect			boxB = box->Bounds();
		BRect			tableF(boxB.left + 1, boxB.top + 5 + bfh, boxB.right - 1, boxB.bottom - 45);
		mOwqTable = new _OwqList( tableF, mPreferences );
		if( mOwqTable ) {
			mOwqTable->SetLatchWidth( 0 );
			box->AddChild( mOwqTable );
			mOwqTable->AddColumn( new BStringColumn(ON_STR, 40, 20, 100, B_TRUNCATE_END), 0 );
			mOwqTable->AddColumn( new BStringColumn(NAME_STR, 100, 20, 150, B_TRUNCATE_END), 1 );
			mOwqTable->AddColumn( new BStringColumn(QUERY_STR, 180, 20, 450, B_TRUNCATE_MIDDLE), 2 );
//			mOwqTable->AddColumn( new BStringColumn(SKIP_TOP_LEVEL_STR, 100, 20, 250, B_TRUNCATE_END), 3 );
			mOwqTable->SetSortColumn(mOwqTable->ColumnAt(1), false, true);
//			mOwqTable->SetSortColumn(mOwqTable->ColumnAt(), true, true);
			mOwqTable->SetSelectionMode( B_SINGLE_SELECTION_LIST );

			BRect		bF(tableF.left + 5, tableF.bottom + 5, tableF.left + 65, tableF.Height() - 10);
			BButton*	add = new BButton( bF, "owq_add", "Add", new BMessage(OWQ_INVOKE_ADD), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM );
			if( add ) box->AddChild( add );
			bF.OffsetBy( bF.Width() + 5, 0 );
			BButton*	change = new BButton( bF, "owq_change", "Change", new BMessage(OWQ_CHANGE), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM );
			if( change ) {
				change->SetEnabled( false );
				box->AddChild( change );
			}
			bF.OffsetBy( bF.Width() + 5, 0 );
			BButton*	remove = new BButton( bF, "owq_remove", "Remove", new BMessage(OWQ_REMOVE), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM );
			if( remove ) {
				remove->SetEnabled( false );
				box->AddChild( remove );
			}
			mOwqTable->SetButtons( add, change, remove );
		}
		v->AddChild( box );
	}
	return v;
}

BView* SeqPrefWin::NewEditView(BRect bounds, const BMessage& prefs)
{
	BView*		v = new BView(bounds, EDIT_STR, B_FOLLOW_ALL, 0);
	if (!v) return v;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	const char*	label = "Undo history:";
	float		top = INDENT;
	float		fh = view_font_height(v);
	float		ih = Prefs().Size(INT_CTRL_Y);
	float		max = (fh > ih) ? fh : ih;
	BRect		r(INDENT, top, INDENT + v->StringWidth(label), top + max);
	BStringView* sv = new BStringView(r, "undolevellabel", label);
	if (sv) v->AddChild(sv);
	r.left = r.right + LABEL_PAD;
	r.right = r.left + v->StringWidth("999") + 5;
	mUndoLevelCtrl = new ArpIntControl( r, UNDO_LEVEL_INT, NULL, new BMessage(CHANGE_UNDO_HISTORY_MSG) );
	if (mUndoLevelCtrl) {
		mUndoLevelCtrl->SetLimits(0, UNDO_HISTORY_MAX);
		int32		level;
		if (prefs.FindInt32(UNDO_HISTORY_PREF, &level) != B_OK) level = AM_DEFAULT_UNDO_HISTORY;
		if (level < 0 || level > UNDO_HISTORY_MAX) level = AM_DEFAULT_UNDO_HISTORY;
		mUndoLevelCtrl->SetValue(level);
		v->AddChild(mUndoLevelCtrl);
	}
	return v;
}

/*************************************************************************
 * _UP-ONE-FORMAT
 * A class that increases the value by 1.
 *************************************************************************/
class _UpOneFormat : public ArpIntFormatterI
{
public:
	_UpOneFormat()	{ ; }

	virtual void FormatInt(int32 number, BString& out) const
	{
		out << number + 1;
	}
};

BView* SeqPrefWin::NewTrackView(BRect bounds, const BMessage& prefs)
{
	BView*			v = new BView(bounds, TRACK_STR, B_FOLLOW_ALL, 0);
	if (!v) return v;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float			fh = view_font_height(v);
	/* The Track windows follow playback position preference.
	 */
	const char*		str = "Track windows follow playback position";
	float			w = v->StringWidth(str) + 25;
	BRect			f(bounds.left + 5, bounds.top + 5, bounds.left + 5 + w, bounds.top + 5 + fh);
	mTrackWinFollowCtrl = new BCheckBox( f, TRACK_WIN_FOLLOW_STR, str, new BMessage(TRACK_WIN_FOLLOW_MSG) );
	if (mTrackWinFollowCtrl) {
		bool		b;
		if (prefs.FindBool(TRACK_WIN_FOLLOW_PREF, &b) != B_OK) b = false;
		mTrackWinFollowCtrl->SetValue( (b) ? B_CONTROL_ON : B_CONTROL_OFF );
		v->AddChild(mTrackWinFollowCtrl);
		f.top = mTrackWinFollowCtrl->Frame().bottom + 5;
	}
	/* The Track windows play to end of song preference.
	 */
	str = "Track windows play to end of song";
	w = v->StringWidth(str) + 25;
	f.bottom = f.top + Prefs().Size(INT_CTRL_Y);
	f.right = f.left + w;
	mTrackWinPlayToEndCtrl = new BCheckBox(f, TRACK_WIN_PLAY_TO_END_STR, str, new BMessage(TRACK_WIN_PLAY_TO_END_MSG) );
	if (mTrackWinPlayToEndCtrl) {
		bool		b;
		if (prefs.FindBool(TRACK_WIN_PLAY_TO_END_PREF, &b) != B_OK) b = false;
		mTrackWinPlayToEndCtrl->SetValue( (b) ? B_CONTROL_ON : B_CONTROL_OFF );
		v->AddChild(mTrackWinPlayToEndCtrl);
		f.top = mTrackWinPlayToEndCtrl->Frame().bottom + 5;
	}
	/* The Initial track height preference.
	 */
	const char*		label = "Initial track height:";
	float			divider = v->StringWidth(label) + 5;
	f.bottom = f.top + Prefs().Size(INT_CTRL_Y);
	f.right = f.left + divider + v->StringWidth("200") + 10;
	mTrackHeightCtrl = new ArpIntControl(f, "track_height_ctrl", label, new BMessage(TRACK_HEIGHT_MSG) );
	if (mTrackHeightCtrl) {
		mTrackHeightCtrl->SetLimits(AM_DEFAULT_TRACK_HEIGHT, 199);
		int32		level;
		if (prefs.FindInt32(TRACK_HEIGHT_PREF, &level) != B_OK) level = AM_DEFAULT_TRACK_HEIGHT;
		if (level < AM_DEFAULT_TRACK_HEIGHT || level > 199) level = AM_DEFAULT_TRACK_HEIGHT;
		mTrackHeightCtrl->SetValue(level);
		mTrackHeightCtrl->SetFormatter(new _UpOneFormat() );
		mTrackHeightCtrl->SetDivider(divider);
		v->AddChild(mTrackHeightCtrl);
	}
	/* The Track label height preference.
	 */
	label = "Track label height:";
	f.top = f.bottom + 5;
	f.bottom = f.top + Prefs().Size(INT_CTRL_Y);
	mLabelHeightCtrl = new ArpIntControl(f, "label_height_ctrl", label, new BMessage(LABEL_HEIGHT_MSG) );
	if (mLabelHeightCtrl) {
		mLabelHeightCtrl->SetLimits(6, 50);
		int32		level;
		if (prefs.FindInt32(PHRASE_LABEL_HEIGHT_PREF, &level) != B_OK) level = 8;
		if (level < 6 || level > 50) level = 8;
		mLabelHeightCtrl->SetValue(level);
		mLabelHeightCtrl->SetDivider(divider);
		v->AddChild(mLabelHeightCtrl);
	}

	return v;
}

class _FacHeightFormat : public ArpIntFormatterI
{
public:
	_FacHeightFormat()	{ ; }
	virtual void FormatInt(int32 number, BString& out) const
	{
		if (number < AM_MIN_FAC_VIEW_HEIGHT) out << "Default";
		else out << number;
	}
};

BView* SeqPrefWin::NewFactoriesView(BRect bounds, const BMessage& prefs)
{
	BView*		v = new BView(bounds, VIEWS_STR, B_FOLLOW_ALL, 0);
	if (!v) return v;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		sx = Prefs().Size(SPACE_X), sy = Prefs().Size(SPACE_Y);
	float		iH = Prefs().Size(INT_CTRL_Y);
	float		listB = (bounds.Height() * 0.5) - (sy * 2) - (iH * 3);
	/* The factory list.
	 */
	BRect		f(sx, sy, bounds.Width() - sx, sy + listB);
	mFactoryList = new SeqFactoryListView(f, VIEWS_STR);
	if (!mFactoryList) return v;
	mFactoryList->SetSelectionMessage(new BMessage(VIEW_ROW_SELECTED_MSG));
	v->AddChild(mFactoryList);

	return v;
}

static float check_box_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

static float text_control_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

static float button_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading + 10;
}

static const uint32 OWQ_ON_MSG			= 'i_qO';
static const uint32 OWQ_NAME_MSG		= 'i_qN';
static const uint32 OWQ_CHOOSE_FILE_MSG	= 'i_qF';
static const uint32 OWQ_SKIP_MSG		= 'i_qS';

// #pragma mark -

/********************************************************
 * _OWQ-WINDOW
 ********************************************************/
static const uint32		SET_PREFS_MSG	= 'iset';

class _OwqWindow : public BWindow
{
public:
	_OwqWindow(BMessenger target, const BMessage* prefs = 0)
			: BWindow(	BRect(100, 100, 300, 300),
						"Open from query",
						B_TITLED_WINDOW_LOOK,
						B_NORMAL_WINDOW_FEEL,
						B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
			  mTarget(target), mFilePanel(0), mOnBox(0), mNameCtrl(0), mFileButton(0), mSkipBox(0),
			  mRef(0)
	{
		BRect	b = Bounds();
		BView*	v = new BView( b, "bg", B_FOLLOW_ALL, 0 );
		float	bottom = b.bottom;
		if (v) {
			v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
			float	cbH = check_box_height( v );
			BRect	f(5, 5, b.right - 5, 5 + cbH);
			mOnBox = new BCheckBox( f, "on_box", "On", new BMessage( OWQ_ON_MSG ) );
			if( mOnBox ) {
				mOnBox->SetValue( B_CONTROL_ON );
				v->AddChild( mOnBox );
			}
			f.OffsetBy( 0, f.Height() + 10 );
			f.bottom = f.top + text_control_height( v );
			mNameCtrl = new BTextControl( f, "name_ctrl", "Name: ", "", new BMessage( OWQ_NAME_MSG ) );
			if( mNameCtrl ) {
				mNameCtrl->SetDivider( v->StringWidth( "Name:" ) + 10 );
				v->AddChild( mNameCtrl );
			}
			f.OffsetBy( 0, f.Height() + 10 );
			f.bottom = f.top + button_height( v );
			mFileButton = new BButton( f, "file_ctrl", "Choose query", new BMessage( OWQ_CHOOSE_FILE_MSG ) );
			if( mFileButton ) v->AddChild( mFileButton );

/* I've turned off skip for now -- I think just having it on
 * is easier than explaining it.
 */
#if 0
			f.OffsetBy( 0, f.Height() + 5 );
			f.bottom = f.top + cbH;
			mSkipBox = new BCheckBox( f, "skip_box", "Skip top level", new BMessage( OWQ_SKIP_MSG ) );
			if( mSkipBox ) v->AddChild( mSkipBox );
#endif
			f.top = f.bottom + 10;
			f.bottom = f.top + button_height( v );
			float	buttonW = 60;
			f.left = f.right - buttonW;
			BButton*	button = new BButton( f, "ok_button", "OK", new BMessage( OK_MSG ) );
			if( button ) {
				v->AddChild( button );
				button->MakeDefault( true );
			}
			f.OffsetBy( 0-(buttonW + 10), 0 );
			button = new BButton( f, "cancel_button", "Cancel", new BMessage( CANCEL_MSG ) );
			if( button ) v->AddChild( button );

			bottom = f.bottom + 5;
			AddChild( v );
		}

		ResizeTo( b.Width(), bottom );
		if( prefs ) SetPrefs( prefs );

		bool	enable = false;
		if( mOnBox && mOnBox->Value() == B_CONTROL_ON ) enable = true;
		SetControlEnable(enable);
	}

	virtual ~_OwqWindow()
	{
		delete mRef;
		delete mFilePanel;
	}

	virtual void MessageReceived(BMessage* msg)
	{
		switch( msg->what ) {
			case OK_MSG:
				if( mTarget.IsValid() ) {
					BMessage		msg(OWQ_ADD);
					if( !mOldPrefs.IsEmpty() ) msg.AddMessage( "old item", &mOldPrefs );
					if( GetPrefs( msg ) == B_OK ) mTarget.SendMessage( &msg );
				}
				Quit();
				break;
			case CANCEL_MSG:
				Quit();
				break;
			case OWQ_CHOOSE_FILE_MSG:
				{
					if( !mFilePanel ) mFilePanel = new BFilePanel(B_OPEN_PANEL, 0, 0, 0,
																	false);
					if( mFilePanel->IsShowing() ) break;
					if( mFilePanel->Window() ) {
						mFilePanel->Window()->SetTitle("Select file" B_UTF8_ELLIPSIS);
					}

					mFilePanel->SetTarget(BMessenger(this));
					mFilePanel->SetMessage( new BMessage('entr') );
					mFilePanel->Show();
				}
				break;
			case 'entr':
				{
					entry_ref	ref;
					if( msg->FindRef( "refs", &ref ) == B_OK ) {
						delete mRef;
						mRef = new entry_ref( ref );
					}
				}
				break;
			case OWQ_ON_MSG:
				{
					bool	enable = false;
					if( mOnBox && mOnBox->Value() == B_CONTROL_ON ) enable = true;
					SetControlEnable(enable);
				}
				break;
			case SET_PREFS_MSG:
				SetPrefs( msg );
				break;
			default:
				BWindow::MessageReceived(msg);
				break;
		}
	}

private:
	BMessenger		mTarget;
	BMessage		mOldPrefs;
	BFilePanel*		mFilePanel;
	BCheckBox*		mOnBox;
	BTextControl*	mNameCtrl;
	BButton*		mFileButton;
	BCheckBox*		mSkipBox;
	entry_ref*		mRef;

	status_t GetPrefs(BMessage& pref)
	{
		if( !mOnBox || !mNameCtrl ) return B_ERROR;
//		if( !mOnBox || !mNameCtrl || !mSkipBox ) return B_ERROR;
		BMessage		msg('null');
		if( msg.AddBool( "on", (mOnBox->Value() == B_CONTROL_ON) ? true : false ) != B_OK ) return B_ERROR;
		if( msg.AddString( "name", mNameCtrl->Text() ) != B_OK ) return B_ERROR;
		if( mRef ) msg.AddRef( "ref", mRef );
//		if( msg.AddBool( "skip", (mSkipBox->Value() == B_CONTROL_ON) ? true : false ) != B_OK ) return B_ERROR;

		if( pref.AddMessage("new item", &msg) != B_OK ) return B_ERROR;
		return B_OK;
	}

	void SetPrefs(const BMessage* prefs)
	{
		if( !prefs ) return;
		mOldPrefs.MakeEmpty();
		mOldPrefs = *prefs;
		delete mRef;
		mRef = 0;
		bool		b;
		const char*	str;
		entry_ref	ref;
		if( mOnBox && prefs->FindBool( "on", &b ) == B_OK ) mOnBox->SetValue( (b) ? B_CONTROL_ON : B_CONTROL_OFF );
		if( mNameCtrl && prefs->FindString( "name", &str ) == B_OK ) mNameCtrl->SetText( str );
		if( prefs->FindRef( "ref", &ref ) == B_OK ) mRef = new entry_ref( ref );
		if( mSkipBox && prefs->FindBool( "skip", &b ) == B_OK ) mSkipBox->SetValue( (b) ? B_CONTROL_ON : B_CONTROL_OFF );
	}

	void SetControlEnable(bool enable)
	{
		if( mNameCtrl ) mNameCtrl->SetEnabled( enable );
		if( mFileButton ) mFileButton->SetEnabled( enable );
		if( mSkipBox ) mSkipBox->SetEnabled( enable );
	}

};

/********************************************************
 * _OWQ-LIST
 ********************************************************/
_OwqList::_OwqList(BRect frame, const BMessage& prefs)
		: inherited( frame, "owq_table", B_FOLLOW_ALL, B_WILL_DRAW, B_NO_BORDER ),
		mAdd(0), mChange(0), mRemove(0)
{
	BMessage	msg;
	float		fh = view_font_height(this);
	for( int32 k = 0; prefs.FindMessage(OPEN_FROM_QUERY_PREF, k, &msg) == B_OK; k++ ) {
		_OwqRow*	row = new _OwqRow( &msg, fh );
		if( row )  AddRow( row );
	}
}

_OwqList::~_OwqList()
{
	if( mAddWin.IsValid() ) mAddWin.SendMessage(B_QUIT_REQUESTED);
}

void _OwqList::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if( mAdd ) mAdd->SetTarget( this );
	if( mChange ) mChange->SetTarget( this );
	if( mRemove ) mRemove->SetTarget( this );
}

void _OwqList::MessageReceived(BMessage* msg)
{
	switch( msg->what ) {
		case OWQ_INVOKE_ADD:
			OpenAddWindow();
			break;
		case OWQ_ADD:
			Add( msg );
			break;
		case OWQ_CHANGE:
			Change();
			break;
		case OWQ_REMOVE:
			{
				BRow*	selection = CurrentSelection();
				if( selection ) {
					RemoveRow( selection );
					delete selection;
				}
			}
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void _OwqList::ItemInvoked()
{
	inherited::ItemInvoked();
	Change();
}

void _OwqList::MessageDropped(BMessage* msg, BPoint point)
{
	inherited::MessageDropped(msg, point);
	entry_ref		ref;
	if( msg->FindRef( "refs", &ref ) == B_OK ) {
		_OwqRow*	row = dynamic_cast<_OwqRow*>( RowAt(point) );
		if( !row ) {
			BMessage	prefs;
			prefs.AddBool( "on", true );
			prefs.AddRef( "ref", &ref );
			row = new _OwqRow( &prefs, view_font_height(this) );
			if( row )  AddRow( row );
		} else {
			row->SetRef( ref );
			UpdateRow(row);
		}
	}
}

void _OwqList::SelectionChanged()
{
	inherited::SelectionChanged();
	BRow*	selection = CurrentSelection();
	if( selection ) {
		if( mChange ) mChange->SetEnabled( true );
		if( mRemove ) mRemove->SetEnabled( true );
	} else {
		if( mChange ) mChange->SetEnabled( false );
		if( mRemove ) mRemove->SetEnabled( false );
	}
}

void _OwqList::SetButtons(BButton* add, BButton* change, BButton* remove)
{
	mAdd = add;
	mChange = change;
	mRemove = remove;
}

void _OwqList::ReplacePreferences(BMessage& prefs)
{
	while( prefs.HasMessage( OPEN_FROM_QUERY_PREF ) )
		prefs.RemoveData( OPEN_FROM_QUERY_PREF );
	_OwqRow*	row;
	for( int32 k = 0; (row = dynamic_cast<_OwqRow*>( RowAt(k) )); k++ ) {
		BMessage	msg;
		if( row->GetPreferences( &msg ) == B_OK ) prefs.AddMessage( OPEN_FROM_QUERY_PREF, &msg );
	}
}

void _OwqList::OpenAddWindow(BMessage* row)
{
	if( !mAddWin.IsValid() ) {
		_OwqWindow*	win = new _OwqWindow( BMessenger(this), row );
		if( win ) {
			mAddWin = BMessenger(win);
			win->Show();
		}
	} else {
		BHandler*		target;
		BLooper*		looper;
		if( (target = mAddWin.Target(&looper)) != 0 ) {
			_OwqWindow*	win = dynamic_cast<_OwqWindow*>( target );
			if( win ) {
				if( row && win->Lock() ) {
					win->MessageReceived( row );
					win->Unlock();
				}
				win->Activate( true );
			}
		}
	}
}

void _OwqList::Add(BMessage* msg)
{
	if( !msg ) return;
	BMessage	newItem, oldItem;
	if( msg->FindMessage( "new item", &newItem ) != B_OK ) return;
	if( msg->FindMessage( "old item", &oldItem ) == B_OK ) {
		_OwqRow*	row;
		for( int32 k = 0; (row = dynamic_cast<_OwqRow*>( RowAt(k) )); k++ ) {
			if( row->Matches( oldItem ) ) {
				row->SetPreferences( &newItem );
				UpdateRow( row );
				return;
			}
		}
	}

	_OwqRow*	row = new _OwqRow( &newItem, view_font_height(this) );
	if( row )  AddRow( row );
}

void _OwqList::Change()
{
	_OwqRow*	selection = dynamic_cast<_OwqRow*>( CurrentSelection() );
	if( !selection ) return;
	BMessage	row( SET_PREFS_MSG );
	if( selection->GetPreferences( &row ) == B_OK )
		OpenAddWindow( &row );
}

/********************************************************
 * _OWQ-ROW
 ********************************************************/
static const char*	DEFAULT_ROW_NAME	= "<no name>";

_OwqRow::_OwqRow(const BMessage* config, float height)
		: BRow(height),
		  mOn(false), mSkip(true), mName(DEFAULT_ROW_NAME), mRef(0)
{
	SetPreferences(config);
}

_OwqRow::~_OwqRow()
{
	delete mRef;
}

bool _OwqRow::Matches( const BMessage& prefs ) const
{
	bool		b;
	const char*	str;
	entry_ref	ref;
	if( prefs.FindBool("on", &b) != B_OK ) return false;
	if( b != mOn ) return false;
	if( prefs.FindString("name", &str) != B_OK ) return false;
	if( mName != str ) return false;
	if( prefs.FindBool("skip", &b) != B_OK ) return false;
	if( mSkip != b ) return false;
	status_t	err = prefs.FindRef( "ref", &ref );
	if( err == B_OK && !mRef ) return false;
	if( err != B_OK && mRef ) return false;
	if( err != B_OK && !mRef ) return true;
	return ref == *mRef;
}

status_t _OwqRow::GetPreferences(BMessage* prefs)
{
	if( !prefs ) return B_ERROR;
	if( prefs->AddBool( "on", mOn ) != B_OK ) return B_ERROR;
	if( prefs->AddString( "name", mName.String() ) != B_OK ) return B_ERROR;
	if( mRef ) {
		if( prefs->AddRef( "ref", mRef ) != B_OK ) return B_ERROR;
	}
	if( prefs->AddBool( "skip", mSkip ) != B_OK ) return B_ERROR;
	return B_OK;
}

status_t _OwqRow::SetPreferences(const BMessage* prefs)
{
	delete mRef;
	mRef = 0;
	entry_ref	ref;
	if( prefs ) {
		bool		b;
		const char*	str;
		if( prefs->FindBool( "on", &b ) == B_OK ) mOn = b;
		if( prefs->FindString( "name", &str ) == B_OK ) mName = str;
		prefs->FindRef( "ref", &ref );
		if( prefs->FindBool( "skip", &b ) == B_OK ) mSkip = b;
	}

	SetField( new BStringField( (mOn) ? "Yes" : "No" ), 0 );
	SetField( new BStringField( mName.String() ), 1 );
	SetRef( ref );
	return B_OK;
}

void _OwqRow::SetRef(const entry_ref& ref)
{
	delete mRef;
	mRef = new entry_ref(ref);

	BString		filename("<drop query or folder here>");
	if( mRef ) {
		BPath	path(mRef);
		if( path.InitCheck() == B_OK ) {
			filename = path.Path();
			if ( mName.Length() < 1 || (mName.Compare(DEFAULT_ROW_NAME) == 0) ) {
				mName = path.Leaf();
				SetField(new BStringField( mName.String() ), 1);
			}
		}
	}
	SetField( new BStringField( filename.String() ), 2 );
}
