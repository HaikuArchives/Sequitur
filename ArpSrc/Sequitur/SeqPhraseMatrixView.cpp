/* SeqPhraseMatrixView.cpp
 */
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/Window.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpViews/ArpMultiScrollBar.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPhraseRendererI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "Sequitur/SeqMeasureControl.h"
#include "Sequitur/SeqPhraseMatrixView.h"
#include "Sequitur/SeqPhrasePropertyWindow.h"
#include "Sequitur/SeqSongToolTarget.h"
#include "Sequitur/SeqSongWindow.h"
#include "Sequitur/SequiturDefs.h"

static const float		MUTE_SHADE				= -0.15;
static const float		BACKDROP_MUTE_SHADE		= -0.10;

static const uint32		SHOW_POPUP_MSG			= 'prmp';
static const uint32		PROPERTIES_MSG			= 'pprp';
static const BBitmap*	gPhraseBg				= NULL;

static const char*		WHERE_STR				= "where";

/*************************************************************************
 * _SEQ-PHRASE-TOOL-TARGET
*************************************************************************/
class _SeqPhraseToolTarget : public SeqSongToolTarget
{
public:
	/* The default 0 is provided as a convenience in certain situations,
	 * but be aware that until you set a valid view and time converter
	 * this class will not function.
	 */
	_SeqPhraseToolTarget(SeqSongWinPropertiesI* win = NULL,
						BView* view = NULL,
						const AmTimeConverter* mtc = NULL);

	/*---------------------------------------------------------
	 * DRAWING
	 *---------------------------------------------------------*/
	virtual void			ShowDragMark(AmTime time, BPoint where, const BMessage* dragMessage);
	virtual void			ClearDragMark();
	
private:
	typedef SeqSongToolTarget		inherited;
};

/*************************************************************************
 * SEQ-PHRASE-MATRIX-VIEW
*************************************************************************/
SeqPhraseMatrixView::SeqPhraseMatrixView(	BRect frame,
											AmSongRef songRef,
											const AmTimeConverter& mtc)
		: inherited(frame,
					"phrase_matrix",
					B_FOLLOW_ALL,
					B_WILL_DRAW | B_FRAME_EVENTS),
		  mSongRef(songRef), mMtc(mtc), mMeasureView(NULL),
		  mTool(songRef, mtc),
		  mDrawMuted(false), mCurLabelH(0), mSongPosition(-1),
		  mDownTime(-1), mPopUpRunner(NULL), mCachedEndTime(0),
		  mVsb(NULL), mHsb(NULL)
{
	if (!gPhraseBg) gPhraseBg = ImageManager().FindBitmap(AM_PHRASE_BG_STR);
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseMatrixView::SeqPhraseMatrixView() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) FillTrackMetrics(song);
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
}

SeqPhraseMatrixView::~SeqPhraseMatrixView()
{
	if ( mPhrasePropWin.IsValid() ) mPhrasePropWin.SendMessage(B_QUIT_REQUESTED);
	StopPopUpTimer();
}

void SeqPhraseMatrixView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	AddAsObserver();
	SetViewColor(B_TRANSPARENT_COLOR);
	if (Parent() && mMeasureView) {
		BRect		f = Parent()->Frame();
		mMeasureView->SetLeftIndent(f.left);
		BRect		b = Window()->Bounds();
		mMeasureView->SetRightIndent(b.Width() - f.right);
	}
}

void SeqPhraseMatrixView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mSongRef.RemoveObserverAll(this);
}

void SeqPhraseMatrixView::Draw(BRect clip)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>( Window() );
	if (cache) into = cache->StartDrawing(this, clip);
	
	DrawOn(clip, into);
	if (cache) cache->FinishDrawing(into);
}

void SeqPhraseMatrixView::FrameMoved(BPoint new_position)
{
	inherited::FrameMoved(new_position);
	if (mMeasureView)
		mMeasureView->SetLeftIndent(new_position.x);
}

/* This is a hack necessary because sometimes this view gets bogus FrameResize()
 * messages.  Not sure what's happening exactly, but fairly frequently I receive
 * a correct FrameResized() view followed by one that reports the Window()'s frame
 * to be 140 pixels shorter then the immediately preceding call.  Checking the
 * frame of the child views seems to bypass the bug.
 */
static float furthest_right(BWindow* window)
{
	float	right = 0;
	for (BView* child = window->ChildAt(0); child; child = child->NextSibling() ) {
		BRect	f = child->Frame();
		if (f.right > right) right = f.right;
	}
	return right;
}

void SeqPhraseMatrixView::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized(new_width, new_height);
	AddAsObserver();
	if (mVsb) SetVsbSteps();
	SetupScrollBars(true, true);
	if (Parent() && Window() && mMeasureView && Window()->Lock() ) {
		BRect		f = Parent()->Frame();
		float		topW = furthest_right( Window() );
		mMeasureView->SetRightIndent(topW - f.right);
		Window()->Unlock();
	}
}

