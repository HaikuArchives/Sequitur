/* SeqSignatureWindow.cpp
 */
#include <cstdio>
#include <app/Message.h>
#include <interface/Box.h>
#include <interface/Menu.h>
#include <interface/MenuItem.h>
#include <interface/View.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViews/ArpIntControl.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqSignatureWindow.h"
#include "Sequitur/SequiturDefs.h"

static const uint32		FLIP_LOCK_TO_SONG_MSG = 'iFls';
static const uint32		OK_MSG			= 'iOK_';
static const uint32		CANCEL_MSG		= 'iCnc';
static const uint32		MORE_MSG		= 'iMOR';
static const uint32		FEWER_MSG		= 'iFEW';
static const float		INDENT			= 5;
static const float		LABEL_PAD		= 9;
static const float		CHOICES_SPACE_Y	= 5;
static const char*		MEASURE_INT		= "measureint";

/*************************************************************************
 * SEQ-SIGNATURE-WINDOW
 *************************************************************************/
SeqSignatureWindow::SeqSignatureWindow(AmSongRef songRef, BRect frame)
		: inherited(frame, "Signature", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
		  AmSongObserver(songRef),
		  mBg(NULL), mLockToSong(NULL), mOkButton(NULL), mCancelButton(NULL),
		  mMoreButton(NULL), mFewerButton(NULL), mBottomLine(NULL),
		  mChoicesTop(0)
{
	AddViews(false);
	// READ SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		BString		name("Set ");
		if ( song->Name() ) name << song->Name();
		name << " Signature";
		SetTitle( name.String() );
	}
	WriteUnlock(song);
	// END READ SONG BLOCK
}

SeqSignatureWindow::SeqSignatureWindow(	AmSongRef songRef, AmTrackRef trackRef,
										BRect frame)
		: inherited(frame, "Signature", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS | B_NOT_RESIZABLE),
		  AmSongObserver(songRef), mTrackRef(trackRef),
		  mBg(NULL), mOkButton(NULL), mCancelButton(NULL),
		  mMoreButton(NULL), mFewerButton(NULL), mBottomLine(NULL),
		  mChoicesTop(0)
{
	AddViews(true);
	// READ TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : 0;
	if (track) {
		BString		name("Set ");
		if ( track->Title() ) name << track->Title();
		name << " Signature";
		SetTitle( name.String() );
	}
	WriteUnlock(song);
	// END READ TRACK BLOCK
}

SeqSignatureWindow::~SeqSignatureWindow()
{
}

