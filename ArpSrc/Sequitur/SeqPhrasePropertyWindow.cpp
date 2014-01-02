/* SeqPhrasePropertyWindow.cpp
 */
#include <InterfaceKit.h>
#include <support/String.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmEventControls.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPhraseEvent.h"
#include "Sequitur/SequiturDefs.h"
#include "Sequitur/SeqApplication.h"
#include "Sequitur/SeqPhrasePropertyWindow.h"

static const uint32		OK_MSG				= '#ok_';
static const uint32		CANCEL_MSG			= '#cnc';
static const uint32		NAME_MSG			= '#nam';
static const uint32		BACKGROUND_C_MSG	= 'pcbg';
static const uint32		FOREGROUND_C_MSG	= 'pcfg';
static const uint32		COLOR_CHANGED_MSG	= 'pcch';

static const char*		BACKGROUND_STR		= "Background";
static const char*		FOREGROUND_STR		= "Events";

static AmPhrase::ColorCode code_for_what(uint32 what)
{
	ArpASSERT(what == BACKGROUND_C_MSG || what == FOREGROUND_C_MSG);
	if (what == BACKGROUND_C_MSG) return AmPhrase::BACKGROUND_C;
	else if (what == FOREGROUND_C_MSG) return AmPhrase::FOREGROUND_C;
	return AmPhrase::BACKGROUND_C;
}

/*************************************************************************
 * SEQ-PHRASE-PROPERTY-WINDOW
 *************************************************************************/
