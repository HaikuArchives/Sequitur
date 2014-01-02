/* SeqEditRosterWindow.cpp
 */
#include <stdlib.h>
#include <interface/Button.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "Sequitur/SeqEditRosterWindow.h"
#include "Sequitur/SeqSplitterView.h"
#include "Sequitur/SequiturDefs.h"

ArpMOD();

static const char*		TABLE_STR				= "table";
static const char*		PAGE_STR				= "Page";
static const char*		OK_BUTTON_STR			= "ok_button";
static const char*		CANCEL_BUTTON_STR		= "cancel_button";

static const uint32		OK_MSG					= 'iOK_';
static const uint32		CANCEL_MSG				= 'iCnc';

static const uint32		NAME_COL_INDEX			= 0;

/********************************************************
 * _TOOL-PAGE-LIST-VIEW
 ********************************************************/
class _ToolPageListView : public BColumnListView
{
public:
	_ToolPageListView(BRect rect, const char *name,
					SeqEditRosterWindow* window);
	
	virtual void SelectionChanged();
	
private:
	typedef BColumnListView	inherited;
	SeqEditRosterWindow* mWindow;
};

/********************************************************
 * _TOOL-PAGE-ROW
 ********************************************************/
class _ToolPageRow : public BRow
{
public:
	_ToolPageRow(const char* name, BView* view, float width);
	virtual ~_ToolPageRow();
	
	virtual bool		HasLatch() const;
	BView*				View() const;
	float				Width() const;
		
protected:
	BView*				mView;
	float				mWidth;
};

/******************************************************************
 * SEQ-EDIT-ROSTER-WINDOW
 ******************************************************************/
SeqEditRosterWindow::SeqEditRosterWindow(	BRect frame, const char* name,
											window_look look, window_feel feel,
											uint32 flags)
	: BWindow(frame, name, look, feel, flags, B_CURRENT_WORKSPACE),
	  mAuthorCtrl(NULL), mEmailCtrl(NULL), mShortDescriptionCtrl(NULL),
	  mBg(NULL), mListView(NULL), mSplitter(NULL),
	  mPageView(NULL), mBlankPageView(NULL),
	  mHasChanges(false), mForceClose(false)
{
	AddCommonFilter(this);

	mBlankPageView = new BView(BRect(0, 0, 0, 0), "blank", B_FOLLOW_ALL, 0);
	if (mBlankPageView) mBlankPageView->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );

	BRect			b(Bounds() );
	BRect			r(b);
	r.bottom = b.bottom - B_H_SCROLL_BAR_HEIGHT;
	AddViews(r);
	/* This accounts for the document window tab.  That tab does
	 * not play nicely with a background that goes completely behind
	 * it -- it leaves trails -- so we have to chunk the background
	 * into two parts.
	 */
	r.top = r.bottom + 1;
	r.bottom = b.bottom;
	r.right = b.right - B_V_SCROLL_BAR_WIDTH;
	BView*		v = new BView(r, "filler", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, 0);
	if (v) {
		v->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		AddChild(v);
	}
}

SeqEditRosterWindow::~SeqEditRosterWindow()
{
	RemoveCommonFilter(this);
	if (mPageView) RemoveChild(mPageView);
	if (mBlankPageView) delete mBlankPageView;
}

void SeqEditRosterWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case OK_MSG:
			if (!Validate() ) return;
			SaveChanges();
			PostMessage(B_QUIT_REQUESTED);
			break;
		case CANCEL_MSG:
			mForceClose = true;
			PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			inherited::MessageReceived(message);
	}
}

void SeqEditRosterWindow::Quit()
{
	RemoveCommonFilter(this);
	inherited::Quit();
}

bool SeqEditRosterWindow::QuitRequested()
{
	if (mForceClose) return true;
	if (!HasChanges() ) return true;
	return SetEntryCheck();
}

void SeqEditRosterWindow::SetPage(BView* view)
{
	if (!mBg || !mSplitter) return;
	if (!view) view = mBlankPageView;
	if (!view) return;
	BRect		targetF(CurrentPageFrame() );
	BRect		pageF = view->Frame();
	view->SetResizingMode(B_FOLLOW_ALL);
	if (pageF.LeftTop() != targetF.LeftTop() ) view->MoveTo(targetF.LeftTop() );
	if (mPageView) mBg->RemoveChild(mPageView);
	mBg->AddChild(view);
	if (pageF.Width() != targetF.Width() || pageF.Height() != targetF.Height() )
		view->ResizeTo(targetF.Width(), targetF.Height() );
	mPageView = view;
}

bool SeqEditRosterWindow::IsSignificant() const
{
	return false;
}

status_t SeqEditRosterWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT(config);
	config->what = ConfigWhat();
	status_t	err = GetDimensions(config, this);
	if (err != B_OK) return err;
	if (mSplitter) {
		BRect		b(Bounds() );
		config->AddFloat("split_point", mSplitter->Frame().left / b.Width() );
	}
	return B_OK;
}

