/* AmCommandView.cpp
 */
#include <assert.h>
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
#include "AmStdFactory/AmCommandView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const float	PREFERRED_HEIGHT	= 25;
static const uint8	INITIAL_CC			= 10;
static const char*	INFO_NAME			= "Command Info";
static const char*	DATA_NAME			= "Command Data";

static const int32	COMMAND_XDATA		= 0x00000001;
static const int32	VALUE_XDATA			= 0x00000002;

/*************************************************************************
 * _AM-COMMAND-TARGET
 * The tool API implementation for the sysex command view.
 *************************************************************************/
class _AmCommandTarget : public AmToolTarget
{
public:
	_AmCommandTarget(AmTrackWinPropertiesI& trackWinProps, BView* view);
	virtual ~_AmCommandTarget();

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
	AmPerformer		mPerformer;

	/* Given a y position, answer a control value.  This method assumes the mView
	 * is set.
	 */
	int32	SysExValueFromPixel(float y) const;
	int32	BoundlessSysExValueFromPixel(float y) const;
	BRect	RectForD(	const AmEvent* event, AmRange eventRange,
						ArpCRef<AmDeviceI> device) const;
	float	EventWidth(	const AmEvent* event,
						ArpCRef<AmDeviceI> device) const;
	BRect	CommandRectFor(	const AmEvent* event, AmRange eventRange,
							ArpCRef<AmDeviceI> device) const;
	void	SetValueTransform(	const AmTrack& track,
								AmPhraseEvent& topPhrase,
								AmEvent* event,
								const am_trans_params& params);
};

/*************************************************************************
 * AM-COMMAND-INFO-VIEW
 *************************************************************************/
AmCommandInfoView::AmCommandInfoView(	BRect frame,
										AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI* property,
										TrackViewType viewType)
		: inherited(frame, INFO_NAME, songRef, trackWinProps, viewType)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmCommandInfoView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(mFactorySignature.String(), mViewName.String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmCommandInfoView::DrawOn(BRect clip, BView* view)
{
	inherited::DrawOn(clip, view);

	float		bottom = Bounds().bottom;
	view->SetLowColor(mViewColor);
	view->SetHighColor( Prefs().Color(AM_DATA_FG_C) );
	view->DrawString( "Command", BPoint(2, bottom - 2) );
	view->StrokeLine( BPoint(clip.left, bottom), BPoint(clip.right, bottom) );
}

/*************************************************************************
 * AM-COMMAND-DATA-VIEW
 *************************************************************************/
AmCommandDataView::AmCommandDataView(	BRect frame,
										AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI& viewProp,
										TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, DATA_NAME,
					B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL), mDevice(NULL)
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmCommandTarget(trackWinProps, this);

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	bg = new ArpFloorBackground( this, Prefs().Color(AM_DATA_FG_C) );
	if (bg) AddBackground(bg);
}

AmCommandDataView::~AmCommandDataView()
{
	mDevice = NULL;
}

void AmCommandDataView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmCommandDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll(this);
	mSongRef.RemoveObserverAll(this);
}

void AmCommandDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	AddAsObserver();
}

void AmCommandDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	int32		pref;
	if (AmPrefs().GetFactoryInt32(FactorySignature().String(), ViewName().String(),
							AM_HEIGHT_PREF_STR, &pref) != B_OK)
		pref = AM_MIN_FAC_VIEW_HEIGHT - 1;
	if (pref < AM_MIN_FAC_VIEW_HEIGHT) *height = PREFERRED_HEIGHT;
	else *height = float(pref);
}

void AmCommandDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

void AmCommandDataView::MessageReceived(BMessage *msg)
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

void AmCommandDataView::PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	if (track) mDevice = track->Device();
}

void AmCommandDataView::PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track)
{
	mDevice = NULL;
}

static bool is_interesting(const AmEvent* event)
{
	if (event->Type() == event->SYSTEMEXCLUSIVE_TYPE) return true;
	if (event->Type() != event->PHRASE_TYPE) return false;
	const AmPhraseEvent*		pe = dynamic_cast<const AmPhraseEvent*>(event);
	if (!pe || !pe->Phrase()) return false;
	return pe->Phrase()->IncludesOnly(event->SYSTEMEXCLUSIVE_TYPE);
}