void SeqPhraseMatrixView::MessageReceived(BMessage* msg)
{
	if (TrackMessageReceived(msg) ) return;

	SeqSongWinPropertiesI*		win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	_SeqPhraseToolTarget*	target;
	if (win && (target = new _SeqPhraseToolTarget(win, this, &mMtc)) ) {
		track_id	trackId = CurrentTrackId();
		bool		handled = mTool.HandleMessage(msg, trackId, target);
		delete target;
		if (handled) return;
	}

	switch (msg->what) {
		case AmSong::END_TIME_CHANGE_OBS:
			AmTime		endTime;
			if(find_time(*msg, "end_time", &endTime ) == B_OK) {
				mCachedEndTime = endTime;
				SetupScrollBars(true, false);
			}
			break;
		case AmSong::TRACK_CHANGE_OBS:
			TrackChangeReceived(msg);
			break;
		case AmTrack::MODE_CHANGE_OBS: {
			BRect			invalid = arp_invalid_rect();
			track_id		tid;
			BRect			b(Bounds() );
			for (uint32 k = 0; msg->FindPointer(SZ_TRACK_ID, k, & tid) == B_OK; k++) {
				_SeqTrackMetric*	metric = TrackMetric(tid);
				if (metric) {
					BRect	r(b.left, metric->mTop, b.right, metric->mBottom);
					invalid = arp_merge_rects(invalid, r);
				}
			}
			if (arp_is_valid_rect(invalid) ) Invalidate(invalid);
		} break;
		case SHOW_POPUP_MSG:
			if (mDownTime >= 0) {
				StopPopUpTimer();
				ShowPopUp();
				mDownTime = -1;
			}
			break;
		case PROPERTIES_MSG: {
			BPoint		where;
			if (msg->FindPoint(WHERE_STR, &where) == B_OK)
				ShowProperties(where);
		} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void SeqPhraseMatrixView::MouseDown(BPoint where)
{
	inherited::MouseDown(where);
	mDownTime = -1;
	mDownPt = where;
	StopPopUpTimer();

	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	SeqSongWinPropertiesI*		win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	_SeqPhraseToolTarget*	target;
	if (win && (target = new _SeqPhraseToolTarget(win, this, &mMtc)) ) {
		mTool.MouseDown(target, where);
		delete target;
	}

	uint32		buttons;
	GetMouse(&where, &buttons, false);
	if (buttons&B_SECONDARY_MOUSE_BUTTON) {
		bigtime_t	doubleClickTime;
		get_click_speed(&doubleClickTime);
//		doubleClickTime *= 2;
		StartPopUpTimer(BMessage(SHOW_POPUP_MSG), doubleClickTime);
		mDownTime = system_time();
	}
}

void SeqPhraseMatrixView::MouseMoved(BPoint where,
									uint32 code,
									const BMessage* dragMessage)
{
	inherited::MouseMoved(where, code, dragMessage);
	/* If the user moves the mouse, that automatically wipes
	 * out any chance they had of the popup opening automatically.
	 */
	if (mDownPt != where) StopPopUpTimer();

	SeqSongWinPropertiesI*		win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	_SeqPhraseToolTarget*	target;
	if( win && (target = new _SeqPhraseToolTarget(win, this, &mMtc)) ) {
		mTool.MouseMoved(target, where, code, dragMessage);
		delete target;
	}
}

void SeqPhraseMatrixView::MouseUp(BPoint where)
{
	inherited::MouseUp(where);
	SeqSongWinPropertiesI*		win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	_SeqPhraseToolTarget*		target;
	if( win && (target = new _SeqPhraseToolTarget(win, this, &mMtc)) ) {
		mTool.MouseUp(target, where);
		delete target;
	}
}

void SeqPhraseMatrixView::ScrollTo(BPoint where)
{
	inherited::ScrollTo(where);
	if (mMeasureView) mMeasureView->ScrollTo(where);
	AddAsObserver();
}

void SeqPhraseMatrixView::SetHorizontalScrollBar(BScrollBar* sb)
{
	mHsb = sb;
}

void SeqPhraseMatrixView::SetVerticalScrollBar(ArpMultiScrollBar* sb)
{
	mVsb = sb;
}

void SeqPhraseMatrixView::SetMeasureView(SeqMeasureControl* measureView)
{
	mMeasureView = measureView;
	if (Parent() && mMeasureView)
		mMeasureView->SetLeftIndent(Parent()->Frame().left);
}

void SeqPhraseMatrixView::SetupScrollBars(bool horizontal, bool vertical)
{
	if (horizontal && mHsb) {
		SetHsbRange();
		SetHsbSteps();
	}
	if (vertical && mVsb) {
		SetVsbRange();
		SetVsbSteps();
	}
}

void SeqPhraseMatrixView::InvalidateSelections(AmRange range, vector<track_id>& tracks)
{
	BRect		invalid(Bounds() );
	invalid.top = invalid.bottom = -1;
//	if (range.IsValid() ) {
//		invalid.left = mMtc.TickToPixel(range.start);
//		invalid.right = mMtc.TickToPixel(range.end);
//	}
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseMatrixView::InvalidateSelections() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = mSongRef.ReadLock();
	if (song) {
		for (uint32 k = 0; k < tracks.size(); k++) {
			float			top = TrackTop(tracks[k]);
			float			bottom = top;
			const AmTrack*	track = song->Track(tracks[k] );
			if (track) bottom = top + track->PhraseHeight();
			if (invalid.top < 0 || top < invalid.top) invalid.top = top;
			if (invalid.bottom < 0 || bottom > invalid.bottom) invalid.bottom = bottom;
		}
	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK
	if (invalid.top >= 0 && invalid.bottom >= 0) Invalidate(invalid);
}

void SeqPhraseMatrixView::DrawSongPosition(AmTime time)
{
	BRect		b( Bounds() );
	BRect		invalid(-1, b.top, -1, b.bottom);

	if (mSongPosition >= 0) invalid.left = invalid.right = mSongPosition;
//	float		delta = (mHsb) ? mHsb->Value() : 0;
	float		delta = 0;
	mSongPosition = mMtc.TickToPixel(time) - delta;
	if (mSongPosition >= 0) {
		if (invalid.left == -1 || mSongPosition < invalid.left)
			invalid.left = mSongPosition;
		if (invalid.right == -1 || mSongPosition > invalid.right)
			invalid.right = mSongPosition;
	}
	if (invalid.left >= 0 && invalid.right >= 0) Invalidate(invalid);
}

void SeqPhraseMatrixView::ShowDragMark(	AmTime time, BPoint where,
										const BMessage* dragMessage)
{
	ArpASSERT(dragMessage);
	if (!dragMessage) return;
	_SeqDragMetric		oldMetric(mDragMetric);

	mDragMetric.SetFrom(time, where, dragMessage, mTrackMetrics);
	if (oldMetric != mDragMetric) {
		AmRange		range = oldMetric.mRange + mDragMetric.mRange;
		if (range.IsValid() ) {
			BRect		dragClip( Bounds() );
			dragClip.left = mMtc.TickToPixel(range.start);	
			dragClip.right = mMtc.TickToPixel(range.end);	
			Invalidate(dragClip);
		}
	}
}

void SeqPhraseMatrixView::ClearDragMark()
{
	BRect		b(Bounds() );
	if (mDragMetric.mRange.start >= 0)
		b.left = mMtc.TickToPixel(mDragMetric.mRange.start);
	if (mDragMetric.mRange.end >= 0)
		b.right = mMtc.TickToPixel(mDragMetric.mRange.end);
		
	mDragMetric.Clear();
	Invalidate(b);
}

static AmPhraseRendererI* get_phrase_renderer(	const AmTrack* track,
												const AmTimeConverter& mtc)
{
	BString			sigStr;
	BString			nameStr;
	BString			problem;
	const AmViewPropertyI*	prop = track->Property(ARRANGE_VIEW);
	if (prop) {
		sigStr = prop->Signature();
		nameStr = prop->Name();
	}
	
	// If there was no factory specified, then use the default.
	if (sigStr.Length() < 1) {
		sigStr = DEFAULT_FACTORY_SIGNATURE;
		nameStr = "Default Note";
	}

	// If the factory doesn't exist, return nothing.
	AmViewFactory*	factory = AmGlobals().FactoryNamed(sigStr);
	if (!factory) return NULL;

	// Answer the renderer, if any.
	return factory->NewPhraseRenderer(mtc, *prop);
}

void SeqPhraseMatrixView::FillTrackMetrics(const AmSong* song)
{
	ArpASSERT(song);
	mTrackMetrics.resize(0);
	float		top = 0;
	const AmTrack*	track;
	for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
		float	bottom = top + track->PhraseHeight();
		AmPhraseRendererI*	renderer = get_phrase_renderer(track, mMtc);
		mTrackMetrics.push_back( _SeqTrackMetric(track->Id(), top, bottom, renderer) );
		top = bottom + 1;
	}
}

void SeqPhraseMatrixView::DrawOn(BRect clip, BView* view)
{
	int32		labelH;
	if (seq_get_int32_preference(PHRASE_LABEL_HEIGHT_PREF, &labelH) != B_OK) labelH = 8;
	if (labelH < 6 || labelH > 50) labelH = 8;
	view->SetFontSize(float(labelH) );
	mCurLabelH = arp_get_font_height(view);

	if (gPhraseBg) arp_tile_bitmap_on(view, clip, gPhraseBg, BPoint(0,0) );
	else {
		view->SetHighColor( Prefs().Color(AM_DATA_BACKDROP_C) );
		view->FillRect(clip);
	}		
	float				top = 0;
	const BRect			b( Bounds() );
	AmPhraseRendererI*	renderer = NULL;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseMatrixView::DrawOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) {
		const AmTrack*	track;
		for (uint32 k = 0; (track = song->Track(k)) != NULL; k++) {
			float	bottom = top + track->PhraseHeight();
			if (top > clip.bottom) break;
			BRect	r(clip.left, top, clip.right, bottom);
			r.left -= 3;
			if (r.left < b.left) r.left = b.left;
			r.right += 3;
			if (r.right > b.right) r.right = b.right;
			if (r.Intersects(clip)) {
				renderer = get_phrase_renderer(track, mMtc);
				DrawOn(r, view, track, renderer);
				delete renderer;
			}
			top = bottom + 1;
		}
	}
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK

	/* If the user is currently dragging a phrase over my area, draw that.
	 */
	if (mDragMetric.IsValid() ) {
		float		left = mMtc.TickToPixel(mDragMetric.mRange.start);
		float		right = mMtc.TickToPixel(mDragMetric.mRange.end);
		drawing_mode	mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
		view->SetHighColor(0, 0, 0, 125);
		for (uint32 k = 0; k < mDragMetric.mSrcIndexes.size(); k++) {
			int32	dest = (int32)mDragMetric.mSrcIndexes[k] + ((int32)mDragMetric.mDestIndex - (int32)mDragMetric.mSrcIndex);
			if (dest >= 0 && dest < (int32)mTrackMetrics.size() ) {
				view->FillRect( BRect(left, mTrackMetrics[dest].mTop,
								right, mTrackMetrics[dest].mBottom) );
			}
		}
		view->SetDrawingMode(mode);
	}

	if (!gPhraseBg && top <= clip.bottom) {
		view->SetHighColor( Prefs().Color(AM_DATA_BACKDROP_C) );
		view->FillRect( BRect(clip.left, top, clip.right, clip.bottom) );
	}
	/* Draw the song position.
	 */
	if (mSongPosition >= 0) {
		view->SetHighColor(0, 0, 0);
		view->StrokeLine( BPoint(mSongPosition, clip.top), BPoint(mSongPosition, clip.bottom) );
	}
}

void SeqPhraseMatrixView::AddAsObserver()
{
	BRect		b = Bounds();
	AmRange		range( mMtc.PixelToTick(b.left), mMtc.PixelToTick(b.right) );

	mSongRef.RemoveObserverAll(this);
	mSongRef.AddObserver(this, AmSong::END_TIME_CHANGE_OBS);
	mSongRef.AddObserver(this, AmSong::TRACK_CHANGE_OBS);
	mSongRef.AddObserver(this, AmTrack::MODE_CHANGE_OBS);

	mSongRef.AddRangeObserverAll(this, range);
//	mSongRef.AddRangeObserver(this, AmNotifier::SIGNATURE_OBS, range);
}

static rgb_color shade_color(rgb_color c1, float shade)
{
	rgb_color	c2 = c1;
	int32	p = (int32)(c1.red + (c1.red * shade));
	if (p < 0) p = 0; else if (p > 255) p = 255;
	c2.red = (uint8)p;

	p = (int32)(c1.green + (c1.green * shade));
	if (p < 0) p = 0; else if (p > 255) p = 255;
	c2.green = (uint8)p;

	p = (int32)(c1.blue + (c1.blue * shade));
	if (p < 0) p = 0; else if (p > 255) p = 255;
	c2.blue = (uint8)p;

	return c2;
}

void SeqPhraseMatrixView::DrawOn(	BRect clip, BView* view, const AmTrack* track,
									AmPhraseRendererI* renderer)
{
	mDrawMuted = false;
	if ( track->ModeFlags()&track->MUTE_MODE && !(track->ModeFlags()&track->SOLO_MODE) )
		mDrawMuted = true;
	
	rgb_color	bgC = Prefs().Color(AM_DATA_BACKDROP_C);
	if (mDrawMuted) bgC = shade_color(bgC, BACKDROP_MUTE_SHADE);
	view->SetHighColor(bgC);
	if (!gPhraseBg) view->FillRect(clip);

	float	pipeTop = clip.top + ((clip.bottom - clip.top) / 2) - 3;
	seq_draw_hrz_pipe(view, clip.left, pipeTop, clip.right);

	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	mLines.SetTarget(view);

	mSelectionRange.MakeInvalid();
	SeqSongWinPropertiesI*	win = dynamic_cast<SeqSongWinPropertiesI*>( Window() );
	bool		drawSelection = false;
	if (win) {
		SeqSongSelections*	selections = win->Selections();
		if (selections && selections->IncludesTrack( track->Id() ) ) {
			mSelectionRange = selections->TimeRange();
			drawSelection = true;
		}
	}
	
	if (drawSelection) {
		BRect				selectClip(clip);
		if (mSelectionRange.IsValid() ) {
			selectClip.left = mMtc.TickToPixel(mSelectionRange.start);
			selectClip.right = mMtc.TickToPixel(mSelectionRange.end);
		}
		rgb_color	bgC;
		if (win && win->IsRecording() ) bgC = Prefs().Color(AM_SONG_RECORD_SELECTION_C);
		else bgC = Prefs().Color(AM_SONG_SELECTION_C);
		view->SetHighColor(bgC);
		view->FillRect(selectClip);
	}
	view->SetDrawingMode(mode);
#if 0
	_AmBgMeasureBackground		bg( SongRef(), mTrackRef, *mMtc );
	bg.SetLeftIndent( mLeftIndent );
	bg.LockedDraw( view, clip, track->Signatures() );
#endif

	DrawTrack(clip, view, track, renderer);
}

void SeqPhraseMatrixView::DrawTrack(BRect clip,
									BView* view,
									const AmTrack* track,
									AmPhraseRendererI* renderer)
{
	/* Find the first phrase left of the window.
	 */
	const AmPhrase&	phrases = track->Phrases();
	AmTime		start = mMtc.PixelToTick(clip.left);
	AmTime		end = mMtc.PixelToTick(clip.right);
	AmNode*		n = phrases.FindNode(start, BACKWARDS_SEARCH);
	if (!n) n = phrases.FindNode(start);
	if (!n) return;

	while (n && (n->Event()->StartTime() <= end) ) {
		if( n->Event()->Type() == n->Event()->PHRASE_TYPE ) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
			if (pe) DrawTopLevelPhrase(clip, view, track, pe, end, pe, renderer);
		/* Events need to know if they are in a link or not, so they
		 * are required to be bundled up in a phrase.
		 */
		} else {
			debugger("Error condition");
//		} else if( n->Event()->Type() == n->Event()->NOTEON_TYPE ) {
//			DrawEvent( clip, view, dynamic_cast<AmNoteOn*>( n->Event() ) );
		}
		n = n->next;
	}
}