status_t SeqEditRosterWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	status_t	err = SetDimensions(config, this);
	if (err != B_OK) return err;
	SetFirstPage();
	float		f;
	if (config->FindFloat("split_point", &f) == B_OK) {
		if (mListView && mSplitter && mPageView && f >= 0 && f <= 1) {
			BRect		b(Bounds() );
			f = f * b.Width();
			BRect		r = mListView->Frame();
			mListView->ResizeTo(f - 5, r.Height() );
			r = mSplitter->Frame();
			mSplitter->MoveTo(f, r.top);
			r = mSplitter->Frame();
			float		t = mPageView->Frame().top;
			mPageView->MoveTo(r.right + 1, t);
			mPageView->ResizeTo(b.Width() - (r.right + 6), r.Height() );
		}
	}
	return B_OK;
}

bool SeqEditRosterWindow::SetEntryCheck()
{
	Activate(true);
	if (!HasChanges() ) return true;
	BString			warning("Save changes to ");
	if (EntryName() ) warning << EntryName();
	warning << "?";
	BAlert*	alert = new BAlert(	"Warning", warning.String(),
								"Cancel", "Don\'t Save", "Save", B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	uint32			ans = 0;
	if (alert) ans = alert->Go();
	if (ans == 0) return false;
	else if (ans == 1) return true;
	
	if (!Validate() ) return false;
	if (!Lock() ) return false;
	SaveChanges();
	Unlock();
	return true;
}

void SeqEditRosterWindow::SetHiddenPrefs()
{
	if (mAuthorCtrl && mInitialAuthor != mAuthorCtrl->Text() )
		seq_set_string_preference(AUTHOR_PREF, mAuthorCtrl->Text() );
	if (mEmailCtrl && mInitialEmail != mEmailCtrl->Text() )
		seq_set_string_preference(EMAIL_PREF, mEmailCtrl->Text() );
}

bool SeqEditRosterWindow::HasChanges() const
{
	return mHasChanges;
}

void SeqEditRosterWindow::SetHasChanges(bool hasChanges)
{
	mHasChanges = hasChanges;
	if ( !Lock() ) return;
	BButton*		okButton = dynamic_cast<BButton*>(FindView(OK_BUTTON_STR) );
	if (mHasChanges && okButton && !okButton->IsEnabled() )
		okButton->SetEnabled(true);
	else if (!mHasChanges && okButton && okButton->IsEnabled() )
		okButton->SetEnabled(false);
	Unlock();
}

status_t SeqEditRosterWindow::AddPage(BView* page)
{
	ArpVALIDATE(page, return B_ERROR);
	if (!mListView) return B_NO_MEMORY;
	float			width = 0;
	if (page->Name() ) width = mListView->StringWidth(page->Name() );
	_ToolPageRow*	newRow = new _ToolPageRow(page->Name(), page, width);
	if (newRow) mListView->AddRow(newRow);
	return B_OK;
}

BRect SeqEditRosterWindow::CurrentPageFrame() const
{
	if (!mSplitter) return arp_invalid_rect();
	BRect		f = Bounds();
	BRect		splitterF = mSplitter->Frame();
	return BRect(splitterF.right + 1, splitterF.top, f.right - 5, splitterF.bottom);
}

status_t SeqEditRosterWindow::SetFirstPage()
{
	if (!mListView) return B_ERROR;
	BRow*		r = mListView->RowAt(0);
	if (!r) return B_ERROR;
	mListView->AddToSelection(r);
	mListView->SelectionChanged();
	/* By default, the width of the mListView is just wide enough
	 * to display the longest page name.  (But not greater than half
	 * the total width of the view)
	 */
	float		w = 0;
	for (int32 k = 0; (r = mListView->RowAt(k)) != NULL; k++) {
		_ToolPageRow*	tp = dynamic_cast<_ToolPageRow*>(r);
		if (tp && tp->Width() > w) w = tp->Width();
	}
	BRect		b = Bounds();
	float		half = b.Width() / 2;
	w += 50;		// Account for the left area of the list view
	if (w < 20) w = 20;
	else if (w > half) w = half;
	if (mSplitter && mPageView) {
		mListView->ResizeTo(w, mListView->Frame().Height() );
		mSplitter->MoveTo(mListView->Frame().right + 1, mSplitter->Frame().top);
		BRect		pf = mPageView->Frame();
		BRect		sf = mSplitter->Frame();
		mPageView->MoveTo(sf.right + 1, pf.top);
		mPageView->ResizeTo(b.Width() - sf.right, pf.Height() );
	}
	return B_OK;
}

bool SeqEditRosterWindow::ReportError(const char* error)
{
	if (!error) low_memory_warning();
	else {
		BAlert*	alert = new BAlert(	"Warning", error, "OK", NULL, NULL,
									B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert) alert->Go();
		else low_memory_warning();
	}
	return false;
}

void SeqEditRosterWindow::SetTextControl(BTextControl* ctrl, const char* text, uint32 what)
{
	if (!ctrl) return;
	ctrl->SetModificationMessage(NULL);
	ctrl->SetText(text);
	if (what != 0) ctrl->SetModificationMessage(new BMessage(what) );
}

void SeqEditRosterWindow::AddViews(BRect frame)
{
	float				x = Prefs().Size(SPACE_X), y = Prefs().Size(SPACE_Y);
	float				buttonX = 60, buttonY = Prefs().Size(BUTTON_Y);
	BRect				buttonF(0, frame.Height() - 5 - buttonY - 5, frame.Width(), frame.Height() );
	BRect				okF(buttonF.right - 20 - buttonX, buttonF.top + 5, buttonF.right - 20, buttonF.top + 5 + buttonY);
	BRect				cancelF(okF.left - 5 - buttonX, okF.top, okF.left - 5, okF.bottom);
	float				tableW = (frame.Width() - x - x) / 2;
	BRect				tableF(x, y, x + tableW, buttonF.top - 1);
	BRect				splitterF(tableF.right + 1, tableF.top, tableF.right + 5, tableF.bottom);
	
	mBg = new BView(frame, "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!mBg) return;
	mBg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	AddChild(mBg);

	BButton*	button = new BButton(okF, OK_BUTTON_STR, "OK", new BMessage(OK_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
	if (button) {
		button->SetEnabled(false);
		mBg->AddChild(button);
	}
	button = new BButton(cancelF, CANCEL_BUTTON_STR, "Cancel", new BMessage(CANCEL_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM );
	if (button) mBg->AddChild(button);

	mListView = new _ToolPageListView(tableF, TABLE_STR, this);
	if (!mListView) return;
	mBg->AddChild(mListView);
	mListView->AddColumn(new BStringColumn(PAGE_STR, 110, 20, 350, B_TRUNCATE_MIDDLE), NAME_COL_INDEX);

	mSplitter = new SeqSplitterView(splitterF, "v_splitter", B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, B_VERTICAL);
	if (mSplitter) {
		mSplitter->SetDrawingFlags(SeqSplitterView::NO_DRAWING_FLAG);
		mSplitter->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
		mBg->AddChild(mSplitter);
	}
}

// #pragma mark -

/********************************************************
 * _TOOL-PAGE-LIST-VIEW
 ********************************************************/
_ToolPageListView::_ToolPageListView(	BRect rect, const char *name,
										SeqEditRosterWindow* window)
		: inherited(rect, name, B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER),
		  mWindow(window)
{
	SetSelectionMode(B_SINGLE_SELECTION_LIST);
	SetSortingEnabled(false);
}

void _ToolPageListView::SelectionChanged()
{
	if (!mWindow) return;
	BView*				v = NULL;
	_ToolPageRow* 	row = dynamic_cast<_ToolPageRow*>(CurrentSelection() );
	if (row) v = row->View();
	mWindow->SetPage(v);
}

// #pragma mark -

/********************************************************
 * _TOOL-PAGE-ROW
 ********************************************************/
_ToolPageRow::_ToolPageRow(	const char* name, BView* view,
							float width)
		: mView(view), mWidth(width)
{
	if (name) SetField(new BStringField(name), NAME_COL_INDEX);
}

_ToolPageRow::~_ToolPageRow()
{
	if (mView) delete mView;
}

bool _ToolPageRow::HasLatch() const
{
	if (mView) return false;
	else return true;
}

BView* _ToolPageRow::View() const
{
	return mView;
}

float _ToolPageRow::Width() const
{
	return mWidth;
}

// #pragma mark -

/******************************************************************
 * SEQ-DUMB-TEST-VIEW
 ******************************************************************/
SeqDumbTextView::SeqDumbTextView(	BRect frame, const char* name,
									BRect textRect, uint32 resizeMask,
									uint32 flags)
		: inherited(frame, name, textRect, resizeMask, flags),
		  mModificationMessage(NULL)
{
}

SeqDumbTextView::~SeqDumbTextView()
{
	delete mModificationMessage;
}
	
void SeqDumbTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	inherited::DeleteText(fromOffset, toOffset);
	if (mModificationMessage && Window() ) {
		BMessage		m(*mModificationMessage);
		if (!Parent() ) Window()->PostMessage(&m);
		else Window()->PostMessage(&m, Parent() );
	}
}

void SeqDumbTextView::FrameResized(float width, float height)
{
	inherited::FrameResized(width, height);
	SetTextRect(BRect(5, 5, Bounds().Width() - 10, 0) );
}

void SeqDumbTextView::InsertText(	const char *inText, int32 inLength, 
									int32 inOffset, const text_run_array *inRuns)
{
	inherited::InsertText(inText, inLength, inOffset, inRuns);
	if (mModificationMessage && Window() ) {
		BMessage		m(*mModificationMessage);
		if (!Parent() ) Window()->PostMessage(&m);
		else Window()->PostMessage(&m, Parent() );
	}
}

void SeqDumbTextView::SetModificationMessage(BMessage* msg)
{
	delete mModificationMessage;
	mModificationMessage = msg;
}