void AmCommandDataView::DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (!is_interesting(event)) return;

	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		view->SetHighColor(EventColor());
	else if (properties&ARPEVENT_SHADOW)
		view->SetHighColor( AmPrefs().ShadowColor() );
	else
		view->SetHighColor( AmPrefs().SelectedColor() );

	BPoint			pt( mTarget->TimeConverter().TickToPixel(eventRange.start),
						Bounds().bottom - 2 );
	BString			str;
	if (mDevice) mDevice->GetSysexCommandLabel(event, str);
	if (str.Length() < 1) str << "Sysex";
	view->DrawString(str.String(), pt);
}

void AmCommandDataView::AddAsObserver()
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
/* Bleh!  Awful hack to get the device from an event, because I was
 * rather shortsighted with my editing API.
 */
static const AmTrack* track_from_event(const AmEvent* event)
{
	ArpASSERT(event);
	const AmEventParent*	p = event->Parent();
	while (p) {
		const AmTrack*		track = dynamic_cast<const AmTrack*>(p);
		if (track) return track;
		p = p->Parent();
	}
	return NULL;
}

static ArpCRef<AmDeviceI> device_from_event(const AmEvent* event)
{
	const AmTrack*		track = track_from_event(event);
	if (!track) return NULL;
	return track->Device();
}

_AmCommandTarget::_AmCommandTarget(	AmTrackWinPropertiesI& trackWinProps,
									BView* view)
		: AmToolTarget(trackWinProps, view, 200, 200)
{
}

_AmCommandTarget::~_AmCommandTarget()
{
}

bool _AmCommandTarget::IsInteresting(const AmEvent* event) const
{
	ArpASSERT(event);
	return (is_interesting(event));
//	return event->Type() == event->SYSTEMEXCLUSIVE_TYPE;
}

bool _AmCommandTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::SYSTEMEXCLUSIVE_TYPE;
}

BRect _AmCommandTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	return RectForD(event, eventRange, device_from_event(event));
}

AmEvent* _AmCommandTarget::InterestingEventAt(	const AmTrack* track,
												const AmPhraseEvent& topPhrase,
												const AmPhrase& phrase,
												AmTime time,
												float y,
												int32* extraData) const
{
	/* Because I grab my bank for this method, MAKE SURE to NULL it out
	 * before returning.
	 */
	ArpCRef<AmDeviceI>	device = track->Device();
	/* The command view seeks from the back forwards.  This is
	 * because of the nature of the visual display of commands --
	 * while a command is a single point in time, its name in
	 * the window might span several beats.  If the user clicks on
	 * multiple overlapping commands, we want the 'top most one,'
	 * which is the one latest in time, to be what's activated.
	 */ 
	AmNode*		node = phrase.TailNode();
	BPoint		pt(mMtc.TickToPixel(time), y);
	while (node) {
		ArpASSERT(node->Event() != NULL);
		AmRange		eventRange = topPhrase.EventRange( node->Event() );
		if ( ( IsInteresting( node->Event() ) )
				&& (time >= eventRange.start) ) {
			BRect	wholeR = RectForD(node->Event(), eventRange, device);
			if (wholeR.Contains(pt)) {
				BRect	bankR = CommandRectFor(node->Event(), eventRange, device);
				if (bankR.Contains(pt)) *extraData = COMMAND_XDATA;
				else *extraData = VALUE_XDATA;
				device = NULL;
				return node->Event();
			}
		}
		node = node->prev;
	}
	device = NULL;
	return NULL;
}

AmEvent* _AmCommandTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	ArpASSERT(mView);
	ArpCRef<AmDeviceI>		device = track.Device();
	if (!device) return NULL;
	BString					key;
	if (device->GetSysexCommandKey(0, key) != B_OK) return NULL;
	AmEvent*				sx = device->NewSysexCommand(key);
	if (!sx) return NULL;
	sx->SetStartTime(time);
	return sx;
}

int32 _AmCommandTarget::MoveYValueFromPixel(float y) const
{
	return 0;
}

void _AmCommandTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
										const AmEvent* event,
										AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	*y = 0;
}

void _AmCommandTarget::GetMoveDelta(BPoint origin, BPoint where,
									AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	*yDelta = 0;
}