static pattern LINK_PATTERN = {	0x07, 0x0e, 0x1c, 0x38,
								0x70, 0xe0, 0xc1, 0x83 };

static inline rgb_color weighted_shade(const rgb_color& c1, const rgb_color& c2, float weight)
{
	rgb_color	c;
	c.red = uint8(((c1.red * weight) + c2.red) / (weight + 1));
	c.green = uint8(((c1.green * weight) + c2.green) / (weight + 1));
	c.blue = uint8(((c1.blue * weight) + c2.blue) / (weight + 1));
	c.alpha = 255;
	return c;
}

static inline void draw_label_border(	BView* view, BRect b, rgb_color clr,
										ArpLineArrayCache& lines,
										bool drawMuted,
										bool fillBottom)
{
	rgb_color	c = tint_color(clr, B_LIGHTEN_2_TINT);
	if (drawMuted) c = shade_color(c, MUTE_SHADE);
	lines.AddLine(BPoint(b.left, b.top - 1), BPoint(b.right, b.top - 1), c);
	lines.AddLine(BPoint(b.left - 1, b.top), BPoint(b.left - 1, b.bottom), c);
	lines.AddLine(BPoint(b.left - 1, b.top - 1), BPoint(b.left - 1, b.top - 1), clr);
	lines.AddLine(BPoint(b.right + 1, b.top - 1), BPoint(b.right + 1, b.top - 1), clr);
	c = tint_color(clr, B_DARKEN_2_TINT);
	if (drawMuted) c = shade_color(c, MUTE_SHADE);
	lines.AddLine(BPoint(b.right + 1, b.top), BPoint(b.right + 1, b.bottom), c);
	if (fillBottom) {
		lines.AddLine(BPoint(b.left, b.bottom + 1), BPoint(b.right + 1, b.bottom + 1), c);
		lines.AddLine(BPoint(b.left - 1, b.bottom + 1), BPoint(b.left - 1, b.bottom + 1), clr);
	} else {
		lines.AddLine(BPoint(b.left, b.bottom - 1), BPoint(b.right, b.bottom - 1), c);
		c = tint_color(c, B_DARKEN_2_TINT);
		lines.AddLine(BPoint(b.left - 1, b.bottom), BPoint(b.right + 1, b.bottom), c);
	}
}