SeqPhrasePropertyWindow::SeqPhrasePropertyWindow(	AmSongRef songRef,
													AmPhraseEvent* event,
													const BMessage* config)
		: inherited(BRect(0, 0, 0, 0),
					"Phrase Properties",
					B_TITLED_WINDOW_LOOK,
					B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		  mSongRef(songRef), mEvent(NULL),
		  mBg(NULL), mNameCtrl(NULL), mTimeLabel(NULL), mTimeCtrl(NULL),
		  mColorField(NULL), mColorCtrl(NULL)
{
	AddViews(BRect(0, 0, 50, 50));
	float	w = 0, h = 0;
	/* Set my dimensions.
	 */
	if (mBg) {
		for (BView* view = mBg->ChildAt(0); view; view = view->NextSibling() ) {
			BRect	f = view->Frame();
			if (f.right > w) w = f.right;
			if (f.bottom > h) h = f.bottom;
		}
		ResizeTo(w + Prefs().Size(BORDER_X), h + Prefs().Size(BORDER_Y));
	}
	/* Set my location.
	 */
	BScreen	s(this);
	if (s.IsValid() ) {
		BRect	sf = s.Frame();
		float	newX = 10, newY = 10;
		if (w < sf.Width() ) newX = (sf.Width() - w) / 2;
		if (h < sf.Height() ) newY = (sf.Height() - h) / 2;
		MoveTo(newX, newY);
	}

	if (config) SetConfiguration(config);
	if (event) SetPhraseEvent(event);
}

SeqPhrasePropertyWindow::~SeqPhrasePropertyWindow()
{
	mSongRef.RemoveObserverAll(this);
	if (mEvent) mEvent->DecRefs();
	if (mTimeCtrl) mTimeCtrl->StopWatchingAll(this);
}

void SeqPhrasePropertyWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case NAME_MSG: {
			if (mSongRef.IsValid() && mEvent && mNameCtrl) {
				// WRITE SONG BLOCK
				AmSong*		song = mSongRef.WriteLock();
				if (song) {
					AmPhrase*	phrase = mEvent->Phrase();
					if (phrase) {
						BString		str(mNameCtrl->Text() );
						if (str != phrase->Name() ) phrase->SetName(str);
					}
				}
				mSongRef.WriteUnlock(song);
				// END WRITE SONG BLOCK
			}
		} break;
		case BACKGROUND_C_MSG:
		case FOREGROUND_C_MSG:
			GetColor();
			break;
		case COLOR_CHANGED_MSG:
			SetColor();
			break;
		case AmNotifier::RANGE_OBS:
			RangeChanged();
			break;
		case B_OBSERVER_NOTICE_CHANGE: {
			int32		change = 0;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			if (change == ARPMSG_TIME_VIEW_CHANGED && mSongRef.IsValid()
					&& mEvent && mTimeCtrl) {
				// WRITE SONG BLOCK
				AmSong*		song = mSongRef.WriteLock();
				if (song) {
					AmTime	oldTime = mEvent->StartTime();
					AmTime	newTime = mTimeCtrl->DisplayTime();
					if (newTime != oldTime) mEvent->SetStartTime(newTime);
				}
				mSongRef.WriteUnlock(song);
				// END WRITE SONG BLOCK
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool SeqPhrasePropertyWindow::QuitRequested()
{
	if (!inherited::QuitRequested() ) return false;
	BMessage	config;
	if (GetConfiguration(&config) == B_OK) {
		if (seq_is_quitting()) seq_app->AddShutdownMessage("window_settings", &config);
//		else seq_app->SetEditDeviceSettings(config);
	}
	return true;
}

void SeqPhrasePropertyWindow::SetPhraseEvent(AmPhraseEvent* event)
{
	mSongRef.RemoveObserverAll(this);

	if (mEvent) mEvent->DecRefs();
	mEvent = event;
	if (mEvent) mEvent->IncRefs();
	
	if (mEvent)
		mSongRef.AddRangeObserverAll(	this,
										mEvent->TimeRange() );
	Refresh();
}

void SeqPhrasePropertyWindow::Refresh()
{
	if (!Lock()) return;
	bool		setTo = false;

	/* Have to do this outside of the lock because SetTime()
	 * gets its own lock.
	 */
	AmTime		time = -1;
	if (mEvent && !mEvent->IsDeleted() ) {
		// READ SONG BLOCK
		const AmSong*		song = mSongRef.ReadLock();
		if (song) {
			const AmPhrase*	phrase = mEvent->Phrase();
			if (phrase) {
				if (mNameCtrl) {
					if (phrase->Name() != mNameCtrl->Text() )
						mNameCtrl->SetText( phrase->Name().String() );
				}
				time = mEvent->TimeRange().start;
				setTo = true;
			}
		}
		mSongRef.ReadUnlock(song);
		// END READ SONG BLOCK
	}
	if (time >= 0 && mTimeCtrl && time != mTimeCtrl->DisplayTime() )
		mTimeCtrl->SetTime(time);
	SetControls(setTo);

	GetColor();
	Unlock();
}

void SeqPhrasePropertyWindow::RangeChanged()
{
	if (mEvent) {
		if (mEvent->IsDeleted() || !mEvent->Parent() ) {
			SetPhraseEvent(NULL);
			return;
		}
		SetPhraseEvent(mEvent);
	}
}

bool SeqPhrasePropertyWindow::IsSignificant() const
{
	return false;
}

status_t SeqPhrasePropertyWindow::GetConfiguration(BMessage* config)
{
	ArpASSERT( config );
	config->what = PHRASE_PROPERTY_WINDOW_SETTING_MSG;
	status_t	err = GetDimensions(config, this);
	if (err != B_OK) return err;
	return B_OK;
}

status_t SeqPhrasePropertyWindow::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	status_t	err = SetDimensions(config, this);
	if (err != B_OK) return err;
	return B_OK;
}

void SeqPhrasePropertyWindow::GetColor()
{
	if (!mSongRef.IsValid() || !mEvent || !mColorField || !mColorCtrl) return;
	BMenuItem*	item = mColorField->Menu()->FindMarked();
	if (!item || !item->Message() ) return;
	AmPhrase::ColorCode		colorCode = code_for_what(item->Message()->what);

	// READ SONG BLOCK
	const AmSong*		song = mSongRef.ReadLock();
	if (song) {
		AmPhrase*	phrase = mEvent->Phrase();
		if (phrase) {
			mColorCtrl->SetValue( phrase->Color(colorCode) );
		}
	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
}

void SeqPhrasePropertyWindow::SetColor()
{
	if (!mSongRef.IsValid() || !mEvent || !mColorField || !mColorCtrl) return;
	BMenuItem*	item = mColorField->Menu()->FindMarked();
	if (!item || !item->Message() ) return;
	AmPhrase::ColorCode		colorCode = code_for_what(item->Message()->what);

	// WRITE SONG BLOCK
	AmSong*		song = mSongRef.WriteLock();
	if (song) {
		AmPhrase*	phrase = mEvent->Phrase();
		if (phrase) {
			phrase->SetColor(colorCode, mColorCtrl->ValueAsColor() );
		}
	}
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void SeqPhrasePropertyWindow::SetControls(bool enabled)
{
	if (mNameCtrl) mNameCtrl->SetEnabled(enabled);
	if (mTimeCtrl) mTimeCtrl->SetEnabled(enabled);
	if (mColorField) mColorField->SetEnabled(enabled);
	if (mColorCtrl) mColorCtrl->SetEnabled(enabled);
}

static BMenuField* new_color_field(BRect f)
{
	BMenu*		menu = new BMenu("colors_menu");
	if (!menu) return NULL;
	BMenuItem*	item = new BMenuItem(BACKGROUND_STR, new BMessage(BACKGROUND_C_MSG));
	if (item) menu->AddItem(item);
	item = new BMenuItem(FOREGROUND_STR, new BMessage(FOREGROUND_C_MSG));
	if (item) menu->AddItem(item);
	
	item = menu->ItemAt(0);
	if (item) item->SetMarked(true);

	menu->SetLabelFromMarked(true);
	menu->SetRadioMode(true);	
	BMenuField*	field = new BMenuField(f, "colors_field", "Color:", menu, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (!field) {
		delete menu;
		return NULL;
	}
	return field;
}

void SeqPhrasePropertyWindow::AddViews(BRect frame)
{
	mBg = new BView(frame, "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	if (!mBg) return;
	mBg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	AddChild(mBg);
	
	float		fh = arp_get_font_height(mBg);
	float		borderX = Prefs().Size(BORDER_X), borderY = Prefs().Size(BORDER_Y);
	float		spaceX = Prefs().Size(SPACE_X), spaceY = Prefs().Size(SPACE_Y);
	float		divider = mBg->StringWidth("Color:") + 10;
	/* The Name field.
	 */
	BRect		f(borderX, borderY, frame.Width() - borderX, borderY + fh);
	mNameCtrl = new BTextControl(f, "name_ctrl", "Name:", NULL, new BMessage(NAME_MSG), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	if (mNameCtrl) {
		f.top = mNameCtrl->Frame().bottom;
		mNameCtrl->SetDivider(divider);
		mNameCtrl->MakeFocus(true);
		mNameCtrl->SetModificationMessage(new BMessage(NAME_MSG) );
		mBg->AddChild(mNameCtrl);
	}
	/* The Time field.
	 */
	f.top += spaceY;
	const char*		timeStr = "Time:";
	f.right = divider;
	f.bottom = f.top + fh;
	mTimeLabel = new BStringView(f, "time_label", timeStr, B_FOLLOW_LEFT | B_FOLLOW_TOP);
	if (mTimeLabel) mBg->AddChild(mTimeLabel);
	mTimeCtrl = new AmTimeView(mSongRef);
	if (mTimeCtrl) {
		mTimeCtrl->MoveTo(f.right + 5, f.top);
		mTimeCtrl->SetEnabled(false);
		mBg->AddChild(mTimeCtrl);
		mTimeCtrl->StartWatching(this, ARPMSG_TIME_VIEW_CHANGED);
		f.top = mTimeCtrl->Frame().bottom;
	}
	/* The color controls.
	 */
	f.top += spaceY;
	f.bottom = f.top + fh;
	f.right = frame.Width() - spaceX;
	mColorField = new_color_field(f);
	if (mColorField) {
		mColorField->SetDivider(divider);
		mBg->AddChild(mColorField);
		f.top = f.bottom;
	}

	mColorCtrl = new BColorControl(	BPoint(spaceX, f.top + spaceY + spaceY),
									B_CELLS_32x8, 2, "color_control",
									new BMessage(COLOR_CHANGED_MSG) );

	if (mColorCtrl) {
		mBg->AddChild(mColorCtrl);
	}

}
