/* AmPianoRollView.cpp
 */
#include <stdio.h>
#include <math.h>
#include <interface/ColorTools.h>
#include <interface/Bitmap.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/ScrollBar.h>
#include <interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpLineArrayCache.h"
#include "ArpViewsPublic/ArpViewDefs.h"

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

#include "AmStdFactory/AmPianoRollGenerator.h"
#include "AmStdFactory/AmPianoRollView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"

static const float			DEFAULT_NOTE_HEIGHT	= 7;
/* These are shared with the hybrid view, so users can switch back
 * and forth between these views and be in the same place.
 */
static const char*			TOP_NOTE_STR		= "top_note";
static const char*			NOTE_Y_STR			= "note_y";

static const uint32			HIGHLIGHT_NOTE_MSG	= 'ihln';

static float				zoom_y_to_note_height(float zoomY);
static inline float			pixel_from_note(uchar note, float noteHeight);
static inline uchar			note_from_pixel(float y, float noteHeight);

/*************************************************************************
 * _AM-PIANO-ROLL-TARGET
 * The piano roll view implementation of AmToolTarget
 *************************************************************************/
class _AmPianoRollTarget : public AmToolTarget
{
public:
	_AmPianoRollTarget(	AmTrackWinPropertiesI& trackWinProps,
						BView* view, float leftFudge, float rightFudge,
						float noteHeight);

	void				SetNoteHeight(float noteHeight);

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
	virtual void Stop();

private:
	AmPerformer	mPerformer;
	
	/* The pixel height of the notes.
	 */
	float		mNoteHeight;
	
	/* Answer the pixel location of the supplied MIDI note value
	 */
	int32	BoundlessNoteFromPixel(float y) const;
};

/*************************************************************************
 * _APR-BACKGROUND
 *************************************************************************/
class _AprBackground : public ArpBackground
{
public:
	_AprBackground(float noteHeight)	{ mNoteHeight = noteHeight; }

	void SetNoteHeight(float noteHeight)
	{
		mNoteHeight = noteHeight;
	}
	
protected:
	virtual void DrawOn(BView* view, BRect clip)
	{
		float		y = 0 - 1 - (mNoteHeight * 4);
		float		top;
		view->SetHighColor( Prefs().Color(AM_MEASURE_BEAT_C) );
		for(int8 octave = 8; octave >= -2; octave--) {
			top = y + (mNoteHeight * 1) + 1;
			view->FillRect( BRect(clip.left, top, clip.right, top + mNoteHeight - 1) );
			top = y + (mNoteHeight * 3) + 1;
			view->FillRect( BRect(clip.left, top, clip.right, top + mNoteHeight - 1) );
			top = y + (mNoteHeight * 5) + 1;
			view->FillRect( BRect(clip.left, top, clip.right, top + mNoteHeight - 1) );

			top = y + (mNoteHeight * 8) + 1;
			view->FillRect( BRect(clip.left, top, clip.right, top + mNoteHeight - 1) );
			top = y + (mNoteHeight * 10) + 1;
			view->FillRect( BRect(clip.left, top, clip.right, top + mNoteHeight - 1) );

			y += (mNoteHeight * 12);
			if (y > clip.bottom) return;
		}
	}

private:
	float	mNoteHeight;
};

/*************************************************************************
 * AM-PIANO-ROLL-DATA-VIEW
 *************************************************************************/