static inline void draw_content_border(	BView* view, BRect b, rgb_color clr,
										ArpLineArrayCache& lines,
										bool drawMuted,
										bool fillTop)
{
	rgb_color	c = tint_color(clr, B_LIGHTEN_2_TINT);
	if (drawMuted) c = shade_color(c, MUTE_SHADE);
	if (fillTop) {
		lines.AddLine(BPoint(b.left, b.top - 1), BPoint(b.right, b.top - 1), c);
		lines.AddLine(BPoint(b.left - 1, b.top - 1), BPoint(b.left - 1, b.top - 1), clr);
		lines.AddLine(BPoint(b.right + 1, b.top - 1), BPoint(b.right + 1, b.top - 1), clr);
		lines.AddLine(BPoint(b.left - 1, b.top), BPoint(b.left - 1, b.bottom), c);
	} else {
		lines.AddLine(BPoint(b.left - 1, b.top), BPoint(b.left - 1, b.bottom), c);
	}
	c = tint_color(clr, B_DARKEN_2_TINT);
	if (drawMuted) c = shade_color(c, MUTE_SHADE);
	lines.AddLine(BPoint(b.right + 1, b.top), BPoint(b.right + 1, b.bottom), c);
	lines.AddLine(BPoint(b.left, b.bottom + 1), BPoint(b.right + 1, b.bottom + 1), c);

	lines.AddLine(BPoint(b.left - 1, b.bottom + 1), BPoint(b.left - 1, b.bottom + 1), clr);
}

