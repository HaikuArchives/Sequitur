/* AmHybridView.cpp
 */
#include <stdio.h>
#include <math.h>
#include <be/experimental/ColorTools.h>
#include <be/interface/Bitmap.h>
#include <be/interface/MenuField.h>
#include <be/interface/MenuItem.h>
#include <be/interface/PopUpMenu.h>
#include <be/interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpLineArrayCache.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewFactory.h"

#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

#include "AmStdFactory/AmHybridView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const BBitmap*	treble_clef				= NULL;
static const BBitmap*	bass_clef				= NULL;

/* Here is where I determine how many notes are above, below, and
 * between the treble and bass clef.  These do not include the notes
 * ON each of the bordering lines.
 */
static const uint32		NOTES_ABOVE_CLEFS		= 6;
static const uint32		NOTES_BELOW_CLEFS		= 6;
static const uint32		NOTES_IN_CLEFS			= 9;

static const uint32		OCTAVE_CHANGE_MSG		= 'iOcC';
						// STR_OCTAVE
#define STR_OCTAVE							"octave"
static const uint32		NOTES_BETWEEN_CLEFS_MSG	= 'iNbC';
static const char*		NOTES_BETWEEN_CLEFS_STR	= "notes_between_clefs";

static const float		INITIAL_NOTE_HEIGHT		= 7;
static const int32		INITIAL_OCTAVE			= 5;
static const int32		INITIAL_NOTES_BETWEEN_CLEFS = 3;
static const char*		INFO_OCTAVE_MENU_STR	= "octmenu";

// The pixel width of the sharp insignia, including the amount of blank
// space that should appear between it and the left of the note.
#define SS_SHARP_WIDTH			(12)
/* These are shared with the piano roll view, so users can switch back
 * and forth between these views and be in the same place.
 */
static const char*		TOP_NOTE_STR		= "top_note";
static const char*		NOTE_Y_STR			= "note_y";

static uchar			note_from_pixel(float y, _AmHybridSharedData& shared);
static uchar			sharp_note_from_pixel(float y, _AmHybridSharedData& shared);
static float			pixel_from_note(uchar note, _AmHybridSharedData& shared);
static int32			octave_from_top_note(int32 i);
static void				draw_no_mans_land(BView* view, BRect clip, _AmHybridSharedData& shared);

/*************************************************************************
 * _AM-HYBRID-TARGET
 * The hybrid note view implementation of AmToolTarget
 *************************************************************************/
class _AmHybridTarget : public AmToolTarget
{
public:
	_AmHybridTarget(AmTrackWinPropertiesI& trackWinProps,
					BView* view, float leftFudge, float rightFudge,
					_AmHybridSharedData& shared);

	virtual bool		IsInteresting(const AmEvent* event) const;
	virtual bool		IsInteresting(const BMessage* flatEvent) const;
	virtual BRect		RectFor(const AmEvent* event, AmRange eventRange) const;
	virtual AmEvent*	InterestingEventAt(	const AmTrack* track,
											const AmPhraseEvent& topPhrase,
											const AmPhrase& phrase,
											AmTime time,
											float y,
											int32* extraData) const;
	virtual bool EventIntersects(	const AmEvent* event, AmRange eventRange,
									AmTime left, int32 top,
									AmTime right, int32 bottom) const;

	virtual AmEvent* NewEvent(const AmTrack& track, AmTime time, float y);
	// Moving
	virtual int32 MoveYValueFromPixel(float y) const;
	virtual void GetMoveValues(	const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmTime* x, int32* y) const;
	virtual void GetMoveDelta(	BPoint origin, BPoint where,
								AmTime* xDelta, int32* yDelta) const;
	virtual void SetMove(		AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags);
	// Transforming
	virtual void		GetOriginalTransform(	AmEvent* event,
												am_trans_params& params) const;
	virtual void		GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const;
	virtual uint32		SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params);
	virtual uint32		SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										BPoint where,
										const am_trans_params& params);
	// Performing
	virtual void Perform(const AmSong* song, const AmSelectionsI* selections); 

private:
	AmPerformer				mPerformer;
	_AmHybridSharedData&	mShared;
};

static float zoom_y_to_note_height(float zoomY)
{
	float	ans = zoomY * INITIAL_NOTE_HEIGHT;
	if (ans <= 4) return 4;
	else return floor(ans);
}

/*************************************************************************
 * AM-HYBRID-DATA-VIEW
 *************************************************************************/
