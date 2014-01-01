/* ArpBankChangeView.h
 *
 * This file contains both views needed (an info view and a data view)
 * to display and edit bank and program change MIDI events.
 * 
 * Copyright (c)2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com> or <hackborn@genomica.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
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
 * 2001.01.27			hackborn@angryredplanet.com
 * Created this file.
 */

#ifndef AMSTDFACTORY_AMBANKCHANGEVIEW_H
#define AMSTDFACTORY_AMBANKCHANGEVIEW_H

#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
#include "AmPublic/AmDeviceI.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewPropertyI.h"
class AmTrackMeasureBackground;

/*************************************************************************
 * AM-BANK-CHANGE-INFO-VIEW
 *************************************************************************/
class AmBankChangeInfoView : public AmTrackInfoView
{
public:
	AmBankChangeInfoView(BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual	void	GetPreferredSize(float* width, float* height);

protected:
	virtual void	DrawOn(BRect clip, BView* view);
};

/*************************************************************************
 * AM-BANK-CHANGE-DATA-VIEW
 *************************************************************************/
class AmBankChangeDataView : public AmTrackDataView
{
public:
	AmBankChangeDataView(	BRect frame,
							AmSongRef songRef,
							AmTrackWinPropertiesI& trackWinProps,
							const AmViewPropertyI& viewProp,
							TrackViewType viewType);

	virtual ~AmBankChangeDataView();

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual void MessageReceived(BMessage* msg);
	virtual	void ScrollTo(BPoint where);

protected:
	virtual void 	PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void 	PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView 	inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;
	ArpCRef<AmDeviceI>			mDevice;

	void						AddAsObserver();
};

#endif 
