/* ArpPitchBendView.h
 *
 * This file contains both views needed (an info view and a data view)
 * to display and edit pitch bend MIDI events.
 * 
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com> or <hackborn@genomica.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	-None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 11.22.98		hackborn
 * Mutated from the original SeqPitchBendView file.
 */

#ifndef AMSTDFACTORY_AMPITCHBENDVIEW_H
#define AMSTDFACTORY_AMPITCHBENDVIEW_H

#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewPropertyI.h"
class AmTrackMeasureBackground;

#define STR_PITCHBENDINFO		"PitchBendInfo"
#define STR_PITCHBENDDATA		"PitchBendData"

/***************************************************************************
 * AM-PITCH-BEND-INFO-VIEW
 ***************************************************************************/
class AmPitchBendInfoView : public AmTrackInfoView
{
public:
	AmPitchBendInfoView(BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual	void	GetPreferredSize(float *width, float *height);

protected:
	virtual void	DrawOn(BRect clip, BView* view);
};

/***************************************************************************
 * AM-PITCH-BEND-DATA-VIEW
 ***************************************************************************/
class AmPitchBendDataView : public AmTrackDataView
{
public:
	AmPitchBendDataView(	BRect frame,
							AmSongRef songRef,
							AmTrackWinPropertiesI& trackWinProps,
							const AmViewPropertyI& viewProp,
							TrackViewType viewType);

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual	void ScrollTo(BPoint where);
	virtual void MessageReceived(BMessage* msg);

protected:
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView 	inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;

	BPoint	mPt1, mPt2;	// cached for drawing purposes
	float	mScale;		// cached for drawing purposes
	float	mMiddle;	// cached for speed purposes, defines
						// the middle of the bounds.  FIX:
						// we need to hear about frame events,
						// and reset this as appropriate.

	void AddAsObserver();
};


#endif
