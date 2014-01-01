/* AmVelocityView.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <be/interface/MenuField.h>
#include <be/interface/MenuItem.h>
#include <be/interface/PopUpMenu.h>
#include <be/interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewPropertyI.h"
#include "AmStdFactory/AmVelocityView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

// The configuration message I generate and that gets passed back to me.
#define ARPMSG_CONFIGDATA		'pCnf'

enum {
	DISPLAY_ON_VEL		= (1<<0),
	DISPLAY_OFF_VEL		= (1<<1)
};

static const uint32	INIT_DISPLAY_FLAGS		= DISPLAY_ON_VEL;
static const char*	INFO_NAME_STR			= "Velocity Info";
static const char*	DATA_NAME_STR			= "Velocity Data";
static const char*	DISPLAY_FLAGS_STR		= "display_flags";
static const float	PREFERRED_HEIGHT		= 50;
static const uint32	VEL_CHANGE_MSG			= 'iVlC';

/*************************************************************************
 * _AM-VELOCITY-TARGET
 * The tool API implementation for the velocity view.
 *************************************************************************/
class _AmVelocityTarget : public AmToolTarget
{
public:
	_AmVelocityTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);
	
	void				SetDisplayFlags(uint32 displayFlags);

	virtual uint32		Flags() const;
	virtual bool		IsInteresting(const AmEvent* event) const;
	virtual bool		IsInteresting(const BMessage* flatEvent) const;
	virtual BRect		RectFor(const AmEvent* event, AmRange eventRange) const;
	virtual AmTime		EventAtFudge() const;
	virtual AmEvent*	InterestingEventAt(	const AmTrack* track,
											const AmPhraseEvent& topPhrase,
											const AmPhrase& phrase,
											AmTime time,
											float y,
											int32* extraData) const;
	AmEvent*			InterestingReleaseEventAt(	const AmTrack* track,
													const AmPhrase& phrase,
													AmTime time,
													float y) const;

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
	AmPerformer		mPerformer;
	uint32			mDisplayFlags;

	/* Given a y position, answer a velocity.  These methods assume the mView
	 * is set.
	 */
	uchar VelocityFromPixel(float y) const;
	int32 BoundlessVelocityFromPixel(float y) const;
};

/*************************************************************************
 * AM-VELOCITY-INFO-VIEW
 *************************************************************************/
AmVelocityInfoView::AmVelocityInfoView(	BRect frame,
										AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI* property,
										TrackViewType viewType)
		: inherited(frame, INFO_NAME_STR, songRef, trackWinProps, viewType),
		mDisplayFlags(INIT_DISPLAY_FLAGS), mCachedVelocityMenu(NULL)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
	if ( property->Configuration() ) SetConfiguration( property->Configuration() );
}

void AmVelocityInfoView::SetConfiguration(const BMessage* config)
{
	int32	i;
	if (config->FindInt32(DISPLAY_FLAGS_STR, &i) == B_OK) {
		mDisplayFlags = (uint32)i;
		BMenu*	menu = VelocityMenu();
		if (menu) {
//			BMenuItem*	item;
//			for (int32 index = 0; (item = menu->ItemAt( index )); index++) {
//				if( index == mCc ) item->SetMarked( false );
//				else item->SetMarked( true );
//			}
		}
	}
}

void AmVelocityInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmVelocityInfoView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case VEL_CHANGE_MSG:
			int32	i;
			if (msg->FindInt32(DISPLAY_FLAGS_STR, &i) == B_OK) {
				mDisplayFlags = (uint32)i;
				mTrackWinProps.PostMessageToDataView(*msg, this);
			}
			break;
		case ARPMSG_CONFIGDATA:
			SetConfiguration(msg);
			break;				
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmVelocityInfoView::DrawOn(BRect clip, BView* view)
{
	inherited::DrawOn(clip, view);
	float		bottom = Bounds().bottom;
	float		left = 2;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString( "Velocity", BPoint(left, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );

	if (mDisplayFlags&DISPLAY_OFF_VEL) {
		float	h = arp_get_font_height(this);
		view->DrawString( "Release", BPoint(left, bottom - 2 - h) );
	}
}

BPopUpMenu* AmVelocityInfoView::NewPropertiesMenu() const
{
	BPopUpMenu*		menu = inherited::NewPropertiesMenu();
	if (!menu) return menu;
	mCachedVelocityMenu = NewVelocityMenu();
	if (!mCachedVelocityMenu) return menu;
	BMenuItem*		item;
	item = new BMenuItem(mCachedVelocityMenu);
	if (!item) {
		delete mCachedVelocityMenu;
		mCachedVelocityMenu = NULL;
		return menu;
	}
	menu->AddSeparatorItem();
	menu->AddItem(item);
	return menu;
}

BMenu* AmVelocityInfoView::NewVelocityMenu() const
{
	BMenu*		menu = new BMenu("Edit");
	if (!menu) return NULL;
	BMessage*	msg = new BMessage(VEL_CHANGE_MSG);
	BMenuItem*	item = new BMenuItem("Velocity", msg);
	if (msg && item) {
		msg->AddInt32(DISPLAY_FLAGS_STR, DISPLAY_ON_VEL);
		menu->AddItem(item);
		if ( (mDisplayFlags&DISPLAY_ON_VEL) && !(mDisplayFlags&DISPLAY_OFF_VEL) )
			item->SetMarked(true);
		item->SetTarget(this);
	}
	msg = new BMessage(VEL_CHANGE_MSG);
	item = new BMenuItem("Release Velocity", msg);
	if (msg && item) {
		msg->AddInt32(DISPLAY_FLAGS_STR, DISPLAY_OFF_VEL);
		menu->AddItem(item);
		if ( (mDisplayFlags&DISPLAY_OFF_VEL) && !(mDisplayFlags&DISPLAY_ON_VEL) )
			item->SetMarked(true);
		item->SetTarget(this);
	}
#if 0
	msg = new BMessage(VEL_CHANGE_MSG);
	item = new BMenuItem("Velocity and Release Velocity", msg);
	if (msg && item) {
		msg->AddInt32(DISPLAY_FLAGS_STR, DISPLAY_ON_VEL | DISPLAY_OFF_VEL);
		menu->AddItem(item);
		if ( (mDisplayFlags&DISPLAY_ON_VEL) && (mDisplayFlags&DISPLAY_OFF_VEL) )
			item->SetMarked(true);
		item->SetTarget(this);
	}
#endif	
	menu->SetRadioMode(true);
	menu->SetFontSize( Prefs().Size(FONT_Y) );
	menu->SetFont(be_plain_font);
	return menu;
}

BMenu* AmVelocityInfoView::VelocityMenu() const
{
	return mCachedVelocityMenu;
}

/*************************************************************************
 * AM-VELOCITY-DATA-VIEW
 *************************************************************************/
AmVelocityDataView::AmVelocityDataView(	BRect frame,
										AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI& viewProp,
										TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, DATA_NAME_STR,
				B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL), mDisplayFlags(INIT_DISPLAY_FLAGS)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmVelocityTarget(trackWinProps, this);
	if (viewProp.Configuration() ) Configure(viewProp.Configuration() );

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
}

void AmVelocityDataView::SetDisplayFlags(uint32 displayFlags)
{
	mDisplayFlags = (uint32)displayFlags;
	_AmVelocityTarget*	target = dynamic_cast<_AmVelocityTarget*>(mTarget);
	if (target) target->SetDisplayFlags(displayFlags);
	Invalidate();
}

void AmVelocityDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmVelocityDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll(this);
	mSongRef.RemoveObserverAll(this);
}

void AmVelocityDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	mPt2.y = Bounds().bottom - 1;
	mScale = (mPt2.y -1) / 127;	
	AddAsObserver();
}

void AmVelocityDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmVelocityDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

