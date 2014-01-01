/* AmMeasureBackground.cpp
 */
#include <assert.h>
#include <stdio.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmMeasureBackground.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

/*************************************************************************
 * AM-MEASURE-BACKGROUND
 *************************************************************************/
AmMeasureBackground::AmMeasureBackground(const AmTimeConverter& mtc)
		: mMtc(mtc), mTrackId(0), mScrollX(0), mLeftIndent(0), mFlags(0)
{
	mBeatTop = mBeatBottom = mMeasureTop = mMeasureBottom = -1;
	mBeatC = Prefs().Color(AM_MEASURE_BEAT_C);
}

void AmMeasureBackground::SetTrack(track_id trackId)
{
	mTrackId = trackId;
}

void AmMeasureBackground::SetMeasureLineLimits(float top, float bottom)
{
	mMeasureTop = top;
	mMeasureBottom = bottom;
}

void AmMeasureBackground::SetBeatLineLimits(float top, float bottom)
{
	mBeatTop = top;
	mBeatBottom = bottom;
}

void AmMeasureBackground::SetScrollX(float scrollX)
{
	mScrollX = scrollX;
}

void AmMeasureBackground::SetLeftIndent(float leftIndent)
{
	mLeftIndent = leftIndent;
}

#if 0
	/* If set to true, the view will draw the signature changes above
	 * the measure numbers.
	 */
	void SetDrawSignatures(bool on);

void AmMeasureBackground::SetDrawSignatures(bool on)
{
	if (on) mFlags |= DRAW_SIGNATURES_FLAG;
	else mFlags &= ~DRAW_SIGNATURES_FLAG;
}
#endif

void AmMeasureBackground::SetBeatColor(rgb_color c)
{
	mBeatC = c;
}

void AmMeasureBackground::SetFlag(uint32 flag, bool on)
{
	if (on) mFlags |= flag;
	else mFlags &= ~flag;
}

void AmMeasureBackground::LockedDraw(	BView* view,
										BRect updateRect,
										const AmPhrase& signatures,
										const AmPhrase* motions)
{
	BRect				b = updateRect;
	AssignPoints(&mBeatStart, &mBeatEnd, mBeatTop, mBeatBottom, b);
	AssignPoints(&mMeasureStart, &mMeasureEnd, mMeasureTop, mMeasureBottom, b);
	AmTime				tick = mMtc.PixelToTick(b.left);

	AmNode*				node = signatures.FindNode( tick, BACKWARDS_SEARCH );
	if( !node ) return;
	AmSignature*		sig = dynamic_cast<AmSignature*>( node->Event() );
	if (sig == 0) return;
	AmSignature			currentSig(*sig);
	AmTime				sigLength = currentSig.Duration();
	AmSignature*		nextSig = 0;
	AmNode*				nextNode = (AmNode*)node->next;
	if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );

	// Seek to the first visible signature.
	while (currentSig.EndTime() < tick) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			currentSig.Set( *nextSig );
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = (AmNode*)nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
		}
	}

	mLines.SetTarget(view);
	mLines.BeginLineArray();
	AmNode*		motionNode = NULL;
	if (motions) motionNode = motions->HeadNode();
	DrawMeasuresOn(b, view, node, currentSig, motionNode);
	mLines.EndLineArray();
	mLines.SetTarget(0);
}

// Set the points so that they have the correct top and bottom info when drawing.
void AmMeasureBackground::AssignPoints(	BPoint* startPt,
										BPoint* endPt,
										float top,
										float bottom,
										BRect updateRect)
{
	if (top < 0) {
		startPt->y = updateRect.top;
	} else {
		if (top > updateRect.top) {
			startPt->y = top;
		} else {
			startPt->y = updateRect.top;
		}
	}
	if (bottom < 0) {
		endPt->y = updateRect.bottom;
	} else {
		if (bottom < updateRect.bottom) {
			endPt->y = bottom;
		} else {
			endPt->y = updateRect.bottom;
		}
	}
}			

static float view_font_height(BView* view)
{
	font_height		fh;
	view->GetFontHeight( &fh );
	return fh.ascent + fh.descent + fh.leading;
}

static void add_motion_label(BString& str, AmMotionChange* event)
{
	if (!event) return;
	if (str.Length() > 0) str << " - ";
	if (!event->HasMotion() ) str << "None";
	else {
		BString		l = event->Label();
		if (l.Length() < 1) str < " <motion>";
		else str << l;
	}
}