void SeqPhraseMatrixView::DrawTopLevelPhrase(	BRect clip,
												BView* view,
												const AmTrack* track,
												const AmPhraseEvent* event,
												AmTime end,
												AmPhraseEvent* topPhrase,
												AmPhraseRendererI* renderer)
{
	ArpASSERT(event && topPhrase);
	if (!event || !event->Phrase() ) return;
	const AmPhrase*		phrase = event->Phrase();
	BRect				entireF(0, clip.top + 3, 0, clip.bottom - 3);
	AmTime				phraseEnd = event->EndTime();
	BString				n = event->Phrase()->Name();
	float				labelH = mCurLabelH + 3;
	bool				drawLabel = mCurLabelH > 0 && labelH < entireF.Height() && n.Length() > 0;

	entireF.left = mMtc.TickToPixel(event->StartTime()) - 2;
	entireF.right = mMtc.TickToPixel(phraseEnd) + 2;
	BRect				labelF = arp_invalid_rect();
	BRect				contentF = arp_invalid_rect();
	if (drawLabel) {
		if (entireF.Height() - 4 > labelH + 4) {
			labelF.Set(entireF.left + 2, entireF.top + 2, entireF.right - 2, entireF.bottom - 2);
			contentF = labelF;
			labelF.bottom = labelF.top + labelH - 1;
			contentF.top = labelF.bottom + 1;
		} else labelF.Set(entireF.left + 2, entireF.top + 2, entireF.right - 2, entireF.bottom - 2);
	} else contentF.Set(entireF.left + 2, entireF.top + 2, entireF.right - 2, entireF.bottom - 2);

	rgb_color			bgC = phrase->Color(phrase->BACKGROUND_C);
	rgb_color			fgC = phrase->Color(phrase->FOREGROUND_C);
	if (mDrawMuted) {
		bgC = shade_color(bgC, MUTE_SHADE);
		fgC = shade_color(fgC, MUTE_SHADE);
	}
	if (arp_is_valid_rect(contentF) ) {
		/* Fill the background.
		 */
		if (!mDrawMuted) {
			view->SetHighColor(bgC);
			view->FillRect(contentF);
		}
		/* Draw the measure markers.
		 */
		AmTrackMeasureBackground		bg(mSongRef, AmTrackRef(track), mMtc);
		if (mDrawMuted) bg.SetFlag(bg.DRAW_BEATS_FLAG, false);
		else {
			rgb_color	c = weighted_shade(bgC, Prefs().Color(AM_MEASURE_FG_C), 5);
			bg.SetBeatColor(c);
		}
		bg.LockedDraw(view, contentF, track->Signatures() );
	}

	mLines.BeginLineArray();
	if (arp_is_valid_rect(labelF) ) {
		rgb_color	c = weighted_shade(bgC, fgC, 3);
		view->SetHighColor(c);
		view->SetLowColor(c);
		view->FillRect(labelF);
		draw_label_border(view, labelF, c, mLines, mDrawMuted, !arp_is_valid_rect(contentF));
		view->SetHighColor(Prefs().Color(AM_MEASURE_FG_C) );
		BPoint		pt(labelF.left + 2, labelF.bottom - 2);
		if (arp_is_valid_rect(contentF) ) pt.y -= 2;
		view->TruncateString(&n, B_TRUNCATE_END, labelF.Width() );
		view->DrawString(n.String(), pt);
	}
	
	/* Shade the background.
	 */
	rgb_color		c;
	c.red = c.green = c.blue = 0;
	if (mDrawMuted) c = shade_color(c, MUTE_SHADE);
	mLines.AddLine(BPoint(entireF.left + 1, entireF.top), BPoint(entireF.right - 1, entireF.top), c);
	mLines.AddLine(BPoint(entireF.left + 1, entireF.bottom), BPoint(entireF.right - 1, entireF.bottom), c);
	mLines.AddLine(BPoint(entireF.left, entireF.top + 1), BPoint(entireF.left, entireF.bottom - 1), c);
	mLines.AddLine(BPoint(entireF.right, entireF.top + 1), BPoint(entireF.right, entireF.bottom - 1), c);

	if (arp_is_valid_rect(contentF) ) {
		draw_content_border(view, contentF, bgC, mLines, mDrawMuted, !arp_is_valid_rect(labelF));
	}

	c = tint_color(Prefs().Color(AM_DATA_BACKDROP_C), B_DARKEN_1_TINT);
	if (mDrawMuted) c = shade_color(c, MUTE_SHADE);
	if (!gPhraseBg) {
		mLines.AddLine(BPoint(entireF.right + 1, entireF.top + 2), BPoint(entireF.right + 1, entireF.bottom + 0), c);
		mLines.AddLine(BPoint(entireF.left + 2, entireF.bottom + 1), BPoint(entireF.right + 0, entireF.bottom + 1), c);
		mLines.AddLine(BPoint(entireF.right + 0, entireF.bottom + 0), BPoint(entireF.right + 0, entireF.bottom + 0), c);
	}

	/* Indicate if this phrase is linked.
	 */
	if (phrase->CountLinks() > 0) {
		view->SetHighColor(bgC);	
		view->SetLowColor(0, 0, 0);
		view->StrokeRect(entireF, LINK_PATTERN);
	}

	if (arp_is_valid_rect(contentF) ) {
		/* Draw the events.
		 */
		if (renderer) {
			BRect					trackR(clip.left, contentF.top, clip.right, contentF.bottom);
			view->SetHighColor(fgC);
			renderer->BeginTrack(trackR, view, track, mLines);
			renderer->DrawPhrase(contentF, view, track, event, end < phraseEnd ? end : phraseEnd, topPhrase, mLines);
			renderer->EndTrack(trackR, view, track, mLines);
		}
	}
	/* Finish.
	 */
	mLines.EndLineArray();
}