void AmVelocityDataView::MessageReceived(BMessage *msg)
{
	if (mTarget && mTarget->HandleMessage(msg)) return;

	switch (msg->what) {
		case AM_ORDERED_TRACK_MSG: {
			int32		order;
			if (msg->FindInt32("track_order", &order) == B_OK && order == 0) {
				mCachedPrimaryTrack.RemoveObserverAll(this);
				mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
				if (mMeasureBg) mMeasureBg->SetTrackRef(mCachedPrimaryTrack);
				BRect		b = Bounds();
				AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
				mCachedPrimaryTrack.AddRangeObserver(	this,
														AmNotifier::NOTE_OBS,
														range);
				Invalidate();
			}
		} break;
		case VEL_CHANGE_MSG:
			int32	i;
			if (msg->FindInt32(DISPLAY_FLAGS_STR, &i) == B_OK) {
				SetDisplayFlags( (uint32)i );
			}
			break;
		case ARPMSG_CONFIGDATA:
			Configure(msg);
			break;
				
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

BMessage* AmVelocityDataView::ConfigurationData()
{
	BMessage	*msg = new BMessage(ARPMSG_CONFIGDATA);
	if (msg == NULL) return NULL;
	msg->AddInt32(DISPLAY_FLAGS_STR, mDisplayFlags);
	return msg;
}

void AmVelocityDataView::Configure(const BMessage* msg)
{
	int32	i;
	if (msg->FindInt32(DISPLAY_FLAGS_STR, &i) == B_OK) {
		SetDisplayFlags( (uint32)i );
	}
}

void AmVelocityDataView::DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->NOTEON_TYPE) return;
	const AmNoteOn*		noEvent = dynamic_cast<const AmNoteOn*>(event);
	if (!noEvent) return;

	BPoint		on, off;
	/* Setup
	 */
	on.x = mMtc.TickToPixel(eventRange.start);
	on.y = abs((int)(mPt2.y - (mScale * noEvent->Velocity())));
	if (on.y < 0) on.y = 0;

	off.x = mMtc.TickToPixel(eventRange.end);
	off.y = abs((int)(mPt2.y - (mScale * noEvent->ReleaseVelocity())));
	if (off.y < 0) off.y = 0;

	/* Draw the background
	 */
	BPoint		pts[4];
	pts[0].Set(on.x, mPt2.y);
	pts[1] = on;
	pts[2] = off;
	pts[3].Set(off.x, mPt2.y);

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->SetLowColor( view->ViewColor() );
	rgb_color	c;
	if (properties&ARPEVENT_PRIMARY)
		c = EventColor();
	else if (properties&ARPEVENT_SHADOW)
		c = AmPrefs().ShadowColor();
	else
		c = AmPrefs().SelectedColor();
	c.alpha = 75;
	view->SetHighColor(c);
	view->FillPolygon( pts, 4, BRect(on.x, min(on.y, off.y), off.x, mPt2.y) );
	view->SetDrawingMode( mode );

	/* Draw the foreground
	 */
	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor() );
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else
		view->SetHighColor( AmPrefs().SelectedColor() );

	if (mDisplayFlags&DISPLAY_ON_VEL && mDisplayFlags&DISPLAY_OFF_VEL)
		view->StrokeLine(on, off);
	if (mDisplayFlags&DISPLAY_ON_VEL)
		view->StrokeLine(on, BPoint(on.x, mPt2.y) );
	if (mDisplayFlags&DISPLAY_OFF_VEL)
		view->StrokeLine(off, BPoint(off.x, mPt2.y) );
}

void AmVelocityDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::NOTE_OBS,
											range);
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

/*************************************************************************
 * _AM-VELOCITY-TARGET
 *************************************************************************/
_AmVelocityTarget::_AmVelocityTarget(	AmTrackWinPropertiesI& trackWinProps,
										BView* view)
		: AmToolTarget(trackWinProps, view),
		  mDisplayFlags(INIT_DISPLAY_FLAGS)
{
}

void _AmVelocityTarget::SetDisplayFlags(uint32 displayFlags)
{
	mDisplayFlags = displayFlags;
}

uint32 _AmVelocityTarget::Flags() const
{
	return DRAG_TIME_ONLY;
}

bool _AmVelocityTarget::IsInteresting(const AmEvent* event) const
{
	ArpASSERT(event);
	return event->Type() == event->NOTEON_TYPE;
}

bool _AmVelocityTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::NOTEON_TYPE;
}

BRect _AmVelocityTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + 1;
	r.bottom = r.bottom -1;
	return r;
}

AmTime _AmVelocityTarget::EventAtFudge() const
{
	/* My fudge is three pixels.
	 */
	return mMtc.PixelToTick(3);
}

AmEvent* _AmVelocityTarget::InterestingEventAt(	const AmTrack* track,
												const AmPhraseEvent& topPhrase,
												const AmPhrase& phrase,
												AmTime time,
												float y,
												int32* extraData) const
{
	if (mDisplayFlags&DISPLAY_OFF_VEL)
		return InterestingReleaseEventAt(track, phrase, time, y);
		
	AmNode*		n = phrase.ChainHeadNode(time);
	if (!n) return NULL;
	/* Since control changes are single pixels, it can be a bit tricky
	 * for users to hit exactly the right pixel.  Compensate for this
	 * by finding the closest control change within a given fudge factor.
	 */
	AmTime		fudge = EventAtFudge();
	AmEvent*	closest = NULL;
	/* As soon as I've hit events that are within range of where I'm
	 * looking, I set this flag.  This lets me know to end the search
	 * as soon as I'm out of range.
	 */
	bool		beenInRange = false;
	while (n) {
		AmRange		eventRange = topPhrase.EventRange( n->Event() );
		if( (eventRange.start >= time - fudge) && (eventRange.start <= time + fudge) ) {
			beenInRange = true;
			if( IsInteresting( n->Event() ) ) {
				if (!closest)
					closest = n->Event();
				else if ( abs(time - eventRange.start) < abs(time - topPhrase.EventRange(closest).start) )
					closest = n->Event();
			}
		} else {
			if (beenInRange) return closest;
		}
		n = n->next;
	}
	return closest;
}