AmPianoRollDataView::AmPianoRollDataView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI& viewProp,
											TrackViewType viewType)
		: inherited(songRef, trackWinProps, viewProp, viewType, frame, "PianoRollData", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		  mNoteHeight(DEFAULT_NOTE_HEIGHT), mHighlightedNote(-1), mInitialTop(0),
		  mAprBg(NULL), mMeasureBg(NULL)
{
	uchar				initialNote = 74;
	const BMessage*		configuration = viewProp.Configuration();
	if (configuration) {
		if (configuration->FindFloat(NOTE_Y_STR, &mNoteHeight) != B_OK) mNoteHeight = DEFAULT_NOTE_HEIGHT;
		int32			i;
	 	if (configuration->FindInt32(TOP_NOTE_STR, &i) == B_OK) {
	 		initialNote = i;
		}
	}
	mInitialTop = pixel_from_note(initialNote, mNoteHeight);
	
	mTarget = new _AmPianoRollTarget(trackWinProps, this, 0, 1, mNoteHeight);

	ArpBackground*		bg = new AmPropGridBackground(trackWinProps);
	if (bg) AddBackground(bg);
	mAprBg = new _AprBackground(mNoteHeight);
	if (mAprBg) AddBackground(mAprBg);
	mMeasureBg = new AmTrackMeasureBackground(mSongRef, mTrackWinProps.OrderedTrackAt(0), mMtc);
	if (mMeasureBg) AddBackground(mMeasureBg);
}

AmPianoRollDataView::~AmPianoRollDataView()
{
	delete mTarget;
}

void AmPianoRollDataView::AttachedToWindow()
{
	BScrollBar*		sb = ScrollBar(B_VERTICAL);
	if (sb) sb->SetValue(mInitialTop);
	inherited::AttachedToWindow();
	AddAsObserver();
}

void AmPianoRollDataView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mSongRef.RemoveObserverAll(this);
}

void AmPianoRollDataView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	AddAsObserver();
}

void AmPianoRollDataView::GetPreferredSize(float *width, float *height)
{
	*width = 0;
	*height = (mNoteHeight * 128);
}

void AmPianoRollDataView::MessageReceived(BMessage* msg)
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
		case 'zoom': {
			float	valueY;
			if (msg->FindFloat("y_value", &valueY) == B_OK) {
				SetNoteHeight(zoom_y_to_note_height(valueY) );
			}
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmPianoRollDataView::MouseMoved(	BPoint where,
										uint32 code,
										const BMessage* msg)
{
	inherited::MouseMoved(where, code, msg);
	int32		note = -1;
	BRect		b(Bounds() );
	if (b.Contains(where) ) note = note_from_pixel(where.y, mNoteHeight);
	if (note != mHighlightedNote || (note < 0 && mHighlightedNote >= 0) ) {
		mHighlightedNote = note;
		BMessage	m(HIGHLIGHT_NOTE_MSG);
		m.AddInt32("note", mHighlightedNote);
		mTrackWinProps.PostMessageToInfoView(m, this);

	}
}

void AmPianoRollDataView::ScrollTo(BPoint where)
{
	inherited::ScrollTo(where);
	AddAsObserver();
}

BMessage* AmPianoRollDataView::ConfigurationData()
{
	BMessage*	msg = new BMessage('null');
	if (!msg) return NULL;
	msg->AddInt32(TOP_NOTE_STR, note_from_pixel(Bounds().top, mNoteHeight) );
	msg->AddFloat(NOTE_Y_STR, mNoteHeight);
	return msg;
}

void AmPianoRollDataView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range(mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );
	mSongRef.AddRangeObserver( this, AmNotifier::NOTE_OBS, range );
	mSongRef.AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, range );
}

