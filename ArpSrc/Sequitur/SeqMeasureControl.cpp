/* SeqMeasureControl.cpp
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
//TODO:
//#include <experimental/BitmapTools.h>
#include <interface/Bitmap.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/Screen.h>
#include <interface/Window.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmMotionI.h"
#include "AmPublic/AmPrefsI.h"

#include "ArpKernel/ArpBitmapCache.h"
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpLineArrayCache.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmTransport.h"

#include "Sequitur/SeqMeasureControl.h"
#include "Sequitur/SequiturDefs.h"

static const uint32		CHANGE_MOTION_MSG		= 'iChM';
static const char*		MOTION_KEY_STR			= "motion key";
static const char*		NONE_STR				= "None";
static const char*		CLEAR_STR				= "Clear";
enum {
	MOTION_NONE		= 1,
	MOTION_CLEAR	= 2
};

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
//TODO:
//static void			write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa);

/*************************************************************************
 * SEQ-MEASURE-CONTROL
 *************************************************************************/
SeqMeasureControl::SeqMeasureControl(	BRect frame,
										const char* name,
										AmSongRef songRef,
										AmTimeConverter& mtc,
										float leftIndent,
										float rightIndent,
										int32 resizeMask)
		: inherited(frame,
					name,
					resizeMask,
					B_WILL_DRAW | B_FRAME_EVENTS),
		AmSongObserver(songRef),
		mMtc(mtc),
		mLeftIndent(leftIndent), mRightIndent(rightIndent), mScrollX(0),
		mMouseDown(NO_MARKER), mTrackWinProps(NULL),
		mLeftBg(0), mRightBg(0),
		mDownPt(0, 0), mCachedEndTime(0)
{
	Initialize();
}

SeqMeasureControl::SeqMeasureControl(	BRect frame,
										const char* name,
										AmSongRef songRef,
										AmTrackWinPropertiesI* trackWinProps,
										AmTimeConverter& mtc,
										float leftIndent,
										float rightIndent,
										int32 resizeMask)
		: inherited(frame,
					name,
					resizeMask,
					B_WILL_DRAW | B_FRAME_EVENTS),
		AmSongObserver(songRef),
		mMtc(mtc),
		mLeftIndent(leftIndent), mRightIndent(rightIndent), mScrollX(0),
		mMouseDown(NO_MARKER), mTrackWinProps(trackWinProps),
		mLeftBg(0), mRightBg(0),
		mDownPt(0, 0), mCachedEndTime(0)
{
	Initialize();

}

SeqMeasureControl::~SeqMeasureControl()
{
	delete mLeftBg;
	delete mRightBg;
}

AmTime SeqMeasureControl::MarkerTime(Markers marker) const
{
	return mMarker[marker].time;
}

bool SeqMeasureControl::MarkerEnabled(Markers marker) const
{
	return mMarker[marker].IsEnabled();
}

bool SeqMeasureControl::MarkerVisible(Markers marker) const
{
	return mMarker[marker].IsVisible();
}

void SeqMeasureControl::SetLeftIndent(float leftIndent)
{
	mLeftIndent = leftIndent;
	if (mLeftIndent > 0) {
		BRect		b = Bounds();
		delete mLeftBg;
		ConstructLeftBg(BRect(0, 0, mLeftIndent - 1, b.Height() - 1));
	}
	Invalidate();
}

void SeqMeasureControl::SetRightIndent(float rightIndent)
{
	mRightIndent = rightIndent;
	if (mRightIndent > 0) {
		BRect		b = Bounds();
		delete mRightBg;
		ConstructRightBg(BRect(0, 0, mRightIndent, b.Height() - 1));
	}
	Invalidate();
}

void SeqMeasureControl::SetMarkerTime(uint32 markerMask, AmTime time)
{
	if (markerMask&AM_POSITION_MARKER) {
		/* If the user is dragging the mouse, let them -- overwrite
		 * whatever the transport wants.
		 */
		if (mMouseDown == POSITION_MARKER) return;
		mMarker[POSITION_MARKER].time = time;
	}
	if (markerMask&AM_LEFT_LOOP_MARKER) mMarker[LEFT_LOOP_MARKER].time = time;
	if (markerMask&AM_RIGHT_LOOP_MARKER) mMarker[RIGHT_LOOP_MARKER].time = time;
	Invalidate();
}

void SeqMeasureControl::SetMarkerEnabled(uint32 markerMask, bool enable)
{
	if (markerMask&AM_POSITION_MARKER) mMarker[POSITION_MARKER].SetEnabled(enable);
	if (markerMask&AM_LEFT_LOOP_MARKER) mMarker[LEFT_LOOP_MARKER].SetEnabled(enable);
	if (markerMask&AM_RIGHT_LOOP_MARKER) mMarker[RIGHT_LOOP_MARKER].SetEnabled(enable);
}

static bool need_to_initialize_loops(	AmTime leftTime, AmTime rightTime,
										AmTime leftLoopTime, AmTime rightLoopTime)
{
	if (leftLoopTime == rightLoopTime) return true;
	if (leftLoopTime < leftTime || leftLoopTime > rightTime) return true;
	if (rightLoopTime < leftTime || rightLoopTime > rightTime) return true;
	return false;
}

void SeqMeasureControl::SetMarkerVisible(uint32 markerMask, bool visible)
{
	if (markerMask&AM_POSITION_MARKER) mMarker[POSITION_MARKER].SetVisible(visible);
	if (markerMask&AM_LEFT_LOOP_MARKER) mMarker[LEFT_LOOP_MARKER].SetVisible(visible);
	if (markerMask&AM_RIGHT_LOOP_MARKER) mMarker[RIGHT_LOOP_MARKER].SetVisible(visible);

	if (markerMask&AM_LEFT_LOOP_MARKER || markerMask&AM_RIGHT_LOOP_MARKER) {
		AmTime		leftTime = CenterLeftTime();
		AmTime		rightTime = CenterRightTime();
		if ( visible && need_to_initialize_loops(leftTime, rightTime, mMarker[LEFT_LOOP_MARKER].time, mMarker[RIGHT_LOOP_MARKER].time) ) {
			InitializeLoopTimes();
		}
		SetTransportLooping();
		Invalidate();
	}
}

