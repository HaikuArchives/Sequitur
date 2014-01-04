/* SeqPhraseMatrixView.h
 * Copyright (c)2001 by Angry Red Planet.
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
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2001.03.05		hackborn@angryredplanet.com
 * Created this file
 */

#ifndef SEQUITUR_SEQPHRASEMATRIXVIEW_H
#define SEQUITUR_SEQPHRASEMATRIXVIEW_H

#include <app/Messenger.h>
#include <interface/View.h>
#include "ArpKernel/ArpLineArrayCache.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTimeConverter.h"
#include "Sequitur/SeqPhraseTool.h"
class AmPhraseRendererI;
class AmTrack;
class ArpMultiScrollBar;
class SeqMeasureControl;

/*************************************************************************
 * _SEQ-TRACK-METRIC
 *************************************************************************/
class _SeqTrackMetric
{
public:
	_SeqTrackMetric();
	_SeqTrackMetric(const _SeqTrackMetric& o);
	_SeqTrackMetric(track_id trackId, float top, float bottom,
					AmPhraseRendererI* renderer);
	virtual ~_SeqTrackMetric();
	
	_SeqTrackMetric&	operator=(const _SeqTrackMetric &o);
	bool				Contains(float pointY) const;
	
	track_id			mTrackId;
	float				mTop, mBottom;
	AmPhraseRendererI*	mRenderer;
};
typedef vector<_SeqTrackMetric>		trackmetric_vec;

/*************************************************************************
 * _SEQ-DRAG-METRIC
 *************************************************************************/
class _SeqDragMetric
{
public:
	_SeqDragMetric();
	_SeqDragMetric(const _SeqDragMetric& o);
	
	_SeqDragMetric&	operator=(const _SeqDragMetric &o);
	bool			operator!=(const _SeqDragMetric &o);

	void			Clear();
	void			SetFrom(AmTime time, BPoint where,
							const BMessage* dragMessage,
							trackmetric_vec& trackMetrics);
	bool			IsValid() const;

	AmRange					mRange;
	vector<uint32>			mSrcIndexes;
	int32					mSrcIndex;
	int32					mDestIndex;
};

/*************************************************************************
 * SEQ-PHRASE-MATRIX-VIEW
 * This class displays all phrases from all tracks as a single view.  It
 * replaces the old method, which was to let the view factories
 * instantiate a view for each track and do its own drawing.  While that
 * was conceptually cleaner, the bottom line is it wreaks havoc with the
 * song position line -- there's simply no way for the line drawing to
 * be synchronized across all the views.  This drawing method should provide
 * a bit of a performance improvement, too.
 *************************************************************************/
class SeqPhraseMatrixView : public BView
{
public:
	SeqPhraseMatrixView(BRect frame,
						AmSongRef songRef,
						const AmTimeConverter& mtc);
	virtual ~SeqPhraseMatrixView();

	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void	Draw(BRect clip);
	virtual	void	FrameMoved(BPoint new_position);
	virtual void	FrameResized(float new_width, float new_height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage* dragMessage);
	virtual	void	MouseUp(BPoint where);
	virtual	void	ScrollTo(BPoint where);

	void			SetHorizontalScrollBar(BScrollBar* sb);
	void			SetVerticalScrollBar(ArpMultiScrollBar* sb);
	/* The measure control should be aligned directly with this arrange view:
	 * It should move and resize based on the position and size of this view.
	 * To accomplish this, assign the measure control here.
	 */
	void			SetMeasureView(SeqMeasureControl* measureView);
	void			SetupScrollBars(bool horizontal = true, bool vertical = true);
	/* Given what is essentially an SeqSongSelections object -- a
	 * time range and a series of track indexes -- invalidate all my
	 * tracks at the supplied indexes by the given range.
	 */
	void			InvalidateSelections(AmRange range, vector<track_id>& tracks);
	/* Clear the last song position, and draw a new one at the supplied time.
	 * Pass in -1 to draw no song position.
	 */
	void			DrawSongPosition(AmTime time);
	void			ShowDragMark(	AmTime time, BPoint where,
									const BMessage* dragMessage);
	void			ClearDragMark();

	void			FillTrackMetrics(const AmSong* song);

private:
	typedef	BView			inherited;
	AmSongRef				mSongRef;
	const AmTimeConverter&	mMtc;
	trackmetric_vec			mTrackMetrics;
	SeqMeasureControl*		mMeasureView;
	/* This is to handle the mouse interaction -- selecting and
	 * dragging phrases.
	 */
	SeqPhraseTool			mTool;
	/* A convenience object for drawing all my lines.
	 */
	ArpLineArrayCache		mLines;
	/* This gets generated each time this class draws -- it's just
	 * set to true if the track is muted (and not solo'd), meaning
	 * that the track should draw in a little darker colour.
	 */
	bool					mDrawMuted;
	/* This gets generated with each DrawOn() -- just cache the
	 * current font_height, so I'm not constantly finding it.
	 */
	float					mCurLabelH;
	/* The start of every DrawOn() checks to see if this track
	 * is part of the window's selections and, if so, fills the
	 * selection range in.  If this track has no selections,
	 * the range is invalid.
	 */
	AmRange					mSelectionRange;
	float					mSongPosition;
	_SeqDragMetric			mDragMetric;
	/* Time at which primary mouse button went down.
	 * -1 if not pressed.
	 */
	bigtime_t				mDownTime;
	BPoint					mDownPt;
	/* Timer for bringing up text edit box after
	 * double-click time elapses.
	 */
	BMessageRunner*			mPopUpRunner;
	
	AmTime					mCachedEndTime;
	ArpMultiScrollBar*		mVsb;
	BScrollBar*				mHsb;

	BMessenger				mPhrasePropWin;

	void	AddAsObserver();

	void	DrawOn(BRect clip, BView* view);
	void	DrawOn(BRect clip, BView* view, const AmTrack* track, AmPhraseRendererI* renderer);
	void	DrawTrack(BRect clip, BView* view, const AmTrack* track, AmPhraseRendererI* renderer);
	void	DrawTopLevelPhrase(	BRect clip, BView* view,
								const AmTrack* track,
								const AmPhraseEvent* event,
								AmTime end,
								AmPhraseEvent* topPhrase,
								AmPhraseRendererI* renderer);

	bool		TrackMessageReceived(const BMessage* msg);
	void		TrackChangeReceived(BMessage* msg);
	
	void	SetHsbRange();
	void	SetHsbSteps();
	float	HorizontalWidth() const;

	void	SetVsbRange();
	void	SetVsbSteps();
	_SeqTrackMetric*	TrackMetric(track_id tid);
	float				TrackTop(uint32 position) const;
	float				TrackTop(track_id tid) const;
	/* This answers the pixel bottom of the list of tracks.  This
	 * is just a convenience for the scroll bars.
	 */
	float	TrackBottom(float* smallest = NULL) const;

	/* This is sort-of a hack because the SeqPhraseTool has been
	 * moved over from one that was based on having a view for
	 * each track.  This method finds the current mouse position
	 * and answers the track id there, if any.
	 */
	track_id	CurrentTrackId();
	track_id	TrackId(BPoint where);

	void		StartPopUpTimer(const BMessage& msg, bigtime_t delay);
	void		StopPopUpTimer();
	void		ShowPopUp();

	void		ShowProperties(BPoint where);
	void		ShowPropertiesWin(AmPhraseEvent* pe);
};

#endif
