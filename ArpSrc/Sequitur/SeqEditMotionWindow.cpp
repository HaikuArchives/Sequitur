/* SeqEditMotionWindow.cpp
 */
#include <InterfaceKit.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmMotion.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqEditMotionWindow.h"
#include "Sequitur/SequiturDefs.h"

static const uint32		NAME_MSG					= '#nam';
static const uint32		AUTHOR_MSG					= '#aut';
static const uint32		EMAIL_MSG					= '#eml';
static const uint32		CHANGED_MSG					= 'a_gc';
static const uint32		DESCRIPTION_MOD_MSG			= 'iDsM';

static const char*		DESCRIPTION_STR				= "Description";
static const char*		GENERAL_STR					= "General";
static const char*		MOTION_STR					= "Motion";

/*************************************************************************
 * SEQ-EDIT-MOTION-WINDOW
 *************************************************************************/
SeqEditMotionWindow::SeqEditMotionWindow(	BRect frame,
											const BMessage* config,
											const BMessage* motionMsg)
		: inherited(frame, "Edit Motion"),
		  mMotion(NULL), mNameCtrl(NULL), mEditor(NULL)
{
	BRect		targetF(CurrentPageFrame() );
	BView*		view = NULL;
	if ((view = NewGeneralView(targetF))) AddPage(view);
	if ((view = NewMotionView(targetF))) AddPage(view);
	if ((view = NewDescriptionView(targetF))) AddPage(view);

	if (config) SetConfiguration(config);
	else SetFirstPage();
	if (motionMsg) SetMotion(motionMsg);
}

SeqEditMotionWindow::~SeqEditMotionWindow()
{
	delete mMotion;
}

void SeqEditMotionWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case SHOW_EDIT_MOTION_MSG:
			SetMotion(msg);
			break;
		case NAME_MSG: {
			if (mMotion && mNameCtrl) {
				mMotion->SetLabel(mNameCtrl->Text());
				SetWindowTitle();
				SetHasChanges(true);
			}
		} break;
		case AUTHOR_MSG: {
			if (mMotion && mAuthorCtrl) {
				mMotion->SetAuthor(mAuthorCtrl->Text());
				SetHasChanges(true);
			}
		} break;
		case EMAIL_MSG: {
			if (mMotion && mEmailCtrl) {
				mMotion->SetEmail(mEmailCtrl->Text());
				SetHasChanges(true);
			}
		} break;
		case DESCRIPTION_MOD_MSG: {
			SetHasChanges(true);
		} break;
		case CHANGED_MSG: {
			SetHasChanges(true);
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqEditMotionWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
		else seq_app->SetAuxiliaryWindowSettings(SeqApplication::EDIT_MOTION_WIN_INDEX, config);
	}
	return true;
}

void SeqEditMotionWindow::SetMotion(const BMessage* motionMsg)
{
	if (!motionMsg) return;
	const char*		path;
	if (motionMsg->FindString("path", &path) != B_OK) path = NULL;
	const char*		motionName;
	if (motionMsg->FindString("motion_unique_name", &motionName) == B_OK)
		SetMotion(motionName, path);
}

void SeqEditMotionWindow::SetMotion(const BString& key, const BString& path)
{
	if (!SetEntryCheck() ) return;
	if (!Lock()) return;

	mInitialKey = (const char*)NULL;
	mInitialAuthor = (const char*)NULL;
	mInitialEmail = (const char*)NULL;
	SetHasChanges(false);

	delete mMotion;
	mMotion = NULL;
	AmMotionRoster*	roster = AmMotionRoster::Default();
	if (roster) {
		const char*		s = NULL;
		if (path.Length() > 0) s = path.String();

		AmMotionI*	motion = roster->NewMotion(key, s);
		if (motion) mMotion = new AmMotion(*motion);
		if (mMotion) mInitialKey = mMotion->Key();
		else {
			const char*		auth;
			const char*		email;
			if (seq_get_string_preference(AUTHOR_PREF, &auth) != B_OK) auth = NULL;
			if (seq_get_string_preference(EMAIL_PREF, &email) != B_OK) email = NULL;
			mMotion = new AmMotion(auth, email);
		}
		if (mMotion) {
			SetTextControl(mNameCtrl, mMotion->Label().String(), NAME_MSG);
			SetTextControl(mAuthorCtrl, mMotion->Author().String(), AUTHOR_MSG);
			SetTextControl(mEmailCtrl, mMotion->Email().String(), EMAIL_MSG);
			if (mEditor) mEditor->SetMotion(mMotion);
			if (mShortDescriptionCtrl) mShortDescriptionCtrl->SetText(mMotion->ShortDescription().String() );
		}
		delete motion;
	}
	SetWindowTitle();
	Unlock();
}