void SeqMeasureControl::SetTransportLooping()
{
	
	AmTransport*		t = SongRef().Transport();
	if (t) {
		t->SetLooping(	mMarker[LEFT_LOOP_MARKER].IsVisible(),
						mMarker[LEFT_LOOP_MARKER].time,
						mMarker[RIGHT_LOOP_MARKER].time);
	}
}

void SeqMeasureControl::Draw(BRect updateRect)
{
	BView* into = this;
	
	ArpBitmapCache* cache = dynamic_cast<ArpBitmapCache*>(Window());
	if( cache ) into = cache->StartDrawing(this, updateRect);
	
	DrawOn(updateRect, into);
	if( cache ) cache->FinishDrawing(into);
}

void SeqMeasureControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SongRef().AddObserver( this, AmSong::END_TIME_CHANGE_OBS );
	SongRef().AddRangeObserver( this, AmNotifier::SIGNATURE_OBS, AmRange() );
	SetViewColor( B_TRANSPARENT_COLOR );
	mViewColor = Prefs().Color( AM_MEASURE_TOP_BG_C );

	BRect		b = Bounds();
	if (mLeftIndent > 0) {
		delete mLeftBg;
		ConstructLeftBg(BRect(0, 0, mLeftIndent - 1, b.Height() - 1));
	}
	if (mRightIndent > 0) {
		delete mRightBg;
		ConstructRightBg(BRect(0, 0, mRightIndent, b.Height() - 1));
	}

	SetFontSize(10);
}

void SeqMeasureControl::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	SongRef().RemoveObserverAll( this );
}

void SeqMeasureControl::FrameResized(float new_width, float new_height)
{
	inherited::FrameResized( new_width, new_height );
	Invalidate();
}

void SeqMeasureControl::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case AmNotifier::SIGNATURE_OBS:
			Invalidate();
			break;
		case AmSong::END_TIME_CHANGE_OBS:
			AmTime		endTime;
			if( find_time( *msg, "end_time", &endTime ) == B_OK ) {
				mCachedEndTime = endTime;
				Invalidate();
			}
			break;
		case CHANGE_SIGNATURE_MSG:
			HandleSignatureMsg(msg);
			break;
		case CHANGE_MOTION_MSG:
			HandleMotionMsg(msg);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void SeqMeasureControl::MouseDown(BPoint pt)
{
	MakeFocus(true);
	BPoint		where;
	ulong		buttons;
	GetMouse(&where, &buttons, false);
	if (buttons&B_SECONDARY_MOUSE_BUTTON) {
		ShowTimeSignatureMenu(pt);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,
					  B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
	BPoint pos;
	mMouseDown = MarkerAt(pt, &pos);
	if (mMouseDown != NO_MARKER) {
		mDownPt = pt;
		mDownPt.x -= pos.x;
	}
}

void SeqMeasureControl::MouseUp(BPoint pt)
{
	mMouseDown = NO_MARKER;
}

void SeqMeasureControl::MouseMoved(	BPoint pt,
									uint32 code,
									const BMessage* msg)
{
	ArpASSERT(mMouseDown < _NUM_MARKERS);
	if (mMouseDown == NO_MARKER) return;
	AmTime		newPosition = mMarker[mMouseDown].TickFromPixel(pt.x-mDownPt.x, this);
	if (newPosition != mMarker[mMouseDown].time) {
		mMarker[mMouseDown].time = newPosition;
		if (mMarker[mMouseDown].time < 0) mMarker[mMouseDown].time = 0;
		/* If the user is dragging a loop marker, reset the transport's loop bounds.
		 */
		if (mMouseDown == LEFT_LOOP_MARKER || mMouseDown == RIGHT_LOOP_MARKER) {
			SetTransportLooping();
		}

		Invalidate();
	}
}

void SeqMeasureControl::ScrollTo(BPoint where)
{
	mScrollX = where.x;
	Invalidate();
}

static float view_font_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

void SeqMeasureControl::DrawOn(BRect updateRect, BView* view)
{
	BRect				b = Bounds();
	BRect				lBounds(b.left, b.top, mLeftIndent - 1, b.bottom);
	BRect				cBounds(b.left + mLeftIndent, b.top, b.right - mRightIndent, b.bottom);
	BRect				rBounds(b.right - mRightIndent + 1, b.top, b.right, b.bottom);

	AmTime				endTime = 0;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqMeasureControl::DrawOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) endTime = song->CountEndTime();
	ReadUnlock(song);
	// END READ SONG BLOCK

	if (mLeftIndent > 0) DrawLeftBgOn(lBounds, view, endTime);
	DrawCenterBgOn(cBounds, view, endTime);
	if (mRightIndent > 0) DrawRightBgOn(rBounds, view, endTime);

	DrawCenterOn(b, view);
	if (mLeftIndent > 0) DrawLeftOn( lBounds, view );
	if (mRightIndent > 0) DrawRightOn( rBounds, view );

	// Draw some edges
	view->SetHighColor( tint_color( mViewColor, B_LIGHTEN_2_TINT) );
	view->StrokeLine( BPoint(b.left, b.top), BPoint(b.right, b.top) );
	view->SetHighColor( 0, 0, 0 );
	view->StrokeLine( BPoint(b.left, b.top), BPoint(b.right, b.top) );
	view->StrokeLine( BPoint(b.left, b.bottom), BPoint(b.right, b.bottom) );

	float				fh = view_font_height(this);
	BRect				r(b);
	r.bottom = fh;
	mMarker[LEFT_LOOP_MARKER].DrawOn(r, view, this);
	mMarker[RIGHT_LOOP_MARKER].DrawOn(r, view, this);

	r.top = r.bottom;
	r.bottom = b.bottom;
	mMarker[POSITION_MARKER].DrawOn(r, view, this);
}

