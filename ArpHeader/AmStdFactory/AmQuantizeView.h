/* AmQuantizeView.h
 *
 * This file contains both views needed (an info view and a data view)
 * to display and edit note quantize information.
 * 
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.06.17			hackborn@angryredplanet.com
 * Mutated from the original SeqPitchBendView file.
 */

#ifndef AMSTDFACTORY_AMQUANTIZEVIEW_H
#define AMSTDFACTORY_AMQUANTIZEVIEW_H

#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewPropertyI.h"
class AmTrackMeasureBackground;

/***************************************************************************
 * AM-QUANTIZE-INFO-VIEW
 ***************************************************************************/
class AmQuantizeInfoView : public AmTrackInfoView
{
public:
	AmQuantizeInfoView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual	void	GetPreferredSize(float *width, float *height);

protected:
	virtual void	DrawOn(BRect clip, BView* view);
};

/***************************************************************************
 * AM-QUANTIZE-DATA-VIEW
 ***************************************************************************/
class AmQuantizeDataView : public AmTrackDataView
{
public:
	AmQuantizeDataView(	BRect frame,
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
	virtual void	PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView 	inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;
	int32						mCachedGridMult;
	AmTime						mCachedGridVal;
	int32						mCachedGridDiv;
	AmTime						mCachedGridTime;
	
	BPoint	mPt1, mPt2;	// cached for drawing purposes
	float	mScale;		// cached for drawing purposes
	float	mMiddle;	// cached for speed purposes, defines
						// the middle of the bounds.  FIX:
						// we need to hear about frame events,
						// and reset this as appropriate.

	void AddAsObserver();
};


#endif