void SeqSignatureWindow::MessageReceived(BMessage* msg)
{
	switch ( msg->what ) {
		case FLIP_LOCK_TO_SONG_MSG: {
			if (mLockToSong) {
				bool	enable = mLockToSong->Value() == B_CONTROL_OFF;
				for (uint32 k = 0; k < mEntries.size(); k++)
					mEntries[k].SetEnabled(enable);
			}
			} break;
		case OK_MSG:
			SetSignature();
			SetPreferences();
			Quit();
			break;
		case CANCEL_MSG:
			Quit();
			break;
		case MORE_MSG:
			AddPopUpChoice();
			break;
		case FEWER_MSG:
			RemovePopUpChoice();
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

bool SeqSignatureWindow::QuitRequested()
{
	if ( !inherited::QuitRequested() ) return false;
	return true;
}

void SeqSignatureWindow::SetMeasure(int32 measure, uint32 beats, uint32 beatValue)
{
	if (!mBg) return;
	if ( Lock() ) {
		ArpIntControl* intCtrl = dynamic_cast<ArpIntControl*>( mBg->FindView(MEASURE_INT) );
		if (intCtrl) intCtrl->SetValue(measure);
		if (mEntries.size() > 0) mEntries[0].Set(beats, beatValue);
		Unlock();
	}
}

void SeqSignatureWindow::SetSignature()
{
	/* If the user selected lock to song, then actually I UNSET
	 * all the track's signatures.
	 */
	if (mLockToSong && mLockToSong->Value() == B_CONTROL_ON) {
		// WRITE TRACK BLOCK
		AmSong*		song = WriteLock();
		AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
		if (track) track->LockSignaturesToSong();
		WriteUnlock(song);
		// END WRITE TRACK BLOCK
		return;
	}
	
	AmSignature		sig;
	if (GetSignature(sig) != B_OK) return;
	/* This window is slightly modal -- do something different depending upon
	 * whether or not I have a valid track.
	 */
	// WRITE SONG OR TRACK BLOCK
	AmSong*		song = WriteLock();
	if (song) {
		if ( mTrackRef.IsValid() ) {
			AmTrack*	track = song->Track(mTrackRef);
			if (track) track->SetSignature( sig.Measure(), sig.Beats(), sig.BeatValue() );
		} else {
			song->SetSignature( sig.Measure(), sig.Beats(), sig.BeatValue() );
		}
	}
	WriteUnlock(song);
	// END WRITE SONG OR TRACK BLOCK
}

void SeqSignatureWindow::SetPreferences()
{
	BMessage	prefs;
	if (seq_app->GetPreferences(&prefs) == B_OK) {
		BMessage	sigPrefs;
		AmSignature	sig;
		for(uint32 k = 1; k < mEntries.size(); k++) {
			if (mEntries[k].GetSignature(sig) == B_OK) {
				sigPrefs.AddInt32( "beats", sig.Beats() );
				sigPrefs.AddInt32( "beat value", sig.BeatValue() );
			}
		}
		if ( prefs.HasMessage(SIGNATURE_CHOICES_PREF, 0) ) prefs.ReplaceMessage(SIGNATURE_CHOICES_PREF, 0, &sigPrefs);
		else prefs.AddMessage(SIGNATURE_CHOICES_PREF, &sigPrefs);
		
		seq_app->SetPreferences(&prefs);
	}
}

void SeqSignatureWindow::AddPopUpChoice()
{
	if (!mBg) return;
	float		top = mChoicesTop;
	if (mEntries.size() > 1) top = mEntries[mEntries.size()-1].mBottom;
	top += CHOICES_SPACE_Y;
	float		bottom = AddSigRow(mBg, top, 4, 4) + CHOICES_SPACE_Y;
	ResizeBy(0, bottom - top);
	if ( mFewerButton && !mFewerButton->IsEnabled() )
		mFewerButton->SetEnabled(true);
}

void SeqSignatureWindow::RemovePopUpChoice()
{
	if (mEntries.size() <= 1) return;
	uint32		k = mEntries.size() - 1;
	float		bottom = mEntries[k].mBottom;
	mEntries[k].Remove();
	mEntries.resize(k);
	float		top = mChoicesTop;
	if (mEntries.size() > 1) {
		top = mEntries[mEntries.size()-1].mBottom;
	}
	ResizeBy(0, top - bottom);
	if (mFewerButton && mEntries.size() <= 1)
		mFewerButton->SetEnabled(false);
	/* If no row is on, turn on the first on.
	 */
	bool	isOn = false;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].IsOn()) {
			isOn = true;
			break;
		}
	}
	if (!isOn && mEntries.size() > 0) {
		if (mEntries[0].mRadio) mEntries[0].mRadio->SetValue(B_CONTROL_ON);
	}
}

static float view_font_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

class _LineView : public BView
{
public:
	_LineView(	BRect frame,
				const char* name = "Line",
				const char* label = NULL,
				uint32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
				uint32 flags = B_WILL_DRAW)
			: BView(frame, name, resizeMask, flags)
	{
		if (label) {
			float			w = StringWidth(label);
			BStringView*	sv = new BStringView( BRect(INDENT + 10, 0, w + 20, frame.Height() ), "label", label);
			if (sv) AddChild(sv);
		}
	}
	
	virtual void AttachedToWindow()
	{
		BView::AttachedToWindow();
		if ( Parent() ) SetViewColor( Parent()->ViewColor() );
	}

