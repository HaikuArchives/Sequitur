/* SeqSongTitleMatrixView.h
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
 * 2001.06.04		hackborn@angryredplanet.com
 * Created this file
 */
#ifndef SEQUITUR_SEQSONGTITLEMATRIXVIEW_H
#define SEQUITUR_SEQSONGTITLEMATRIXVIEW_H

#include <app/Message.h>
#include <app/MessageRunner.h>
#include <interface/PopUpMenu.h>
#include <interface/ScrollBar.h>
#include <support/List.h>
#include "AmPublic/AmSongRef.h"
class ArpInlineTextView;

/*************************************************************************
 * _SEQ-TITLE-METRIC
 *************************************************************************/
class _SeqTitleMetric
{
public:
	_SeqTitleMetric();
	_SeqTitleMetric(const _SeqTitleMetric& o);
	_SeqTitleMetric(float top, float bottom, track_id trackId,
					const char* name);
	
	_SeqTitleMetric&	operator=(const _SeqTitleMetric& o);
	void				DrawOn(BView* view, BRect clip, float fontHeight);
	float				Line1(float fontHeight) const;

	float				mTop, mBottom;
	track_id			mTrackId;
	BString				mName;
};

/*************************************************************************
 * SEQ-SONG-TITLE-MATRIX-VIEW
 * This class displays a vertically stacked list of the titles for each
 * track in a song.  It allows users to set a track title by clicking and
 * waiting, and provides a simple menu of editing options.
 *************************************************************************/
class SeqSongTitleMatrixView : public BView
{
public:
	SeqSongTitleMatrixView(	BRect frame,
							AmSongRef songRef);
	virtual ~SeqSongTitleMatrixView();

	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void	Draw(BRect clip);
	virtual	void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage* dragMessage);
	virtual	void	MouseUp(BPoint where);
	virtual void	WindowActivated(bool state);

	void			SetHorizontalScrollBar(BScrollBar* sb);
	void			SetupScrollBars(bool horizontal = true, bool vertical = true);

	void			InvalidateTracks(vector<track_id>& tracks);
	void			StartEdit(track_id tid, bool sticky = true);
	void			StopEdit(bool keepChanges=true);
	void			FillMetrics(const AmSong* song);

protected:
//	void			StopAllEdits();
	void			RemoveTrack(track_id tid);

private:
	typedef	BView			inherited;
	AmSongRef				mSongRef;
	vector<_SeqTitleMetric>	mMetrics;
	BScrollBar*				mHsb;
	/* Time at which primary mouse button went down.
	 * -1 if not pressed.
	 */
	bigtime_t				mDownTime;
	track_id				mDownTrackId;
	/* Timer for bringing up text edit box after
	 * double-click time elapses.
	 */
	BMessageRunner*			mEditRunner;
	/* This is the text control I use to allow users to
	 * change the title.
	 */
	ArpInlineTextView*		mTextCtrl;
	track_id				mEditTrackId;
	/* This is a flag that can be set for editing.  If it's
	 * set to true, then the edit text ctrl stays active even
	 * if the mouse leaves its bounds.  In this case, the caller
	 * probably wants some additional way to deactive the control,
	 * like if the window loses focus, or another control becomes
	 * active.
	 */
	bool					mSticky;

	void					DrawOn(BView* view, BRect clip);
	void					TrackChangeReceived(BMessage* msg);
	_SeqTitleMetric*		MetricAt(BPoint pt, int32* index = NULL);
	_SeqTitleMetric*		MetricFor(track_id tid, int32* index = NULL);
	void					StartTimer(const BMessage& msg, bigtime_t delay);
	void					StopTimer();
	void					ShowMenu(track_id tid);	
};

#endif