AmEvent* _AmVelocityTarget::InterestingReleaseEventAt(	const AmTrack* track,
														const AmPhrase& phrase,
														AmTime time,
														float y) const
{
	AmNode*		n = phrase.ChainHeadNode( time );
	if (!n) return NULL;
	/* Since control changes are single pixels, it can be a bit tricky
	 * for users to hit exactly the right pixel.  Compensate for this
	 * by finding the closest control change within a given fudge factor.
	 */
	AmTime		fudge = EventAtFudge();
	AmEvent*	closest = NULL;
	/* As soon as I've hit events that are within range of where I'm
	 * looking, I set this flag.  This lets me know to end the search
	 * as soon as I'm out of range.
	 */
	bool		beenInRange = false;
	while (n) {
		if( (n->EndTime() >= time - fudge) && (n->EndTime() <= time + fudge) ) {
			beenInRange = true;
			if( IsInteresting( n->Event() ) ) {
				if( !closest )
					closest = n->Event();
				else if( abs(time - n->EndTime()) < abs(time - closest->EndTime()) )
					closest = n->Event();
			}
		} else {
			if (beenInRange) return closest;
		}
		n = n->next;
	}
	return closest;
}


AmEvent* _AmVelocityTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	return NULL;
}

int32 _AmVelocityTarget::MoveYValueFromPixel(float y) const
{
	return VelocityFromPixel(y);
}

void _AmVelocityTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
										const AmEvent* event,
										AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmNoteOn*		noEvent = dynamic_cast<const AmNoteOn*>(event);
	if (noEvent) *y = noEvent->Velocity();
	else *y = 0;
}

void _AmVelocityTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	*xDelta = 0;
	
	/* I don't want the normal bounded transformation so that the move
	 * tool can 'flatten' all the events as you drag to the edge of the view.
	 */
	int32		originVal = BoundlessVelocityFromPixel(origin.y);
	int32		whereVal = BoundlessVelocityFromPixel(where.y);
	*yDelta = (whereVal - originVal);
}

void _AmVelocityTarget::SetMove(AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	AmNoteOn*		noEvent = dynamic_cast<AmNoteOn*>(event);
	if (!noEvent) return;

	int32			newVal = originalY + deltaY;
	if (newVal > 127) newVal = 127;
	else if (newVal < 0) newVal = 0;
	noEvent->SetVelocity(newVal);
}

void _AmVelocityTarget::GetOriginalTransform(	AmEvent* event,
												am_trans_params& params) const
{
	const AmNoteOn*		noEvent = dynamic_cast<AmNoteOn*>(event);
	params.original_x = 0;
	params.original_y = 0;
	if (noEvent) {
		if (mDisplayFlags&DISPLAY_ON_VEL) {
			params.original_x = noEvent->Velocity();
			params.original_y = noEvent->Velocity();
		} else if (mDisplayFlags&DISPLAY_OFF_VEL) {
			params.original_x = noEvent->ReleaseVelocity();
			params.original_y = noEvent->ReleaseVelocity();
		}
	}
}

void _AmVelocityTarget::GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const
{
	params.delta_x = 0;
	params.delta_y = (int32) (where.y - origin.y);
}

uint32 _AmVelocityTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params)
{
	AmNoteOn*	noEvent = dynamic_cast<AmNoteOn*>(event);
	if (!noEvent) return 0;
	int32			newValue = params.original_y - params.delta_y;
	if (newValue > 127) newValue = 127;
	if (newValue < 0  ) newValue = 0;

	if (mDisplayFlags&DISPLAY_ON_VEL)
		noEvent->SetVelocity(newValue);
	else if (mDisplayFlags&DISPLAY_OFF_VEL)
		noEvent->SetReleaseVelocity(newValue);
	return 0;
}

uint32 _AmVelocityTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										BPoint where,
										const am_trans_params& params)
{
	AmNoteOn*	noEvent = dynamic_cast<AmNoteOn*>(event);
	if (!noEvent) return 0;
	if (params.flags&TRANSFORM_Y) {
		if (mDisplayFlags&DISPLAY_ON_VEL)
			noEvent->SetVelocity( VelocityFromPixel(where.y) );
		else if (mDisplayFlags&DISPLAY_OFF_VEL)
			noEvent->SetReleaseVelocity( VelocityFromPixel(where.y) );
	}
	return 0;
}

void _AmVelocityTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	mPerformer.SetBPM(song->BPM() );
	if (event) mPerformer.Play(event);
}

uchar _AmVelocityTarget::VelocityFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	if( y < b.top ) return 127;
	else if( y > (b.bottom - 1) ) return 0;

	return (uchar) (127 - ((127 / (b.Height() -1 )) * y));
}

int32 _AmVelocityTarget::BoundlessVelocityFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	return (int32) (127 - ((127 / (b.Height() -1 )) * y));
}