static bool has_signature_change(const BMessage* msg)
{
	ArpASSERT(msg);
	if (msg->what == AmNotifier::SIGNATURE_OBS) return true;
	int32		i;
	for (int32 k = 0; msg->FindInt32(RANGE_ALL_EVENT_STR, k, &i) == B_OK; k++) {
		if (i == AmNotifier::SIGNATURE_OBS) return true;
	}
	return false;
}

bool SeqPhraseMatrixView::TrackMessageReceived(const BMessage* msg)
{
#if 0
	if ( (msg->what != AmNotifier::NOTE_OBS)
			&& (msg->what != AmNotifier::CONTROL_CHANGE_OBS)
			&& (msg->what != AmNotifier::PITCH_OBS)
			&& (msg->what != AmNotifier::SIGNATURE_OBS)
			&& (msg->what != AmNotifier::TEMPO_OBS)
			&& (msg->what != AmNotifier::OTHER_EVENT_OBS) )
		return false;
#endif
	if (msg->what != AmNotifier::RANGE_OBS)
		return false;
	int32	start, end;
	if (msg->FindInt32("start_time", &start) != B_OK) return true;
	if (msg->FindInt32("end_time", &end) != B_OK) return true;
	BRect	b = Bounds();
	b.left = mMtc.TickToPixel(start) - 3;
	/* If the measure is a signature message, than leave the right
	 * bound alone -- i.e., the signature should invalidate the
	 * start of the signature til the right edge of the view.
	 */
	if (!has_signature_change(msg) ) b.right = mMtc.TickToPixel(end) + 3;
	if (b.left < 0) b.left = 0;
	Invalidate(b);
	return true;
}

static status_t lowest_position(const BMessage& msg, int32* lowest)
{
	status_t	err;
	int32		position;
	if ( (err = msg.FindInt32("position", 0, lowest)) != B_OK) return err;
	for (int32 k = 1; msg.FindInt32("position", k, &position) == B_OK; k++) {
		if (position < *lowest) *lowest = position;
	}
	return B_OK;
}

void SeqPhraseMatrixView::TrackChangeReceived(BMessage* msg)
{
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqPhraseMatrixView::TrackChangeReceived() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	if (song) FillTrackMetrics(song);
	mSongRef.ReadUnlock(song);
	// END READ SONG BLOCK

	int32		position;
	if (lowest_position(*msg, &position) != B_OK) return;

	float		top = TrackTop( (uint32)position);
	BRect		b = Bounds();
	Invalidate( BRect(b.left, top, b.right, b.bottom) );

	SetupScrollBars(false, true);
}

void SeqPhraseMatrixView::SetHsbRange()
{
	float 	width = HorizontalWidth();
	if (width <= Frame().Width()) width = 0;
	else width = width - Frame().Width();
	mHsb->SetRange(0, width);
}

void SeqPhraseMatrixView::SetHsbSteps()
{
	float	min, max;
	BRect	bounds = Bounds();
	float 	width = HorizontalWidth();

	mHsb->GetRange(&min, &max);
	if (min == 0 && max == 0) return;

	float	bigStep = bounds.Width();
	float	smallStep = 10;

	if (bigStep > 20) bigStep -= smallStep;
	if (bigStep > width - bounds.Width()) bigStep = width - bounds.Width();
	if ((bounds.right + smallStep) > width) smallStep = width - bounds.right;
	mHsb->SetSteps(smallStep, bigStep);
	
	float	prop = bounds.Width() / width;
	if (prop > 1) prop = 1;
	if (prop < 0) prop = 0;
	mHsb->SetProportion(prop);
}

