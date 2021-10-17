/* AmQuantizeView.cpp
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

#include "AmStdFactory/AmQuantizeView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"


static const char*	STR_QUANTIZEINFO	= "PitchBendInfo";
static const char*	STR_QUANTIZEDATA	= "PitchBendData";

#define I_ARP_PREFERREDHEIGHT		(50)

static float get_middle(const BRect& bounds);

/*************************************************************************
 * AM-QUANTIZE-INFO-VIEW
 *************************************************************************/
AmQuantizeInfoView::AmQuantizeInfoView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI* property,
											TrackViewType viewType)
		: AmTrackInfoView(frame, STR_QUANTIZEINFO, songRef, trackWinProps, viewType)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmQuantizeInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmQuantizeInfoView::DrawOn(BRect clip, BView* view)
{
	AmTrackInfoView::DrawOn(clip, view);
	
	float		bottom = Bounds().bottom;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString( "Quantize", BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

/*************************************************************************
 * _AM-QUANTIZE-TARGET
 *************************************************************************/
class _AmQuantizeTarget : public AmToolTarget
{
public:
	_AmQuantizeTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);

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

	AmTime		OffsetFromPixel(float whereY);
};

// #pragma mark -

/*************************************************************************
 * AM-PITCH-BEND-DATA-VIEW
 *************************************************************************/
AmQuantizeDataView::AmQuantizeDataView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI& viewProp,
											TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, STR_QUANTIZEDATA,
				B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL), mCachedGridMult(0), mCachedGridVal(0),
		  mCachedGridDiv(0), mCachedGridTime(0)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	trackWinProps.GetSplitGridTime(&mCachedGridMult, &mCachedGridVal, &mCachedGridDiv);
	mCachedGridTime = trackWinProps.GridTime();
	mTarget = new _AmQuantizeTarget(trackWinProps, this);

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	bg = new ArpCenterBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color( AM_DATA_FG_C ) );
	if (bg) AddBackground(bg);
}

void AmQuantizeDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmQuantizeDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll(this);
	mSongRef.RemoveObserverAll(this);
}

void AmQuantizeDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	mMiddle = get_middle(Bounds());
	mPt2.y = Bounds().bottom - 1;
	mScale = mMiddle / (mTrackWinProps.GridTime() / 2);
	AddAsObserver();
}

void AmQuantizeDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmQuantizeDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo(where);
	AddAsObserver();
}

void AmQuantizeDataView::MessageReceived(BMessage* msg)
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

void AmQuantizeDataView::PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	inherited::PreDrawEventsOn(clip, view, track);
	mTrackWinProps.GetSplitGridTime(&mCachedGridMult, &mCachedGridVal, &mCachedGridDiv);
	mCachedGridTime = mTrackWinProps.GridTime();
}

static inline AmTime quantize(AmTime inTime, AmTime fullTime)
{
	const int64 t = ((int64)inTime);
	return (t-(t%fullTime));
}

void AmQuantizeDataView::DrawEvent(BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->NOTEON_TYPE) return;

	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor() );
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else if (properties&ARPEVENT_SELECTED)
		view->SetHighColor( AmPrefs().SelectedColor() );

	AmTime			qTime = quantize(eventRange.start, mCachedGridTime);
	AmTime			r = eventRange.start - qTime;
	AmTime			half = mCachedGridTime / 2;

	mPt1.y = mMiddle;
	double			scale = mMiddle / double(half);
	if (r <= half) {
		mPt1.x = mPt2.x = mMtc.TickToPixel(qTime);
		mPt2.y = mMiddle - (r * scale);
	} else {
		mPt1.x = mPt2.x = mMtc.TickToPixel(qTime + mCachedGridTime);
		mPt2.y = mMiddle + mMiddle - ((r - half) * scale);
	}

	// FIX: should do a lineArray thing!  Cool!
	view->StrokeLine(mPt1, mPt2);
}

void AmQuantizeDataView::AddAsObserver()
{
	BRect		b = Bounds();
	/* Fudge: Increase the watch range by a measure.  This is because I
	 * will draw events at a different time than they might actually exist
	 * (i.e. the quantize time).  The correct thing would be to look at
	 * the current grid time and add that to the range, and readd myself
	 * as an observer whenever the grid changes, but I'm not notified about
	 * grid changes, and the grid currently can't be much larger than this,
	 * so no big deal.
	 */
	AmTime		left = mMtc.PixelToTick(b.left) - (PPQN * 4);
	if (left < 0) left = 0;
	AmRange		range(left, mMtc.PixelToTick(b.right) + (PPQN * 4));
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::NOTE_OBS,
											range);
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

// #pragma mark -

/*************************************************************************
 * _AM-QUANTIZE-TARGET
 *************************************************************************/
_AmQuantizeTarget::_AmQuantizeTarget(	AmTrackWinPropertiesI& trackWinProps,
								BView* view)
		: AmToolTarget(trackWinProps, view, 100, 100)
{
}

uint32 _AmQuantizeTarget::Flags() const
{
	return DRAG_TIME_ONLY;
}

bool _AmQuantizeTarget::IsInteresting(const AmEvent* event) const
{
	assert( event );
	return event->Type() == event->NOTEON_TYPE;
}

bool _AmQuantizeTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::NOTEON_TYPE;
}

