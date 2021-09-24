#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmKernel/AmNotifier.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmTool.h"
#include "Sequitur/SeqTempoViewFactory.h"

static const char*	TEMPO			= "Tempo";
static const char*	I_TOP_STR		= "top";	// A configuration value
/* The view is drawn as a series of bands, where each has a pixel height,
 * and a size.
 */
static const float	BAND_HEIGHT		= 20;
static const int32	BAND_START		= 0;
static const int32	BAND_SIZE		= 20;	// The number of BPMS of each band
static const int32	BAND_NUM		= 20;	// With a band size of 20, this is a max of 400 BPM	
static const float	END_CAP			= 10;	// Some whitespace at the top and bottom
static const float	TEMPO_HALF		= 3;	// Half the pixel height and width of a tempo event

/*************************************************************************
 * _AM-TEMPO-DATA-VIEW
 *************************************************************************/
class _AmTempoDataView : public AmTrackDataView
{
public:
	_AmTempoDataView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI& viewProp,
						TrackViewType viewType);
	virtual ~_AmTempoDataView();

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float* width, float* height);
	virtual	void ScrollTo(BPoint where);
	virtual void MessageReceived(BMessage*);
	virtual BMessage* ConfigurationData();

protected:
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);
	virtual void	DrawPrimaryEvents(BRect clip, BView* view, const AmTrack* track);

private:
	typedef AmTrackDataView		inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;
	float						mBandHeight;
	/* This is my initial y scroll value.  It's pulled out of the
	 * configuration supplied in the constructor.
	 */
	float						mInitialTop;
	
	void AddAsObserver();
};

/*************************************************************************
 * _AM-TEMPO-INFO-VIEW
 *************************************************************************/
class _AmTempoInfoView : public AmTrackInfoView
{
public:
	_AmTempoInfoView(BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual	void		GetPreferredSize(float* width, float* height);
		
protected:
	virtual BPopUpMenu*	NewPropertiesMenu() const	{ return 0; }
	virtual void		DrawOn(BRect clip, BView* view);

private:
	typedef AmTrackInfoView	inherited;
	float		mBandHeight;
};

/*************************************************************************
 * _TEMPO-BACKGROUND
 *************************************************************************/
class _TempoBackground : public ArpBackground
{
public:
	_TempoBackground(float bandHeight, rgb_color color)
			: mBandHeight(bandHeight), mColor(color) { }

protected:
	virtual void DrawOn(BView* view, BRect clip)
	{
		/* Starting at the bottom, draw a light line for each band.
		 */
		view->SetHighColor(mColor);
		for (float bottom = (mBandHeight * BAND_NUM) + END_CAP; bottom >= END_CAP; bottom -= mBandHeight) {
			view->StrokeLine( BPoint( clip.left, bottom ), BPoint( clip.right, bottom ) );
		}
	}

private:
	float		mBandHeight;
	rgb_color	mColor;
};

/*************************************************************************
 * _AM-TEMPO-TARGET
 * The piano roll view implementation of AmToolTarget
 *************************************************************************/
class _AmTempoTarget : public AmToolTarget
{
public:
	_AmTempoTarget(	AmTrackWinPropertiesI& trackWinProps,
					BView* view, float leftFudge, float rightFudge,
					float noteHeight);

	virtual uint32	Flags() const;
	virtual bool	IsInteresting(const AmEvent* event) const;
	virtual bool	IsInteresting(const BMessage* flatEvent) const;
	virtual BRect	RectFor(const AmEvent* event, AmRange eventRange) const;

