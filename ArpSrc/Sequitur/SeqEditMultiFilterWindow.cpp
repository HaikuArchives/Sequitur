/* SeqEditMultiFilterWindow.cpp
 */
#include <support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqBitmapEditor.h"
#include "Sequitur/SeqEditMultiFilterWindow.h"
#include "Sequitur/SeqPipelineMatrixView.h"
#include "Sequitur/SequiturDefs.h"

static const uint32		OK_MSG					= '#ok_';
static const uint32		CANCEL_MSG				= '#cnc';
static const uint32		NAME_MSG				= '#lab';
static const uint32		KEY_MSG					= '#uni';
static const uint32		AUTHOR_MSG				= '#aut';
static const uint32		EMAIL_MSG				= '#eml';
static const uint32		ADD_PIPELINE_MSG		= '#AdP';
static const uint32		DELETE_PIPELINE_MSG		= '#DlP';
static const uint32		DESCRIPTION_MOD_MSG		= 'iDsM';
static const uint32		BITMAP_CHANGE_MSG		= 'bmch';
static const uint32		NEW_ICON_MSG			= 'inic';
static const uint32		COPY_ICON_MSG			= 'icic';
static const uint32		PASTE_ICON_MSG			= 'ipic';
static const uint32		FLIP_VERTICALLY_ICON_MSG	= 'ifvi';
static const uint32		FLIP_HORIZONTALLY_ICON_MSG	= 'ifhi';
static const uint32		FILL_WITH_ALPHA_MSG			= 'ibfa';

static const char*		GENERAL_STR				= "General";
static const char*		PIPELINE_STR			= "Pipeline";
static const char*		DESCRIPTION_STR			= "Description";
static const char*		ICON_STR				= "Icon";

static AmGlobalsImpl* gobals_impl()
{
	AmGlobalsI*		globalsI = &(AmGlobals());
	if (!globalsI) printf("COULDN'T GET GLOBALS AT ALL\n");
	return dynamic_cast<AmGlobalsImpl*>(globalsI);
}

/*************************************************************************
 * SEQ-EDIT-TOOL-WINDOW
 *************************************************************************/
SeqEditMultiFilterWindow::SeqEditMultiFilterWindow(	BRect frame,
													const BMessage* config,
													const BMessage* multiMsg)
		: inherited(frame, "Edit Multi Filter"),
		  mMultiFilterAddOn(NULL), mMultiFilter(NULL),
		  mNameCtrl(NULL), mKeyCtrl(NULL),
		  mLongDescriptionCtrl(NULL),
		  mPipelineView(NULL), mIconCtrl(NULL), mIconEditor(NULL)
{
	BRect		targetF(CurrentPageFrame() );
	BView*		view = NULL;
	if ((view = NewGeneralView(targetF))) AddPage(view);
	if ((view = NewPipelineView(targetF))) AddPage(view);
	if ((view = NewDescriptionView(targetF))) AddPage(view);
	if ((view = NewIconView(targetF))) AddPage(view);

	if (config) SetConfiguration(config);
	else SetFirstPage();
	if (multiMsg) SetMultiFilter(multiMsg);
}

SeqEditMultiFilterWindow::~SeqEditMultiFilterWindow()
{
	delete mMultiFilter;
	mMultiFilter = NULL;
	if (mMultiFilterAddOn) mMultiFilterAddOn->RemReference();
	mMultiFilterAddOn = NULL;
}

void SeqEditMultiFilterWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SHOW_EDIT_MULTIFILTER_MSG:
			SetMultiFilter(msg);
			break;
		case NAME_MSG: {
			if (mMultiFilterAddOn && mNameCtrl) {
				mMultiFilterAddOn->SetLabel(mNameCtrl->Text() );
				SetHasChanges(true);
			}
		} break;
		case KEY_MSG: {
			if (mMultiFilterAddOn && mKeyCtrl) {
				mMultiFilterAddOn->SetKey(mKeyCtrl->Text() );
				SetHasChanges(true);
			}
		} break;
		case AUTHOR_MSG: {
			if (mMultiFilterAddOn && mAuthorCtrl) {
				mMultiFilterAddOn->SetAuthor(mAuthorCtrl->Text() );
				SetHasChanges(true);
			}
		} break;
		case DESCRIPTION_MOD_MSG: {
			SetHasChanges(true);
		} break;

		case ADD_PIPELINE_MSG: {
			if (mMultiFilter) {
				mMultiFilter->PushPipeline();
				SetHasChanges(true);
			}
			if (mPipelineView) {
				mPipelineView->FillMetrics();
				mPipelineView->Invalidate();
			}
			SetPipelineScrollBars();
		} break;
		case DELETE_PIPELINE_MSG: {
			if (mMultiFilter) {
				mMultiFilter->PopPipeline();
				SetHasChanges(true);
			}
			if (mPipelineView) {
				mPipelineView->FillMetrics();
				mPipelineView->Invalidate();
			}
			SetPipelineScrollBars();
		} break;
		case AmNotifier::PIPELINE_CHANGE_OBS: {
			SetHasChanges(true);
			SetPipelineScrollBars();
		} break;
		case AmNotifier::FILTER_CHANGE_OBS: {
			SetHasChanges(true);
			SetPipelineScrollBars();
		} break;
		case NEW_ICON_MSG: {
			if (mMultiFilterAddOn && mIconEditor) {
				BPoint		size = seq_app->FilterImageSize();
				BBitmap*	bm = new BBitmap(BRect(0, 0, size.x -1, size.y -1), B_RGBA32);
				if (bm) {
					mMultiFilterAddOn->SetIcon(bm);
					mIconEditor->SetBitmap( const_cast<BBitmap*>(mMultiFilterAddOn->Icon(BPoint(20, 20))) );
					delete bm;
					SetHasChanges(true);
				}
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
		default:
			inherited::MessageReceived(msg);
	}
}