void AmMeasureBackground::DrawMeasuresOn(	BRect updateRect,
											BView* view,
											AmNode* node,
											AmSignature& currentSig,
											AmNode* motionNode)
{
	AmTime				endTime = mMtc.PixelToTick(updateRect.right + mScrollX - mLeftIndent);
	float				fh = view_font_height(view);
	AmTime				sigLength = currentSig.Duration();
	AmSignature*		nextSig = 0;
	AmNode*				nextNode = (AmNode*)node->next;
	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	drawing_mode oldMode = view->DrawingMode();
	view->SetDrawingMode(B_OP_OVER);
	BRect				r(updateRect);
	r.top = r.bottom - fh;
	float				sigOffsert = 3;
	AmMotionChange*		currentMotion = NULL;
	if (motionNode) currentMotion = dynamic_cast<AmMotionChange*>(motionNode->Event() );
	/* Draw a separating line in the center.
	 */
	if (mFlags&DRAW_SIGNATURES_FLAG) {
		mLines.AddLine( BPoint(r.left,r.top-1), BPoint(r.right,r.top-1), Prefs().Color(AM_MEASURE_FG_C) );
		mLines.AddLine( BPoint(r.left,r.top), BPoint(r.right,r.top), Prefs().Color(AM_MEASURE_HIGHLIGHT_C) );
	}

	while (currentSig.StartTime() <= endTime + 1) {
		DrawMeasureOn(r, view, currentSig);
		if (mFlags&DRAW_SIGNATURES_FLAG && currentSig.Measure() == 1) {
			float	x = mMtc.TickToPixel( currentSig.StartTime() ) - mScrollX + mLeftIndent;
			BString	change;
			change << currentSig.Beats() << "/" << currentSig.BeatValue();
			if (currentMotion && currentMotion->Measure() == 1) {
				add_motion_label(change, currentMotion);
				motionNode = motionNode->next;
				if (motionNode) currentMotion = dynamic_cast<AmMotionChange*>(motionNode->Event() );
				else currentMotion = NULL;
			}
			view->DrawString( change.String(), BPoint(x + 2, updateRect.bottom - sigOffsert - fh) );
			mLines.AddLine( BPoint(x,updateRect.top), BPoint(x,r.top), Prefs().Color(AM_MEASURE_FG_C) );
		}
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );
		if (nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			int32	measure = currentSig.Measure();
			currentSig.Set( *nextSig );
			currentSig.SetMeasure( measure );
			sigLength = currentSig.Duration();
			nextNode = nextNode->next;
			if( nextNode ) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;

			if (mFlags&DRAW_SIGNATURES_FLAG) {
				float	x = mMtc.TickToPixel( currentSig.StartTime() ) - mScrollX + mLeftIndent;
				BString	change;
				change << currentSig.Beats() << "/" << currentSig.BeatValue();
				if (currentMotion && currentMotion->Measure() == currentSig.Measure() ) {
					add_motion_label(change, currentMotion);
					motionNode = motionNode->next;
					if (motionNode) currentMotion = dynamic_cast<AmMotionChange*>(motionNode->Event() );
					else currentMotion = NULL;
				}
				view->DrawString( change.String(), BPoint(x + 2, updateRect.bottom - sigOffsert - fh) );
				mLines.AddLine( BPoint(x,updateRect.top), BPoint(x,r.top), Prefs().Color(AM_MEASURE_FG_C) );
			}
		}
		if (mFlags&DRAW_SIGNATURES_FLAG && currentMotion && currentMotion->Measure() == currentSig.Measure() ) {
			float	x = mMtc.TickToPixel( currentSig.StartTime() ) - mScrollX + mLeftIndent;
			BString		change;
			add_motion_label(change, currentMotion);
			motionNode = motionNode->next;
			if (motionNode) currentMotion = dynamic_cast<AmMotionChange*>(motionNode->Event() );
			else currentMotion = NULL;
			view->DrawString( change.String(), BPoint(x + 2, updateRect.bottom - sigOffsert - fh) );
			mLines.AddLine( BPoint(x,updateRect.top), BPoint(x,r.top), Prefs().Color(AM_MEASURE_FG_C) );
		}
	}
	view->SetDrawingMode(oldMode);
}

/*************************************************************************
 * AM-TRACK-MEASURE-BACKGROUND
 *************************************************************************/
AmTrackMeasureBackground::AmTrackMeasureBackground(	AmSongRef songRef,
													AmTrackRef trackRef,
													const AmTimeConverter& mtc)
		: inherited(mtc), mSongRef(songRef), mTrackRef(trackRef)
{
	mFlags = DRAW_MEASURE_FLAG | DRAW_BEATS_FLAG;
}

void AmTrackMeasureBackground::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
}

