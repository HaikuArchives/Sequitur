/* SeqTrackWindowAux.cpp
 */
#include <cstdio>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpIntFormatterI.h"
#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpKnobControl.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmMotion.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTool.h"
#include "Sequitur/SeqTrackWindow.h"
#include "Sequitur/SeqTrackWindowAux.h"
#include "Sequitur/SequiturDefs.h"

const char*				ACTIVE_TOOL_VIEW_NAME	= "active_tool";
static const BBitmap*	gControlBg				= NULL;

/*************************************************************************
 * _TW-ORDER-TRACK-MENU
 * This is a dynamic submenu of the dynamic menu _TwChooseTrackMenu.  It
 * presents options for ordering the selected track.
 *************************************************************************/
class _TwOrderTrackMenu : public BMenu
{
public:
	_TwOrderTrackMenu(	const char* name, track_id tid, AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const BMessenger& target);

protected:
	virtual bool AddDynamicItem(add_state state);

private:
	typedef BMenu		inherited;
	track_id			mTrackId;
	AmSongRef			mSongRef;
	AmTrackWinPropertiesI& mTrackWinProps;
	BMessenger			mTarget;
	bool				mMenuBuilt;
	std::vector<BMenuItem*>	mItems;
	uint32				mCurrIndex;
	
	bool	StartBuildingItemList();
	bool	AddNextItem(const AmSong* song);
	void	DoneBuildingItemList();
	void	ClearMenuBuildingState();
	int32	IndexOf(track_id tid) const;
};

/*************************************************************************
 * _TW-CHOOSE-TRACK-MENU-ITEM
 * This little menu item just draws the order number next to the label.
 *************************************************************************/
class _TwChooseTrackMenuItem : public BMenuItem
{
public:
	_TwChooseTrackMenuItem(BMenu* menu, BMessage* message, uint32 order);

protected:
	virtual	void		Draw();

private:
	typedef BMenuItem	inherited;
	uint32				mOrder;
};


/*************************************************************************
 * _TW-ACTIVE-TOOL-VIEW
 *************************************************************************/
_TwActiveToolView::_TwActiveToolView(BPoint origin)
		: inherited(BRect(origin, origin), ACTIVE_TOOL_VIEW_NAME, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		  mMouseType(1), mMouseBitmap(0), mPrimaryIcon(0), mSecondaryIcon(0),
		  mTertiaryIcon(0), mLeftOverhang(11), mRightOverhang(11),
		  mTopOverhang(4)
{
	if (!gControlBg) gControlBg = ImageManager().FindBitmap(AM_TRACK_CONTROL_BG_STR);

	if (get_mouse_type(&mMouseType) != B_OK) mMouseType = 1;
	/* Get my mouse image based on the mouse type.
	 */	
	if (mMouseType == 1) {
		mMouseBitmap = ImageManager().FindBitmap("Mouse 1");
	} else if (mMouseType == 2) {
		mMouseBitmap = ImageManager().FindBitmap("Mouse 2");
	} else if (mMouseType == 3) {
		mMouseBitmap = ImageManager().FindBitmap("Mouse 3");
	}
}

_TwActiveToolView::~_TwActiveToolView()
{
	Uncache();
}

void _TwActiveToolView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetViewColor(B_TRANSPARENT_COLOR);
	if (Parent() ) mViewC = Parent()->ViewColor();
	AmToolRoster*	roster = AmToolRoster::Default();
	if (roster) roster->AddObserver(this);
}

void _TwActiveToolView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	AmToolRoster*	roster = AmToolRoster::Default();
	if (roster) roster->RemoveObserver(this);
	Uncache();
}

void _TwActiveToolView::Draw(BRect clip)
{
	BView* into = this;
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	DrawOn(into, clip);
	if (cache) cache->FinishDrawing(into);
}

void _TwActiveToolView::GetPreferredSize(float* width, float* height)
{
	float	w = mLeftOverhang + mRightOverhang;
	float	h = mTopOverhang;
	if (mMouseBitmap) {
		BRect	b( mMouseBitmap->Bounds() );
		w += b.Width();
		h += b.Height();
	}
	*width = w;
	*height = h;
}