AmHybridDataView::AmHybridDataView(	BRect frame,
									AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI& viewProp,
									TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, "HybridData", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		  mShared(INITIAL_NOTE_HEIGHT, INITIAL_OCTAVE, INITIAL_NOTES_BETWEEN_CLEFS),
		  mBg(NULL), mMeasureBg(NULL)
{
	const BMessage*	config = viewProp.Configuration();
	if (config) {
		if (config->FindFloat(NOTE_Y_STR, &(mShared.mNoteHeight) ) != B_OK) mShared.mNoteHeight = INITIAL_NOTE_HEIGHT;
		int32		i;
		 if (config->FindInt32(TOP_NOTE_STR, &i) == B_OK) {
			mShared.mOctave = octave_from_top_note(i);
		 }
		if (config->FindInt32(NOTES_BETWEEN_CLEFS_STR, &i) == B_OK) {
			mShared.mNotesBetweenClefs = i;
			if (i == 17) mShared.mOctave--;
		}
	}

	mTarget = new _AmHybridTarget(trackWinProps, this, 12, 12, mShared);

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mBg = new AmHybridBackground(mShared);
	if (mBg) AddBackground(mBg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mTrackWinProps.OrderedTrackAt(0), mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
}

AmHybridDataView::~AmHybridDataView()
{
	delete mTarget;
}

void AmHybridDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmHybridDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mSongRef.RemoveObserverAll( this );
}

void AmHybridDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	AddAsObserver();
}

void AmHybridDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = (mShared.NotesOnScreen() * mShared.mNoteHeight) + mShared.mNoteHeight / 2;
}

void AmHybridDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo(where);
	AddAsObserver();
}