	virtual AmEvent* InterestingEventAt(const AmTrack* track,
										const AmPhraseEvent& topPhrase,
										const AmPhrase& phrase,
										AmTime time,
										float y,
										int32* extraData) const;
	virtual bool	EventIntersects(const AmEvent* event,
									AmRange eventRange,
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

private:
	float		mBandHeight;
	
	float	PixelFromTempo(float tempo) const;
	uint32	TempoFromPixel(float y) const;
};

/*************************************************************************
 * SEQ-TEMPO-VIEW-FACTORY
 *************************************************************************/
SeqTempoViewFactory::SeqTempoViewFactory()
{
	SetSignature( "arp:Tempo" );
}

status_t SeqTempoViewFactory::GetPhraseRendererInfo(uint32 index,
													BString& outLabel,
													BString& outKey) const
{
	return B_ERROR;
}

status_t SeqTempoViewFactory::DataNameAt(uint32 index, TrackViewType type, BString& outName) const
{
	if (type == PRI_VIEW && index == 0) {
		outName << TEMPO;
		return B_OK;
	}
	return B_ERROR;
}

AmPhraseRendererI* SeqTempoViewFactory::NewPhraseRenderer(	const AmTimeConverter& mtc,
															const AmViewPropertyI& property) const
{
	return NULL;
}

BView* SeqTempoViewFactory::NewDataView(AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI* property,
										TrackViewType type)
{
	ArpVALIDATE(property, return NULL);
	return new _AmTempoDataView(BRect(0,0,0,0),
								songRef, trackWinProps,
								*property, type);
}

BView* SeqTempoViewFactory::NewInfoView(AmSongRef songRef,
										AmTrackWinPropertiesI& trackWinProps,
										const AmViewPropertyI* property,
										TrackViewType type)
{
	return new _AmTempoInfoView(BRect(0, 0, 0, 0),
								songRef, trackWinProps,
								property,
								type );
}

BView* SeqTempoViewFactory::NewPrefView(BRect f, BMessage* prefs,
										const BString& key)
{
	return 0;
}

// #pragma mark -

/*************************************************************************
 * _AM-TEMPO-DATA-VIEW
 *************************************************************************/
_AmTempoDataView::_AmTempoDataView(	BRect frame,
									AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI& viewProp,
									TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, "TempoData", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		  mMeasureBg(NULL),
		  mBandHeight(BAND_HEIGHT), mInitialTop( BAND_HEIGHT * (BAND_NUM / 2) )
{
	mCachedPrimaryTrack = mTrackWinProps.OrderedTrackAt(0);
	mTarget = new _AmTempoTarget(trackWinProps, this, AM_FUDGE_TO_EDGE, AM_FUDGE_TO_EDGE, mBandHeight);

	ArpBackground*	bg = new _TempoBackground(mBandHeight, Prefs().Color(AM_MEASURE_BEAT_C) );
	if (bg) AddBackground(bg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mCachedPrimaryTrack, mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
	
	const BMessage*	configuration = viewProp.Configuration();
	if (configuration && configuration->FindFloat( I_TOP_STR, &mInitialTop ) != B_OK )
		mInitialTop = 0;
}

_AmTempoDataView::~_AmTempoDataView()
{
	delete mTarget;
}

void _AmTempoDataView::AttachedToWindow()
{
	BScrollBar*		sb = ScrollBar( B_VERTICAL );
	if( sb  ) sb->SetValue( mInitialTop );
	inherited::AttachedToWindow();
	AddAsObserver();
}

void _AmTempoDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mCachedPrimaryTrack.RemoveObserverAll( this );
	mSongRef.RemoveObserverAll( this );
}

void _AmTempoDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	AddAsObserver();
}

void _AmTempoDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = (mBandHeight * BAND_NUM) + (END_CAP * 2);
}

void _AmTempoDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo( where );
	AddAsObserver();
}

void _AmTempoDataView::MessageReceived(BMessage* msg)
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
				mCachedPrimaryTrack.AddRangeObserver(this, AmNotifier::TEMPO_OBS, range);
				Invalidate();
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

BMessage* _AmTempoDataView::ConfigurationData()
{
	BMessage*	msg = new BMessage('null');
	if( !msg ) return 0;
	msg->AddFloat( I_TOP_STR, Bounds().top );
	return msg;
}

void _AmTempoDataView::DrawPrimaryEvents(BRect clip, BView* view, const AmTrack* track)
{
	/* HACK:  The drawing gets messed up because tempi have lines between
	 * them.  This will need to get straightened out later on (because CCs,
	 * pitch, etc will do the same kind of drawing), presumably by giving
	 * tempi duration (i.e. each tempo will be its start to its next's end).
	 * For now, I just draw all the tempo changes.
	 */ 
	clip.left = 0;
	inherited::DrawPrimaryEvents(clip, view, track);
}

