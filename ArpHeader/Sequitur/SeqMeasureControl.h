/* SeqMeasureControl.h
 * Copyright (c)2000 by Eric Hackborn.
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
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.08.00		hackborn
 * Created this file.
 */

#ifndef SEQUITUR_SEQMEASURECONTROL_H
#define SEQUITUR_SEQMEASURECONTROL_H

#include <be/interface/Bitmap.h>
#include <be/interface/View.h>
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTimeConverter.h"
#include "AmPublic/AmViewFactory.h"
class SeqMeasureControl;

enum {
	CHANGE_SIGNATURE_MSG = 'aChS'
	// This message gets sent to my window when I want a signature changed.
	// If it has a single int32 named "measure", then open the sig change
	// window for that measure.
};

/**********************************************************************
 * _AM-MARKER-ENTRY
 * This class encapsulates the state of one entry in the measure's list
 * of markers.
 **********************************************************************/
class _AmMarkerEntry
{
public:
	_AmMarkerEntry();

	/* The MIDI time value of the marker.
	 */
	AmTime			time;
	/* The left-top pixel position of the marker.
	 */
	BPoint			origin;
	/* The bottom edge of the marker.
	 */
	float			bottom;
	/* Each element can have its own offset -- for example, the song
	 * position should be displayed with its current time value at the
	 * center of its image.  The left loop should be displayed left
	 * justified, the right loop right justified, etc.
	 */
	float			offset;
	const BBitmap*	image;
	/* Each element can determine what happens when the user grabs
	 * and drags them:  The drag can be freeform (NO_LOCK), it can
	 * be locked to each beat, and that's all right now.
	 */
	enum DragLock {
		NO_LOCK,
		LOCK_TO_BEAT
	};
	DragLock		dragLock;
	/* Some markers have a corresponding pair -- for example, the
	 * left loop marker is paired with the right loop marker, and
	 * vice versa.
	 */
	_AmMarkerEntry*	pair;
	enum {
		NO_PAIR		= 0,
		LEFT_PAIR,
		RIGHT_PAIR
	};
	int32			pairPosition;
	
	_AmMarkerEntry&	operator=(const _AmMarkerEntry &e);
	void			DrawOn(BRect clip, BView* view, const SeqMeasureControl* control);
	float			LeftPixel(const SeqMeasureControl* control) const;

	AmTime			TickFromPixel(float pixel, const SeqMeasureControl* control) const;
	AmTime			NoLockTickFromPixel(float pixel, const SeqMeasureControl* control) const;
	AmTime			LockToBeatTickFromPixel(float pixel, const SeqMeasureControl* control) const;
	
	bool			Contains(BPoint pt, const SeqMeasureControl* control, BPoint* outLeftTop) const;

	bool			IsVisible() const;
	bool			IsEnabled() const;
	void			SetVisible(bool visible);
	void			SetEnabled(bool enable);
	
private:
	bool			mVisible;
	bool			mEnabled;
};

/***************************************************************************
 * SEQ-MEASURE-CONTROL
 * This control displays a horizontal strip of measure information, as
 * well as a draggable position indicator.
 ***************************************************************************/