void _TwActiveToolView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case AM_FILE_ROSTER_CHANGED: {
			int32		button;
			if (msg->FindInt32("button", &button) == B_OK) {
//printf("_TwActiveToolView::MessageReceived() AM_FILE_ROSTER_CHANGED\n");
				Uncache();
				Invalidate();
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void _TwActiveToolView::DrawOn(BView* view, BRect clip)
{
//printf("_TwActiveToolView::DrawOn()\n");
	view->SetHighColor(mViewC);
	view->FillRect(clip);
	if (gControlBg) arp_tile_bitmap_on(view, clip, gControlBg, Frame().LeftTop() );

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	if (mMouseBitmap) view->DrawBitmapAsync(mMouseBitmap, BPoint(mLeftOverhang, mTopOverhang) );

	/* Cache the tools, if necessary.
	 */
	uint32			mods = modifiers();
	if (!mPrimaryIcon) mPrimaryIcon = NewIcon(B_PRIMARY_MOUSE_BUTTON, mods);
	if (!mSecondaryIcon) mSecondaryIcon = NewIcon(B_SECONDARY_MOUSE_BUTTON, mods);
	if (!mTertiaryIcon) mTertiaryIcon = NewIcon(B_TERTIARY_MOUSE_BUTTON, mods);

	/* Draw the tools.
	 */
	if (mMouseType == 1) {
		if (mPrimaryIcon)	view->DrawBitmapAsync(mPrimaryIcon, BPoint(15, 2));
	} else if (mMouseType == 2) {
		if (mPrimaryIcon)	view->DrawBitmapAsync(mPrimaryIcon, BPoint(4, 2));
		if (mSecondaryIcon)	view->DrawBitmapAsync(mSecondaryIcon, BPoint(28, 2));
	} else if (mMouseType == 3) {
		if (mPrimaryIcon)	view->DrawBitmapAsync(mPrimaryIcon, BPoint(0, 2));
		if (mTertiaryIcon)	view->DrawBitmapAsync(mTertiaryIcon, BPoint(24, 2));
		if (mSecondaryIcon)	view->DrawBitmapAsync(mSecondaryIcon, BPoint(48, 2));
	}
	SetDrawingMode(mode);
}

BBitmap* _TwActiveToolView::NewIcon(uint32 button, uint32 mods)
{
	BBitmap*			ans = 0;
	AmToolRef			toolRef = AmGlobals().Tool(button, mods);
	// READ TOOL BLOCK
	const AmTool*		tool = toolRef.ReadLock();
	if (tool) {
		const BBitmap*	icon = tool->Icon();
		if (icon) ans = new BBitmap(icon);
	}
	toolRef.ReadUnlock(tool);
	// END READ TOOL BLOCK;
	return ans;
}

void _TwActiveToolView::Uncache()
{
	delete mPrimaryIcon;
	mPrimaryIcon = 0;
	delete mSecondaryIcon;
	mSecondaryIcon = 0;
	delete mTertiaryIcon;
	mTertiaryIcon = 0;
}

// #pragma mark -

/*************************************************************************
 * _TW-CHOOSE-TRACK-MENU
 *************************************************************************/
_TwChooseTrackMenu::_TwChooseTrackMenu(	const char* name, AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const BMessenger& target)
		: inherited(name, B_ITEMS_IN_COLUMN),
		  mSongRef(songRef), mTrackWinProps(trackWinProps), mTarget(target),
		  mMenuBuilt(false)
{
}

void _TwChooseTrackMenu::EmptyState()
{
	RemoveItems(0, CountItems(), true);
	mMenuBuilt = false;
}

static const int32		gItemsToAddChunk = 20;
static const bigtime_t	gMaxTimeBuildingMenu = 200000;

bool _TwChooseTrackMenu::AddDynamicItem(add_state state)
{
	if (mMenuBuilt) return false;
	if (state == B_ABORT) {
		ClearMenuBuildingState();
		return false;
	}

	if (state == B_INITIAL_ADD && !StartBuildingItemList() ) {
		ClearMenuBuildingState();
		return false;
	}

	bool		answer = true;
	bigtime_t timeToBail = system_time() + gMaxTimeBuildingMenu;
	// READ SONG BLOCK
	const AmSong*	song = mSongRef.ReadLock();
	if (!song) answer = false;
	else {
		for (int32 count = 0; count < gItemsToAddChunk; count++) {
			if ( !AddNextItem(song) ) {
				mMenuBuilt = true;
				DoneBuildingItemList();
				ClearMenuBuildingState();
				answer = false;				// done with menu, don't call again
				break;
			}
			// we have been in here long enough, come back later
			if (system_time() > timeToBail) break;
		}
	}
	mSongRef.ReadUnlock(song);		
	// END READ SONG BLOCK
	
	return answer;	// call me again, got more to show
}

bool _TwChooseTrackMenu::StartBuildingItemList()
{
	return mSongRef.IsValid();
}

bool _TwChooseTrackMenu::AddNextItem(const AmSong* song)
{
	// limit nav menus to 500 items only
	if (mItems.size() >= 500) return false;
	const AmTrack*		track = song->Track(mItems.size() );
	if (!track) return false;

	if (track->Id() == mTrackWinProps.OrderedTrackAt(0).TrackId() ) {
		BMenuItem*		item = new BMenuItem(track->Name(), new BMessage('null'));
		if (!item) return false;
		item->SetMarked(true);
		item->SetEnabled(false);
		mItems.push_back(item);
	} else {
		BMessage*		msg = new BMessage(TW_PRIMARY_TRACK_MSG);
		if (!msg) return false;
		msg->AddPointer("track_id", track->Id() );
		msg->AddString("track_name", track->Title() );
		uint32			count = mTrackWinProps.CountOrderedTracks();
		int32			index = IndexOf(track->Id() );
		BString			label;
		if (index > 0 && count > 9) label << "(" << index  + 1 << ") ";
		label << track->Name();
		BMenu*			menu = new _TwOrderTrackMenu(	label.String(), track->Id(),
														mSongRef, mTrackWinProps,
														mTarget);
		if (!menu) {
			delete msg;
			return false;
		}
		BMenuItem*		item;
		if (index > 0 && count <= 10) item = new _TwChooseTrackMenuItem(menu, msg, index);
		else item = new BMenuItem(menu, msg);
		if (!item) {
			delete msg;
			delete menu;
			return false;
		}
		mItems.push_back(item);
	}
	return true;
}

void _TwChooseTrackMenu::DoneBuildingItemList()
{
	for (uint32 k = 0; k < mItems.size(); k++) AddItem(mItems[k]);
	mItems.resize(0);
	SetTargetForItems(mTarget);
}

void _TwChooseTrackMenu::ClearMenuBuildingState()
{
	/* For whatever reason, the operation has been aborted.  Need
	 * to delete any items in the list.
	 */
	for (uint32 k = 0; k < mItems.size(); k++)
		delete mItems[k];
	mItems.resize(0);
}

int32 _TwChooseTrackMenu::IndexOf(track_id tid) const
{
	uint32		count = mTrackWinProps.CountOrderedTracks();
	for (uint32 k = 0; k < count; k++) {
		if (mTrackWinProps.OrderedTrackAt(k).TrackId() == tid) return int32(k);
	}
	return -1;
}

// #pragma mark -

/*************************************************************************
 * _TW-CREATE-MOTION-WIN
 *************************************************************************/
static const uint32		PROGRESSION_MSG	= 'iPRG';
static const uint32		RHYTHM_MSG		= 'iRht';
static const uint32		PITCH_ENV_MSG	= 'iPtE';
static const uint32		VEL_ENV_MSG		= 'iVeE';
static const uint32		CANCEL_MSG		= 'iCnc';
static const uint32		OK_MSG			= 'iOk_';
static const float		EDGE			= 8;

static void no_notes_warning()
{
	BAlert*	alert = new BAlert(	"Warning", "You must have one or more notes selected to create a motion.",
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
}

static void no_name_warning()
{
	BAlert*	alert = new BAlert(	"Warning", "You must supply a name",
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
}

static void unknown_error_warning()
{
	BAlert*	alert = new BAlert(	"Warning", "An unknown error occurred",
								"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) alert->Go();
}

static int32 duplicate_name_warning()
{
	BAlert*	alert = new BAlert(	"Warning", "A motion exists with this name",
								"Cancel", "Replace", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	if (alert) return alert->Go();
	return 0;
}

TwCreateMotionWin::TwCreateMotionWin(	AmSongRef songRef,
										BMessenger trackWin,
										uint8 initialNote)
		: inherited( BRect(100, 100, 100, 100), "Create Motion",
					B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		  mSongRef(songRef), mTrackWin(trackWin),
		  mNameCtrl(NULL), mInitialNoteCtrl(NULL), mCenterCtrl(NULL),
		  mFlags(PROGRESSION_FLAG)
{
	AddViews();
	if (mInitialNoteCtrl) mInitialNoteCtrl->SetValue(initialNote);
}

TwCreateMotionWin::~TwCreateMotionWin()
{
}

void TwCreateMotionWin::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case PROGRESSION_MSG:
			mFlags = PROGRESSION_FLAG;
			if (mInitialNoteCtrl && !mInitialNoteCtrl->IsEnabled() ) mInitialNoteCtrl->SetEnabled(true);
			if (mCenterCtrl && mCenterCtrl->IsEnabled() ) mCenterCtrl->SetEnabled(false);
			break;
		 case PITCH_ENV_MSG:
		 	mFlags = PITCH_ENVELOPE_FLAG;
			if (mInitialNoteCtrl && !mInitialNoteCtrl->IsEnabled() ) mInitialNoteCtrl->SetEnabled(true);
			if (mCenterCtrl && mCenterCtrl->IsEnabled() ) mCenterCtrl->SetEnabled(false);
		 	break;
		case RHYTHM_MSG:
			mFlags = RHYTHM_FLAG;
			if (mInitialNoteCtrl && mInitialNoteCtrl->IsEnabled() ) mInitialNoteCtrl->SetEnabled(false);
			if (mCenterCtrl && !mCenterCtrl->IsEnabled() ) mCenterCtrl->SetEnabled(true);
			break;
		case VEL_ENV_MSG:
			mFlags = VELOCITY_ENVELOPE_FLAG;
			if (mInitialNoteCtrl && mInitialNoteCtrl->IsEnabled() ) mInitialNoteCtrl->SetEnabled(false);
			if (mCenterCtrl && !mCenterCtrl->IsEnabled() ) mCenterCtrl->SetEnabled(true);
			break;
		case OK_MSG:
			if (CreateMotion() ) PostMessage(B_QUIT_REQUESTED);
			break;
		case CANCEL_MSG:
			PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			inherited::MessageReceived(msg);
	}
}

bool TwCreateMotionWin::CreateMotion()
{
	if (!mNameCtrl || !mCenterCtrl || !mInitialNoteCtrl || !mTrackWin.IsValid() ) {
		low_memory_warning();
		return false;
	}
	BString			name(mNameCtrl->Text() );
	if (name.Length() <= 0) {
		no_name_warning();
		return false;
	}
	AmMotionRoster*	roster = AmMotionRoster::Default();
	if (!roster) {
		no_name_warning();
		return false;
	}
	BString			label, key;
	bool			readOnly;
	for (uint32 k = 0; roster->GetMotionInfo(k, label, key, &readOnly) == B_OK; k++) {
		if (label == name) {
			int32	ans = duplicate_name_warning();
			if (ans == 0) return false;
			break;
		}
	}

	BHandler*		target;
	BLooper*		looper;
	if ( (target = mTrackWin.Target(&looper)) == NULL) {
		unknown_error_warning();
		return false;
	}
	SeqTrackWindow*	win = dynamic_cast<SeqTrackWindow*>(target);
	if (!win || !win->Lock() ) {
		unknown_error_warning();
		return false;
	}
	// READ SONG BLOCK
	const AmSong*	song = mSongRef.ReadLock();
	if (song) CreateMotion(*win, *song);
	mSongRef.ReadUnlock(song);
	// END READ TRACK BLOCK
	win->Unlock();
	return true;
}

static float time_percent(AmTime time, AmSignature& sig)
{
	return (double)(time - sig.StartTime() ) / (double)sig.Duration();
}

static float hit_percent(uint8 vel, int32 center)
{
	if (vel > center) {
		return (float)(vel - center) / (float)(127 - center);
	} else if (vel < center) {
		return (float)(vel - center) / (float)(center);
	} else return 0.0;
}

void TwCreateMotionWin::CreateMotion(AmTrackWinPropertiesI& props, const AmSong& song)
{
	AmMotion*				motion = new AmMotion(mNameCtrl->Text(), NULL, NULL);
	if (!motion) {
		low_memory_warning();
		return;
	}
	if (mFlags&PROGRESSION_FLAG) {
		motion->SetEditingMode(PROGRESSION_MODE);
	} else if (mFlags&RHYTHM_FLAG) {
		motion->SetEditingMode(RHYTHM_MODE);
	} else {
		motion->SetEditingMode(ENVELOPE_MODE);
	}
	
	const AmTrack*			track = song.Track(props.OrderedTrackAt(0).TrackId() );
	AmSelectionsI*			selections = props.Selections();
	if (!track || !selections) return;

	AmPhraseEvent*			topPhrase;
	AmEvent*				event;
	AmSignaturePhrase		sigs;
	int32					measure = -1;
	int32					initialMeasure = -1;
	int32					initialNote = mInitialNoteCtrl->Value();
	int32					center = mCenterCtrl->Value();
	for (uint32 k = 0; selections->EventAt(track->Id(), k, &topPhrase, &event) == B_OK; k++) {
		AmNoteOn*	noteOn = NULL;
		if (event->Type() == event->NOTEON_TYPE
				&& ((noteOn = dynamic_cast<AmNoteOn*>(event)) != NULL) ) {
			AmRange			range = topPhrase->EventRange(event);
			AmSignature		sig;
			if (track->GetSignature(range.start, sig) == B_OK) {
				if (initialMeasure < 0) initialMeasure = sig.Measure();
				if (measure != sig.Measure() ) sigs.SetSignature(sig.Measure() - initialMeasure + 1, sig.Beats(), sig.BeatValue() );
				measure = sig.Measure();
				/* Here's where I actually add the hit.
				 */
				if (mFlags&PROGRESSION_FLAG) {
					int32		shift = noteOn->Note() - initialNote;
					float		step = float(1) / float(128);
					float		y = shift * step;
					if (y > 1) y = 1;
					else if (y < -1) y = -1;
					motion->AddHit( BPoint(	time_percent(range.start, sig) + sig.Measure() - initialMeasure, y),
									time_percent(range.end, sig) + sig.Measure() - initialMeasure);
				} else if (mFlags&RHYTHM_FLAG) {
					motion->AddHit( BPoint(	time_percent(range.start, sig) + sig.Measure() - initialMeasure,
											hit_percent(noteOn->Velocity(), center) ),
									time_percent(range.end, sig) + sig.Measure() - initialMeasure);
				} else if (mFlags&PITCH_ENVELOPE_FLAG) {
					motion->AddHit( BPoint(	time_percent(range.start, sig) + sig.Measure() - initialMeasure,
											hit_percent(noteOn->Note(), initialNote) ) );
				} else if (mFlags&VELOCITY_ENVELOPE_FLAG) {
					motion->AddHit( BPoint(	time_percent(range.start, sig) + sig.Measure() - initialMeasure,
											hit_percent(noteOn->Velocity(), center) ) );
				}
			}
		}
	}

	motion->SetSignatures(sigs);
	sigs.DeleteEvents();
	if (motion->IsEmpty() ) {
		no_notes_warning();
		delete motion;
		return;
	}

	AmMotionRoster*		roster = AmMotionRoster::Default();
	if (roster) roster->CreateEntry(motion);
	delete motion;
}

void TwCreateMotionWin::AddViews()
{
	BView*			bg = new BView(Bounds(), "bg_view", B_FOLLOW_ALL, 0);
	if (!bg) return;
	bg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	AddChild(bg);
	float			x = EDGE, y = EDGE;
	const char*		progL = "Progression";
	const char*		initialNoteL = "Initial note:";
	const char*		rhythmL = "Rhythm";
	const char*		pitchEnvL = "Envelope from pitch";
	const char*		velEnvL = "Envelope from velocity";
	float			radioW = bg->StringWidth(velEnvL) + 20;
	const char*		centerLabel = "Center line:";
	float			divider = bg->StringWidth(initialNoteL) + 10;
	const char*		cancelLabel = "Cancel";
	float			buttonW = bg->StringWidth(cancelLabel) + 30, buttonH = Prefs().Size(BUTTON_Y);
	/* Calculate everyone's Y frame.
	 */
	float			intL = x + 19;
	float			bbY = Prefs().Size(BUTTON_BORDER_Y);
	BRect			nameF( x, y, 0, y + Prefs().Size(INT_CTRL_Y) );
	BRect			progF(x, nameF.bottom + bbY + 3, 0, nameF.bottom + bbY + 3 + Prefs().Size(INT_CTRL_Y) );
	BRect			pitchEnvF(x, progF.bottom + y, 0, progF.bottom + y + Prefs().Size(INT_CTRL_Y) );
	BRect			initialNoteF(intL, pitchEnvF.bottom + y, 0, pitchEnvF.bottom + y + Prefs().Size(INT_CTRL_Y) );
	BRect			rhythmF(x, initialNoteF.bottom + y, 0, initialNoteF.bottom + y + Prefs().Size(INT_CTRL_Y) );
	BRect			velEnvF(x, rhythmF.bottom + y, 0, rhythmF.bottom + y + Prefs().Size(INT_CTRL_Y) );
	BRect			intF(intL, velEnvF.bottom + y, 0, velEnvF.bottom + y + Prefs().Size(INT_CTRL_Y) );
	BRect			cancelF(Prefs().Size(BUTTON_BORDER_X), intF.bottom + bbY + y, 0, intF.bottom + bbY + y + buttonH);
	BRect			okF(cancelF);
	/* Calculate everyone's X bounds.
	 */
	nameF.right = nameF.left + divider + bg->StringWidth("New motion") + 5;
	initialNoteF.right = initialNoteF.left + divider + bg->StringWidth("127") + 10;
	progF.right = progF.left + radioW + x;
	rhythmF.right = rhythmF.left + radioW + x;
	pitchEnvF.right = pitchEnvF.left + radioW + x;
	velEnvF.right = velEnvF.left + radioW + x;
	intF.right = intF.left + divider + bg->StringWidth("127") + 10;
	cancelF.right = cancelF.left + buttonW;
	okF.left = cancelF.right + Prefs().Size(BUTTON_BORDER_X);
	okF.right = okF.left + buttonW;
	/* Center everybody.
	 */
	/* Add the controls.
	 */
	mNameCtrl = new BTextControl(nameF, "name_ctrl", "Name:", "New motion", NULL);
	if (mNameCtrl) {
		mNameCtrl->SetDivider(bg->StringWidth("Name:") + 10);
		bg->AddChild(mNameCtrl);
	}
	BRadioButton*	radio = new BRadioButton(progF, "prog_butn", progL, new BMessage(PROGRESSION_MSG));
	if (radio) {
		radio->SetValue(B_CONTROL_ON);
		bg->AddChild(radio);
	}
	radio = new BRadioButton(rhythmF, "rhythm_butn", rhythmL, new BMessage(RHYTHM_MSG));
	if (radio) bg->AddChild(radio);
	radio = new BRadioButton(pitchEnvF, "pitch_env_butn", pitchEnvL, new BMessage(PITCH_ENV_MSG));
	if (radio) bg->AddChild(radio);
	radio = new BRadioButton(velEnvF, "vel_env_butn", velEnvL, new BMessage(VEL_ENV_MSG));
	if (radio) bg->AddChild(radio);
	
	mInitialNoteCtrl = new AmKeyControl(initialNoteF, "initial_note_ctrl", initialNoteL, NULL);
	if (mInitialNoteCtrl) {
		mInitialNoteCtrl->SetDivider(divider);
		bg->AddChild(mInitialNoteCtrl);
	}

	mCenterCtrl = new ArpIntControl(intF, "center_ctrl", centerLabel, NULL);
	if (mCenterCtrl) {
		mCenterCtrl->SetLimits(0, 127);
		mCenterCtrl->SetValue(64);
		mCenterCtrl->SetDivider(divider);
		mCenterCtrl->SetEnabled(false);
		bg->AddChild(mCenterCtrl);
	}

	BButton*	button = new BButton(cancelF, "cancel_button", cancelLabel, new BMessage(CANCEL_MSG) );
	if (button) bg->AddChild(button);

	button = new BButton(okF, "ok_button", "OK", new BMessage(OK_MSG) );
	if (button) {
		bg->AddChild(button);
		button->MakeDefault(true);
	}


	float	w = 0, h = 0;
	for(BView* view = bg->ChildAt(0); view; view = view->NextSibling() ) {
		BRect	f = view->Frame();
		if (f.right > w) w = f.right;
		if (f.bottom > h) h = f.bottom;
	}
	ResizeTo(w + EDGE, h + EDGE);

	if (mNameCtrl) mNameCtrl->ResizeTo(w - EDGE, mNameCtrl->Frame().Height() );
}

// #pragma mark -

/*************************************************************************
 * _TW-ORDER-TRACK-MENU
 *************************************************************************/
_TwOrderTrackMenu::_TwOrderTrackMenu(	const char* name, track_id tid,
										AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const BMessenger& target)
		: inherited(name, B_ITEMS_IN_COLUMN),
		  mTrackId(tid), mSongRef(songRef), mTrackWinProps(trackWinProps),
		  mTarget(target), mMenuBuilt(false), mCurrIndex(1)
{
}

bool _TwOrderTrackMenu::AddDynamicItem(add_state state)
{
	if (mMenuBuilt) return false;
	
	if (state == B_ABORT) {
		ClearMenuBuildingState();
		return false;
	}

	if (state == B_INITIAL_ADD && !StartBuildingItemList() ) {
		ClearMenuBuildingState();
		return false;
	}

	bool		answer = true;
	bigtime_t timeToBail = system_time() + gMaxTimeBuildingMenu;
	// READ SONG BLOCK
	const AmSong*	song = mSongRef.ReadLock();
	if (!song) answer = false;
	else {
		for (int32 count = 0; count < gItemsToAddChunk; count++) {
			if ( !AddNextItem(song) ) {
				mMenuBuilt = true;
				DoneBuildingItemList();
				ClearMenuBuildingState();
				answer = false;				// done with menu, don't call again
				break;
			}
			// we have been in here long enough, come back later
			if (system_time() > timeToBail) break;
		}
	}
	mSongRef.ReadUnlock(song);		
	// END READ SONG BLOCK
	
	return answer;	// call me again, got more to show
}

bool _TwOrderTrackMenu::StartBuildingItemList()
{
	return mSongRef.IsValid();
}

bool _TwOrderTrackMenu::AddNextItem(const AmSong* song)
{
	BMessage*			msg = new BMessage(TW_SET_ORDERED_TRACK_MSG);
	if (!msg) return false;
	msg->AddPointer("track_id", mTrackId);
	msg->AddInt32("order", mCurrIndex);
	BString				str;
	str << mCurrIndex + 1;
	BMenuItem*			item = new BMenuItem(str.String(), msg);
	if (!item) {
		delete msg;
		return false;
	}
	if (mTrackId == mTrackWinProps.OrderedTrackAt(mCurrIndex).TrackId() ) {
		item->SetMarked(true);
		item->SetEnabled(false);
	}
	mItems.push_back(item);
	mCurrIndex++;
	
	uint32		count = mTrackWinProps.CountOrderedTracks();
	if (mCurrIndex >= count + 1) return false;
	return true;
}

void _TwOrderTrackMenu::DoneBuildingItemList()
{
	bool		needSep = (mItems.size() > 0);
	for (uint32 k = 0; k < mItems.size(); k++) AddItem(mItems[k]);
	mItems.resize(0);
	/* Add an item to clear out my track_id's position.
	 */
	BMessage*			msg = new BMessage(TW_CLEAR_ORDERED_TRACK_MSG);
	if (msg) {
		BMenuItem*		item = new BMenuItem("Clear", msg);
		if (item) {
			int32		index = IndexOf(mTrackId);
			if (index >= 0) msg->AddPointer("track_id", mTrackId);
			else item->SetEnabled(false);
			if (needSep) AddSeparatorItem();
			AddItem(item);
		}
	}
	SetTargetForItems(mTarget);
}

void _TwOrderTrackMenu::ClearMenuBuildingState()
{
	/* For whatever reason, the operation has been aborted.  Need
	 * to delete any items in the list.
	 */
	for (uint32 k = 0; k < mItems.size(); k++)
		delete mItems[k];
	mItems.resize(0);
}

int32 _TwOrderTrackMenu::IndexOf(track_id tid) const
{
	uint32		count = mTrackWinProps.CountOrderedTracks();
	for (uint32 k = 0; k < count; k++) {
		if (mTrackWinProps.OrderedTrackAt(k).TrackId() == tid) return int32(k);
	}
	return -1;
}

// #pragma mark -

/*************************************************************************
 * _TW-CHOOSE-TRACK-MENU-ITEM
 *************************************************************************/
_TwChooseTrackMenuItem::_TwChooseTrackMenuItem(	BMenu* menu,
												BMessage* message,
												uint32 order)
		: inherited(menu, message),
		  mOrder(order)
{
}

void _TwChooseTrackMenuItem::Draw()
{
	inherited::Draw();
	if (!Menu() ) return;
	font_height		fh;
	Menu()->GetFontHeight(&fh);
	BString			s;
	s << mOrder + 1;
	BRect			b(Frame() );
	Menu()->DrawString(s.String(),  BPoint(b.left + 2, b.bottom - fh.descent) );
}

// #pragma mark -

/*************************************************************************
 * _TW-SATURATION-WIN
 *************************************************************************/
static const char*	ORDERED_TRACKS_STR		= "Ordered tracks:";
static const char*	SHADOW_TRACKS_STR		= "Shadow tracks:";

_TwSaturationWin::_TwSaturationWin(	BMessenger target,
									float orderedSat,
									float shadowSat)
		: inherited( BRect(100, 100, 100, 100), "Saturation",
					B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	AddViews(target, orderedSat, shadowSat);
}

class _AtwSaturationFormatter : public ArpIntFormatterI
{
public:
	_AtwSaturationFormatter()		{ }

	virtual void FormatInt(int32 number, BString& out) const
	{
		if( number <= 0 ) out << "Saturation: Off";
		else out << "Saturation: " << number << '%';
	}
};


void _TwSaturationWin::AddViews(BMessenger target, float orderedSat, float shadowSat)
{
	BView*			bg = new BView(BRect(0, 0, 0, 0), "bg_view", B_FOLLOW_NONE, 0);
	if (!bg) return;
	bg->SetViewColor( Prefs().Color(AM_AUX_WINDOW_BG_C) );
	AddChild(bg);
	float			x = EDGE, y = EDGE;
	float			r = 0, b = 0;
	float			labelW = bg->StringWidth(ORDERED_TRACKS_STR) + 5;
	
	ArpKnobPanel*	panel = new ArpKnobPanel(	"ordered_panel", ORDERED_TRACKS_STR,
												new BMessage(TW_ORDERED_SAT_MSG), 0, 100, true,
												B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW);
	if (panel) {
		ArpKnobControl*	knob = panel->KnobControl();
		if (knob) {
			knob->SetTarget(target);
			knob->SetTipFormatter( new _AtwSaturationFormatter() );
			knob->SetValue(orderedSat * 100);
			knob->SetModificationMessage( new BMessage(TW_ORDERED_SAT_MSG) );
		}
		panel->MoveTo(x, y);
		bg->AddChild(panel);
		r = panel->Frame().right;
		b = panel->Frame().bottom;
	}
	panel = new ArpKnobPanel(	"saturation_panel", SHADOW_TRACKS_STR,
								new BMessage(TW_SHADOW_SAT_MSG), 0, 100, true,
								B_HORIZONTAL, ARP_TIGHT_RING_ADORNMENT, labelW);
	if (panel) {
		ArpKnobControl*	knob = panel->KnobControl();
		if (knob) {
			knob->SetTarget(target);
			knob->SetTipFormatter( new _AtwSaturationFormatter() );
			knob->SetValue(shadowSat * 100);
			knob->SetModificationMessage( new BMessage(TW_SHADOW_SAT_MSG) );
		}
		panel->MoveTo(x, b + 5);
		bg->AddChild(panel);
		r = panel->Frame().right;
		b = panel->Frame().bottom;
	}

	bg->ResizeTo(r + EDGE, b + EDGE);
	ResizeTo(r + EDGE, b + EDGE);
}