	virtual void Draw(BRect clip)
	{
		BView::Draw(clip);
		float		mid = Bounds().Height() / 2;
		SetHighColor( tint_color(ViewColor(), B_DARKEN_2_TINT) );
		StrokeLine( BPoint(clip.left, mid), BPoint(clip.right, mid) );
		SetHighColor( tint_color(ViewColor(), B_LIGHTEN_2_TINT) );
		StrokeLine( BPoint(clip.left, mid + 1), BPoint(clip.right, mid + 1) );
	}
};
	
void SeqSignatureWindow::AddViews(bool showLockToSong)
{
	BRect		b = Bounds();
	mBg = new BView(b, "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!mBg) return;
	mBg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	AddChild(mBg);
	float		fh = view_font_height(mBg);
	float		ih = Prefs().Size(INT_CTRL_Y);
	float		max = (fh > ih) ? fh : ih;
	float		top = INDENT;
	
	if (showLockToSong) {
		const char*	label = "Lock to song";
		float	w = mBg->StringWidth(label) + 20;
		mLockToSong = new BCheckBox( BRect(INDENT, top, INDENT + w, top + 15), "locktosong", label, new BMessage(FLIP_LOCK_TO_SONG_MSG) );
		if (mLockToSong) {
			top += mLockToSong->Bounds().Height() + 5;
			mBg->AddChild(mLockToSong);
		}
	}

	float		divider = mBg->StringWidth("Measure:") + LABEL_PAD;
	float		width = mBg->StringWidth("999999") + 5;
	BRect		r(INDENT, top, INDENT + divider + width, top + max);
	ArpIntControl* intCtrl = new ArpIntControl(r, MEASURE_INT, "Measure:", NULL);
	if (intCtrl) {
		intCtrl->SetLimits(1, 999999);
		intCtrl->SetValue(1);
		intCtrl->SetDivider(divider);
		mBg->AddChild(intCtrl);
	}
	top = AddSigRow(mBg, top + max + INDENT, 4, 4, true) + 5;
	
	/* Build rows for each popup choice in the preferences.
	 */
	_LineView*	lv = new _LineView(BRect( b.left, top, b.right, top + fh), "Line", "Popup choices");
	if (lv) mBg->AddChild(lv);
	top = top + fh + CHOICES_SPACE_Y;
	mChoicesTop = top - CHOICES_SPACE_Y;
	BMessage	signatureChoices;
	if (seq_get_message_preference(SIGNATURE_CHOICES_PREF, &signatureChoices) == B_OK) {
		int32	beats;
		for(int32 k = 0; signatureChoices.FindInt32("beats", k, &beats) == B_OK; k++) {
			int32	beatvalue;
			if (signatureChoices.FindInt32("beat value", k, &beatvalue) == B_OK) {
				top = AddSigRow(mBg, top, beats, beatvalue) + CHOICES_SPACE_Y;
			}
		}
	}
	float		buttonW = 60, buttonH = 24;
	r.Set(b.left + INDENT, top, b.left + INDENT + buttonW, top + buttonH);
	mMoreButton = new BButton( r, "more_button", "More", new BMessage(MORE_MSG) );
	if (mMoreButton) mBg->AddChild(mMoreButton);
	r.left = r.right + INDENT;
	r.right = r.left + buttonW;
	mFewerButton = new BButton( r, "fewer_button", "Fewer", new BMessage(FEWER_MSG) );
	if (mFewerButton) {
		mBg->AddChild(mFewerButton);
		if (mEntries.size() <= 1) mFewerButton->SetEnabled(false);
	}
	mBottomLine = new _LineView(BRect( b.left, r.bottom + 5, b.right, r.bottom + 6), "Line", NULL);
	if (mBottomLine) mBg->AddChild(mBottomLine);

	/* Add the OK / Cancel buttons.
	 */
	top = r.bottom + 6 + INDENT + 4;	
	r.Set(b.left + INDENT + 10, top, b.left + INDENT + 10 + buttonW, top + buttonH);
	mCancelButton = new BButton(r, "cnl_button", "Cancel", new BMessage(CANCEL_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mCancelButton) mBg->AddChild(mCancelButton);

	r.left = r.right + 10;
	r.right = r.left + buttonW;
	mOkButton = new BButton(r, "ok_button", "OK", new BMessage(OK_MSG), B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if( mOkButton ) {
		mBg->AddChild(mOkButton);
		mOkButton->MakeDefault(true);
	}

	float	w = 0, h = 0;
	for(BView* view = mBg->ChildAt(0); view; view = view->NextSibling() ) {
		if (strcmp(view->Name(), "Line") != 0) {
			BRect	f = view->Frame();
			if( f.right > w ) w = f.right;
			if( f.bottom > h ) h = f.bottom;
		}
	}
	ResizeTo( w + INDENT, h + INDENT );
	if (mOkButton) mOkButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	if (mCancelButton) mCancelButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	if (mMoreButton) mMoreButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	if (mFewerButton) mFewerButton->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	if (mBottomLine) mBottomLine->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
}

static BMenuField* new_beat_value_menu(BPoint topLeft, int32 val)
{
	BMenu*		menu = new BMenu("beatvalmenu");
	if (!menu) return NULL;
	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);
	BMenuItem*	item = new BMenuItem("1", NULL);
	if (item) { menu->AddItem(item); if (val == 1) item->SetMarked(true); }
	if ( (item = new BMenuItem("2", NULL)) )  { menu->AddItem(item); if (val == 2) item->SetMarked(true); }
	if ( (item = new BMenuItem("4", NULL)) )  { menu->AddItem(item); if (val == 4) item->SetMarked(true); }
	if ( (item = new BMenuItem("8", NULL)) )  { menu->AddItem(item); if (val == 8) item->SetMarked(true); }
	if ( (item = new BMenuItem("16", NULL)) ) { menu->AddItem(item); if (val == 16) item->SetMarked(true); }
	if ( (item = new BMenuItem("32", NULL)) ) { menu->AddItem(item); if (val == 32) item->SetMarked(true); }
	return new BMenuField(BRect(topLeft, topLeft), "beatvalfield", NULL, menu);
}

float SeqSignatureWindow::AddSigRow(BView* toView, float top, uint32 beats, uint32 beatValues,
									bool on)
{
	ArpASSERT(toView);
	float			ih = Prefs().Size(INT_CTRL_Y);
	float			intW = toView->StringWidth("999") + 10;
	BRect			rbR(INDENT, top + 4, INDENT + 15, top + ih);
	BRect			r1(rbR.right + 10, top + 5, rbR.right + 10 + intW, top + ih + 5);
	BPoint			pt1(r1.right + 15, top);
	BRadioButton*	rb = new BRadioButton(rbR, "radio", NULL, NULL);
	ArpIntControl*	beatCtrl = new ArpIntControl(r1, "beatctrl", NULL, NULL);
	BMenuField*		beatValField = new_beat_value_menu( pt1, beatValues );
	if (!rb || !beatCtrl || !beatValField) {
		delete rb;
		delete beatCtrl;
		delete beatValField;
		return top;
	}
	if (on) rb->SetValue(B_CONTROL_ON);
	beatCtrl->SetLimits(1, 99);
	beatCtrl->SetValue(beats);
	toView->AddChild(rb);
	toView->AddChild(beatCtrl);
	toView->AddChild(beatValField);
	
	r1.left = r1.right + INDENT;
	r1.right = pt1.x - INDENT;
	BStringView* sv = new BStringView(r1, "slashlabel", "/");
	if (sv) toView->AddChild(sv);
	float	bottom = beatValField->Frame().bottom;
	mEntries.push_back( _SeqSigPopUpEntry(rb, beatCtrl, beatValField, sv, bottom) );
	return bottom;
}

status_t SeqSignatureWindow::GetSignature(AmSignature& signature) const
{
	if (!mBg) return B_ERROR;
	ArpIntControl*		intCtrl = dynamic_cast<ArpIntControl*>( mBg->FindView("measureint") );
	if (!intCtrl) return B_ERROR;
	signature.SetMeasure( intCtrl->Value() );

	for (uint32 k = 0; k < mEntries.size(); k++) {
		if ( mEntries[k].IsOn() ) return mEntries[k].GetSignature(signature);
	}
	return B_ERROR;
}

/**********************************************************************
 * _SEQ-SIG-POP-UP-ENTRY
 **********************************************************************/
_SeqSigPopUpEntry::_SeqSigPopUpEntry()
		: mRadio(NULL), mIntCtrl(NULL), mField(NULL), mSlash(NULL),
		  mBottom(0)
{
}

_SeqSigPopUpEntry::_SeqSigPopUpEntry(	BRadioButton* rb,
										ArpIntControl* ic,
										BMenuField* mf,
										BStringView* s,
										float bottom)
		: mRadio(rb), mIntCtrl(ic), mField(mf), mSlash(s),
		  mBottom(bottom)
{
}

_SeqSigPopUpEntry& _SeqSigPopUpEntry::operator=(const _SeqSigPopUpEntry &e)
{
	mRadio = e.mRadio;
	mIntCtrl = e.mIntCtrl;
	mField = e.mField;
	mSlash = e.mSlash;
	mBottom = e.mBottom;
	return *this;
}

void _SeqSigPopUpEntry::Remove()
{
	if ( mRadio && mRadio->RemoveSelf() ) {
		delete mRadio;
		mRadio = NULL;
	}
	if ( mIntCtrl && mIntCtrl->RemoveSelf() ) {
		delete mIntCtrl;
		mIntCtrl = NULL;
	}
	if ( mField && mField->RemoveSelf() ) {
		delete mField;
		mField = NULL;
	}
	if ( mSlash && mSlash->RemoveSelf() ) {
		delete mSlash;
		mSlash = NULL;
	}
}

bool _SeqSigPopUpEntry::IsOn() const
{
	return mRadio && mRadio->Value() == B_CONTROL_ON;
}

void _SeqSigPopUpEntry::SetEnabled(bool enable)
{
	if (mRadio) mRadio->SetEnabled(enable);
	if (mIntCtrl) mIntCtrl->SetEnabled(enable);
	if (mField) mField->SetEnabled(enable);
}

static uint32 beat_value_from_menu(BMenu* menu)
{
	if (!menu) return 0;
	BMenuItem*	item = menu->FindMarked();
	if (!item) return 0;
	if (strcmp(item->Label(), "1") == 0) return 1;
	if (strcmp(item->Label(), "2") == 0) return 2;
	if (strcmp(item->Label(), "4") == 0) return 4;
	if (strcmp(item->Label(), "8") == 0) return 8;
	if (strcmp(item->Label(), "16") == 0) return 16;
	if (strcmp(item->Label(), "32") == 0) return 32;
	return 0;
}

status_t _SeqSigPopUpEntry::GetSignature(AmSignature& signature) const
{
	if (mIntCtrl && mField) {
		uint32		beatValue = beat_value_from_menu( mField->Menu() );
		if (beatValue > 0) {
			signature.Set(0, mIntCtrl->Value(), beatValue);
			return B_OK;
		}
	}
	return B_ERROR;
}

void _SeqSigPopUpEntry::Set(uint32 beats, uint32 beatValue)
{
	if (mIntCtrl) mIntCtrl->SetValue(beats);
	if ( mField && mField->Menu() ) {
		BString		label;
		label << beatValue;
		BMenuItem*	item;
		for (int32 k = 0; (item = mField->Menu()->ItemAt(k)); k++) {
			if ( label == item->Label() ) item->SetMarked(true);
			else item->SetMarked(false);
		}
	}
}