void SeqEditMultiFilterWindow::Quit()
{
	AmGlobalsImpl*	globals = gobals_impl();
	if (mMultiFilter && globals) {
		globals->UnregisterTemporaryMatrix(mMultiFilter->Id() );
	}

	/* The reason I do this is covered in SetMultiFilter().
	 */
	if (mPipelineView) mPipelineView->SetMatrixRef( AmPipelineMatrixRef(NULL) );
	inherited::Quit();
}

bool SeqEditMultiFilterWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::EDIT_MULTIFILTER_WIN_INDEX, config);
	}
	return true;
}

void SeqEditMultiFilterWindow::SetMultiFilter(const BMessage* multiMsg)
{
	if (!multiMsg) return;
	const char*		path = NULL;
	const char*		key = NULL;
	multiMsg->FindString("path", &path);
	multiMsg->FindString("key", &key);
	BString			k(key), p(path);
	SetMultiFilter(k, p);
}

void SeqEditMultiFilterWindow::SetMultiFilter(const BString& key, const BString& path)
{
	if (!SetEntryCheck() ) return;
	if (!Lock()) return;
	mInitialKey = (const char*)NULL;
	mInitialAuthor = (const char*)NULL;
	mInitialEmail = (const char*)NULL;
	SetHasChanges(false);

	if (mMultiFilter) {
		AmGlobalsImpl*	globals = gobals_impl();
		if (globals) globals->UnregisterTemporaryMatrix(mMultiFilter->Id() );
		mMultiFilter->RemoveMatrixObserver(0, this);
	}
	/* This is a bit of a hack, but the AmMultiFilter (which is the
	 * AmPipelineMatrixI object) doesn't actually store references to itself.
	 * So what happens is the mMultiFilter gets deleted here, then at the
	 * end of this method the mPipelineView's matrix ref -- which is the now-
	 * deleted mMultiFilter -- gets replaced with the new mMultiFilter.  During
	 * the replacement, the ref tries to remove a ref to the old multi filter,
	 * and crash.  So I clear it out here.
	 */
	if (mPipelineView) mPipelineView->SetMatrixRef( AmPipelineMatrixRef(NULL) );

	delete mMultiFilter;
	mMultiFilter = NULL;
	if (mMultiFilterAddOn) mMultiFilterAddOn->RemReference();
	mMultiFilterAddOn = NULL;

	const char*		s = NULL;
	if (path.Length() > 0) s = path.String();

	AmMultiFilterRoster*		roster = AmMultiFilterRoster::Default();
	if (roster) mMultiFilterAddOn = roster->NewFilter(key, s);
	if (!mMultiFilterAddOn) {
		const char*		auth;
		const char*		email;
		if (seq_get_string_preference(AUTHOR_PREF, &auth) != B_OK) auth = NULL;
		if (seq_get_string_preference(EMAIL_PREF, &email) != B_OK) email = NULL;
		mMultiFilterAddOn = new AmMultiFilterAddOn(auth, email);
	}

	if (mMultiFilterAddOn) {
		mMultiFilterAddOn->AddReference();
		mInitialKey = mMultiFilterAddOn->Key();
		mInitialAuthor = mMultiFilterAddOn->Author();
		mInitialEmail = mMultiFilterAddOn->Email();

		mMultiFilter = dynamic_cast<AmMultiFilter*>(mMultiFilterAddOn->NewInstance(NULL, NULL) );
		if (mMultiFilter) {
			AmGlobalsImpl*	globals = gobals_impl();
			if (globals) globals->RegisterTemporaryMatrix(mMultiFilter);
		}

		SetTextControl(mNameCtrl, mMultiFilterAddOn->Label().String(), NAME_MSG);
		SetTextControl(mKeyCtrl, mMultiFilterAddOn->Key().String(), KEY_MSG);
		SetTextControl(mAuthorCtrl, mMultiFilterAddOn->Author().String(), AUTHOR_MSG);
		SetTextControl(mEmailCtrl, mMultiFilterAddOn->Email().String(), EMAIL_MSG);
		if (mShortDescriptionCtrl) mShortDescriptionCtrl->SetText( mMultiFilterAddOn->ShortDescription().String() );
		if (mLongDescriptionCtrl) mLongDescriptionCtrl->SetText( mMultiFilterAddOn->LongDescriptionContents() );

		if (mIconEditor) mIconEditor->SetBitmap(const_cast<BBitmap*>(mMultiFilterAddOn->Icon(BPoint(20, 20))) );
	}
	if (mMultiFilter) {
		mMultiFilter->AddMatrixPipelineObserver(0, this);
		mMultiFilter->AddMatrixFilterObserver(0, this);
	}
	if (mPipelineView) mPipelineView->SetMatrixRef( AmPipelineMatrixRef(mMultiFilter) );

	SetWindowTitle();
	Unlock();
}

