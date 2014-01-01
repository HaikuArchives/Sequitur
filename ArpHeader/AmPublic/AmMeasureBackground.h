/* AmMeasureBackground.h
 * Copyright (c)1998-2000 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- Something about the drawing routine is not right -- dragging
 * a window across a view with this background doesn't redraw the measures.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.10.00		hackborn
 * Updated old SeqMeasureBackground class and redid the drawing routines.
 * They were a little unnecessarily complex before.
 *
 * 11.01.98		hackborn
 * This class was having problems drawing when windows were dragged left-to-right
 * across it one pixel at a time.  This has been fixed.  One change was in do_Draw().
 * where it now prevents sending a value of less than 1 to NodeAt(), plus it does
 * and ARP_BACKWARDS search.  The other changes were in ArpIndexList and
 * AmTimeConveter, check there header files for comments.
 *
 * 10.20.98		hackborn
 * Created this file
 */
 
#ifndef AMPUBLIC_AMMEASUREBACKGROUND_H
#define AMPUBLIC_AMMEASUREBACKGROUND_H

#include "ArpKernel/ArpLineArrayCache.h"
#include "ArpViews/ArpBackground.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
#include "AmPublic/AmTimeConverter.h"
class AmNode;
class AmPhrase;
class AmSignature;
class AmSignaturePhrase;
class AmSong;
class AmTrack;

/*************************************************************************
 * AM-MEASURE-BACKGROUND
 * This abstract class is used for drawing signature information, which
 * could be measure markers, beat markers, time signature, or whatever a
 * subclass wants to render.
 *
 * It uses the background scheme that is implemented between
 * ArpLookPolicyView and ArpBackground, see those classes for
 * an explanation.
 *************************************************************************/
class AmMeasureBackground : public ArpBackground
{
public:
	AmMeasureBackground(const AmTimeConverter& mtc);

	void		SetTrack(track_id trackId);
	void		SetMeasureLineLimits(float top, float bottom);
	void		SetBeatLineLimits(float top, float bottom);
	/* Very few clients will need these to be anything but the
	 * default -- they are necessary if the client is handling the
	 * scrolling itself.
	 */
	void		SetScrollX(float scrollX = 0);
	void		SetLeftIndent(float leftIndent = 0);
	/* Set the color to draw each beat.  This is different then
	 * the measure color.
	 */
	void		SetBeatColor(rgb_color c);
	enum {
		DRAW_SIGNATURES_FLAG	= 0x00000001,
		DRAW_MEASURE_FLAG		= 0x00000002,
		DRAW_BEATS_FLAG			= 0x00000004
	};
	void			SetFlag(uint32 flag, bool on);

	/* Perform the actual drawing.  Most clients will want to ignore this
	 * and just call Draw(); however, if you already have a lock on your
	 * object, there's no reason to get it again.
	 */
	void			LockedDraw(	BView* view,
								BRect updateRect,
								const AmPhrase& signatures,
								const AmPhrase* motions = NULL);

protected:
	const AmTimeConverter&	mMtc;
	track_id				mTrackId;
	/* An object to help handle drawing all those lines.
	 */
	ArpLineArrayCache		mLines;
	/* These points have been populated with the correct y values.  Each
	 * measure will typically fill in the x and draw them.
	 */
	BPoint					mBeatStart, mBeatEnd;
	BPoint					mMeasureStart, mMeasureEnd;
	/* Values to offset where the drawing occurs.  These are only
	 * necessary if the client is handling the scrolling itself.
	 */
	float					mScrollX;
	float					mLeftIndent;
	uint32					mFlags;
	/* The color to draw the beats.
	 */
	rgb_color				mBeatC;

