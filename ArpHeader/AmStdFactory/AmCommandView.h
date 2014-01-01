/* AmCommandView.h
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
 *  * None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2002.03.24			hackborn@angryredplanet.com
 * Mutated from the AmControlChangeView.h
 */

#ifndef AMSTDFACTORY_AMCOMMANDVIEW_H
#define AMSTDFACTORY_AMCOMMANDVIEW_H

#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
class AmTrackMeasureBackground;
class AmViewPropertyI;

/***************************************************************************
 * AM-COMMAND-INFO-VIEW
 * The info view for a view that allows selecting and changing sysex
 * commands grabbed from the current device.
 ***************************************************************************/
class AmCommandInfoView : public AmTrackInfoView
{
public:
	AmCommandInfoView(	BRect frame,
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
 * AM-COMMAND-DATA-VIEW
 * A view for editing AmChannelPressure events
 ***************************************************************************/
class AmCommandDataView : public AmTrackDataView
{
public:
	AmCommandDataView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI& viewProp,
						TrackViewType viewType);
	virtual ~AmCommandDataView();
	
	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual void MessageReceived(BMessage *msg);
	virtual	void ScrollTo(BPoint where);

protected:
	virtual void	PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;
	ArpCRef<AmDeviceI>			mDevice;

	BPoint	mPt1, mPt2;	// cached for drawing purposes
	float	mScale;		// cached for drawing purposes

	void AddAsObserver();
};


#endif 