uint32 SeqEditMotionWindow::ConfigWhat() const
{
	return EDIT_MOTION_WINDOW_SETTING_MSG;
}

bool SeqEditMotionWindow::Validate()
{
	if (!mMotion) return ReportError("There is no motion");

	BString			key = mMotion->Key();	
	if (key.Length() < 1) return ReportError("This motion must have a name");
	if (mInitialKey.Length() < 1 || mInitialKey != key) {
		AmMotionRoster*	roster = AmMotionRoster::Default();
		if (!roster) return ReportError();
		if (roster && roster->KeyExists(key.String() ) ) {
			BString	error = "There is already a motion with the name \'";
			error << key.String() << "\'";
			return ReportError(error.String() );
		}
	}	
	return true;
}

void SeqEditMotionWindow::SaveChanges()
{
	if (!HasChanges() || !mMotion) return;

	SetHiddenPrefs();

	if (mNameCtrl && mEditor) {
		const char*		auth = NULL;
		const char*		email = NULL;
		if (mAuthorCtrl) auth = mAuthorCtrl->Text();
		if (mEmailCtrl) email = mEmailCtrl->Text();
		AmMotion*		m = new AmMotion(	mNameCtrl->Text(), auth, email,
											mMotion->LocalFilePath().String() );
		/* Add hits also sets the editing mode and signatures.
		 */
		if (m && mEditor->AddHitsTo(m) == B_OK) {
			if (mShortDescriptionCtrl) m->SetShortDescription(mShortDescriptionCtrl->Text() );
			AmMotionRoster*		roster = AmMotionRoster::Default();
			if (roster) roster->CreateEntry(m);
		}
		delete m;
	}
	SetHasChanges(false);
}

const char* SeqEditMotionWindow::EntryName() const
{
	return "motion";
}

void SeqEditMotionWindow::SetWindowTitle()
{
	BString		title("Edit ");
	if (mMotion) {
		BString	label = mMotion->Label();
		title << label;
		if (label.Length() > 0) title << " ";
	}
	title << "Motion";
	SetTitle( title.String() );
}

BView* SeqEditMotionWindow::NewGeneralView(BRect frame)
{
	BView*		v = new BView(frame, GENERAL_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	float		fh = arp_get_font_height(v);
	float		spaceX = 5, spaceY = 5;
	float		divider = v->StringWidth("Author:") + 10;
	/* The name field.
	 */
	BRect		f(spaceX, 0, frame.Width() - spaceX, fh);
	mNameCtrl = new BTextControl(f, "name_ctrl", "Name:", NULL, new BMessage(NAME_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mNameCtrl) {
		f.top = mNameCtrl->Frame().bottom;
		mNameCtrl->SetDivider(divider);
		mNameCtrl->MakeFocus(true);
		v->AddChild(mNameCtrl);
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

BView* SeqEditMotionWindow::NewMotionView(BRect frame)
{
	BView*		v = new BView(frame, MOTION_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	float		x = Prefs().Size(SPACE_X);
	BRect		f(x, 0, frame.Width() - x, frame.Height() );

	mEditor = new AmMotionEditor(f, "rhythm_editor", B_FOLLOW_ALL);
	if (mEditor) v->AddChild(mEditor);

	return v;
}

BView* SeqEditMotionWindow::NewDescriptionView(BRect frame)
{
	BView*			v = new BView(frame, DESCRIPTION_STR, B_FOLLOW_ALL, 0);
	if (!v) return NULL;
	v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	float			fh = arp_get_font_height(v);
	float			spaceX = 5, spaceY = 5;
	BRect			shortLabelR(spaceX, 0, frame.Width() - spaceX, fh);
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