void AmHybridDataView::MessageReceived(BMessage* msg)
{
	if (mTarget && mTarget->HandleMessage(msg)) return;

	switch (msg->what) {
		case AM_ORDERED_TRACK_MSG: {
			if (mMeasureBg) mMeasureBg->SetTrackRef(mTrackWinProps.OrderedTrackAt(0));
			Invalidate();
		} break;
		case AM_SATURATION_MSG:
			Invalidate();
			break;
		case OCTAVE_CHANGE_MSG: {
			int32	oct;
			if (msg->FindInt32(STR_OCTAVE, &oct) == B_OK) SetOctave(oct);
		} break;
		case 'zoom': {
			float	valueY;
			if (msg->FindFloat( "y_value", &valueY ) == B_OK)
				SetNoteHeight( zoom_y_to_note_height(valueY) );
		} break;
		case NOTES_BETWEEN_CLEFS_MSG: {
			int32	i;
			if (msg->FindInt32(NOTES_BETWEEN_CLEFS_STR, &i) == B_OK)
				SetNotesBetweenClefs(i);
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

BMessage* AmHybridDataView::ConfigurationData()
{
	BMessage*	msg = new BMessage('null');
	if (!msg) return NULL;
	msg->AddInt32(TOP_NOTE_STR, note_from_pixel(Bounds().top, mShared) );
	msg->AddFloat(NOTE_Y_STR, mShared.mNoteHeight);
	msg->AddInt32(NOTES_BETWEEN_CLEFS_STR, mShared.mNotesBetweenClefs);
	return msg;
}

void AmHybridDataView::SetNoteHeight(float noteHeight)
{
	mShared.mNoteHeight = noteHeight;
	Invalidate();
	BScrollBar*		sb = ScrollBar(B_VERTICAL);
	if (sb) arp_setup_vertical_scroll_bar(sb, this);
}

void AmHybridDataView::SetOctave(int32 octave)
{
	mShared.mOctave = octave;
	Invalidate();
}

void AmHybridDataView::SetNotesBetweenClefs(int32 notesBetweenClefs)
{
	if (mShared.mNotesBetweenClefs == notesBetweenClefs) return;
	int32		octaveChange = 0;
	if (mShared.mNotesBetweenClefs == 3 && notesBetweenClefs == 17) octaveChange = -1;
	if (mShared.mNotesBetweenClefs == 17 && notesBetweenClefs == 3) octaveChange = 1;
	mShared.mNotesBetweenClefs = notesBetweenClefs;
	mShared.mOctave += octaveChange;
	Invalidate();
	BScrollBar*		sb = ScrollBar(B_VERTICAL);
	if (sb) arp_setup_vertical_scroll_bar(sb, this);
}

void AmHybridDataView::Configure(BMessage *msg)
{
	int32	tmp;
	if (msg->FindInt32(STR_OCTAVE, &tmp) == B_NO_ERROR) {
		SetOctave(tmp);
	}
}

void AmHybridDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range( mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mSongRef.AddRangeObserver( this, AmNotifier::NOTE_OBS, range );
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

void AmHybridDataView::DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->NOTEON_TYPE) return;
	const AmNoteOn*		no = dynamic_cast<const AmNoteOn*>(event);
	if (!no) return;
	rgb_color	c;
	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		c = EventColor(no->Velocity() );
	else if (properties&ARPEVENT_ORDERED) {
		c = EventColor(no->Velocity() );
		c.alpha = (uint8)(mOrderedSaturation*255);
	} else if (properties&ARPEVENT_SHADOW) {
		c = AmPrefs().ShadowColor( no->Velocity() );
		c.alpha = (uint8)(mShadowSaturation*255);
	} else
		c = AmPrefs().SelectedColor( no->Velocity() );

	// Draw the note
	BRect		rect( -1,-1,-1,-1 );
	rect = mTarget->RectFor(event, eventRange);
	if( (rect.left == -1)
			&& (rect.right == -1)
			&& (rect.top == -1)
			&& (rect.bottom == -1) )
		return;

	if (no->IsSharp() == false) {
		view->SetHighColor(c);
		view->FillRect(rect);
	} else {
		rect.left = rect.left + SS_SHARP_WIDTH;
		view->SetHighColor(c);	
		view->FillRect(rect);

		// Draw a sharp...
		ArpLineArrayCache		lines(view);
		float					h = rect.Height();
		float					t, b;
		// Just hardcode the smaller cases, they look better this way
		if (h + 1 == 5) { t = 1; b = 3; }
		else if (h + 1 == 6) { t = 1; b = 4; }
		else { t = (h/3); b = ((h/3) * 2); }
		
		lines.BeginLineArray(6);
		// Draw the vertical bars
		lines.AddLine(BPoint(rect.left - 4, rect.top), BPoint(rect.left - 6, rect.bottom), view->HighColor());
		lines.AddLine(BPoint(rect.left - 3, rect.top), BPoint(rect.left - 5, rect.bottom), view->HighColor());
		lines.AddLine(BPoint(rect.left - 8, rect.top), BPoint(rect.left - 10, rect.bottom), view->HighColor());
		lines.AddLine(BPoint(rect.left - 9, rect.top), BPoint(rect.left - 11, rect.bottom), view->HighColor());
		// Draw the horizontal bars
		lines.AddLine(BPoint(rect.left - 11, rect.top + t), BPoint(rect.left - 2, rect.top + t), view->HighColor());
		lines.AddLine(BPoint(rect.left - 12, rect.top + b), BPoint(rect.left - 3, rect.top + b), view->HighColor());
		lines.EndLineArray();
	}

	/* Lighten the top left edges
	 */	
	if (properties&ARPEVENT_SHADOW && mShadowSaturation < 1) {
		rgb_color	c2 = tint_color(c, B_LIGHTEN_2_TINT);
		c2.alpha = (uint8)(mShadowSaturation*255);
		view->SetHighColor(c2);
	} else if (properties&ARPEVENT_ORDERED && mOrderedSaturation < 1) {
		rgb_color	c2 = tint_color(c, B_LIGHTEN_2_TINT);
		c2.alpha = (uint8)(mOrderedSaturation*255);
		view->SetHighColor(c2);
	} else view->SetHighColor( tint_color(c, B_LIGHTEN_2_TINT) );
	view->StrokeLine( BPoint(rect.right, rect.top), BPoint(rect.left, rect.top) );
	view->StrokeLine( BPoint(rect.left, rect.top + 1), BPoint(rect.left, rect.bottom) );
	/* Darken the bottom right edges
	 */
	if (properties&ARPEVENT_SHADOW && mShadowSaturation < 1) {
		rgb_color	c2 = tint_color(c, B_DARKEN_2_TINT);
		c2.alpha = (uint8)(mShadowSaturation*255);
		view->SetHighColor( c2 );
	} else if (properties&ARPEVENT_ORDERED && mOrderedSaturation < 1) {
		rgb_color	c2 = tint_color(c, B_DARKEN_2_TINT);
		c2.alpha = (uint8)(mOrderedSaturation*255);
		view->SetHighColor( c2 );
	} else view->SetHighColor( tint_color(c, B_DARKEN_2_TINT) );
	view->StrokeLine( BPoint(rect.right, rect.top + 1), BPoint(rect.right, rect.bottom) );
	view->StrokeLine( BPoint(rect.right - 1, rect.bottom), BPoint(rect.left + 1, rect.bottom) );
}

void AmHybridDataView::PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	draw_no_mans_land(view, clip, mShared);
}

// #pragma mark -

/*************************************************************************
 * AM-HYBRID-INFO-VIEW
 *************************************************************************/
AmHybridInfoView::AmHybridInfoView(	BRect frame,
									AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI* property,
									TrackViewType viewType)
		: inherited(frame, "Clef", songRef, trackWinProps, viewType),
		  mShared(INITIAL_NOTE_HEIGHT, INITIAL_OCTAVE, INITIAL_NOTES_BETWEEN_CLEFS),
		  mBg(NULL)
{
	if ( property->Configuration() ) Configure( property->Configuration() );

	if (!treble_clef) treble_clef = ImageManager().FindBitmap(TREBLE_CLEF_IMAGE_STR);
	if (!bass_clef)   bass_clef   = ImageManager().FindBitmap(BASS_CLEF_IMAGE_STR);
	mFactorySignature = property->Signature();
	mViewName = property->Name();

	mBg = new AmHybridBackground(mShared);
	if (mBg) AddBackground(mBg);
}

void AmHybridInfoView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	mViewColor = Prefs().Color(AM_DATA_BG_C);
	AddOctaveMenu();
}

void AmHybridInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = (mShared.NotesOnScreen() * mShared.mNoteHeight) + mShared.mNoteHeight / 2;
}