void _AmTempoDataView::DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->TEMPOCHANGE_TYPE) return;

	/* First find connecting lines to the prev and next events
	 */
	float	prevLineY = -1;
	float	nextLineX = view->Bounds().right;
	if ( event->Parent() ) {
		AmPhrase*	parent = dynamic_cast<AmPhrase*>( event->Parent() );
		if (parent) {
			AmNode*		node = parent->FindNode( const_cast<AmEvent*>(event) );
			AmNode*		node2 = node ? node->prev : NULL;
			if (node2 && node2->Event()->Type() == event->TEMPOCHANGE_TYPE) {
				BRect	r(mTarget->RectFor(node2->Event(), topPhrase.EventRange(node2->Event())));
				prevLineY = r.top + TEMPO_HALF;
			}
			node2 = node ? node->next : NULL;
			if (node2 && node2->Event()->Type() == event->TEMPOCHANGE_TYPE) {
				BRect	r(mTarget->RectFor(node2->Event(), topPhrase.EventRange(node2->Event())));
				nextLineX = r.left + TEMPO_HALF;
			}
		}
	}

	// Draw the note
	BRect		rect( -1,-1,-1,-1 );
	rect = mTarget->RectFor(event, eventRange);
	if( (rect.left == -1)
			&& (rect.right == -1)
			&& (rect.top == -1)
			&& (rect.bottom == -1) )
		return;
	rgb_color	c;
	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		c = EventColor();
	else if (properties&ARPEVENT_SHADOW)
		c = AmPrefs().ShadowColor();
	else
		c = AmPrefs().SelectedColor();

	/* Draw a line from the y of the previous event to the center of this one
	 */
	view->SetHighColor(EventColor() );
	float		penSize = view->PenSize();
	view->SetPenSize(2);
	if (prevLineY != -1)
		view->StrokeLine(	BPoint(rect.left + TEMPO_HALF, prevLineY),
							BPoint(rect.left + TEMPO_HALF, rect.top + TEMPO_HALF) );
	/* Draw a line from the center of this event to the next one
	 */
	view->StrokeLine(	BPoint(rect.left + TEMPO_HALF, rect.top + TEMPO_HALF),
						BPoint(nextLineX, rect.top + TEMPO_HALF) );
	view->SetPenSize(penSize);

	/* Can't draw an event shorter than 2 pixels.
	 */
	if( rect.Width() < 2 ) rect.right = rect.left + 1;

	if( rect.Width() >= 2 ) {
		view->SetHighColor( c );
		view->FillRect( BRect( rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2 ) );
	}
	/* Draw an outline.
	 */
	view->SetHighColor(0, 0, 0);
	view->StrokeRect( rect );

	if (rect.Width() >= 2) {
		/* Lighten the top left edges
		 */	
		view->SetHighColor( tint_color(c, B_LIGHTEN_2_TINT) );
		view->StrokeLine( BPoint(rect.right - 1, rect.top + 1), BPoint(rect.left + 1, rect.top + 1) );
		view->StrokeLine( BPoint(rect.left + 1, rect.top + 2), BPoint(rect.left + 1, rect.bottom - 1) );
		/* Darken the bottom right edges
		 */
		view->SetHighColor( tint_color(c, B_DARKEN_2_TINT) );
		view->StrokeLine( BPoint(rect.right - 1, rect.top + 2), BPoint(rect.right - 1, rect.bottom - 1) );
		view->StrokeLine( BPoint(rect.right - 2, rect.bottom - 1), BPoint(rect.left + 2, rect.bottom - 1) );
	}
}

void _AmTempoDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mCachedPrimaryTrack.AddRangeObserver( this, AmNotifier::TEMPO_OBS, range );
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

// #pragma mark -

/*************************************************************************
 * _AM-TEMPO-INFO-VIEW
 *************************************************************************/
_AmTempoInfoView::_AmTempoInfoView(	BRect frame,
									AmSongRef songRef,
									AmTrackWinPropertiesI& trackWinProps,
									const AmViewPropertyI* property,
									TrackViewType viewType)
		: inherited(frame, "TempoInfo", songRef, trackWinProps, viewType),
		  mBandHeight(BAND_HEIGHT)
{
	mFactorySignature = property->Signature();
	mViewName = property->Name();

	ArpBackground*	bg = new _TempoBackground(mBandHeight,
								tint_color(Prefs().Color(AM_INFO_BG_C), B_DARKEN_1_TINT) );
	if (bg) AddBackground(bg);
}

