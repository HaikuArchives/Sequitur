/* AmPitchBendView.cpp
 */
#include <cmath>
#include <cstdio>
#include <cstring>
#include "ArpKernel/ArpDebug.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewFactory.h"

#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

#include "AmStdFactory/AmPitchBendView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"


#define I_ARP_PREFERREDHEIGHT		(50)
#define I_ARP_PITCH_ILLEGAL			(AM_PITCH_MAX + 1)

/*************************************************************************
 * AM-PITCH-BEND-INFO-VIEW
 * This class draws the info to the left of the dividing line in a track
 *************************************************************************/
AmPitchBendInfoView::AmPitchBendInfoView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI* property,
											TrackViewType viewType)
		: AmTrackInfoView(frame, STR_PITCHBENDINFO, songRef, trackWinProps, viewType)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmPitchBendInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmPitchBendInfoView::DrawOn(BRect clip, BView* view)
{
	AmTrackInfoView::DrawOn(clip, view);
	
	float		bottom = Bounds().bottom;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString( "Pitch", BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

// #pragma mark -

/*************************************************************************
 * _AM-PITCH-TARGET
 *************************************************************************/
class _AmPitchTarget : public AmToolTarget
{
public:
	_AmPitchTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);

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
	AmPerformer				mPerformer;

	int16		PitchValueFromPixel(float whereY) const;
};

/*************************************************************************
 * AM-PITCH-BEND-DATA-VIEW
 *************************************************************************/
AmPitchBendDataView::AmPitchBendDataView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI& viewProp,
											TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, STR_PITCHBENDDATA,
				B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmPitchTarget(trackWinProps, this);

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	bg = new ArpCenterBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color( AM_DATA_FG_C ) );
	if (bg) AddBackground(bg);
}

void AmPitchBendDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmPitchBendDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll(this);
	mSongRef.RemoveObserverAll(this);
}

void AmPitchBendDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	mMiddle = (Bounds().bottom - Bounds().top) / 2;
	mPt2.y = Bounds().bottom - 1;
	mScale = mMiddle / AM_PITCH_MAX;
	AddAsObserver();
}

void AmPitchBendDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmPitchBendDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

void AmPitchBendDataView::MessageReceived(BMessage* msg)
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
														AmNotifier::PITCH_OBS,
														range);
				Invalidate();
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmPitchBendDataView::DrawEvent(BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->PITCHBEND_TYPE) return;
	const AmPitchBend		*pb = dynamic_cast<const AmPitchBend*>(event);
	if (!pb) return;

	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor() );
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else if (properties&ARPEVENT_SELECTED)
		view->SetHighColor( AmPrefs().SelectedColor() );

	mPt1.x = mPt2.x = mMtc.TickToPixel(eventRange.start);
	mPt1.y = mMiddle;

	mPt2.y = mMiddle - (pb->Value() * mScale);
	// FIX: should do a lineArray thing!  Cool!
	view->StrokeLine(mPt1, mPt2);
}

void AmPitchBendDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::PITCH_OBS,
											range);
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

// #pragma mark -

/*************************************************************************
 * _AM-PITCH-TARGET
 *************************************************************************/
_AmPitchTarget::_AmPitchTarget(	AmTrackWinPropertiesI& trackWinProps,
								BView* view)
		: AmToolTarget(trackWinProps, view)
{
}

uint32 _AmPitchTarget::Flags() const
{
	return DRAG_TIME_ONLY;
}

bool _AmPitchTarget::IsInteresting(const AmEvent* event) const
{
	assert( event );
	return event->Type() == event->PITCHBEND_TYPE;
}

bool _AmPitchTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::PITCHBEND_TYPE;
}

BRect _AmPitchTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(event && mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + 1;
	r.bottom = r.bottom -1;
	return r;
}

AmTime _AmPitchTarget::EventAtFudge() const
{
	/* My fudge is three pixels.
	 */
	return mMtc.PixelToTick(3);
}