void SeqMeasureControl::DrawLeftBgOn(BRect lBounds, BView* view, AmTime songEndTime)
{
	// If the left isn't fixed then the DrawLeftOn() method will
	// handle drawing the background, since it wants to obliterate
	// any drawing the center might have done in the left view.
	if ( !IsLeftFixed() ) return;
	if ( mLeftBg ) view->DrawBitmapAsync( mLeftBg, BPoint(0, 1) );
}

void SeqMeasureControl::DrawCenterBgOn(BRect cBounds, BView* view, AmTime songEndTime)
{
	BPoint		left(cBounds.left, cBounds.top + 1);
	BPoint		right(cBounds.right, cBounds.top + 1);
	rgb_color	c = Prefs().Color( AM_MEASURE_TOP_BG_C );
	rgb_color	bc = Prefs().Color( AM_MEASURE_BOTTOM_BG_C );
	float		r_delta = 0, g_delta = 0, b_delta = 0;
	float		height = cBounds.bottom - left.y;
	if( c.red != bc.red )		r_delta = ((float)bc.red - (float)c.red) / height;
	if( c.green != bc.green )	g_delta = ((float)bc.green - (float)c.green) / height;
	if( c.blue != bc.blue )		b_delta = ((float)bc.blue - (float)c.blue) / height;

	float		i = 1;
	rgb_color	o = c;
	while (left.y < cBounds.bottom) {
		view->SetHighColor( c );
		view->StrokeLine( left, right );
		c.red = (uint8)(o.red + (i * r_delta));
		c.green = (uint8)(o.green + (i * g_delta));
		c.blue = (uint8)(o.blue + (i * b_delta));
		left.y++;
		right.y++;
		i++;
	}
}

void SeqMeasureControl::DrawRightBgOn(BRect rBounds, BView* view, AmTime songEndTime)
{
	if ( mRightBg ) view->DrawBitmapAsync( mRightBg, BPoint(rBounds.left, 1) );
}

void SeqMeasureControl::DrawLeftOn(BRect lBounds, BView* view)
{
	// If it's fixed than I just let the center view take care of drawing it.
	if ( IsLeftFixed() ) return;

	if ( mLeftBg ) view->DrawBitmapAsync( mLeftBg, BPoint(0, 1) );

	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqMeasureControl::DrawLeftOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if( !song ) return;
	LockedDrawLeftOn(song->Signatures(), lBounds, view);
	ReadUnlock( song );
	// END READ SONG BLOCK
}

void SeqMeasureControl::LockedDrawLeftOn(const AmPhrase& signatures, BRect lBounds, BView* view)
{
	AmTime				rightTick = mMtc.PixelToTick(mScrollX);
	AmTimeConverter		mtc( mLeftIndent / ((float)rightTick / (float)PPQN) );
	AmNode*				node = signatures.HeadNode();
	if (node == 0) return;
	AmSignature*		sig = dynamic_cast<AmSignature*>( node->Event() );
	if (sig == 0) return;
	AmSignature			currentSig(*sig);
	AmTime				sigLength = currentSig.Duration();
	AmSignature*		nextSig = 0;
	AmNode*				nextNode = node->next;
	if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	char				buf[16];
	float				lastRight = -1;

	/* Draw the center dividing line.
	 */
	float				fh = view_font_height(view);
	lBounds.top = lBounds.bottom - fh;
	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->StrokeLine( BPoint(lBounds.left, lBounds.top-1), BPoint(lBounds.right, lBounds.top-1) );
	view->SetHighColor( Prefs().Color( AM_MEASURE_HIGHLIGHT_C ) );
	view->StrokeLine( BPoint(lBounds.left, lBounds.top), BPoint(lBounds.right, lBounds.top) );

	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	
	while (currentSig.EndTime() < rightTick) {

		float		pixel = mtc.TickToPixel( currentSig.StartTime() );
		if( pixel > lastRight ) {
			view->StrokeLine( BPoint(pixel, lBounds.top), BPoint(pixel, lBounds.bottom) );
			sprintf( buf, "%ld", currentSig.Measure() );
			float	width = view->StringWidth( buf );
			if( pixel + 2 + width > mLeftIndent ) {
				lastRight = pixel + 2;
			} else {
				view->DrawString( buf, BPoint( pixel + 2, lBounds.bottom - 2 ) );
				lastRight = pixel + 2 + width + 2;
			}
		}
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			int32	measure = currentSig.Measure();
			currentSig.Set( *nextSig );
			currentSig.SetMeasure( measure );
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = (AmNode*)nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	
	view->SetDrawingMode(B_OP_COPY);
}

void SeqMeasureControl::DrawCenterOn(BRect bounds, BView* view)
{
	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->SetLowColor( Prefs().Color( AM_MEASURE_BOTTOM_BG_C ) );

	AmSongMeasureBackground		bg(SongRef(), mMtc);
	if (mTrackWinProps) {
		AmTrackRef				trackRef = mTrackWinProps->OrderedTrackAt(0);
		if (trackRef.IsValid() ) bg.SetTrack(trackRef.TrackId() );
	}
	bg.SetScrollX(mScrollX);
	bg.SetLeftIndent(mLeftIndent);
	bg.SetFlag(bg.DRAW_SIGNATURES_FLAG, true);

	bg.DrawAllOn(view, bounds);
}

void SeqMeasureControl::DrawRightOn(BRect rBounds, BView* view)
{
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqMeasureControl::DrawRightOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) LockedDrawRightOn(song->Signatures(), rBounds, view, song->CountEndTime() );
	ReadUnlock(song);
	// END READ SONG BLOCK
}