void AmHybridInfoView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OCTAVE_CHANGE_MSG: {
			int32	oct;
			if (msg->FindInt32(STR_OCTAVE, &oct) == B_OK) {
				mTrackWinProps.PostMessageToDataView(*msg, this);
				SetOctave(oct);
			}
		} break;
		case 'zoom': {
			float	valueY;
			if (msg->FindFloat( "y_value", &valueY ) == B_OK) {
				mTrackWinProps.PostMessageToDataView(*msg, this);
				SetNoteHeight( zoom_y_to_note_height(valueY) );
			}
		} break;
		case NOTES_BETWEEN_CLEFS_MSG: {
			int32	i;
			if (msg->FindInt32(NOTES_BETWEEN_CLEFS_STR, &i) == B_OK) {
				mTrackWinProps.PostMessageToDataView(*msg, this);
				SetNotesBetweenClefs(i);
			}
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmHybridInfoView::SetNoteHeight(float noteHeight)
{
	mShared.mNoteHeight = noteHeight;
	BMenuField*		field = dynamic_cast<BMenuField*>( FindView(INFO_OCTAVE_MENU_STR) );
	if (field) field->MoveTo( OctaveMenuLeftTop() );
	Invalidate();
}

void AmHybridInfoView::SetOctave(int32 octave)
{
	mShared.mOctave = octave;
	Invalidate();
}

void AmHybridInfoView::SetNotesBetweenClefs(int32 notesBetweenClefs)
{
	if (mShared.mNotesBetweenClefs == notesBetweenClefs) return;
	int32		octaveChange = 0;
	if (mShared.mNotesBetweenClefs == 3 && notesBetweenClefs == 17) octaveChange = -1;
	if (mShared.mNotesBetweenClefs == 17 && notesBetweenClefs == 3) octaveChange = 1;
	mShared.mNotesBetweenClefs = notesBetweenClefs;
	mShared.mOctave += octaveChange;
	BMenuField*		field = dynamic_cast<BMenuField*>( FindView(INFO_OCTAVE_MENU_STR) );
	if (field) field->MoveTo( OctaveMenuLeftTop() );
	Invalidate();

	if (octaveChange != 0) {
		BMenuField*		field = dynamic_cast<BMenuField*>( FindView(INFO_OCTAVE_MENU_STR) );
		if (field && field->Menu() ) {
			BMenuItem*	item;
			for (int32 index = 0; (item = field->Menu()->ItemAt(index)); index++) {
				if (item->Message() && item->Message()->what == OCTAVE_CHANGE_MSG) {
					int32		octave;
					if (item->Message()->FindInt32(STR_OCTAVE, &octave) == B_OK) {
						if (octave == mShared.mOctave) item->SetMarked(true);
					}
				}
			}
		}
	}
}

void AmHybridInfoView::PreDrawSliceOn(BView* view, BRect clip)
{
	float	trebleT = mShared.mNoteHeight * (NOTES_ABOVE_CLEFS);
	float	trebleB = mShared.mNoteHeight * (NOTES_ABOVE_CLEFS + NOTES_IN_CLEFS - 1);
	float	bassT = mShared.mNoteHeight * (NOTES_ABOVE_CLEFS + NOTES_IN_CLEFS + mShared.mNotesBetweenClefs);
	float	bassB = mShared.mNoteHeight * (NOTES_ABOVE_CLEFS + (NOTES_IN_CLEFS *2) + mShared.mNotesBetweenClefs - 1);

	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	// left line
	if (clip.left == 0) {
		view->StrokeLine( BPoint(0, trebleT), BPoint(0, trebleB) );
		view->StrokeLine( BPoint(0, bassT), BPoint(0, bassB) );
	}

	// Draw the black line signifying the boundary between the
	// two views.  This line is inset by one pixel.
	if ((clip.right == Frame().right)
			|| (clip.right == (Frame().right -1))) {
		float	y = clip.right -1;
		view->StrokeLine( BPoint(y, trebleT), BPoint(y, trebleB) );
		view->StrokeLine( BPoint(y, bassT), BPoint(y, bassB) );
	}

	if (treble_clef && bass_clef) {
		drawing_mode	mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
//		view->DrawBitmapAsync( treble_clef, BPoint(5, trebleB - 72) );
		view->DrawBitmapAsync( treble_clef, BPoint(5, trebleB - 52) );
		view->DrawBitmapAsync( bass_clef, BPoint(9, bassT) );
		view->SetDrawingMode(mode);
	}

	draw_no_mans_land(view, clip, mShared);
}

void AmHybridInfoView::PostDrawSliceOn(BView* view, BRect clip)
{
}

void AmHybridInfoView::AddOctaveMenu()
{
	BMenuField*	menuField;
	BPopUpMenu*	popUpMenu;
	
	BPoint		leftTop = OctaveMenuLeftTop();
	BRect		rect(leftTop.x, leftTop.y, Bounds().right - 1, leftTop.y + 20);

	const char	*initialString;
//	if (mInitialOctave == -1) {
//		initialString = "C3";
//	} else {
		initialString = StringForOctave(mShared.mOctave);
//	}
	popUpMenu = new BPopUpMenu(initialString, true, true, B_ITEMS_IN_COLUMN);

	AddOctaveItem( popUpMenu, 10, StringForOctave(10), false );
	AddOctaveItem( popUpMenu, 9, StringForOctave(9) );
	AddOctaveItem( popUpMenu, 8, StringForOctave(8) );
	AddOctaveItem( popUpMenu, 7, StringForOctave(7) );
	AddOctaveItem( popUpMenu, 6, StringForOctave(6) );
	AddOctaveItem( popUpMenu, 5, StringForOctave(5) );
	AddOctaveItem( popUpMenu, 4, StringForOctave(4) );
	AddOctaveItem( popUpMenu, 3, StringForOctave(3) );
	AddOctaveItem( popUpMenu, 2, StringForOctave(2) );
	AddOctaveItem( popUpMenu, 1, StringForOctave(1), false );

	menuField = new BMenuField(rect, INFO_OCTAVE_MENU_STR, "Octave", popUpMenu);
	menuField->SetDivider(0);
	AddChild(menuField);
	menuField->SetViewColor( 255, 255, 255 );
}

BPopUpMenu*	AmHybridInfoView::NewPropertiesMenu() const
{
	BPopUpMenu*		menu = inherited::NewPropertiesMenu();
	if (!menu) return menu;

	BMenuItem*	item;
	for (int32 index = 0; (item = menu->ItemAt( index )); index++) {
		if ( item->Message() ) {
			if (item->Message()->what == DUPLICATE_INFO_MSG
					|| item->Message()->what == REMOVE_INFO_MSG)
				item->SetEnabled(false);
		}
		if ( BString(AM_INFO_CHANGE_VIEW_STR).Compare(item->Label()) == 0)
			item->SetEnabled(false);
	}
	
	BMenu*		hybridMenu = NewHybridMenu();
	if (!hybridMenu) return menu;
	item = new BMenuItem(hybridMenu);
	if (!item) {
		delete hybridMenu;
		return menu;
	}
	menu->AddSeparatorItem();
	menu->AddItem(item);

	return menu;
}

void AmHybridInfoView::Configure(const BMessage* config)
{
	if (config->FindFloat(NOTE_Y_STR, &(mShared.mNoteHeight) ) != B_OK) mShared.mNoteHeight = INITIAL_NOTE_HEIGHT;
	int32		i;
	if (config->FindInt32(TOP_NOTE_STR, &i) == B_OK) {
		mShared.mOctave = octave_from_top_note(i);
	}
	if (config->FindInt32(NOTES_BETWEEN_CLEFS_STR, &i) == B_OK) {
		mShared.mNotesBetweenClefs = i;
		if (i == 17) mShared.mOctave--;
	}
}

void AmHybridInfoView::AddOctaveItem(BPopUpMenu* toMenu, int32 octaveNum, const char* label, bool enable)
{
	BMenuItem	*menuItem;
	BMessage	*msg;

	msg = new BMessage(OCTAVE_CHANGE_MSG);
	msg->AddInt32(STR_OCTAVE, octaveNum);
	menuItem = new BMenuItem(label, msg, 0, 0);
	if (!menuItem) return;
	menuItem->SetEnabled(enable);
	menuItem->SetTarget(this);
	toMenu->AddItem(menuItem);
}

const char* AmHybridInfoView::StringForOctave(int32 octave)
{
	if (octave == 10) return "C8";
	if (octave == 9) return "C7";
	if (octave == 8) return "C6";
	if (octave == 7) return "C5";
	if (octave == 6) return "C4";
	if (octave == 5) return "C3";
	if (octave == 4) return "C2";
	if (octave == 3) return "C1";
	if (octave == 2) return "C0";
	if (octave == 1) return "C-1";
	return "?";
}

BPoint AmHybridInfoView::OctaveMenuLeftTop() const
{
	return BPoint(1, (mShared.mNoteHeight * (NOTES_ABOVE_CLEFS + NOTES_IN_CLEFS + (mShared.mNotesBetweenClefs / 2))) - 10);
}

BMenu* AmHybridInfoView::NewHybridMenu() const
{
	BMenu*		menu = new BMenu("Notes Between Clefs");
	if (!menu) return NULL;
	BMessage*	msg = new BMessage(NOTES_BETWEEN_CLEFS_MSG);
	BMenuItem*	item = new BMenuItem("3", msg);
	if (msg && item) {
		msg->AddInt32(NOTES_BETWEEN_CLEFS_STR, 3);
		menu->AddItem(item);
		if (mShared.mNotesBetweenClefs == 3) item->SetMarked(true);
		item->SetTarget(this);
	}
	msg = new BMessage(NOTES_BETWEEN_CLEFS_MSG);
	item = new BMenuItem("17", msg);
	if (msg && item) {
		msg->AddInt32(NOTES_BETWEEN_CLEFS_STR, 17);
		menu->AddItem(item);
		if (mShared.mNotesBetweenClefs == 17) item->SetMarked(true);
		item->SetTarget(this);
	}

	menu->SetRadioMode(true);
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont(be_plain_font);
	return menu;
}

// #pragma mark -

/*************************************************************************
 * AM-HYBRID-BACKGROUND
 *************************************************************************/
AmHybridBackground::AmHybridBackground(_AmHybridSharedData& shared)
		: mShared(shared)
{
}

void AmHybridBackground::DrawOn(BView* view, BRect clip)
{
	BPoint	startPt, endPt;
	startPt.x = clip.left;
	endPt.x = clip.right;

	uint32		k=0;
	/* These light lines are the notes above the treble clef.
	 */
	view->SetHighColor( Prefs().Color(AM_MEASURE_BEAT_C) );
	while (k < NOTES_ABOVE_CLEFS) {
		startPt.y = endPt.y = (mShared.mNoteHeight * k);
		if (startPt.y > clip.bottom) return;
		if (startPt.y >= clip.top) view->StrokeLine(startPt, endPt);
		k += 2;
	}
	/* Darker lines for the treble clef.
	 */
	view->SetHighColor( Prefs().Color(AM_MEASURE_FG_C) );
	while ( k <= ( NOTES_ABOVE_CLEFS + NOTES_IN_CLEFS ) ) {
		startPt.y = endPt.y = (mShared.mNoteHeight * k);
		if (startPt.y > clip.bottom) return;
		if (startPt.y >= clip.top) view->StrokeLine(startPt, endPt);
		k += 2;
	}
	/* Again with lighter lines, this is the space between clefs.
	 */
	view->SetHighColor( Prefs().Color(AM_MEASURE_BEAT_C) );
	while (k < (NOTES_ABOVE_CLEFS + NOTES_IN_CLEFS + mShared.mNotesBetweenClefs) ) {
		startPt.y = endPt.y = (mShared.mNoteHeight * k);
		if (startPt.y > clip.bottom) return;
		if (startPt.y >= clip.top) view->StrokeLine(startPt, endPt);
		k += 2;
	}
	/* Darker lines for the bass clef.
	 */
	view->SetHighColor( Prefs().Color(AM_MEASURE_FG_C) );
	while (k <= (NOTES_ABOVE_CLEFS + (NOTES_IN_CLEFS * 2) + mShared.mNotesBetweenClefs) ) {
		startPt.y = endPt.y = (mShared.mNoteHeight * k);
		if (startPt.y > clip.bottom) return;
		if (startPt.y >= clip.top) view->StrokeLine(startPt, endPt);
		k += 2;
	}
	/* And now end with the lighter lines below the bass clef.
	 */
	view->SetHighColor( Prefs().Color( AM_MEASURE_BEAT_C ) );
	while (k < (NOTES_ABOVE_CLEFS + (NOTES_IN_CLEFS * 2) + mShared.mNotesBetweenClefs + NOTES_BELOW_CLEFS) ) {
		startPt.y = endPt.y = (mShared.mNoteHeight * k);
		if( startPt.y > clip.bottom ) return;
		if( startPt.y >= clip.top ) view->StrokeLine( startPt, endPt );
		k += 2;
	}
}

// #pragma mark -

/*************************************************************************
 * _AM-HYBRID-TARGET
 *************************************************************************/
_AmHybridTarget::_AmHybridTarget(	AmTrackWinPropertiesI& trackWinProps,
									BView* view, float leftFudge, float rightFudge,
									_AmHybridSharedData& shared)
		: AmToolTarget(trackWinProps, view, leftFudge, rightFudge),
		  mShared(shared)
{
}

bool _AmHybridTarget::IsInteresting(const AmEvent* event) const
{
	if (event->Type() == event->NOTEON_TYPE) return true;
	return false;
}

bool _AmHybridTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::NOTEON_TYPE;
}