void AmPianoRollDataView::DrawEvent(BView* view, const AmPhraseEvent& topPhrase,
									const AmEvent* event, AmRange eventRange, int32 properties)
{
	if (event->Type() != event->NOTEON_TYPE) return;
	rgb_color	c;
	uint8		vel = (dynamic_cast<const AmNoteOn*>(event))->Velocity();
	// Set the properties for the note
	if (properties&ARPEVENT_PRIMARY)
		c = EventColor(vel);
	else if (properties&ARPEVENT_ORDERED) {
		c = EventColor(vel);
		c.alpha = (uint8)(mOrderedSaturation * 255);
	} else if (properties&ARPEVENT_SHADOW) {
		c = AmPrefs().ShadowColor(vel);
		c.alpha = (uint8)(mShadowSaturation * 255);
	} else {
		c = AmPrefs().SelectedColor(vel);
	}
	
	// Draw the note
	BRect		rect( -1,-1,-1,-1 );
	rect = mTarget->RectFor(event, eventRange);
	if( (rect.left == -1)
			&& (rect.right == -1)
			&& (rect.top == -1)
			&& (rect.bottom == -1) )
		return;
	/* Can't draw an event shorter than 2 pixels.
	 */
	if( rect.Width() < 2 ) rect.right = rect.left + 1;

	if( rect.Width() >= 2 ) {
		view->SetHighColor( c );
		view->FillRect( BRect( rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2 ) );
	}
	/* Draw an outline.
	 */
	if (properties&ARPEVENT_SHADOW && mShadowSaturation < 1)
		view->SetHighColor(127 - vel, 127 - vel, 127 - vel, (uint8)(mShadowSaturation*255) );
	else if (properties&ARPEVENT_ORDERED && mOrderedSaturation < 1)
		view->SetHighColor(127 - vel, 127 - vel, 127 - vel, (uint8)(mOrderedSaturation*255) );
	else view->SetHighColor(127 - vel, 127 - vel, 127 - vel);
	view->StrokeRect(rect);

	if( rect.Width() >= 2 ) {
		/* Lighten the top left edges
		 */	
		if (properties&ARPEVENT_SHADOW && mShadowSaturation < 1) {
			rgb_color	c2 = tint_color(c, B_LIGHTEN_2_TINT);
			c2.alpha = (uint8)(mShadowSaturation * 255);
			view->SetHighColor(c2);
		} else if (properties&ARPEVENT_ORDERED && mOrderedSaturation < 1) {
			rgb_color	c2 = tint_color(c, B_LIGHTEN_2_TINT);
			c2.alpha = (uint8)(mOrderedSaturation * 255);
			view->SetHighColor(c2);
		} else view->SetHighColor( tint_color(c, B_LIGHTEN_2_TINT) );
		view->StrokeLine( BPoint(rect.right - 1, rect.top + 1), BPoint(rect.left + 1, rect.top + 1) );
		view->StrokeLine( BPoint(rect.left + 1, rect.top + 2), BPoint(rect.left + 1, rect.bottom - 1) );
		/* Darken the bottom right edges
		 */
		if( properties&ARPEVENT_SHADOW && mShadowSaturation < 1 ) {
			rgb_color	c2 = tint_color(c, B_DARKEN_2_TINT);
			c2.alpha = (uint8)(mShadowSaturation*255);
			view->SetHighColor(c2);
		} else if( properties&ARPEVENT_ORDERED && mOrderedSaturation < 1 ) {
			rgb_color	c2 = tint_color(c, B_DARKEN_2_TINT);
			c2.alpha = (uint8)(mOrderedSaturation*255);
			view->SetHighColor(c2);
		} else view->SetHighColor( tint_color(c, B_DARKEN_2_TINT) );
		view->StrokeLine( BPoint(rect.right - 1, rect.top + 2), BPoint(rect.right - 1, rect.bottom - 1) );
		view->StrokeLine( BPoint(rect.right - 2, rect.bottom - 1), BPoint(rect.left + 2, rect.bottom - 1) );
	}
}

void AmPianoRollDataView::SetNoteHeight(float noteHeight)
{
	mNoteHeight = noteHeight;
	_AmPianoRollTarget*	target = dynamic_cast<_AmPianoRollTarget*>( mTarget );
	if (target) target->SetNoteHeight(noteHeight);
	if (mAprBg) mAprBg->SetNoteHeight(noteHeight);
	Invalidate();
	BScrollBar*		sb = ScrollBar(B_VERTICAL);
	if (sb) arp_setup_vertical_scroll_bar(sb, this);
}

// #pragma mark -

/*************************************************************************
 * AM-PIANO-ROLL-INFO-VIEW
 *************************************************************************/
