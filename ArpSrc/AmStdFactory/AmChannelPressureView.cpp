/* AmChannelPressureView.cpp
 */
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/Window.h>
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
#include "AmStdFactory/AmChannelPressureView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const float	PREFERRED_HEIGHT	= 50;
static const uint8	INITIAL_CC			= 10;
static const char*	INFO_NAME			= "Channel Pressure Info";
static const char*	DATA_NAME			= "Channel Pressure Data";

#define I_ARP_CC_CHANGE_MSG		'aCcm'
#define STR_CONTROL_CHANGE		"ControlChange"

/*************************************************************************
 * _AM-CHANNEL-PRESSURE-TARGET
 * The tool API implementation for the channel pressure view.
 *************************************************************************/
class _AmChannelPressureTarget : public AmToolTarget
{
public:
	_AmChannelPressureTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);
	virtual ~_AmChannelPressureTarget();

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

	/* Given a y position, answer a control value.  This method assumes the mView
	 * is set.
	 */
	uint8 ChannelPressureFromPixel(float y) const;
	int32 BoundlessChannelPressureFromPixel(float y) const;
};

/*************************************************************************
 * AM-CHANNEL-PRESSURE-INFO-VIEW
 *************************************************************************/
AmChannelPressureInfoView::AmChannelPressureInfoView(	BRect frame,
														AmSongRef songRef,
														AmTrackWinPropertiesI& trackWinProps,
														const AmViewPropertyI* property,
														TrackViewType viewType)
		: inherited(frame, INFO_NAME, songRef, trackWinProps, viewType)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmChannelPressureInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmChannelPressureInfoView::DrawOn(BRect clip, BView* view)
{
	inherited::DrawOn(clip, view);

	float		bottom = Bounds().bottom;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString( "Channel", BPoint(2, bottom - 12) );
	view->DrawString( "Aftertouch", BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

/*************************************************************************
 * AM-CHANNEL-PRESSURE-DATA-VIEW
 *************************************************************************/
AmChannelPressureDataView::AmChannelPressureDataView(	BRect frame,
														AmSongRef songRef,
														AmTrackWinPropertiesI& trackWinProps,
														const AmViewPropertyI& viewProp,
														TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, DATA_NAME,
					B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmChannelPressureTarget(trackWinProps, this);

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
}

void AmChannelPressureDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmChannelPressureDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll(this);
	mSongRef.RemoveObserverAll(this);
}

void AmChannelPressureDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	mPt2.y = Bounds().bottom - 1;
	mScale = (mPt2.y -1) / 127;	
	AddAsObserver();
}

void AmChannelPressureDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmChannelPressureDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

void AmChannelPressureDataView::MessageReceived(BMessage *msg)
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
														AmNotifier::OTHER_EVENT_OBS,
														range);						
				Invalidate();
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmChannelPressureDataView::DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
											const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->CHANNELPRESSURE_TYPE) return;
	const AmChannelPressure		*cp = dynamic_cast<const AmChannelPressure*>(event);
	if (!cp) return;

	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor());
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else
		view->SetHighColor( AmPrefs().SelectedColor() );
	
	mPt1.x = mPt2.x = mMtc.TickToPixel(eventRange.start);

	mPt1.y = abs((int)(mPt2.y - (mScale * cp->Pressure())));
	if (mPt1.y < 0) mPt1.y = 0;
	// FIX: should do a lineArray thing!  Cool!
	view->StrokeLine(mPt1, mPt2);
}

void AmChannelPressureDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::OTHER_EVENT_OBS,
											range);						
	mSongRef.AddRangeObserver(this, AmNotifier::SIGNATURE_OBS, range);
}

/*************************************************************************
 * _AM-CHANNEL-PRESSURE-TARGET
 *************************************************************************/
_AmChannelPressureTarget::_AmChannelPressureTarget(	AmTrackWinPropertiesI& trackWinProps,
													BView* view)
		: AmToolTarget(trackWinProps, view)
{
}

_AmChannelPressureTarget::~_AmChannelPressureTarget()
{
}

uint32 _AmChannelPressureTarget::Flags() const
{
	return DRAG_TIME_ONLY;
}

bool _AmChannelPressureTarget::IsInteresting(const AmEvent* event) const
{
	ArpASSERT(event);
	return event->Type() == event->CHANNELPRESSURE_TYPE;
}

bool _AmChannelPressureTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::CHANNELPRESSURE_TYPE;
}

BRect _AmChannelPressureTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + 1;
	r.bottom = r.bottom -1;
	return r;
}

AmTime _AmChannelPressureTarget::EventAtFudge() const
{
	/* My fudge is three pixels.
	 */
	return mMtc.PixelToTick(3);
}