BRect _AmQuantizeTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(event && mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + 1;
	r.bottom = r.bottom -1;
	return r;
}

AmTime _AmQuantizeTarget::EventAtFudge() const
{
	/* My fudge is three pixels.
	 */
	return mMtc.PixelToTick(3);
}

AmEvent* _AmQuantizeTarget::InterestingEventAt(const AmTrack* track,
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

AmEvent* _AmQuantizeTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	return NULL;
}

int32 _AmQuantizeTarget::MoveYValueFromPixel(float y) const
{
	return 0;
//	return PitchValueFromPixel(y);
}

void _AmQuantizeTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
										const AmEvent* event,
										AmTime* x, int32* y) const
{
	*x = 0;
	*y = 0;
#if 0
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmPitchBend*		pbEvent = dynamic_cast<const AmPitchBend*>(event);
	if (pbEvent) *y = pbEvent->Value();
	else *y = 0;
#endif
}

void _AmQuantizeTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	*xDelta = 0;
	*yDelta = 0;
#if 0
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	int32		originVal = PitchValueFromPixel(origin.y);
	int32		whereVal = PitchValueFromPixel(where.y);
	*yDelta = (whereVal - originVal);
#endif
}

void _AmQuantizeTarget::SetMove(AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
#if 0
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
		if (newVal > I_ARP_PITCH_MAX) newVal = I_ARP_PITCH_MAX;
		else if (newVal < I_ARP_PITCH_MIN) newVal = I_ARP_PITCH_MIN;
		pbEvent->SetValue(newVal);
//	}
#endif
}

void _AmQuantizeTarget::GetOriginalTransform(	AmEvent* event,
												am_trans_params& params) const
{
#if 0
	AmNoteOn*	no = dynamic_cast<AmNoteOn*>(event);
	if (!no) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = pb->Value();
		params.original_y = pb->Value();
	}
#endif
}

void _AmQuantizeTarget::GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const
{
	ArpASSERT(mView);
#if 0
	float			middle = (mView->Bounds().bottom - mView->Bounds().top) / 2;
	/* Find a pitch value for the origin... */
	float 			originDist = fabs(middle - origin.y);
	int32			originPitch = (int32) ( (I_ARP_PITCH_MAX / middle) * originDist );
	if (origin.y > middle) originPitch = 0 - originPitch;
	/* Find a pitch value for the where... */
	float 			whereDist = fabs(middle - where.y);
	int32			wherePitch = (int32) ( (I_ARP_PITCH_MAX / middle) * whereDist );
	if (where.y > middle) wherePitch = 0 - wherePitch;

	params.delta_x = 0;
	params.delta_y = (int32) (originPitch - wherePitch);
#endif
}

uint32 _AmQuantizeTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									const am_trans_params& params)
{
#if 0
	AmPitchBend*	pb = dynamic_cast<AmPitchBend*>(event);
	if (!pb) return 0;
	int32			newValue = params.original_y - params.delta_y;
	if (newValue < I_ARP_PITCH_MIN) newValue = I_ARP_PITCH_MIN;
	if (newValue > I_ARP_PITCH_MAX ) newValue = I_ARP_PITCH_MAX;
	pb->SetValue(newValue);
#endif
	return 0;
}

uint32 _AmQuantizeTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									BPoint where,
									const am_trans_params& params)
{
	AmNoteOn*	no = dynamic_cast<AmNoteOn*>(event);
	if (!no) return 0;
	if (params.flags&TRANSFORM_Y) {
		AmTime		t = OffsetFromPixel(where.y);
//printf("Y: %f, Offset: %lld\n", where.y, t);
		AmTime		s = event->StartTime();
		no->SetStartTime(s + t);
//		no->SetValue( PitchValueFromPixel(where.y) );
	}
	return 0;
}

void _AmQuantizeTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
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

AmTime _AmQuantizeTarget::OffsetFromPixel(float whereY)
{
	ArpASSERT(mView);
	float	middle = get_middle(mView->Bounds());

	if (whereY < middle) {
		AmTime		grid = TrackWinProperties().GridTime();
		AmTime		half = grid / 2;
		double		scale = half / double(middle);
		AmTime		t = AmTime((middle - whereY) * scale);
		ArpASSERT(t >= 0 && t <= half);
		if (t < 0) return 0;
		if (t > half) return half;
		return t;
	} else if (whereY > middle) {
		AmTime		grid = TrackWinProperties().GridTime();
		AmTime		half = grid / 2;
		double		scale = half / double(middle);
		AmTime		t = AmTime((whereY - middle) * scale);
		ArpASSERT(t >= 0 && t <= half);
		if (t < 0) return 0;
		if (t > half) return -half;
		return -t;
	} else return 0;
}

#if 0
	int16		PitchValueFromPixel(float whereY) const;
int16 _AmQuantizeTarget::PitchValueFromPixel(float whereY) const
{
	ArpASSERT( mView );
	float	middle = get_middle(mView->Bounds());

	// FIX: 8,000 isn't the limit, find out what it is	
	float	dist = fabs(middle - whereY);
	int16	value = (int16)((8000 / middle) * dist);
	if (whereY > middle) value = 0 - value;
	return value;
}
#endif

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
static float get_middle(const BRect& bounds)
{
	return (bounds.bottom - bounds.top) / 2;
}