void _AmCommandTarget::SetMove(	AmPhraseEvent& topPhrase,
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

void _AmCommandTarget::GetOriginalTransform(AmEvent* event,
											am_trans_params& params) const
{
	ArpCRef<AmDeviceI>	device = device_from_event(event);
	params.original_x = 0;
	params.original_y = 0;
	if (!event || !device) return;
	if (params.extra_data == VALUE_XDATA) {
		int32			value;
		if (device->GetSysexCommandValue(event, &value) == B_OK) {
			params.original_x = value;
			params.original_y = value;
		}
	} else {
		uint32			index;
		if (device->GetSysexCommandInfo(event, &index, NULL) == B_OK) {
			params.original_x = index;
			params.original_y = index;
		}
	}
}

void _AmCommandTarget::GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const
{
	/* The number of pixels to traverse before I will change values.
	 */
	float	pixels = 3;
	params.delta_x = (int32) ((where.x - origin.x) / pixels);
	params.delta_y = (int32) ((where.y - origin.y) / pixels);
}

uint32 _AmCommandTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params)
{
	ArpASSERT(event);
	if (params.extra_data == VALUE_XDATA) {
		SetValueTransform(track, topPhrase, event, params);
		return 0;
	}
	
	ArpCRef<AmDeviceI>	device = track.Device();
	if (!device) return 0;
	int32			newValue = params.original_y - params.delta_y;
	uint32			index;
	if (device->GetSysexCommandInfo(event, &index, NULL) == B_OK) {
		if (newValue == int32(index)) return 0;
	}
	device->TransformSysexCommand(event, newValue);
	return AM_TRANS_NO_PLAY;
}

uint32 _AmCommandTarget::SetTransform(	const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										BPoint where,
										const am_trans_params& params)
{
	return 0;
}

void _AmCommandTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	const AmTrack*		track = song->Track(TrackWinProperties().OrderedTrackAt(0).TrackId());
	if (!track) return;
	AmFilterHolderI*	output = track->Filter(OUTPUT_PIPELINE);
	if (!output) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	if (!event) return;

	AmNoteOn*	on = new AmNoteOn(64, 100, event->StartTime() + 1);
	if (!on) {
		event->DeleteChain();
		return;
	}
	on->SetNextFilter(output);
	event->MergeEvent(on);
	event = event->HeadEvent();
	if (!event) return;
	AmEvent*	e = event;
	while ( e->NextEvent() ) e = e->NextEvent();
	if (e) on->SetEndTime( e->EndTime() );
	if (on->Duration() < PPQN / 8) on->SetEndTime( on->StartTime() + (PPQN / 8) );

	mPerformer.SetBPM(song->BPM() );
	mPerformer.Play(event);
}

int32 _AmCommandTarget::SysExValueFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	if (y < b.top) return 127;
	else if (y > (b.bottom - 1) ) return 0;

	return (int32) (127 - ((127 / (b.Height() -1 )) * y));
}

int32 _AmCommandTarget::BoundlessSysExValueFromPixel(float y) const
{
	ArpASSERT(mView);
	if (!mView) return 0;
	BRect			b = mView->Bounds();
	return (int32) (127 - ((127 / (b.Height() -1 )) * y));
}

BRect _AmCommandTarget::RectForD(	const AmEvent* event, AmRange eventRange,
									ArpCRef<AmDeviceI> device) const
{
	ArpASSERT(mView);
	BRect	r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	r.right = r.left + EventWidth(event, device);
	r.bottom = r.bottom -1;
	return r;
}

float _AmCommandTarget::EventWidth(	const AmEvent* event,
									ArpCRef<AmDeviceI> device) const
{
	if (!mView) return 0;
	if (!event) return 0;
	/* If I can get a string for the patch, draw that.
	 */
	BString				str;
	if (device) device->GetSysexCommandLabel(event, str);
	if (str.Length() < 1) str << "Command";
	return mView->StringWidth( str.String() );
}

BRect _AmCommandTarget::CommandRectFor(	const AmEvent* event, AmRange eventRange,
										ArpCRef<AmDeviceI> device) const
{
	ArpASSERT(mView && event);
	BRect				r = mView->Bounds();
	r.left = mMtc.TickToPixel(eventRange.start);
	
	BString				str;
	if (device) device->GetSysexCommandLabel(event, str, true, false);
	if (str.Length() < 1) str << "Command";
	float				w = mView->StringWidth( str.String() );
	
	r.right = r.left + w;
	r.bottom = r.bottom -1;
	return r;
}

void _AmCommandTarget::SetValueTransform(	const AmTrack& track,
											AmPhraseEvent& topPhrase,
											AmEvent* event,
											const am_trans_params& params)
{
	ArpASSERT(event);
	ArpCRef<AmDeviceI>	device = track.Device();
	if (!device) return;
	int32			newValue = params.original_y - params.delta_y;
	device->SetSysexCommandValue(event, newValue);
}