AmEvent* _AmPitchTarget::InterestingEventAt(const AmTrack* track,
											const AmPhraseEvent& topPhrase,
											const AmPhrase& phrase,
											AmTime time,
											float y,
											int32* extraData) const
{
	AmNode*		n = phrase.ChainHeadNode(time);
	if (!n) return NULL;
	/* Since pitch bends are single pixels, it can be a bit tricky
	 * for users to hit exactly the right pixel.  Compensate for this
	 * by finding the closest pitch bend within a given fudge factor,
	 * currently 3 pixels.
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
				if (!closest) closest = n->Event();
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

AmEvent* _AmPitchTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	ArpASSERT(mView);
	return new AmPitchBend( PitchValueFromPixel(y), time );
}

int32 _AmPitchTarget::MoveYValueFromPixel(float y) const
{
	return PitchValueFromPixel(y);
}

void _AmPitchTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
									const AmEvent* event,
									AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmPitchBend*		pbEvent = dynamic_cast<const AmPitchBend*>(event);
	if (pbEvent) *y = pbEvent->Value();
	else *y = 0;
}

void _AmPitchTarget::GetMoveDelta(	BPoint origin, BPoint where,
									AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	int32		originVal = PitchValueFromPixel(origin.y);
	int32		whereVal = PitchValueFromPixel(where.y);
	*yDelta = (whereVal - originVal);
}

void _AmPitchTarget::SetMove(	AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	AmPitchBend*		pbEvent = dynamic_cast<AmPitchBend*>(event);
	if (!pbEvent) return;
//	if (flags&TRANSFORM_X) {

	AmTime			newStart = originalX + deltaX;
	if (newStart < 0) newStart = 0;
	if (newStart != topPhrase.EventRange(event).start)
		topPhrase.SetEventStartTime(pbEvent, newStart);

//	}
//	if (flags&TRANSFORM_Y) {
		int32			newVal = originalY + deltaY;
		if (newVal > AM_PITCH_MAX) newVal = AM_PITCH_MAX;
		else if (newVal < AM_PITCH_MIN) newVal = AM_PITCH_MIN;
		pbEvent->SetValue(newVal);
//	}
}

void _AmPitchTarget::GetOriginalTransform(	AmEvent* event,
											am_trans_params& params) const
{
	AmPitchBend*	pb = dynamic_cast<AmPitchBend*>(event);
	if (!pb) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = pb->Value();
		params.original_y = pb->Value();
	}
}

void _AmPitchTarget::GetDeltaTransform(	BPoint origin, BPoint where,
										am_trans_params& params) const
{
	ArpASSERT(mView);
	float			middle = (mView->Bounds().bottom - mView->Bounds().top) / 2;
	/* Find a pitch value for the origin... */
	float 			originDist = fabs(middle - origin.y);
	int32			originPitch = (int32) ( (AM_PITCH_MAX / middle) * originDist );
	if (origin.y > middle) originPitch = 0 - originPitch;
	/* Find a pitch value for the where... */
	float 			whereDist = fabs(middle - where.y);
	int32			wherePitch = (int32) ( (AM_PITCH_MAX / middle) * whereDist );
	if (where.y > middle) wherePitch = 0 - wherePitch;

	params.delta_x = 0;
	params.delta_y = (int32) (originPitch - wherePitch);
}

uint32 _AmPitchTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									const am_trans_params& params)
{
	AmPitchBend*	pb = dynamic_cast<AmPitchBend*>(event);
	if (!pb) return 0;
	int32			newValue = params.original_y - params.delta_y;
	if (newValue < AM_PITCH_MIN) newValue = AM_PITCH_MIN;
	if (newValue > AM_PITCH_MAX ) newValue = AM_PITCH_MAX;
	pb->SetValue(newValue);
	return 0;
}

uint32 _AmPitchTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									BPoint where,
									const am_trans_params& params)
{
	AmPitchBend*	pb = dynamic_cast<AmPitchBend*>(event);
	if (!pb) return 0;
	if (params.flags&TRANSFORM_Y)
		pb->SetValue( PitchValueFromPixel(where.y) );
	return 0;
}

void _AmPitchTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
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

int16 _AmPitchTarget::PitchValueFromPixel(float whereY) const
{
	ArpASSERT( mView );
	float	middle = (mView->Bounds().bottom - mView->Bounds().top) / 2;

	float	dist = fabs(middle - whereY);
	int16	value = (int16)((AM_PITCH_MAX / middle) * dist);
	if (whereY > middle) value = 0 - value;
	return value;
}