	/* Subclasses must overwrite this method to lock the song and call
	 * LockedDraw() with the appropriate signature phrase.
	 */
	virtual void DrawOn(BView* view, BRect clip) = 0;
	/* Subclasses can override to change the behaviour of drawing
	 * a single measure.  The measure argument contains the info for the
	 * current measure to be drawn.
	 */
	virtual void DrawMeasureOn(	BRect updateRect,
								BView* view,
								AmSignature& measure) = 0;
	
private:
	float	mBeatTop, mBeatBottom;			// These control the top and
												// bottom lines of the non-measure
												// beat markers.
	float	mMeasureTop, mMeasureBottom;	// These control the top and
												// bottom lines of the measure
												// beat markers.

	void AssignPoints(	BPoint* startPt,
						BPoint* endPt,
						float top,
						float bottom,
						BRect updateRect);
	void DrawMeasuresOn(BRect updateRect,
						BView* view,
						AmNode* node,
						AmSignature& currentSig,
						AmNode* motionNode);
};

/*************************************************************************
 * AM-TRACK-MEASURE-BACKGROUND
 * This concrete subclass of AmMeasureBackground draws the measure
 * information from an AmTrack.  By default, it draws a line for the
 * measure and a lighter line for each beat.
 *************************************************************************/
class AmTrackMeasureBackground : public AmMeasureBackground
{
public:
	AmTrackMeasureBackground(	AmSongRef songRef,
								AmTrackRef trackRef,
								const AmTimeConverter& mtc);

	void			SetTrackRef(AmTrackRef trackRef);
	
protected:
	/* Call LockedDraw() with my track's signatures.
	 */
	virtual void	DrawOn(BView* view, BRect clip);

	/* Subclasses can override to change the behaviour of drawing
	 * a single measure.  The measure argument contains the info for the
	 * current measure to be drawn.
	 */
	virtual void	DrawMeasureOn(	BRect clip,
									BView* view,
									AmSignature& measure);
	
private:
	typedef AmMeasureBackground	inherited;
	AmSongRef					mSongRef;
	AmTrackRef					mTrackRef;
};

/*************************************************************************
 * AM-SONG-MEASURE-BACKGROUND
 * This concrete subclass of AmMeasureBackground draws the measure
 * information from an AmSong.  By default, it draws each measure line
 * and the number of the  measure.
 *************************************************************************/
class AmSongMeasureBackground : public AmMeasureBackground
{
public:
	AmSongMeasureBackground(AmSongRef songRef,
							const AmTimeConverter& mtc);

protected:
	/* Call LockedDraw() with my song's signatures.
	 */
	virtual void DrawOn(BView* view, BRect clip);

	/* Subclasses can override to change the behaviour of drawing
	 * a single measure.  The measure argument contains the info for the
	 * current measure to be drawn.
	 */
	virtual void DrawMeasureOn(	BRect updateRect,
								BView* view,
								AmSignature& measure);
	
private:
	typedef AmMeasureBackground	inherited;
	AmSongRef					mSongRef;
	/* A little buffer to write a string for each measure number.
	 */
	char						mBuf[32];
};

/*************************************************************************
 * AM-SIGNATURE-MEASURE-BACKGROUND
 * This concrete subclass of AmMeasureBackground draws the measure
 * information from an AmSignaturePhrase.  This class doesn't take
 * ownership of the phrase and it doesn't copy it, so make sure it
 * exists while this class exists.
 *************************************************************************/
class AmSignatureMeasureBackground : public AmMeasureBackground
{
public:
	AmSignatureMeasureBackground(	const AmSignaturePhrase& signatures,
									const AmTimeConverter& mtc);

protected:
	/* Call LockedDraw() with my song's signatures.
	 */
	virtual void DrawOn(BView* view, BRect clip);

	/* Subclasses can override to change the behaviour of drawing
	 * a single measure.  The measure argument contains the info for the
	 * current measure to be drawn.
	 */
	virtual void DrawMeasureOn(	BRect clip,
								BView* view,
								AmSignature& measure);
	
private:
	typedef AmMeasureBackground	inherited;
	const AmSignaturePhrase&	mSignatures;
};

#endif 