float SeqPhraseMatrixView::HorizontalWidth() const
{
	return mMtc.TickToPixel(mCachedEndTime + AmGlobals().EndTimeBuffer() );
}

void SeqPhraseMatrixView::SetVsbRange()
{
	float	matrixH = TrackBottom();
	BRect	frame = Frame();
	float	h = matrixH;
	if (h <= frame.Height() ) h = 0;
	else h = h - frame.Height();
	mVsb->SetRange(0, h);
}

void SeqPhraseMatrixView::SetVsbSteps()
{
	BRect	bounds = Bounds();
	BRect	frame = Frame();
	float	trackH;
	float	matrixH = TrackBottom(&trackH);
	
	float	bigStep = frame.Height() - trackH;
	if (bigStep < trackH) bigStep = trackH;
	if (bigStep > matrixH - frame.Height()) bigStep = matrixH - frame.Height();
	mVsb->SetSteps(trackH, bigStep);
	
	float	prop = bounds.Height() / matrixH;
	if (prop > 1) prop = 1;
	else if (prop < 0) prop = 0;
	mVsb->SetProportion(prop);
}

_SeqTrackMetric* SeqPhraseMatrixView::TrackMetric(track_id tid)
{
	for (uint32 k = 0; k < mTrackMetrics.size(); k++) {
		if (mTrackMetrics[k].mTrackId == tid) return &(mTrackMetrics[k]);
	}
	return NULL;
}

float SeqPhraseMatrixView::TrackTop(uint32 position) const
{
//	ArpASSERT( position < mTrackMetrics.size() );
	if (position >= mTrackMetrics.size() ) return 0;
	return mTrackMetrics[position].mTop;
}

float SeqPhraseMatrixView::TrackTop(track_id tid) const
{
	for (uint32 k = 0; k < mTrackMetrics.size(); k++) {
		if (mTrackMetrics[k].mTrackId == tid) return mTrackMetrics[k].mTop;
	}
	return 0;
}

float SeqPhraseMatrixView::TrackBottom(float* smallest) const
{
	if (smallest) *smallest = 0;
	if (mTrackMetrics.size() < 1) return 0;
	if (smallest) {
		for (uint32 k = 0; k < mTrackMetrics.size(); k++) {
			float	height = mTrackMetrics[k].mBottom - mTrackMetrics[k].mTop;
			if (k == 0 || height < *smallest) *smallest = height;
		}
	}
	return mTrackMetrics[mTrackMetrics.size() - 1].mBottom;
}

track_id SeqPhraseMatrixView::CurrentTrackId()
{
	BPoint	pt;
	uint32	button;
	GetMouse(&pt, &button, false);
	return TrackId(pt);
}

track_id SeqPhraseMatrixView::TrackId(BPoint where)
{
	for (uint32 k = 0; k < mTrackMetrics.size(); k++) {
		if (mTrackMetrics[k].Contains(where.y) )
			return mTrackMetrics[k].mTrackId;
	}
	return 0;
}

void SeqPhraseMatrixView::StartPopUpTimer(const BMessage& msg, bigtime_t delay)
{
	StopPopUpTimer();
	mPopUpRunner = new BMessageRunner(BMessenger(this), &msg, delay, 1);
}

void SeqPhraseMatrixView::StopPopUpTimer()
{
	delete mPopUpRunner;
	mPopUpRunner = NULL;
}

void SeqPhraseMatrixView::ShowPopUp()
{
	BPoint	point;
	uint32	buttons;
	GetMouse(&point, &buttons, false);
	BPopUpMenu*		menu = new  BPopUpMenu( "", TRUE, TRUE, B_ITEMS_IN_COLUMN );
	BMenuItem*		item1 = new BMenuItem("Properties", new BMessage(PROPERTIES_MSG) );
//	BMenuItem*		item2 = new BMenuItem("Edit Name", new BMessage(EDIT_START_MSG) );
//	BMenuItem*		item3 = new BMenuItem("Remove", new BMessage(REMOVE_MSG) );
//	if (menu && item1 && item2 && item3) {
	if (menu && item1) {
		if (item1 && item1->Message() ) item1->Message()->AddPoint(WHERE_STR, point);
		menu->SetFontSize(10);
		menu->AddItem(item1);
//		menu->AddItem(item2);
//		menu->AddItem(item3);
		menu->SetTargetForItems(this);
		BRect	r(point, point);
		menu->Go( ConvertToScreen(point), true, false, ConvertToScreen(r), true );
	}
}

void SeqPhraseMatrixView::ShowProperties(BPoint where)
{
	track_id		trackId = TrackId(where);
	AmPhraseEvent*	pe = NULL;
	SeqSongWinPropertiesI*		win = dynamic_cast<SeqSongWinPropertiesI*>(Window() );
	_SeqPhraseToolTarget*	target;
	if (win && trackId && (target = new _SeqPhraseToolTarget(win, this, &mMtc)) ) {
		// READ SONG BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("SeqPhraseMatrixView::ShowProperties() read lock\n"); fflush(stdout);
		#endif
		const AmSong*	song = mSongRef.ReadLock();
		if (song) {
			const AmTrack*	track = song->Track(trackId);
			if (track) {
				pe = target->PhraseEventAt(track, where);
				if (pe) pe->IncRefs();
			}
		}
		mSongRef.ReadUnlock(song);
		// END READ SONG BLOCK
		delete target;
	}
	/* Done this way so I get out of the read lock as soon as
	 * possible (and because opening the properties window causes
	 * a read lock, and I don't want them nested).
	 */
	if (pe) {
		ShowPropertiesWin(pe);
		pe->DecRefs();
	}
}

