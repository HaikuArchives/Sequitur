/* AmChannelPressureView.h
 *
 * Copyright (c)2000 by Eric Hackborn.
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
 *  * None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 10.20.0		hackborn
 * Mutated from the AmControlChangeView.h
 */

#ifndef AMSTDFACTORY_AMCHANNELPRESSUREVIEW_H
#define AMSTDFACTORY_AMCHANNELPRESSUREVIEW_H

#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
class AmTrackMeasureBackground;
class AmViewPropertyI;

/***************************************************************************
 * AM-CHANNEL-PRESSURE-INFO-VIEW
 * The header for the data view.
 ***************************************************************************/
class AmChannelPressureInfoView : public AmTrackInfoView
{
public:
	AmChannelPressureInfoView(	BRect frame,
								AmSongRef songRef,
								AmTrackWinPropertiesI& trackWinProps,
								const AmViewPropertyI* property,
								TrackViewType viewType);

	virtual	void	GetPreferredSize(float *width, float *height);

protected:
	virtual void	DrawOn(BRect clip, BView* view);

private:
	typedef AmTrackInfoView		inherited;
};

/***************************************************************************
 * AM-CHANNEL-PRESSURE-DATA-VIEW
 * A view for editing AmChannelPressure events
 ***************************************************************************/
class AmChannelPressureDataView : public AmTrackDataView
{
public:
	AmChannelPressureDataView(	BRect frame,
								AmSongRef songRef,
								AmTrackWinPropertiesI& trackWinProps,
								const AmViewPropertyI& viewProp,
								TrackViewType viewType);

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual void MessageReceived(BMessage *msg);
	virtual	void ScrollTo(BPoint where);

protected:
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;

	BPoint	mPt1, mPt2;	// cached for drawing purposes
	float	mScale;		// cached for drawing purposes

	void AddAsObserver();
};


#endif 