AmPianoRollInfoView::AmPianoRollInfoView(	BRect frame,
											AmSongRef songRef,
											AmTrackWinPropertiesI& trackWinProps,
											const AmViewPropertyI* property,
											TrackViewType viewType)
		: inherited(frame, "PianoRollInfo", songRef, trackWinProps, viewType),
		  mNoteHeight(DEFAULT_NOTE_HEIGHT), mHighlightNote(-1)
{
	const BMessage* config = property->Configuration();
	if (config && config->FindFloat(NOTE_Y_STR, &mNoteHeight) != B_OK)
		mNoteHeight = DEFAULT_NOTE_HEIGHT;
	
	mFactorySignature = property->Signature();
	mViewName = property->Name();
}

void AmPianoRollInfoView::GetPreferredSize(float* width, float* height)
{
	*width = 0;
	*height = (mNoteHeight * 128);
}

void AmPianoRollInfoView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'zoom': {
			float	valueY;
			if (msg->FindFloat( "y_value", &valueY ) == B_OK) {
				mTrackWinProps.PostMessageToDataView(*msg, this);
				mNoteHeight = zoom_y_to_note_height(valueY);
				Invalidate();
			}
		} break;
		case HIGHLIGHT_NOTE_MSG: {
			int32		note = mHighlightNote;
			if (msg->FindInt32("note", &note) == B_OK) {
				if (note != mHighlightNote) {
					mHighlightNote = note;
					Invalidate();
				}
			}
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmPianoRollInfoView::DrawOn(BRect clip, BView* view)
{
	inherited::DrawOn(clip, view);
	if (!Window()) return;
	AmPianoRollGenerator	gen(Bounds().Width() );
	const BBitmap*	bm = gen.PianoRollView(mNoteHeight);
	if (!bm) return;

	float	top = 0 - (mNoteHeight * 4);
	float	h = bm->Bounds().Height();
	view->SetFontSize(8);
	view->SetHighColor(0, 0, 0);
	char	buf[8];
	int32	lowNote = 120, highNote = 131, steps = 12;
	for (int8 octave = 8; octave >= -2; octave--) {
//		if (top >
		view->DrawBitmap(bm, BPoint(0, top) );
		/* Draw the highlight note, if it's in this octave.
		 */
		if (mHighlightNote >= lowNote && mHighlightNote <= highNote) {
			uint8		key = mHighlightNote % 12;
			BRect		hB(0, top, bm->Bounds().Width(), top + bm->Bounds().Height() );
			rgb_color	hC = Prefs().Color(AM_SONG_SELECTION_C);
			/* With the highlighted key rendered, now I need to redraw
			 * any keys that were partially obliterated by it.
			 */
			rgb_color	bC;
			bC.red = bC.green = bC.blue = 85;
			bC.alpha = 255;
			rgb_color	wC = Prefs().Color(AM_INFO_BG_C);
			/* This is all somewhat ugly because we have to do a bit extra --
			 * whenever a sharp is drawn, it alpha-draws on top of the white
			 * keys, which will excessively blacken the shadow if it's alpha
			 * drawing over a white key that's already had a sharp drawn over it.
			 * What I need to do is chunk the piano into two halves -- the two
			 * halves that have connecting black keys -- and draw the entire half
			 * of whatever key I'm drawing.
			 */
			if (key <= 4) {
				if (key != 0) gen.RenderKey(view, 0, wC, hB); else gen.RenderKey(view, 0, hC, hB);
				if (key != 2) gen.RenderKey(view, 2, wC, hB); else gen.RenderKey(view, 2, hC, hB);
				if (key != 4) gen.RenderKey(view, 4, wC, hB); else gen.RenderKey(view, 4, hC, hB);
				if (key != 1) gen.RenderKey(view, 1, bC, hB); else gen.RenderKey(view, 1, hC, hB);
				if (key != 3) gen.RenderKey(view, 3, bC, hB); else gen.RenderKey(view, 3, hC, hB);
			}
			if (key >= 5 && key <= 11) {
				if (key != 5) gen.RenderKey(view, 5, wC, hB); else gen.RenderKey(view, 5, hC, hB);
				if (key != 7) gen.RenderKey(view, 7, wC, hB); else gen.RenderKey(view, 7, hC, hB);
				if (key != 9) gen.RenderKey(view, 9, wC, hB); else gen.RenderKey(view, 9, hC, hB);
				if (key != 11) gen.RenderKey(view, 11, wC, hB); else gen.RenderKey(view, 11, hC, hB);
				if (key != 6) gen.RenderKey(view, 6, bC, hB); else gen.RenderKey(view, 6, hC, hB);
				if (key != 8) gen.RenderKey(view, 8, bC, hB); else gen.RenderKey(view, 8, hC, hB);
				if (key != 10) gen.RenderKey(view, 10, bC, hB); else gen.RenderKey(view, 10, hC, hB);
			}
		}
		top += h;
		/* Draw the label for the octave.
		 */
		sprintf(buf, "C%d", octave);
		view->DrawString(buf, BPoint(41, top - 4) );
		if (top > clip.bottom) return;
		lowNote -= steps;
		highNote -= steps;
	}
}

// #pragma mark -

/*************************************************************************
 * _AM-PIANO-ROLL-TARGET
 *************************************************************************/
_AmPianoRollTarget::_AmPianoRollTarget(	AmTrackWinPropertiesI& trackWinProps,
										BView* view, float leftFudge, float rightFudge,
										float noteHeight)
		: AmToolTarget(trackWinProps, view, leftFudge, rightFudge),
		  mNoteHeight(noteHeight)
{
}

void _AmPianoRollTarget::SetNoteHeight(float noteHeight)
{
	mNoteHeight = noteHeight;
}

bool _AmPianoRollTarget::IsInteresting(const AmEvent* event) const
{
	if (event->Type() == event->NOTEON_TYPE) return true;
	return false;
}

bool _AmPianoRollTarget::IsInteresting(const BMessage* flatEvent) const
{
	assert( flatEvent );
	int32	type;
	if ( flatEvent->FindInt32( "type", &type ) != B_OK ) return false;
	return type == AmEvent::NOTEON_TYPE;
}

BRect _AmPianoRollTarget::RectFor(const AmEvent* event, AmRange eventRange) const
{
	if (event->Type() != event->NOTEON_TYPE) return BRect( -1, -1, -1, -1 );
	// FIX: Definitely want to make these part of the class
	// so they aren't reconstructed all the time.
	const AmNoteOn*	noteOn = dynamic_cast<const AmNoteOn*>(event);
	BRect			rect(-1,-1,-1,-1);
	float			noteT;

	if( (noteT = pixel_from_note(noteOn->Note(), mNoteHeight)) < 0 ) return rect;
	rect.Set(mMtc.TickToPixel(eventRange.start),	// left
			noteT,									// top
			mMtc.TickToPixel(eventRange.end),		// right
			noteT + mNoteHeight -1);				// bottom

	return rect;
}

AmEvent* _AmPianoRollTarget::InterestingEventAt(const AmTrack* track,
												const AmPhraseEvent& topPhrase,
												const AmPhrase& phrase,
												AmTime time,
												float y,
												int32* extraData) const
{
	AmNode*		node = phrase.HeadNode();
	AmNoteOn*	noteOn;
	AmRange		eventRange;
	
	while (node) {
		if ( IsInteresting( node->Event() ) ) {
			AmEvent*	event = node->Event();
			eventRange = topPhrase.EventRange(event);
			if( (time >= eventRange.start) && (time <= eventRange.end) ) {
				noteOn = dynamic_cast<AmNoteOn*>(event);
				if( noteOn->Note() == note_from_pixel(y, mNoteHeight) )
					return noteOn;
			}
		}
		node = node->next;
	}
	return 0;
}

bool _AmPianoRollTarget::EventIntersects(	const AmEvent* event, AmRange eventRange,
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

AmEvent* _AmPianoRollTarget::NewEvent(const AmTrack& track, AmTime time, float y)
{
	AmNoteOn*	no = new AmNoteOn(note_from_pixel(y, mNoteHeight), TrackWinProperties().Velocity(), time);
	if (!no) return NULL;
	AmTime		grid = TrackWinProperties().GridTime();
//	AmTime		duration = grid - (grid / 16);
	AmTime		duration = grid - 1;
	if (duration <= 0) duration = PPQN;
	no->SetDuration(duration);
	return no;
}

int32 _AmPianoRollTarget::MoveYValueFromPixel(float y) const
{
	return note_from_pixel(y, mNoteHeight);
}

void _AmPianoRollTarget::GetMoveValues(	const AmPhraseEvent& topPhrase,
										const AmEvent* event,
										AmTime* x, int32* y) const
{
	ArpASSERT(event);
	*x = topPhrase.EventRange(event).start;
	const AmNoteOn*		no = dynamic_cast<const AmNoteOn*>(event);
	if (no) *y = no->Note();
	else *y = 0;
}

void _AmPianoRollTarget::GetMoveDelta(	BPoint origin, BPoint where,
										AmTime* xDelta, int32* yDelta) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	*xDelta = (whereTime - originTime);

	int32		originNote = BoundlessNoteFromPixel(origin.y);
	int32		whereNote = BoundlessNoteFromPixel(where.y);
	*yDelta = (whereNote - originNote);
}

void _AmPianoRollTarget::SetMove(	AmPhraseEvent& topPhrase,
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

void _AmPianoRollTarget::GetOriginalTransform(	AmEvent* event,
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

void _AmPianoRollTarget::GetDeltaTransform(	BPoint origin, BPoint where,
											am_trans_params& params) const
{
	AmTime		originTime = mMtc.PixelToTick(origin.x);
	AmTime		whereTime = mMtc.PixelToTick(where.x);
	params.delta_x = (whereTime - originTime);
	params.delta_y = (int32) (where.y - origin.y);
}

uint32 _AmPianoRollTarget::SetTransform(const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										const am_trans_params& params)
{
	AmNoteOn*			noteOn = dynamic_cast<AmNoteOn*>(event);
	if (!noteOn) return 0;
	if (params.flags&TRANSFORM_X && (params.delta_x != 0)) {
		int32			newEnd = params.original_x + params.delta_x;
		if (newEnd < 1) newEnd = 1;
		topPhrase.SetEventEndTime(noteOn, topPhrase.EventRange(noteOn).start + newEnd);
	}
	if (params.flags&TRANSFORM_Y) {
		int32			newVelocity = params.original_y - params.delta_y;
		if (newVelocity > 127) newVelocity = 127;
		if (newVelocity < 0) newVelocity = 0;
		noteOn->SetVelocity(newVelocity);
	}
	return 0;
}

uint32 _AmPianoRollTarget::SetTransform(const AmTrack& track,
										AmPhraseEvent& topPhrase,
										AmEvent* event,
										BPoint where,
										const am_trans_params& params)
{
	return 0;
}

void _AmPianoRollTarget::Perform(const AmSong* song, const AmSelectionsI* selections)
{
	if (!song || !selections) return;
	AmEvent*	event = selections->AsPlaybackList(song);
	mPerformer.SetBPM(song->BPM() );
	if (event) mPerformer.Play(event);
}

void _AmPianoRollTarget::Stop()
{
	mPerformer.Stop();
}

int32 _AmPianoRollTarget::BoundlessNoteFromPixel(float y) const
{
	return (int32)(127 - ( (y - mNoteHeight + 1) / mNoteHeight ));
}

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
static float zoom_y_to_note_height(float zoomY)
{
	float	ans = zoomY * DEFAULT_NOTE_HEIGHT;
	if (ans <= 4) return 4;
	else return floor(ans);
}

static inline float pixel_from_note(uchar note, float noteHeight)
{
	float	inverted = 127 - note;
	return inverted * noteHeight;
}

static inline uchar note_from_pixel(float y, float noteHeight)
{
	float	inverted = 127 - ( (y - noteHeight + 1) / noteHeight );
	if (inverted > 127) return 127;
	if (inverted < 0) return 0;
	return (uchar)inverted;
}