BRect _AmHybridTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	if (event->Type() != event->NOTEON_TYPE) return BRect( -1, -1, -1, -1 );
	// FIX: Definitely want to make these part of the class
	// so they aren't reconstructed all the time.
	const AmNoteOn*	noteOn = dynamic_cast<const AmNoteOn*>(event);
	BRect			rect(-1,-1,-1,-1);
	float			noteT;

	if( (noteT = pixel_from_note(noteOn->Note(), mShared)) < 0 ) return rect;
	rect.Set(mMtc.TickToPixel(eventRange.start),	// left
			noteT,									// top
			mMtc.TickToPixel(eventRange.end),		// right
			noteT + mShared.mNoteHeight -1);		// bottom

	if (noteOn->IsSharp()) rect.left = rect.left - SS_SHARP_WIDTH;

	return rect;
}

AmEvent* _AmHybridTarget::InterestingEventAt(	const AmTrack* track,
												const AmPhraseEvent& topPhrase,
												const AmPhrase& phrase,
												AmTime time,
												float y,
												int32* extraData) const
{
	AmNode*		node = phrase.HeadNode();
	AmNoteOn*	noteOn;

	while (node) {
		if ( IsInteresting( node->Event() ) ) {
			AmEvent*	event = node->Event();
			AmRange		eventRange = topPhrase.EventRange(event);
			if ( (time >= eventRange.start) && (time <= eventRange.end) ) {
				noteOn = dynamic_cast<AmNoteOn*>(event);
				if ( (noteOn->Note() == note_from_pixel(y, mShared))
						|| (noteOn->Note() == sharp_note_from_pixel(y, mShared)) )
					return noteOn;
			}
		}
		node = node->next;
	}
	return NULL;
}