AmEvent* _AmChannelPressureTarget::InterestingEventAt(	const AmTrack* track,
														const AmPhraseEvent& topPhrase,
														const AmPhrase& phrase,
														AmTime time,
														float y,
														int32* extraData) const
{
	AmNode*		n = phrase.ChainHeadNode(time);
	if (!n) return 0;
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
				else if ( llabs(time - eventRange.start) < llabs(time - topPhrase.EventRange(closest).start) )
					closest = n->Event();
			}
		} else {
			if (beenInRange) return closest;
		}
		n = n->next;
	}
	return closest;
}

AmEvent* _AmChannelPressureTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	ArpASSERT(mView);
	return new AmChannelPressure(	ChannelPressureFromPixel(y),
									time);
}

int32 _AmChannelPressureTarget::MoveYValueFromPixel(float y) const
{
	return ChannelPressureFromPixel(y);
}

void _AmChannelPressureTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
												const AmEvent* event,
												AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmChannelPressure*		cp = dynamic_cast<const AmChannelPressure*>(event);
	if (cp) *y = cp->Pressure();
	else *y = 0;
}

void _AmChannelPressureTarget::GetMoveDelta(BPoint origin, BPoint where,
											AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	/* I don't want the normal bounded transformation so that the move
	 * tool can 'flatten' all the events as you drag to the edge of the view.
	 */
	int32		originVal = BoundlessChannelPressureFromPixel(origin.y);
	int32		whereVal = BoundlessChannelPressureFromPixel(where.y);
	*yDelta = (whereVal - originVal);
}

void _AmChannelPressureTarget::SetMove(	AmPhraseEvent& topPhrase,
										AmEvent* event,
										AmTime originalX, int32 originalY,
										AmTime deltaX, int32 deltaY,
										uint32 flags)
{
	ArpASSERT(event);
	AmChannelPressure*		cpEvent = dynamic_cast<AmChannelPressure*>(event);
	if (!cpEvent) return;
//	if (flags&TRANSFORM_X) {

	AmTime			newStart = originalX + deltaX;
	if (newStart < 0) newStart = 0;
	if (newStart != topPhrase.EventRange(event).start)
		topPhrase.SetEventStartTime(cpEvent, newStart);

//	}
//	if (flags&TRANSFORM_Y) {
		int32			newVal = originalY + deltaY;
		if (newVal > 127) newVal = 127;
		else if (newVal < 0) newVal = 0;
		cpEvent->SetPressure(newVal);
//	}
}

void _AmChannelPressureTarget::GetOriginalTransform(AmEvent* event,
													am_trans_params& params) const
{
	AmChannelPressure*	cp = dynamic_cast<AmChannelPressure*>(event);
	if (!cp) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = cp->Pressure();
		params.original_y = cp->Pressure();
	}
}

void _AmChannelPressureTarget::GetDeltaTransform(	BPoint origin, BPoint where,
													am_trans_params& params) const
{
	params.delta_x = 0;
	params.delta_y = (int32) (where.y - origin.y);
}

uint32 _AmChannelPressureTarget::SetTransform(const AmTrack& track,
											AmPhraseEvent& topPhrase,
											AmEvent* event,
											const am_trans_params& params)
{
	AmChannelPressure*	cp = dynamic_cast<AmChannelPressure*>(event);
	if (!cp) return 0;
	int32			newValue = params.original_y - params.delta_y;
	if (newValue > 127) newValue = 127;
	if (newValue < 0  ) newValue = 0;
	cp->SetPressure(newValue);
	return 0;
}

uint32 _AmChannelPressureTarget::SetTransform(const AmTrack& track,
											AmPhraseEvent& topPhrase,
											AmEvent* event,
											BPoint where,
											const am_trans_params& params)
{
	AmChannelPressure*	cp = dynamic_cast<AmChannelPressure*>(event);
	if (!cp) return 0;
	if (params.flags&TRANSFORM_Y)
		cp->SetPressure( ChannelPressureFromPixel(where.y) );
	return 0;
}

void _AmChannelPressureTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	const AmTrack*		track = song->Track(TrackWinProperties().OrderedTrackAt(0).TrackId());
	if (!track) return;
	AmFilterHolderI*	output = track->Filter(OUTPUT_PIPELINE);
	if (!output) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	if (!event) return;

	AmNoteOn*	on = new AmNoteOn(64, 100, 0);
	if (!on) {
		event->DeleteChain();
		return;
	}
	on->SetNextFilter(output);
	event = on->MergeEvent(event);
	if (!event) return;
	AmEvent*	e = event;
	while ( e->NextEvent() ) e = e->NextEvent();
	if (e) on->SetEndTime( e->EndTime() );
	if (on->Duration() < PPQN / 8) on->SetEndTime( on->StartTime() + (PPQN / 8) );

	mPerformer.SetBPM(song->BPM() );
	mPerformer.Play(event);
}

uint8 _AmChannelPressureTarget::ChannelPressureFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	if( y < b.top ) return 127;
	else if( y > (b.bottom - 1) ) return 0;

	return (uchar) (127 - ((127 / (b.Height() -1 )) * y));
}

int32 _AmChannelPressureTarget::BoundlessChannelPressureFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	return (int32) (127 - ((127 / (b.Height() -1 )) * y));
}