void _AmTempoInfoView::GetPreferredSize(float* width, float* height)
{
	*width = 0;
	*height = (mBandHeight * BAND_NUM) + (END_CAP * 2);
}

void _AmTempoInfoView::DrawOn(BRect clip, BView* view)
{
	view->SetHighColor(mViewColor);
	view->FillRect( BRect(clip) );
	
	if (mHeadBackground) mHeadBackground->DrawAllOn(view, clip);

	PreDrawSliceOn(view, clip);

	view->SetHighColor(Prefs().Color(AM_DATA_FG_C) );
	view->SetLowColor(mViewColor);
	char	buf[16];
	int32	band = BAND_START;
	SetFontSize(8);
	for (float bottom = (mBandHeight * BAND_NUM) + END_CAP; bottom >= END_CAP; bottom -= mBandHeight) {
		sprintf(buf, "%ld", band);
		view->DrawString( buf, BPoint(20, bottom + 4) );
		band += BAND_SIZE;
	}

	PostDrawSliceOn(view, clip);
}

// #pragma mark -

/*************************************************************************
 * _AM-TEMPO-TARGET
 *************************************************************************/
_AmTempoTarget::_AmTempoTarget(	AmTrackWinPropertiesI& trackWinProps,
								BView* view, float leftFudge, float rightFudge,
								float noteHeight)
		: AmToolTarget(trackWinProps, view, leftFudge, rightFudge),
		  mBandHeight(noteHeight)
{
}

uint32 _AmTempoTarget::Flags() const
{
	return AmToolTarget::NO_CONTAINER;
}

bool _AmTempoTarget::IsInteresting(const AmEvent* event) const
{
	if (event->Type() == event->TEMPOCHANGE_TYPE) return true;
	return false;
}

bool _AmTempoTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::TEMPOCHANGE_TYPE;
}

BRect _AmTempoTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	if (event->Type() != event->TEMPOCHANGE_TYPE) return BRect( -1, -1, -1, -1 );
	// FIX: Definitely want to make these part of the class
	// so they aren't reconstructed all the time.
	const AmTempoChange*	tempo = dynamic_cast<const AmTempoChange*>(event);
	BRect					rect(-1,-1,-1,-1);
	float					top = PixelFromTempo( tempo->Tempo() );
	if (top < 0) return rect;
	float					left = mMtc.TickToPixel(eventRange.start);

	rect.Set(	left - TEMPO_HALF,	top - TEMPO_HALF,	// left, top
				left + TEMPO_HALF, top + TEMPO_HALF);	// right, bottom
	return rect;
}

AmEvent* _AmTempoTarget::InterestingEventAt(const AmTrack* track,
											const AmPhraseEvent& topPhrase,
											const AmPhrase& phrase,
											AmTime time,
											float y,
											int32* extraData) const
{
	AmNode*		n = phrase.ChainHeadNode(time);
	if (!n) return NULL;
	/* Since control changes are single pixels, it can be a bit tricky
	 * for users to hit exactly the right pixel.  Compensate for this
	 * by finding the closest control change within a given fudge factor.
	 */
	AmTime		fudge = mMtc.PixelToTick(TEMPO_HALF);
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

bool _AmTempoTarget::EventIntersects(	const AmEvent* event,
										AmRange eventRange,
										AmTime left, int32 top,
										AmTime right, int32 bottom) const
{
	ArpASSERT(event);
	const AmTempoChange*		tEvent = dynamic_cast<const AmTempoChange*>(event);
	if (!tEvent) return false;
	if ( (eventRange.start > right) || (eventRange.end < left) ) return false;
	if ( (tEvent->Tempo() < (float)top) || (tEvent->Tempo() > (float)bottom) ) return false;
	return true;
}

AmEvent* _AmTempoTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	return new AmTempoChange(TempoFromPixel(y), time);
}