class SeqMeasureControl : public BView,
						  public AmSongObserver
{
public:
	/* The leftIndent is the number of pixels that will be chopped off the
	 * left edge and replaced with a 'scaled' view that always keeps the
	 * first measure visible.
	 */
	SeqMeasureControl(	BRect frame,
						const char* name,
						AmSongRef songRef,
						AmTimeConverter& mtc,
						float leftIndent = 0,
						float rightIndent = 0,
						int32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	SeqMeasureControl(	BRect frame,
						const char* name,
						AmSongRef songRef,
						AmTrackWinPropertiesI* trackWinProps,
						AmTimeConverter& mtc,
						float leftIndent = 0,
						float rightIndent = 0,
						int32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	virtual ~SeqMeasureControl();

	enum Markers {
		NO_MARKER			= 0,
		POSITION_MARKER		= 1,		// The song position marker
		LEFT_LOOP_MARKER	= 2,		// The range start marker
		RIGHT_LOOP_MARKER	= 3,		// The range end marker
		_NUM_MARKERS		= 4
	};
	AmTime			MarkerTime(Markers marker) const; 
	bool			MarkerEnabled(Markers marker) const; 
	bool			MarkerVisible(Markers marker) const; 
	/* Set the amount of space over that the actual, fixed measure control starts.
	 */
	void			SetLeftIndent(float leftIndent);
	/* Set the amount of space over from the right edge that the actual,
	 * fixed measure control ends.  A value of 0 indicates there is no
	 * right indent.  All other values must be positive.
	 */
	void			SetRightIndent(float rightIndent);
	void			SetMarkerTime(uint32 markerMask, AmTime time);
	void			SetMarkerEnabled(uint32 markerMask, bool enable);
	void			SetMarkerVisible(uint32 markerMask, bool visible);
	void			SetTransportLooping();
		
	virtual	void	Draw(BRect updateRect);
	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual	void	FrameResized(float new_width, float new_height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint pt);
	virtual	void	MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage* msg);
	virtual	void	ScrollTo(BPoint where);

protected:
	friend class _AmMarkerEntry;
	/* The conversion utility that translates measures to screen locations.
	 */
	AmTimeConverter&		mMtc;
	/* These values are the pixel widths of the scaled caps to the
	 * left and right of my fixed-width center.
	 */
	float					mLeftIndent, mRightIndent;
	/* The amount to offset the drawing of this view by.
	 */
	float					mScrollX;
	/* This stores a value of whatever screen element the user has
	 * clicked on and is dragging, or 0 if they haven't clicked on
	 * anything.  The possible screen elements are listed in the
	 * enum following the variable.
	 */
	uint32					mMouseDown;
	rgb_color				mViewColor;
//	AmTrackRef				mTrackRef;
	AmTrackWinPropertiesI*	mTrackWinProps;
	
	/* Render each of the backgrounds.
	 */
	virtual void	DrawLeftBgOn(BRect lBounds, BView* view, AmTime songEndTime);
	virtual void	DrawCenterBgOn(BRect cBounds, BView* view, AmTime songEndTime);
	virtual void	DrawRightBgOn(BRect rBounds, BView* view, AmTime songEndTime);

	void			DrawOn(BRect updateRect, BView* view);

	/* Draw the left area.  This area might be drawn at a fixed
	 * size, if the measure 1 fits in the view.  However, if measure
	 * 1 would appear to the left of the view, this area will be drawn
	 * at a scaled aspect, with measure 1 at the left, scaled to
	 * meet whatever view appears at the left of the center view.
	 */
	virtual void	DrawLeftOn(BRect lBounds, BView* view);
	void			LockedDrawLeftOn(const AmPhrase& signatures, BRect lBounds, BView* view);
	virtual void	DrawCenterOn(BRect bounds, BView* view);
	virtual void	DrawRightOn(BRect rBounds, BView* view);
	void			LockedDrawRightOn(	const AmPhrase& signatures,
										BRect rBounds, BView* view,
										AmTime songEndTime);

	/* Answer true if the current mScrollX value places the
	 * absolute value of measure 1 (according to my mMtc) on or to
	 * the right of my left bound (which is always zero).
	 */
	bool			IsLeftFixed() const;
	bool			IsRightFixed() const;
	/* Answer the time at the left and right edges of the center, fixed view.
	 */
	AmTime			CenterLeftTime() const;
	AmTime			CenterRightTime() const;
	
	/* Some gratuitous graphics.  This is a bit map that fades in
	 * left to right so that the left-hand indent can match up with
	 * the center view.
	 */
	BBitmap*		mLeftBg;
	BBitmap*		mRightBg;

	void			Initialize();
	/* Find the times for the left and right loop markers, based on
	 * the currently visible part of the screen:  Basically, loop over all
	 * visible measures.
	 */
	void			InitializeLoopTimes();
	
private:
	typedef BView			inherited;
	/* Used while dragging a maker, stores the last point you were at.
	 */
	BPoint					mDownPt;
	_AmMarkerEntry			mMarker[_NUM_MARKERS];
	/* Cache the song's end time so I don't have to lock the song
	 * to get access to it.
	 */
	AmTime					mCachedEndTime;
	
	void		HandleSignatureMsg(BMessage* msg);
	void		HandleMotionMsg(BMessage* msg);
	void		ShowTimeSignatureMenu(BPoint pt) const;
	status_t	SignatureForPt(BPoint pt, AmSignature& sig) const;
	
	/* Given a pt, answer whatever marker lies at it, or NO_MARKER if none.
	 */
	uint32		MarkerAt(BPoint pt, BPoint* outLeftTop) const;
	
	void		ConstructLeftBg(BRect bounds);
	void		ConstructRightBg(BRect bounds);
};

/***************************************************************************
 * SEQ-SIGNATURE-MEASURE-CONTROL
 * This control displays a horizontal strip of measure information.  The
 * caller retains ownership of the signatures, so make sure you stay
 * around as long as I do.  (Although not that this class CAN modify
 * the signatures, if the user makes use of the popupmenu)
 ***************************************************************************/
class SeqSignatureMeasureControl : public SeqMeasureControl
{
public:
	SeqSignatureMeasureControl(	BRect frame,
								const char* name,
								AmSignaturePhrase& signatures,
								AmTimeConverter& mtc,
								float leftIndent = 0,
								float rightIndent = 0,
								int32 resizeMask = B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

protected:
	virtual void		DrawCenterOn(BRect bounds, BView* view);

private:
	typedef SeqMeasureControl	inherited;
	AmSignaturePhrase&		mSignatures;
};

#endif