void SeqPhraseMatrixView::ShowPropertiesWin(AmPhraseEvent* pe)
{
	if (!mPhrasePropWin.IsValid() ) {
		BWindow* 		win = new SeqPhrasePropertyWindow(mSongRef, pe);
		if (win) {
			mPhrasePropWin = BMessenger(win);
			win->Show();
		}
	} else {
		BHandler*		target;
		BLooper*		looper;
		if ( (target = mPhrasePropWin.Target(&looper)) != NULL) {
			SeqPhrasePropertyWindow*	win = dynamic_cast<SeqPhrasePropertyWindow*>(target);
			if (win) {
				win->SetPhraseEvent(pe);
				win->Activate(true);
			}
		}
	}
}


/*************************************************************************
 * _SEQ-PHRASE-TOOL-TARGET
*************************************************************************/
_SeqPhraseToolTarget::_SeqPhraseToolTarget(SeqSongWinPropertiesI* win,
											BView* view,
											const AmTimeConverter* mtc)
		: inherited(win, view, mtc)
{
}

void _SeqPhraseToolTarget::ShowDragMark(AmTime time, BPoint where,
										const BMessage* dragMessage)
{
	SeqPhraseMatrixView*	v = dynamic_cast<SeqPhraseMatrixView*>(mView);
	if (!v) return;
	v->ShowDragMark(time, where, dragMessage);
}

void _SeqPhraseToolTarget::ClearDragMark()
{
	SeqPhraseMatrixView*	v = dynamic_cast<SeqPhraseMatrixView*>(mView);
	if (!v) return;
	v->ClearDragMark();
}

/*************************************************************************
 * _SEQ-TRACK-METRIC
 *************************************************************************/
_SeqTrackMetric::_SeqTrackMetric()
		: mTrackId(0), mTop(0), mBottom(0), mRenderer(NULL)
{
}

_SeqTrackMetric::_SeqTrackMetric(const _SeqTrackMetric& o)
		: mTrackId(o.mTrackId), mTop(o.mTop), mBottom(o.mBottom), mRenderer(NULL)
{
	if (o.mRenderer) mRenderer = o.mRenderer->Copy();
}

_SeqTrackMetric::_SeqTrackMetric(	track_id trackId, float top, float bottom,
									AmPhraseRendererI* renderer)
		: mTrackId(trackId), mTop(top), mBottom(bottom), mRenderer(renderer)
{
}

_SeqTrackMetric::~_SeqTrackMetric()
{
	delete mRenderer;
	mRenderer = NULL;
}
	
_SeqTrackMetric& _SeqTrackMetric::operator=(const _SeqTrackMetric &o)
{
	mTrackId = o.mTrackId;
	mTop = o.mTop;
	mBottom = o.mBottom;
	delete mRenderer;
	mRenderer = NULL;
	if (o.mRenderer) mRenderer = o.mRenderer->Copy();
	return *this;
}

bool _SeqTrackMetric::Contains(float pointY) const
{
	return pointY >= mTop && pointY <= mBottom;
}

/*************************************************************************
 * _SEQ-DRAG-METRIC
 *************************************************************************/
_SeqDragMetric::_SeqDragMetric()
		: mDestIndex(-1)
{
}

_SeqDragMetric::_SeqDragMetric(const _SeqDragMetric& o)
		: mRange(o.mRange), mDestIndex(o.mDestIndex)
{
	for (uint32 k = 0; k < o.mSrcIndexes.size(); k++)
		mSrcIndexes.push_back( o.mSrcIndexes[k] );
}

_SeqDragMetric& _SeqDragMetric::operator=(const _SeqDragMetric &o)
{
	mRange = o.mRange;
	mDestIndex = o.mDestIndex;
	mSrcIndexes.resize(0);
	for (uint32 k = 0; k < o.mSrcIndexes.size(); k++)
		mSrcIndexes.push_back( o.mSrcIndexes[k] );
	return *this;
}

bool _SeqDragMetric::operator!=(const _SeqDragMetric &o)
{
	if (mRange != o.mRange) return true;
	if (mDestIndex != o.mDestIndex) return true;
	if (mSrcIndexes.size() != o.mSrcIndexes.size() ) return true;
	for (uint32 k = 0; k < mSrcIndexes.size(); k++) {
		if (mSrcIndexes[k] != o.mSrcIndexes[k]) return true;
	}
	return false;
}

void _SeqDragMetric::Clear()
{
	mRange.MakeInvalid();
	mDestIndex = -1;
	mSrcIndexes.resize(0);
}

void _SeqDragMetric::SetFrom(	AmTime time, BPoint where,
								const BMessage* dragMessage,
								trackmetric_vec& trackMetrics)
{
	Clear();
	for (uint32 k = 0; k < trackMetrics.size(); k++) {
		if (where.y >= trackMetrics[k].mTop
				&& where.y <= trackMetrics[k].mBottom) {
			mDestIndex = (int32)k;
			break;
		}
	}
	if (mDestIndex < 0) return;
	
	mRange.start = mRange.end = time;
	AmTime		s, e;
	if (find_time(*dragMessage, "start", &s) == B_OK
			&& find_time(*dragMessage, "end", &e) == B_OK)
		mRange.end = time + (e - s);

	int32		trackIndex;
	for (int32 k = 0; dragMessage->FindInt32("track", k, &trackIndex) == B_OK; k++)
		mSrcIndexes.push_back( (uint32)trackIndex );

	if (dragMessage->FindInt32("source track", &mSrcIndex) != B_OK) mSrcIndex = 0;
}

bool _SeqDragMetric::IsValid() const
{
	return mRange.IsValid() && mDestIndex >= 0 && mSrcIndexes.size() > 0;
}