bool _AmHybridTarget::EventIntersects(	const AmEvent* event, AmRange eventRange,
										AmTime left, int32 top,
										AmTime right, int32 bottom) const
{
	ArpASSERT(event);
	const AmNoteOn*		nEvent = dynamic_cast<const AmNoteOn*>(event);
	if (!nEvent) return false;
	if( (eventRange.start > right) || (eventRange.end < left) ) return false;
	if( (nEvent->Note() < top) || (nEvent->Note() > bottom) ) return false;
	return true;
}

AmEvent* _AmHybridTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	AmNoteOn*	no = new AmNoteOn(note_from_pixel(y, mShared), TrackWinProperties().Velocity(), time);
	if (!no) return NULL;
	AmTime		grid = TrackWinProperties().GridTime();
	AmTime		duration = grid - 1;
	if (duration <= 0) duration = PPQN;
	no->SetDuration(duration);
	return no;
}

int32 _AmHybridTarget::MoveYValueFromPixel(float y) const
{
	return note_from_pixel(y, mShared);
}

void _AmHybridTarget::GetMoveValues(const AmPhraseEvent& topPhrase,
									const AmEvent* event,
									AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmNoteOn*		no = dynamic_cast<const AmNoteOn*>(event);
	if (no) *y = no->Note();
	else *y = 0;
}