uint32 SeqEditMultiFilterWindow::ConfigWhat() const
{
	return SHOW_EDIT_MULTIFILTER_MSG;
}

bool SeqEditMultiFilterWindow::Validate()
{
	if (!mMultiFilterAddOn || !mMultiFilter) return ReportError("There is no multi filter");

	BString			key = mMultiFilterAddOn->Key();	
	if (key.Length() < 1) return ReportError("This multi filter must have a key");
	if (mInitialKey.Length() < 1 || mInitialKey != key) {
		AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
		if (!roster) return ReportError();
		if (roster && roster->KeyExists(key.String() ) ) {
			BString	error = "There is already a multi filter with the key \'";
			error << key.String() << "\'";
			return ReportError(error.String() );
		}
	}	
	return true;
}

void SeqEditMultiFilterWindow::SaveChanges()
{
	if (!HasChanges() || !mMultiFilterAddOn || !mMultiFilter) return;

	SetHiddenPrefs();

	if (mShortDescriptionCtrl) mMultiFilterAddOn->SetShortDescription(mShortDescriptionCtrl->Text() );
	if (mLongDescriptionCtrl) mMultiFilterAddOn->SetLongDescription(mLongDescriptionCtrl->Text() );

	BMessage		definition;
	if (mMultiFilter->WriteTo(definition) == B_OK) {
		mMultiFilterAddOn->SetDefinition(definition);
		AmMultiFilterRoster*	roster = AmMultiFilterRoster::Default();
		if (roster) roster->CreateEntry(mMultiFilterAddOn);
	}
	
	SetHasChanges(false);
}

const char* SeqEditMultiFilterWindow::EntryName() const
{
	return "multi filter";
}

void SeqEditMultiFilterWindow::SetWindowTitle()
{
	BString		title("Edit ");
	if (mMultiFilterAddOn) {
		BString	label = mMultiFilterAddOn->Label();
		title << label;
		if (label.Length() > 0) title << " ";
	}
	title << "Multi Filter";
	SetTitle( title.String() );
}

void SeqEditMultiFilterWindow::SetPipelineScrollBars()
{
	if (!mPipelineView || !mPipelineScrollView) return;
	BScrollBar*		b = mPipelineScrollView->ScrollBar(B_HORIZONTAL);
	if (b) arp_setup_horizontal_scroll_bar(b, mPipelineView);
	b = mPipelineScrollView->ScrollBar(B_VERTICAL);
	if (b) arp_setup_vertical_scroll_bar(b, mPipelineView);
}

BView* SeqEditMultiFilterWindow::NewGeneralView(BRect frame)
{
	BView*		v = new BView(frame, GENERAL_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	float		divider = v->StringWidth("Author:") + 10;
	BRect		f(spaceX, 0, frame.Width() - spaceX, fh);
	/* The Name field.
	 */
	mNameCtrl = new BTextControl(f, "name_ctrl", "Name:", NULL, new BMessage(NAME_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mNameCtrl) {
		f.top = mNameCtrl->Frame().bottom;
		mNameCtrl->SetDivider(divider);
		mNameCtrl->MakeFocus(true);
		v->AddChild(mNameCtrl);
	}
	/* The Key field.
	 */
	f.top += spaceY;
	f.bottom = f.top + fh;
	mKeyCtrl = new BTextControl(f, "key_ctrl", "Key:", NULL, new BMessage(KEY_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
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
	mEmailCtrl = new BTextControl(f, "email_ctrl", "Email:", NULL, new BMessage(EMAIL_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mEmailCtrl) {
		f.top = mEmailCtrl->Frame().bottom;
		mEmailCtrl->SetDivider(divider);
		v->AddChild(mEmailCtrl);
	}

	return v;
}

BView* SeqEditMultiFilterWindow::NewPipelineView(BRect frame)
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
	mPipelineView = new SeqPipelineMatrixView(	pipelineF, "pipeline_matrix",
												AmPipelineMatrixRef(mMultiFilter),
												NULLINPUTOUTPUT_PIPELINE, SEQ_SUPPRESS_BACKGROUND);
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

BView* SeqEditMultiFilterWindow::NewDescriptionView(BRect frame)
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
	BMenuItem*	item = new BMenuItem("New", new BMessage(NEW_ICON_MSG), 0, 0);
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

BView* SeqEditMultiFilterWindow::NewIconView(BRect frame)
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