int32 _AmTempoTarget::MoveYValueFromPixel(float y) const
{
	return TempoFromPixel(y);
}

void _AmTempoTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
									const AmEvent* event,
									AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmTempoChange*		tEvent = dynamic_cast<const AmTempoChange*>(event);
	if (tEvent) *y = int32(tEvent->Tempo() );
	else *y = 0;
}

void _AmTempoTarget::GetMoveDelta(	BPoint origin, BPoint where,
									AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	int32		originVal = TempoFromPixel(origin.y);
	int32		whereVal = TempoFromPixel(where.y);
	*yDelta = (whereVal - originVal);
}

void _AmTempoTarget::SetMove(	AmPhraseEvent& topPhrase,
								AmEvent* event,
								AmTime originalX, int32 originalY,
								AmTime deltaX, int32 deltaY,
								uint32 flags)
{
	ArpASSERT(event);
	AmTempoChange*		tEvent = dynamic_cast<AmTempoChange*>(event);
	if (!tEvent) return;
	if (!topPhrase.Phrase()) return;
	AmNode*				head = topPhrase.Phrase()->HeadNode();
	if (!head) return;

	/* Don't let users change the time of the first tempo event.
	 */
	if (head->Event() != event) {
		AmTime			newStart = originalX + deltaX;
		if (newStart < 0) newStart = 0;
		if (newStart != topPhrase.EventRange(event).start)
			topPhrase.SetEventStartTime(tEvent, newStart);
	}
	
//	if (flags&TRANSFORM_Y) {
		int32			newVal = originalY + deltaY;
		if (newVal > (int32)AM_TEMPO_MAX) newVal = AM_TEMPO_MAX;
		else if (newVal < (int32)AM_TEMPO_MIN) newVal = AM_TEMPO_MIN;
		tEvent->SetTempo( float(newVal) );
//	}
}

void _AmTempoTarget::GetOriginalTransform(	AmEvent* event,
											am_trans_params& params) const
{
	AmTempoChange*	tc = dynamic_cast<AmTempoChange*>(event);
	if (!tc) {
		params.original_x = 0;
		params.original_y = 0;
	} else {
		params.original_x = int32(tc->Tempo() );
		params.original_y = int32(tc->Tempo() );
	}
}

void _AmTempoTarget::GetDeltaTransform(	BPoint origin, BPoint where,
										am_trans_params& params) const
{
	params.delta_x = 0;
	params.delta_y = (int32) (where.y - origin.y);
}

uint32 _AmTempoTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									const am_trans_params& params)
{
	AmTempoChange*	tc = dynamic_cast<AmTempoChange*>(event);
	if (!tc) return 0;
	int32			newValue = params.original_y - params.delta_y;
	if (newValue > int32(AM_TEMPO_MAX)) newValue = AM_TEMPO_MAX;
	if (newValue < int32(AM_TEMPO_MIN)) newValue = AM_TEMPO_MIN;
	tc->SetTempo(newValue);
	return 0;
}

uint32 _AmTempoTarget::SetTransform(const AmTrack& track,
									AmPhraseEvent& topPhrase,
									AmEvent* event,
									BPoint where,
									const am_trans_params& params)
{
	AmTempoChange*	tc = dynamic_cast<AmTempoChange*>(event);
	if (!tc) return 0;
	if (params.flags&TRANSFORM_Y)
		tc->SetTempo( TempoFromPixel(where.y) );
	return 0;
}

float _AmTempoTarget::PixelFromTempo(float tempo) const
{
	float	pixelHeight = mBandHeight * BAND_NUM;
	float	tempoHeight = BAND_SIZE * BAND_NUM;
	float	convert = (tempo * pixelHeight) / tempoHeight;

	return ((mBandHeight * BAND_NUM) + END_CAP) - convert;
}

uint32 _AmTempoTarget::TempoFromPixel(float y) const
{
	float	bottom = (mBandHeight * BAND_NUM) + END_CAP;
	if (y >= bottom) return AM_TEMPO_MIN;

	y = bottom - y;
	float	pixelHeight = mBandHeight * BAND_NUM;
	float	tempoHeight = BAND_SIZE * BAND_NUM;

	return (uint32)((y * tempoHeight)/ pixelHeight);
}