void AmTrackMeasureBackground::DrawOn(BView* view, BRect clip)
{
	#ifdef AM_TRACE_LOCKS
	printf("AmTrackMeasureBackground::DrawOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	const AmTrack*	track = song ? song->Track( mTrackRef ) : 0;
	if (track) LockedDraw(view, clip, track->Signatures(), &(track->Motions()) );
	mSongRef.ReadUnlock(song);
}

void AmTrackMeasureBackground::DrawMeasureOn(	BRect clip,
												BView* view,
												AmSignature& measure)
{
	float	x = mMtc.TickToPixel( measure.StartTime() ) - mScrollX + mLeftIndent;
	// Draw the measure line
	if (mFlags&DRAW_MEASURE_FLAG) {
		if( x >= (clip.left-1) && x <= (clip.right+1) ) {
			mMeasureStart.x = mMeasureEnd.x = x;
			mLines.AddLine( mMeasureStart, mMeasureEnd, Prefs().Color(AM_MEASURE_FG_C) );
		}
	}
	// Draw the beat lines
	if (mFlags&DRAW_BEATS_FLAG) {
		for (uint32 beat=2; beat<=measure.Beats(); beat++) {
			mBeatStart.x = mBeatEnd.x = mMtc.TickToPixel( measure.StartTime() + (measure.TicksPerBeat() * (beat - 1)) );
			if( mBeatStart.x >= (clip.left-2) && mBeatStart.x <= (clip.right+2) )
				mLines.AddLine( mBeatStart, mBeatEnd, mBeatC );		
		}
	}
}

/*************************************************************************
 * AM-SONG-MEASURE-BACKGROUND
 *************************************************************************/
AmSongMeasureBackground::AmSongMeasureBackground(	AmSongRef songRef,
													const AmTimeConverter& mtc)
		: inherited(mtc), mSongRef(songRef)
{
}

void AmSongMeasureBackground::DrawOn(BView* view, BRect clip)
{
	#ifdef AM_TRACE_LOCKS
	printf("AmSongMeasureBackground::DrawOn() read lock\n"); fflush(stdout);
	#endif
	const AmSong*			song = mSongRef.ReadLock();
	if (song) {
		const AmPhrase*		motions = NULL;
		if (mTrackId != 0) {
			const AmTrack*	track = song->Track(mTrackId);
			if (track) motions = &(track->Motions());
		}
		LockedDraw(view, clip, song->Signatures(), motions);
	}
	mSongRef.ReadUnlock(song);
}

void AmSongMeasureBackground::DrawMeasureOn(BRect updateRect,
											BView* view,
											AmSignature& measure)
{
	float	x = mMtc.TickToPixel( measure.StartTime() ) - mScrollX + mLeftIndent;
	mLines.AddLine( BPoint(x, updateRect.top), BPoint(x, updateRect.bottom), Prefs().Color(AM_MEASURE_FG_C) );
	sprintf( mBuf, "%ld", measure.Measure() );
	view->DrawString( mBuf, BPoint(x + 2, updateRect.bottom - 2) );
}

/*************************************************************************
 * AM-SIGNATURE-MEASURE-BACKGROUND
 *************************************************************************/
AmSignatureMeasureBackground::AmSignatureMeasureBackground(	const AmSignaturePhrase& signatures,
															const AmTimeConverter& mtc)
		: inherited(mtc), mSignatures(signatures)
{
}

void AmSignatureMeasureBackground::DrawOn(BView* view, BRect clip)
{
	LockedDraw(view, clip, mSignatures, NULL);
}

void AmSignatureMeasureBackground::DrawMeasureOn(	BRect clip,
													BView* view,
													AmSignature& measure)
{
	float	x = mMtc.TickToPixel( measure.StartTime() ) - mScrollX + mLeftIndent;
	// Draw the measure line
	if (mFlags&DRAW_MEASURE_FLAG) {
		if( x >= (clip.left-1) && x <= (clip.right+1) ) {
			mMeasureStart.x = mMeasureEnd.x = x;
			mLines.AddLine( mMeasureStart, mMeasureEnd, Prefs().Color(AM_MEASURE_FG_C) );
		}
	}
	// Draw the beat lines
	if (mFlags&DRAW_BEATS_FLAG) {
		for (uint32 beat=2; beat<=measure.Beats(); beat++) {
			mBeatStart.x = mBeatEnd.x = mMtc.TickToPixel( measure.StartTime() + (measure.TicksPerBeat() * (beat - 1)) );
			if( mBeatStart.x >= (clip.left-2) && mBeatStart.x <= (clip.right+2) )
				mLines.AddLine( mBeatStart, mBeatEnd, mBeatC );		
		}
	}
}
