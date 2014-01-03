/* SeqSongIndexMatrixView.h
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
 * 2001.05.05		hackborn@angryredplanet.com
 * Created this file
 */
#ifndef SEQUITUR_SEQSONGINDEXMATRIXVIEW_H
#define SEQUITUR_SEQSONGINDEXMATRIXVIEW_H

#include <vector.h>
#include <interface/View.h>
#include "AmPublic/AmSongRef.h"
class SeqSongSelections;
class SeqSongWinPropertiesI;

/*************************************************************************
 * _SEQ-INDEX-METRIC
 *************************************************************************/
class _SeqIndexMetric
{
public:
	_SeqIndexMetric();
	_SeqIndexMetric(const _SeqIndexMetric& o);
	_SeqIndexMetric(track_id trackId, uint32 trackIndex,
					uint32 modeFlags, uint32 groups,
					BRect frame, float modeW);
	
	_SeqIndexMetric&	operator=(const _SeqIndexMetric& o);
	void				DrawOn(	BView* view, BRect clip,
								SeqSongWinPropertiesI& props,
								float fontHeight);
	/* Answer the code for whichever button contains pt, or
	 * 0 if none of them do.
	 */
	uint32				ModeForPoint(BPoint pt) const;
	
	track_id			mTrackId;
	uint32				mModeFlags;
	uint32				mGroups;
	BRect				mFrame;
	float				mModeW;
	
	BRect				MuteRect() const;
	BRect				SoloRect() const;

private:
	void				SetIndex(uint32 index);
	BString				mLabel;
};

/*************************************************************************
 * SEQ-SONG-INDEX-MATRIX-VIEW
 * This class displays a vertically stacked list of the indexes for each
 * track in a song.  It allows users to set the selection by clicking and
 * dragging across the indexes.  It also includes controls for setting
 * the Mute and Solo track properties.
 *************************************************************************/
class SeqSongIndexMatrixView : public BView
{
public:
	SeqSongIndexMatrixView(	BRect frame,
							AmSongRef songRef,
							float modeWidth = 11);
	virtual ~SeqSongIndexMatrixView();

	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void	Draw(BRect clip);
	virtual void	MessageReceived(BMessage* msg);
	virtual	void	MouseDown(BPoint where);
	virtual	void	MouseMoved(	BPoint where,
								uint32 code,
								const BMessage* dragMessage);
	virtual	void	MouseUp(BPoint where);

	void			InvalidateTracks(vector<track_id>& tracks);
	void			FillMetrics(const AmSong* song);

private:
	typedef	BView			inherited;
	AmSongRef				mSongRef;
	vector<_SeqIndexMetric>	mMetrics;
	float					mModeWidth;		// The width of the area displaying
											// the mode buttons
	enum {
		DOWN_ON_NOTHING				= 0,
		DOWN_ON_SELECTED_TRACK		= 1,
		DOWN_ON_DESELECTED_TRACK	= 2,
		DOWN_ON_MODE				= 3
	};
	uint32					mMouseDown;		// Use one of the constants to 
											// describe what the mouse was pressed on.
	int32					mMouseDownIndex;
											// The index of the metric the mouse
											// was pressed on.
	uint32					mMouseDownOnMode;
											// If the mouse was pressed on a mode
											// button, note which one here.

	void		DrawOn(BView* view, BRect clip);

	void		TrackChangeReceived(BMessage* msg);
	
	bool		SelectBetween(	SeqSongSelections* selections,
								uint32 index1, uint32 index2) const;
	bool		DeselectBetween(SeqSongSelections* selections,
								uint32 index1, uint32 index2) const;

	void					PostMouseUp();
	_SeqIndexMetric*		MetricAt(BPoint pt, int32* index = NULL);
	_SeqIndexMetric*		MetricFor(track_id tid, int32* index = NULL);
	SeqSongWinPropertiesI*	SongWinProperties() const;
};

#endif