void SeqMeasureControl::LockedDrawRightOn(	const AmPhrase& signatures, BRect rBounds, BView* view,
											AmTime songEndTime)
{
	AmTime				centerRightTime = mMtc.PixelToTick( rBounds.left - 1 - mLeftIndent + mScrollX );
	if (centerRightTime < 0) return;
	AmTime				timeWidth = songEndTime - centerRightTime;
	if( timeWidth < 0 ) return;
	AmTimeConverter		mtc( mRightIndent / ((float)timeWidth / (float)PPQN) );
	if( mtc.BeatLength() >= mMtc.BeatLength() ) return;

	AmNode*				node = signatures.HeadNode();
	if (node == 0) return;
	AmSignature*		sig = dynamic_cast<AmSignature*>( node->Event() );
	if (sig == 0) return;
	AmSignature			currentSig(*sig);
	AmTime				sigLength = currentSig.Duration();
	AmSignature*		nextSig = 0;
	AmNode*				nextNode = node->next;
	if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	char				buf[16];
	float				lastRight = -1;

	/* Find the first signature right of the center view.
	 */
	while (currentSig.EndTime() < centerRightTime) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			int32	measure = currentSig.Measure();
			currentSig.Set( *nextSig );
			currentSig.SetMeasure( measure );
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = (AmNode*)nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	DrawRightBgOn(rBounds, view, songEndTime);	
	/* Draw the center dividing line.
	 */
	float				fh = view_font_height(view);
	rBounds.top = rBounds.bottom - fh;
	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->StrokeLine( BPoint(rBounds.left, rBounds.top-1), BPoint(rBounds.right, rBounds.top-1) );
	view->SetHighColor( Prefs().Color( AM_MEASURE_HIGHLIGHT_C ) );
	view->StrokeLine( BPoint(rBounds.left, rBounds.top), BPoint(rBounds.right, rBounds.top) );
	
	/* Now draw til the end.
	 */
	view->SetHighColor( Prefs().Color( AM_MEASURE_FG_C ) );
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	
	while (currentSig.EndTime() < songEndTime) {

		float		pixel = mtc.TickToPixel( currentSig.StartTime() - centerRightTime );
		if( pixel > lastRight ) {
			view->StrokeLine( BPoint(pixel + rBounds.left, rBounds.top), BPoint(pixel + rBounds.left, rBounds.bottom) );
			sprintf( buf, "%ld", currentSig.Measure() );
			float	width = view->StringWidth( buf );
			view->DrawString( buf, BPoint( pixel + rBounds.left + 2, rBounds.bottom - 2 ) );
			lastRight = pixel + 2 + width + 2;
		}
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			int32	measure = currentSig.Measure();
			currentSig.Set( *nextSig );
			currentSig.SetMeasure( measure );
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = (AmNode*)nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	
	view->SetDrawingMode(B_OP_COPY);
}

void SeqMeasureControl::HandleSignatureMsg(BMessage* msg)
{
	int32	measure;
	if (msg->FindInt32("measure", &measure) != B_OK) return;
	int32	beats = -1, beatvalue = -1;
	if (msg->FindInt32("beats", &beats) != B_OK) beats = -1;
	if (msg->FindInt32("beat value", &beatvalue) != B_OK) beatvalue = -1;

	if (beats < 1 || beatvalue < 1) {
		if ( Window() ) Window()->PostMessage(msg);
	} else {
		// WRITE SONG OR TRACK BLOCK
		AmSong*		song = WriteLock();
		if (song) {
#if 0
			if ( mTrackRef.IsValid() ) {
				AmTrack*	track = song ? song->Track(mTrackRef) : 0;
				if (track) track->SetSignature(measure, beats, beatvalue);
			} else {
#endif
				song->SetSignature(measure, beats, beatvalue);
#if 0
			}
#endif
		}
		WriteUnlock(song);
		// END WRITE SONG OR TRACK BLOCK
	}
}

void SeqMeasureControl::HandleMotionMsg(BMessage* msg)
{
	if (!mTrackWinProps) return;
	int32			measure, code;
	const char*		key;
	if (msg->FindInt32("measure", &measure) != B_OK) return;
	if (msg->FindInt32("code", &code) != B_OK) code = 0;
	if (msg->FindString(MOTION_KEY_STR, &key) != B_OK) key = NULL;
	AmMotionI*		motion = NULL;
	if (key) motion = AmGlobals().NewMotion(BString(key) );
	
	// WRITE TRACK BLOCK
	AmSong*			song = WriteLock();
	if (song) {
		AmTrackRef	trackRef = mTrackWinProps->OrderedTrackAt(0);
		AmTrack*	track = song->Track(trackRef);
		if (track) {
			if (code == MOTION_CLEAR) track->ClearMotion(measure);
			else track->SetMotion(measure, motion);
		}
	}
	WriteUnlock(song);
	// END WRITE SONG OR TRACK BLOCK

	delete motion;
}

void SeqMeasureControl::ShowTimeSignatureMenu(BPoint pt) const
{
	AmSignature		sig;
	if (SignatureForPt(pt, sig) != B_OK) return;
	BPopUpMenu*		menu = new BPopUpMenu("menu");
	if (!menu) return;
	menu->SetFontSize(10);
	menu->SetAsyncAutoDestruct(true);

	BMessage	signatureChoices;
	if (seq_get_message_preference(SIGNATURE_CHOICES_PREF, &signatureChoices) == B_OK) {
		int32	beats;
		for(int32 k = 0; signatureChoices.FindInt32("beats", k, &beats) == B_OK; k++) {
			int32	beatvalue;
			if (signatureChoices.FindInt32("beat value", k, &beatvalue) == B_OK) {
				BString		label;
				label << beats << " / " << beatvalue;
				BMessage*	msg = new BMessage(CHANGE_SIGNATURE_MSG);
				BMenuItem*	item;
				if (msg && (item = new BMenuItem(label.String(), msg)) ) {
					msg->AddInt32("measure", sig.Measure() );
					msg->AddInt32("beats", beats);
					msg->AddInt32("beat value", beatvalue);
					menu->AddItem(item);
					item->SetTarget(this);
				}
			}
		}
	}
	BMessage*		msg = new BMessage(CHANGE_SIGNATURE_MSG);
	BMenuItem*		item;
	if ( msg && (item = new BMenuItem("Other...", msg)) ) {
		msg->AddInt32("measure", sig.Measure() );
		msg->AddInt32("beats", sig.Beats() );
		msg->AddInt32("beat value", sig.BeatValue() );
		menu->AddItem(item);
		item->SetTarget( Window() );
	}
	/* If I'm a track measure control, add in my motion list.
	 */
	BMenu*		motionMenu = NULL;
	if (mTrackWinProps && (motionMenu = new BMenu("Motion")) ) {
		BMessage*		msg = new BMessage(CHANGE_MOTION_MSG);
		BMenuItem*		item;
		if (msg && (item = new BMenuItem(NONE_STR, msg)) ) {
			msg->AddInt32("code", MOTION_NONE);
			msg->AddInt32("measure", sig.Measure() );
			motionMenu->AddItem(item);
			item->SetTarget(this);
		}
		msg = new BMessage(CHANGE_MOTION_MSG);
		if (msg && (item = new BMenuItem(CLEAR_STR, msg)) ) {
			msg->AddInt32("code", MOTION_CLEAR);
			msg->AddInt32("measure", sig.Measure() );
			motionMenu->AddItem(item);
			item->SetTarget(this);
		}
		
		BString		label, key;
		for (uint32 k = 0; AmGlobals().GetMotionInfo(k, label, key) == B_OK; k++) {
			msg = new BMessage(CHANGE_MOTION_MSG);
			if (msg && (item = new BMenuItem(label.String(), msg)) ) {
				if (k == 0) motionMenu->AddSeparatorItem();
				msg->AddString(MOTION_KEY_STR, key);
				msg->AddInt32("measure", sig.Measure() );
				motionMenu->AddItem(item);
				item->SetTarget(this);
			}
		}
		if (motionMenu) {
			menu->AddSeparatorItem();
			BMenuItem*		i = new BMenuItem(motionMenu);
			if (i) menu->AddItem(i);
		}
	}
	
	BRect	frame(pt, pt);
	menu->Go( ConvertToScreen(pt), true, false, ConvertToScreen(frame), true);
}

status_t SeqMeasureControl::SignatureForPt(BPoint pt, AmSignature& sig) const
{
	status_t	err = B_ERROR;
	AmTime		time = mMtc.PixelToTick(pt.x + mScrollX - mLeftIndent);
	/* If the time being requested is less than the first measure, constrain
	 * it to the first measure.
	 */
	if (time < 0) time = 0;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqMeasureControl::SignatureForPt() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (song) err = song->GetSignature(time, sig);
	ReadUnlock( song );
	// END READ SONG BLOCK
	return err;
}
uint32 SeqMeasureControl::MarkerAt(BPoint pt, BPoint* outLeftTop) const
{
	for (uint32 k=0; k<_NUM_MARKERS; k++) {
		if ( mMarker[k].Contains(pt, this, outLeftTop) ) return k;
	}
	return NO_MARKER;
}

bool SeqMeasureControl::IsLeftFixed() const
{
	if (mLeftIndent == 0) return true;
	return (mScrollX - mLeftIndent < 0);
}

bool SeqMeasureControl::IsRightFixed() const
{
	AmTime				rightTime = mMtc.PixelToTick( Bounds().right - mLeftIndent + mScrollX );
	if( rightTime < 0 ) return true;
	if( mCachedEndTime <= rightTime ) return true;
	return false;
}

AmTime SeqMeasureControl::CenterLeftTime() const
{
	return mMtc.PixelToTick(mScrollX);
}

AmTime SeqMeasureControl::CenterRightTime() const
{
	BRect		b = Bounds();
	return mMtc.PixelToTick( b.right - mRightIndent - mLeftIndent + mScrollX );
}

void SeqMeasureControl::Initialize()
{
	float		fh = view_font_height(this);
	float		row2B = Bounds().bottom;
	float		row2T = row2B - fh - 1;
	float		row1B = row2T - 1;
	float		row1T = 0;
	if ( !(mMarker[POSITION_MARKER].image) ) {
		mMarker[POSITION_MARKER].image = ImageManager().FindBitmap(SONG_POSITION_MARKER_IMAGE_STR);
		if (mMarker[POSITION_MARKER].image)
			mMarker[POSITION_MARKER].offset = 0 - (mMarker[POSITION_MARKER].image->Bounds().Width() / 2);
		mMarker[POSITION_MARKER].origin.y = row2T;
		mMarker[POSITION_MARKER].bottom = row2B;
	}
	if ( !(mMarker[LEFT_LOOP_MARKER].image) ) {
		mMarker[LEFT_LOOP_MARKER].image = ImageManager().FindBitmap(LEFT_LOOP_STR);
		if (mMarker[LEFT_LOOP_MARKER].image)
			mMarker[LEFT_LOOP_MARKER].offset = 0 - mMarker[LEFT_LOOP_MARKER].image->Bounds().Width();
		mMarker[LEFT_LOOP_MARKER].origin.y = row1T;
		mMarker[LEFT_LOOP_MARKER].bottom = row1B;
	}
	if ( !(mMarker[RIGHT_LOOP_MARKER].image) ) {
		mMarker[RIGHT_LOOP_MARKER].image = ImageManager().FindBitmap(RIGHT_LOOP_STR);
		mMarker[RIGHT_LOOP_MARKER].origin.y = row1T;
		mMarker[RIGHT_LOOP_MARKER].bottom = row1B;
	}

	mMarker[LEFT_LOOP_MARKER].SetVisible(false);
	mMarker[LEFT_LOOP_MARKER].dragLock = _AmMarkerEntry::LOCK_TO_BEAT;
	mMarker[LEFT_LOOP_MARKER].pair = &(mMarker[RIGHT_LOOP_MARKER]);
	mMarker[LEFT_LOOP_MARKER].pairPosition = _AmMarkerEntry::RIGHT_PAIR;

	mMarker[RIGHT_LOOP_MARKER].SetVisible(false);
	mMarker[RIGHT_LOOP_MARKER].dragLock = _AmMarkerEntry::LOCK_TO_BEAT;
	mMarker[RIGHT_LOOP_MARKER].pair = &(mMarker[LEFT_LOOP_MARKER]);
	mMarker[RIGHT_LOOP_MARKER].pairPosition = _AmMarkerEntry::LEFT_PAIR;

	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqMeasureControl::Initialize() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = ReadLock();
	if( song ) mCachedEndTime = song->CountEndTime();
	ReadUnlock( song );
	// END READ SONG BLOCK
}

static bool get_signature(	const AmSong* song, const AmTrack* track,
							AmTime time, AmSignature& sig)
{
	if (track && track->GetSignature(time, sig) == B_OK) return true;
	else if (song && song->GetSignature(time, sig) == B_OK) return true;
	else return false;
}

static bool get_visible_signatures(	const AmSong* song, const AmTrack* track,
									AmTime leftTime, AmTime rightTime,
									AmSignature& leftSig, AmSignature& rightSig)
{
	if (!get_signature(song, track, leftTime, leftSig)) return false;
	if (!get_signature(song, track, rightTime, rightSig)) return false;
	/* If the left signature is bisected, get the next one over.
	 */

	if ( leftSig.StartTime() < leftTime ) {
		if (!get_signature(song, track, leftSig.EndTime() + 1, leftSig)) return false;
	}
	/* If the right signature is bisected, get the previous one.
	 */
	if ( rightSig.EndTime() > rightTime ) {
		if (rightSig.StartTime() - 1 < 0) return false;
		if (!get_signature(song, track, rightSig.StartTime() - 1, rightSig)) return false;
	}
	/* Only succeed if they are either the same signature or the left is actually
	 * left of the right.
	 */
	return leftSig.StartTime() <= rightSig.StartTime();
}

void SeqMeasureControl::InitializeLoopTimes()
{
	AmTime		left = CenterLeftTime();
	AmTime		right = CenterRightTime();
	/* If nothing else is valid, default to the visible bounds of the window.
	 */
	mMarker[LEFT_LOOP_MARKER].time = left;
	mMarker[RIGHT_LOOP_MARKER].time = right;

	/* Find the first completely visible left and right measures for the view.
	 * If they're different, I've got my loop times.
	 */
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("SeqMeasureControl::InitializeLoopTimes() read lock\n"); fflush(stdout);
	#endif
	const AmSong*		song = ReadLock();
	if (!song) return;
	AmTrackRef			trackRef;
	if (mTrackWinProps) trackRef = mTrackWinProps->OrderedTrackAt(0);
	const AmTrack*		track = song->Track(trackRef);
	AmSignature			leftSig, rightSig;
	bool				success = false;
	success = get_visible_signatures(song, track, left, right, leftSig, rightSig);
	ReadUnlock( song );
	// END READ SONG BLOCK

	if (success) {
		mMarker[LEFT_LOOP_MARKER].time = leftSig.StartTime();
		mMarker[RIGHT_LOOP_MARKER].time = rightSig.EndTime();
	}
	
	if (mMarker[LEFT_LOOP_MARKER].time < 0)
		mMarker[LEFT_LOOP_MARKER].time = 0;
	if (mMarker[RIGHT_LOOP_MARKER].time < 0)
		mMarker[RIGHT_LOOP_MARKER].time = 0;
}

void SeqMeasureControl::ConstructLeftBg(BRect bounds)
{
	if ( !Window() ) return;
	BScreen			screen( Window() );

	mLeftBg = new BBitmap(bounds, screen.ColorSpace() );
	if (!mLeftBg) return;
	//TODO:
	//pixel_access	pa(mLeftBg->ColorSpace() );
	
	BRect			b = mLeftBg->Bounds();
	rgb_color		c = Prefs().Color( AM_MEASURE_TOP_BG_C );
	float			r_row_delta = 0, g_row_delta = 0, b_row_delta = 0;
	{
		rgb_color	bc = Prefs().Color(AM_MEASURE_BOTTOM_BG_C);
		float		height = b.bottom;
		if (c.red != bc.red)		r_row_delta = ((float)bc.red - (float)c.red) / height;
		if (c.green != bc.green)	g_row_delta = ((float)bc.green - (float)c.green) / height;
		if (c.blue != bc.blue)		b_row_delta = ((float)bc.blue - (float)c.blue) / height;
	}
	rgb_color		rowC = c;
	rgb_color		leftC = Prefs().Color(AM_MEASURE_LEFT_BG_C);
	float			i = 1;
	for (float k=0; k<b.bottom; k++) {
		float		r_col_delta = 0, g_col_delta = 0, b_col_delta = 0;
		float		width = b.right;
		if (rowC.red != leftC.red)		r_col_delta = ((float)leftC.red - (float)rowC.red) / width;
		if (rowC.green != leftC.green)	g_col_delta = ((float)leftC.green - (float)rowC.green) / width;
		if (rowC.blue != leftC.blue)	b_col_delta = ((float)leftC.blue - (float)rowC.blue) / width;
		for (float j=b.right; j>=0; j--) {
			rgb_color	c;
			c.red = (uint8)(rowC.red + ( fabs(b.right - j) * r_col_delta));
			c.green = (uint8)(rowC.green + ( fabs(b.right - j) * g_col_delta));
			c.blue = (uint8)(rowC.blue + ( fabs(b.right - j) * b_col_delta));
			//TODO:
			//write_pixel(mLeftBg, j, k, c, pa);
		}
		rowC.red = (uint8)(c.red + (i * r_row_delta));
		rowC.green = (uint8)(c.green + (i * g_row_delta));
		rowC.blue = (uint8)(c.blue + (i * b_row_delta));
		i++;
	}
}

void SeqMeasureControl::ConstructRightBg(BRect bounds)
{
	if ( !Window() ) return;
	BScreen			screen( Window() );

	mRightBg = new BBitmap(bounds, screen.ColorSpace() );
	if (!mRightBg) return;
	//TODO:
	//pixel_access	pa(mRightBg->ColorSpace() );
	
	BRect			b = mRightBg->Bounds();
	rgb_color		c = Prefs().Color( AM_MEASURE_TOP_BG_C );
	float			r_row_delta = 0, g_row_delta = 0, b_row_delta = 0;
	{
		rgb_color	bc = Prefs().Color( AM_MEASURE_BOTTOM_BG_C );
		float		height = b.bottom;
		if (c.red != bc.red)		r_row_delta = ((float)bc.red - (float)c.red) / height;
		if (c.green != bc.green)	g_row_delta = ((float)bc.green - (float)c.green) / height;
		if (c.blue != bc.blue)		b_row_delta = ((float)bc.blue - (float)c.blue) / height;
	}
	rgb_color		rowC = c;
	rgb_color		rightC = Prefs().Color( AM_MEASURE_RIGHT_BG_C );
	float			i = 1;
	for (float k=0; k<b.bottom; k++) {
		float		r_col_delta = 0, g_col_delta = 0, b_col_delta = 0;
		float		width = b.right;
		if (rowC.red != rightC.red)			r_col_delta = ((float)rightC.red - (float)rowC.red) / width;
		if (rowC.green != rightC.green)		g_col_delta = ((float)rightC.green - (float)rowC.green) / width;
		if (rowC.blue != rightC.blue)		b_col_delta = ((float)rightC.blue - (float)rowC.blue) / width;
		for (float j=0; j<b.right; j++) {
			rgb_color	c;
			c.red = (uint8)(rowC.red + (j * r_col_delta));
			c.green = (uint8)(rowC.green + (j * g_col_delta));
			c.blue = (uint8)(rowC.blue + (j * b_col_delta));
			//TODO:
			//write_pixel(mRightBg, j, k, c, pa);
		}
		rowC.red = (uint8)(c.red + (i * r_row_delta));
		rowC.green = (uint8)(c.green + (i * g_row_delta));
		rowC.blue = (uint8)(c.blue + (i * b_row_delta));
		i++;
	}
}

/***************************************************************************
 * SEQ-SIGNATURE-MEASURE-CONTROL
 ***************************************************************************/
SeqSignatureMeasureControl::SeqSignatureMeasureControl(	BRect frame,
														const char* name,
														AmSignaturePhrase& signatures,
														AmTimeConverter& mtc,
														float leftIndent,
														float rightIndent,
														int32 resizeMask)
		: inherited(frame, name, AmSongRef(), mtc, leftIndent, rightIndent, resizeMask),
		  mSignatures(signatures)
{
}

void SeqSignatureMeasureControl::DrawCenterOn(BRect bounds, BView* view)
{
	view->SetHighColor( Prefs().Color(AM_MEASURE_FG_C) );
	view->SetLowColor( Prefs().Color(AM_MEASURE_BOTTOM_BG_C) );

	AmSignatureMeasureBackground		bg(mSignatures, mMtc);
	bg.SetScrollX(mScrollX);
	bg.SetLeftIndent(mLeftIndent);
	bg.SetFlag(bg.DRAW_SIGNATURES_FLAG, true);
	bg.SetFlag(bg.DRAW_MEASURE_FLAG, true);
	/* This looks like a hack I did for drawing the measure in
	 * the track window.  However, as of now, I have no plans to
	 * allow tracks to have their own signatures, so this is
	 * currently a waste of time.  Probably, if I do want to reinstate
	 * this, I should do it a better way, like another subclass.  This
	 * is just gross.
	 */
#if 0
	if (mTrackWinProps) {
		AmTrackRef				trackRef = mTrackWinProps->OrderedTrackAt(0);
		if (trackRef.IsValid() ) bg.SetTrackRef(trackRef);
	}
#endif
	bg.DrawAllOn(view, bounds);
}

/**********************************************************************
 * _AM-MARKER-ENTRY
 * This class encapsulates the state of one entry in the measure's list
 * of markers.
 **********************************************************************/
_AmMarkerEntry::_AmMarkerEntry()
		: time(0), origin(0, 0), offset(0), image(NULL), dragLock(NO_LOCK),
		  pair(NULL), pairPosition(NO_PAIR), mVisible(true), mEnabled(true)
{
}

_AmMarkerEntry& _AmMarkerEntry::operator=(const _AmMarkerEntry &e)
{
	time = e.time;
	origin = e.origin;
	offset = e.offset;
	image = e.image;
	mVisible = e.mVisible;
	mEnabled = e.mEnabled;
	dragLock = e.dragLock;
	pair = e.pair;
	pairPosition = e.pairPosition;
	return *this;
}

void _AmMarkerEntry::DrawOn(BRect clip, BView* view, const SeqMeasureControl* control)
{
	if (!image || !mVisible) return;

	float	width = image->Bounds().Width();
	BPoint	pixel( LeftPixel(control), clip.top );
	if ( pixel.x < (clip.left + offset) ) return;
	if ( pixel.x > clip.right + width ) return;
	
	drawing_mode	mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	view->DrawBitmapAsync(image, pixel);
	view->SetDrawingMode(mode);
	origin.x = pixel.x;
}

float _AmMarkerEntry::LeftPixel(const SeqMeasureControl* control) const
{
	ArpASSERT(control);
	if (!image) return 0;
	const BRect				bounds = control->Bounds();
	const AmTime			leftTime = control->mMtc.PixelToTick(control->mScrollX);
	AmTime					centerRightTime;
	float					pos;
	if( time < leftTime && !control->IsLeftFixed() ) {
		AmTimeConverter		mtc( control->mLeftIndent / ((float)leftTime / (float)PPQN) );
		pos = mtc.TickToPixel(time) + offset;
	} else if( time > (centerRightTime = control->CenterRightTime()) && !control->IsRightFixed() ) {
		AmTime	timeWidth = control->mCachedEndTime - centerRightTime;
		AmTimeConverter	mtc( control->mRightIndent / ((float)timeWidth / (float)PPQN) );
		pos = mtc.TickToPixel(time - centerRightTime) + (bounds.right - control->mRightIndent);
	} else {
		pos = control->mMtc.TickToPixel(time) + offset - control->mScrollX + control->mLeftIndent;
	}
	
	if (pos < offset) pos = offset;
	if (pos > (bounds.right + offset)) pos = bounds.right + offset;
	return pos;
}

AmTime _AmMarkerEntry::TickFromPixel(float pixel, const SeqMeasureControl* control) const
{
	if (!image) return 0;
	if (dragLock == NO_LOCK) return NoLockTickFromPixel(pixel, control);
	else if (dragLock == LOCK_TO_BEAT) return LockToBeatTickFromPixel(pixel, control);
	else return 0;
}

AmTime _AmMarkerEntry::NoLockTickFromPixel(float pixel, const SeqMeasureControl* control) const
{
	BRect					b = control->Bounds();
	float					half = image->Bounds().Width() / 2;
	if( pixel + half < control->mLeftIndent && !control->IsLeftFixed() ) {
		AmTime				leftTime = control->mMtc.PixelToTick(control->mScrollX);
		AmTimeConverter		mtc( control->mLeftIndent / ((float)leftTime / (float)PPQN) );
		return mtc.PixelToTick(pixel + half);
	} else if (pixel + half > (b.right - control->mRightIndent) && !control->IsRightFixed() ) {
		AmTime				centerRightTime = control->CenterRightTime();
		AmTime				timeWidth = control->mCachedEndTime - centerRightTime;
		AmTimeConverter	mtc( control->mRightIndent / ((float)timeWidth / (float)PPQN) );
		AmTime ans = centerRightTime + mtc.PixelToTick( pixel - (b.right - control->mRightIndent) + 1 + half);
		if( ans > control->mCachedEndTime ) return control->mCachedEndTime;
		return ans;
	}
	return control->mMtc.PixelToTick(pixel + half + control->mScrollX - control->mLeftIndent);
}

AmTime _AmMarkerEntry::LockToBeatTickFromPixel(float pixel, const SeqMeasureControl* control) const
{
	AmTime			newTime = NoLockTickFromPixel(pixel, control);
	if (newTime <= 0) return 0;
	status_t		err = B_ERROR;
	AmSignature		sig;
	// READ SONG BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("_AmMarkerEntry::LockToBeatTickFromPixel() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = control->ReadLock();
	if (song) err = song->GetSignature(newTime, sig);
	control->ReadUnlock(song);
	// END READ SONG BLOCK
	if (err != B_OK) return newTime;
	if ( newTime < sig.StartTime() || newTime > sig.EndTime() ) {
		ArpASSERT("I should never happen");
		return newTime;
	}
	newTime = sig.BeatForTime(newTime);
	/* If this is marker has a matching pair, then make sure the newTime does not
	 * put this marker on or beyond the bounds of its pair.  If it does, don't move it.
	 */
	if (pair) {
		if (pairPosition == LEFT_PAIR) {
			if (newTime <= pair->time) return time;
		} else if (pairPosition == RIGHT_PAIR) {
			if (newTime >= pair->time) return time;
		}
	}

	return newTime;
}

bool _AmMarkerEntry::Contains(BPoint pt, const SeqMeasureControl* control, BPoint* outLeftTop) const
{
	if (!image || !mVisible || !mEnabled) return false;
	float	left = LeftPixel(control);
	float	width = image->Bounds().Width();
	if ( (pt.x >= left) && (pt.x <= left + width)
			&& (pt.y >= origin.y) && (pt.y <= bottom) ) {
		if (outLeftTop) {
			outLeftTop->x = left;
			outLeftTop->y = origin.y;
		}
		return true;
	}
	return false;
}

bool _AmMarkerEntry::IsVisible() const
{
	return mVisible;
}

bool _AmMarkerEntry::IsEnabled() const
{
	return mEnabled;
}

void _AmMarkerEntry::SetVisible(bool visible)
{
	mVisible = visible;
}

void _AmMarkerEntry::SetEnabled(bool enable)
{
	mEnabled = enable;
}

/*************************************************************************
 * Miscellaneous functions
 *************************************************************************/
//TODO:
/*static void write_pixel(const BBitmap* bm, float x, float y, rgb_color c, pixel_access& pa)
{
	uint8*		pixel = (uint8*)( ((uint8*)bm->Bits()) + (uint32)(x * pa.bpp() ) + (uint32)(y * bm->BytesPerRow() ) );
	pa.write(pixel, c);
}*/