void _AmHybridTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	int32		originNote = note_from_pixel(origin.y, mShared);
	int32		whereNote = note_from_pixel(where.y, mShared);
	*yDelta = (whereNote - originNote);
}

void _AmHybridTarget::SetMove(	AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	AmNoteOn*			noteEvent = dynamic_cast<AmNoteOn*>(event);
	if (!noteEvent) return;
//	if (flags&TRANSFORM_X) {

	AmTime			newStart = originalX + deltaX;
	if (newStart < 0) newStart = 0;
	if (newStart != topPhrase.EventRange(event).start)
		topPhrase.SetEventStartTime(noteEvent, newStart);

//	}
//	if (flags&TRANSFORM_Y) {
		int32			newNote = originalY + deltaY;
		if (newNote > 127) newNote = 127;
		else if (newNote < 0) newNote = 0;
		noteEvent->SetNote(newNote);
//	}
}

void _AmHybridTarget::GetOriginalTransform(	AmEvent* event,
											am_trans_params& params) const
{
	AmNoteOn*			noteOn = dynamic_cast<AmNoteOn*>(event);
	if (!noteOn) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = noteOn->Duration();
		params.original_y = noteOn->Velocity();
	}
}

void _AmHybridTarget::GetDeltaTransform(BPoint origin, BPoint where,
										am_trans_params& params) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	params.delta_x = (whereTime - originTime);
	params.delta_y = (int32) (origin.y - where.y);
}

uint32 _AmHybridTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params)
{
	AmNoteOn*			noteOn = dynamic_cast<AmNoteOn*>(event);
	if (!noteOn) return 0;
	if (params.flags&TRANSFORM_X) {
		int32			newEnd = params.original_x + params.delta_x;
		if (newEnd < 1) newEnd = 1;
		topPhrase.SetEventEndTime(noteOn, topPhrase.EventRange(noteOn).start + newEnd);
	} else if (params.flags&TRANSFORM_Y) {
		int32			newVelocity = params.original_y + params.delta_y;
		if (newVelocity > 127) newVelocity = 127;
		if (newVelocity < 0  ) newVelocity = 0;
		noteOn->SetVelocity(newVelocity);
	}
	return 0;
}

uint32 _AmHybridTarget::SetTransform(	const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									BPoint where,
									const am_trans_params& params)
{
	return 0;
}

void _AmHybridTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	mPerformer.SetBPM(song->BPM() );
	if (event) mPerformer.Play(event);
}

// #pragma mark -

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
static uchar note_from_pixel(float y, _AmHybridSharedData& shared)
{
	int32	screenNote = (int32)( ( (y+1) / shared.mNoteHeight) );
	int32	numberOfNotes = shared.NotesOnScreen();
//	NOTES_ABOVE_CLEFS + (NOTES_IN_CLEFS * 2) + shared.mNotesBetweenClefs + NOTES_BELOW_CLEFS;
	if (screenNote > numberOfNotes) screenNote = numberOfNotes;
	screenNote = numberOfNotes - screenNote;	

	int32	octaveCompensation = 0;
	if (shared.mNotesBetweenClefs == 3) octaveCompensation = 3;
	else if (shared.mNotesBetweenClefs == 17) octaveCompensation = 4;

	int32	octaveOffset = ((shared.mOctave - octaveCompensation) * 12) + 7;
	int32 	smallPattern = (screenNote % 7);
	int32	largePattern = (screenNote / 7) * 5;
	
	if (smallPattern == 1) {
		screenNote = screenNote + 1;
	} else if ((smallPattern == 2) || (smallPattern == 3)) {
		screenNote = screenNote + 2;
	} else if (smallPattern == 4) {
		screenNote = screenNote + 3;
	} else if ((smallPattern == 5) || (smallPattern == 6)) {
		screenNote = screenNote + 4;
	}
	
	int32	midiNote = screenNote + largePattern + octaveOffset;
	if (midiNote < 0) return(0);
	if (midiNote > 127) return(127);
	return midiNote;
}

