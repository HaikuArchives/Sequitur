/* AmProgramChangeView.cpp
 */
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "ArpKernel/ArpDebug.h"

#include "AmPublic/AmEvents.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewPropertyI.h"
#include "AmPublic/AmViewFactory.h"

#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPerformer.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

#include "AmStdFactory/AmProgramChangeView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const char*		PROGRAMCHANGEINFO_STR		= "ProgramChangeInfo";
static const char*		PROGRAMCHANGEDATA_STR		= "ProgramChangeData";
/* The default height of the program change view.
 */
static const float		I_ARP_PREFERREDHEIGHT		= 25;

/*************************************************************************
 * _AM-PROGRAM-TARGET
 * The tool API implementation for the program change view.
 *************************************************************************/
class _AmProgramTarget : public AmToolTarget
{
public:
	_AmProgramTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);
	~_AmProgramTarget();
	
	virtual bool		IsInteresting(const AmEvent* event) const;
	virtual bool		IsInteresting(const BMessage* flatEvent) const;
	virtual BRect		RectFor(const AmEvent* event, AmRange eventRange) const;
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
	AmPerformer					mPerformer;
	/* This is a fake program change that can be populated and sent to the
	 * EventWidth() method.  How this is done will probably change once I'm
	 * displaying banks, as well.
	 */
	mutable AmProgramChange*	mFakePc;
	/* This a temporary pointer to my current bank.  The bank is grabbed
	 * from the track, and NULL'd out once the function grabbing it has finished.
	 * If any function wants EventWidth() to find the width of the pc based
	 * on the actual name of the program, they need to make sure to populate
	 * this var before calling that method.
	 */
	mutable ArpCRef<AmBankI>	mBank;

	/* Answer the width of the supplied event, based on the current
	 * font in my view.
	 */
	float EventWidth(const AmProgramChange* pc) const;
};

/*************************************************************************
 * AM-PROGRAM-CHANGE-INFO-VIEW
 *************************************************************************/
AmProgramChangeInfoView::AmProgramChangeInfoView(	BRect frame,
													AmSongRef songRef,
													AmTrackWinPropertiesI& trackWinProps,
													const AmViewPropertyI* property,
													TrackViewType viewType)
		: AmTrackInfoView(frame, PROGRAMCHANGEINFO_STR, songRef, trackWinProps, viewType)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmProgramChangeInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmProgramChangeInfoView::DrawOn(BRect clip, BView* view)
{
	AmTrackInfoView::DrawOn(clip, view);

	float	bottom = Bounds().bottom;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString("Patch", BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

// #pragma mark -

/*************************************************************************
 * AM-PROGRAM-CHANGE-DATA-VIEW
 *************************************************************************/
AmProgramChangeDataView::AmProgramChangeDataView(	BRect frame,
													AmSongRef songRef,
													AmTrackWinPropertiesI& trackWinProps,
													const AmViewPropertyI& viewProp,
													TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, BRect(0,0,0,0), PROGRAMCHANGEDATA_STR,
				B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
		  mMeasureBg(NULL), mBank(NULL)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmProgramTarget(trackWinProps, this);
	
	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
}

AmProgramChangeDataView::~AmProgramChangeDataView()
{
}

void AmProgramChangeDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmProgramChangeDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll( this );
	mSongRef.RemoveObserverAll( this );
}

void AmProgramChangeDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	AddAsObserver();
}

void AmProgramChangeDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = I_ARP_PREFERREDHEIGHT;
	else *height = float(pref);
}

void AmProgramChangeDataView::MessageReceived(BMessage* msg)
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

void AmProgramChangeDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

static ArpCRef<AmBankI> get_bank(const AmTrack* track)
{
	if (!track) return NULL;
	AmFilterHolderI*		holder = track->Filter(DESTINATION_PIPELINE);
	if ( !holder || !holder->Filter() ) return NULL;
	ArpCRef<AmDeviceI>		device = holder->Filter()->Device();
	if (!device) return NULL;
	return device->Bank(uint32(0));
}

void AmProgramChangeDataView::PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	mBank = get_bank(track);
}

void AmProgramChangeDataView::PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	mBank = NULL;
}

static void get_program_label(ArpCRef<AmBankI> bank, const AmProgramChange* pc, BString& out)
{
	out << pc->ProgramNumber() + ( (bank) ? bank->FirstPatchNumber() : 0);
	BString		patchName;
	if (bank) patchName = bank->PatchName(pc->ProgramNumber() );
	if (patchName.Length() > 0) out << " - " << patchName.String();
}

void AmProgramChangeDataView::DrawEvent(BView* view, const AmPhraseEvent& topPhrase,
										const AmEvent* event, AmRange eventRange, int32 properties)
{
	const AmProgramChange*	pc = dynamic_cast<const AmProgramChange*>(event);
	if (!pc) return;

	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor() );
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else
		view->SetHighColor( AmPrefs().SelectedColor() );
	
	BPoint			pt( mTarget->TimeConverter().TickToPixel(eventRange.start),
						Bounds().bottom - 2 );
	/* If I can get a string for the patch, draw that.
	 */
	BString			str;
	get_program_label(mBank, pc, str);
	view->DrawString(str.String(), pt);
}

void AmProgramChangeDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver(	this,
											AmNotifier::OTHER_EVENT_OBS,
											range);
	mSongRef.AddRangeObserver(this, AmNotifier::SIGNATURE_OBS, range);
}

// #pragma mark -

/*************************************************************************
 * _AM-PROGRAM-TARGET
 *************************************************************************/
_AmProgramTarget::_AmProgramTarget(	AmTrackWinPropertiesI& trackWinProps,
									BView* view)
		: AmToolTarget(trackWinProps, view, 100, 100),
		  mFakePc(new AmProgramChange), mBank(NULL)
{
}

_AmProgramTarget::~_AmProgramTarget()
{
	if (mFakePc) mFakePc->Delete();
}

bool _AmProgramTarget::IsInteresting(const AmEvent* event) const
{
	assert( event );
	return event->Type() == event->PROGRAMCHANGE_TYPE;
}

bool _AmProgramTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::PROGRAMCHANGE_TYPE;
}

BRect _AmProgramTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	ArpASSERT(mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + EventWidth( dynamic_cast<const AmProgramChange*>(event) );
	r.bottom = r.bottom -1;
	return r;
}

AmEvent* _AmProgramTarget::InterestingEventAt(	const AmTrack* track,
												const AmPhraseEvent& topPhrase,
												const AmPhrase& phrase,
												AmTime time,
												float y,
												int32* extraData) const
{
	/* Because I grab my bank for this method, MAKE SURE to NULL it out
	 * before returning.
	 */
	mBank = get_bank(track);
	/* The program change view seeks from the back forwards.  This is
	 * because of the nature of the visual display of program changes --
	 * which a program change is a single point in time, it's name in
	 * the window might span several beats.  If the user clicks on
	 * multiple overlapping program changes, we want the 'top most one,'
	 * which is the one latest in time, to be what's activated.
	 */ 
	AmNode*		node = phrase.TailNode();
	BPoint		pt( mMtc.TickToPixel(time), y );
	while (node) {
		ArpASSERT(node->Event() != NULL);
		AmRange		eventRange = topPhrase.EventRange( node->Event() );
		if ( ( IsInteresting( node->Event() ) )
				&& (time >= eventRange.start) 
				&& RectFor(dynamic_cast<AmProgramChange*>(node->Event()), eventRange).Contains(pt) ) {
			mBank = NULL;
			return node->Event();
			}
		node = node->prev;
	}
	mBank = NULL;
	return NULL;
}

AmEvent* _AmProgramTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	/* FIX:  This needs to go to the device to get the first available program number.
	 */
	return new AmProgramChange(0, time);

}

int32 _AmProgramTarget::MoveYValueFromPixel(float y) const
{
	return 0;
}

void _AmProgramTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
										const AmEvent* event,
										AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	*y = 0;
}

void _AmProgramTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	*yDelta = 0;
}

void _AmProgramTarget::SetMove(	AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	if (!event) return;

	AmTime			newStart = originalX + deltaX;
	if (newStart < 0) newStart = 0;
	if (newStart != topPhrase.EventRange(event).start)
		topPhrase.SetEventStartTime(event, newStart);
}

void _AmProgramTarget::GetOriginalTransform(AmEvent* event,
											am_trans_params& params) const
{
	AmProgramChange*	pc = dynamic_cast<AmProgramChange*>(event);
	if (!pc) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = pc->ProgramNumber();
		params.original_y = pc->ProgramNumber();
	}
}

void _AmProgramTarget::GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const
{
	/* The number of pixels to traverse before I will change values.
	 */
	float	pixels = 3;
	params.delta_x = (int32) ( (where.x - origin.x) / pixels );
	params.delta_y = (int32) ( (where.y - origin.y) / pixels );
}

uint32 _AmProgramTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params)
{
	AmProgramChange*	pc = dynamic_cast<AmProgramChange*>(event);
	if (!pc) return 0;
	int32				newNumber = params.original_x + params.delta_x - params.delta_y;
	if (newNumber > 127) newNumber = 127;
	if (newNumber < 0  ) newNumber = 0;
	pc->SetProgramNumber(newNumber);
	return 0;
}

uint32 _AmProgramTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										BPoint where,
										const am_trans_params& params)
{
	return 0;
}

void _AmProgramTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	const AmTrack*		track = song->Track(TrackWinProperties().OrderedTrackAt(0).TrackId());
	if (!track) return;
	AmFilterHolderI*	output = track->Filter(OUTPUT_PIPELINE);
	if (!output) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	if (!event) return;

	AmTime		lastTime = 0;
	AmEvent*	e = event;
	while (e) {
		if (e->StartTime() > lastTime) lastTime = e->StartTime();
		e = e->NextEvent();
	}

	AmNoteOn*	on = new AmNoteOn(64, 127, lastTime + 1);
	if (on) {
		on->SetDuration(PPQN);
		on->SetNextFilter(output);
		event = event->MergeEvent(on);
	}

	mPerformer.SetBPM(song->BPM() );
	mPerformer.Play(event);
}

// Convenience function to find the width of the data we are working with.
float _AmProgramTarget::EventWidth(const AmProgramChange* pc) const
{
	if (!mView) return 0;
	if (!pc) return 0;

	BString			str;
	get_program_label(mBank, pc, str);
	return mView->StringWidth( str.String() );
}