static uchar sharp_note_from_pixel(float y, _AmHybridSharedData& shared)
{
	uchar	midiNote = note_from_pixel(y, shared);
	uchar	rem = midiNote % 12;

	if (rem == 0) return(midiNote + 1);
	if (rem == 2) return(midiNote + 1);
	if (rem == 5) return(midiNote + 1);
	if (rem == 7) return(midiNote + 1);
	if (rem == 9) return(midiNote + 1);
	return midiNote;
}

static float pixel_from_note(uchar note, _AmHybridSharedData& shared)
{
	uchar	div = (note / 12);		// produces 0 - 10;
	uchar	rem = (note % 12);		// produces 0 - 11;
	int		pixel, tmp = 0;
	
	if ((rem == 0) || (rem == 1)) tmp = 0;
	if ((rem == 2) || (rem == 3)) tmp = 1;
	if (rem == 4) tmp = 2;
	if ((rem == 5) || (rem == 6)) tmp = 3;
	if ((rem == 7) || (rem == 8)) tmp = 4;
	if ((rem == 9) || (rem == 10)) tmp = 5;
	if (rem == 11) tmp = 6;
	
	int32	notesAboveC = 0;
	if (shared.mNotesBetweenClefs == 3) notesAboveC = 17;
	else if (shared.mNotesBetweenClefs == 17) notesAboveC = 24;
	if (div >= shared.mOctave) {
		pixel = (notesAboveC) - ((div - shared.mOctave) * 7);
		pixel -= tmp;
	} else {
		pixel = (notesAboveC + 1) + ((shared.mOctave - div - 1) * 7);
		pixel += (int)(fabs(6 - tmp));
	}
	return ( (pixel-1) * shared.mNoteHeight) - (shared.mNoteHeight / 2);
}

static int32 octave_from_top_note(int32 top_note)
{
	/* The top note is a D, so first I need to transform the top note
	 * into the nearest D.
	 */
	int32	delta = 2 - (top_note % 12);
	int32	new_top = top_note + delta;
	/* These are hardcoded limits -- if the D is out of bounds, force it into
	 * bounds.
	 */
	if (new_top <= 50) return 2;
	int32	step = 12;
	int32	oct = 2;
	int32	curr = 50;
	while (curr < new_top) {
		curr += step;
		oct++;
	}
	if (oct > 10) return 10;
	return oct;
}

static void draw_no_mans_land(BView* view, BRect clip, _AmHybridSharedData& shared)
{
	int32	numberOfNotes = shared.NotesOnScreen();
	float	bottomY = (numberOfNotes * shared.mNoteHeight) + shared.mNoteHeight / 2;
	if (clip.bottom < bottomY) return;

	rgb_color	cbg = Prefs().Color(AM_CONTROL_BG_C);
	rgb_color	dbg = Prefs().Color(AM_DATA_BG_C);
	
	BRect	r(clip.left, bottomY + 3, clip.right, clip.bottom);
	view->SetHighColor(cbg);
	view->FillRect(r);

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->SetLowColor(dbg);
	
	cbg.alpha = 64;
	view->SetHighColor(cbg);
	view->StrokeLine( BPoint(r.left, bottomY), BPoint(r.right, bottomY) );

	cbg.alpha = 128;
	view->SetHighColor(cbg);
	view->StrokeLine( BPoint(r.left, bottomY + 1), BPoint(r.right, bottomY + 1) );

	cbg.alpha = 192;
	view->SetHighColor(cbg);
	view->StrokeLine( BPoint(r.left, bottomY + 2), BPoint(r.right, bottomY + 2) );


#if 0
	cbg.alpha = 85;
	view->SetHighColor(cbg);
	view->StrokeLine( BPoint(r.left, bottomY), BPoint(r.right, bottomY) );

	cbg.alpha = 170;
	view->SetHighColor(cbg);
	view->StrokeLine( BPoint(r.left, bottomY + 1), BPoint(r.right, bottomY + 1) );
#endif

	view->SetDrawingMode(mode);

#if 0
	view->SetHighColor( mix_color(dbg, cbg, (uint8)85) );
	view->StrokeLine( BPoint(r.left, bottomY), BPoint(r.right, bottomY) );

	view->SetHighColor( mix_color(dbg, cbg, (uint8)170) );
	view->StrokeLine( BPoint(r.left, bottomY + 1), BPoint(r.right, bottomY + 1) );
#endif
}

// #pragma mark -

_AmHybridSharedData::_AmHybridSharedData(float noteHeight, int32 octave, int32 notesBetweenClefs)
		: mNoteHeight(noteHeight), mOctave(octave),
		  mNotesBetweenClefs(notesBetweenClefs)
{
}

int32 _AmHybridSharedData::NotesOnScreen() const
{
	return NOTES_ABOVE_CLEFS + (NOTES_IN_CLEFS * 2) + mNotesBetweenClefs + NOTES_BELOW_CLEFS;
}
