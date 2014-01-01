/* ArpProgramChangeView.h
 *
 * This file contains both views needed (an info view and a data view)
 * to display and edit program change MIDI events.
 * 
 * Copyright (c)1997 by Eric Hackborn.
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
 *	• There is a difficulty in InterestingEventAt():   -- program changes
 * can be drawn on the screen to any length, but the actual data representation
 * is that they only exist at a single time.  This creates an issue with using
 * the line ChainHeadFrom() -- the user might click on the later part of a
 * program change name displayed on the screen, but the program change itself
 * might be stored in a chain before where the user clicked, causing it not to
 * be found.  Until we can think of a way around this, we'll just have to search
 * from the beginning of the list.
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
 * 11.22.98		hackborn
 * Mutated from the original SeqProgramChangeView file.
 */

#ifndef AMSTDFACTORY_AMPROGRAMCHANGEVIEW_H
#define AMSTDFACTORY_AMPROGRAMCHANGEVIEW_H

#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
class AmTrackMeasureBackground;
class AmViewPropertyI;

/*************************************************************************
 * AM-PROGRAM-CHANGE-INFO-VIEW
 *************************************************************************/
class AmProgramChangeInfoView : public AmTrackInfoView
{
public:
	AmProgramChangeInfoView(BRect frame,
							AmSongRef songRef,
							AmTrackWinPropertiesI& trackWinProps,
							const AmViewPropertyI* property,
							TrackViewType viewType);

	virtual	void	GetPreferredSize(float *width, float *height);

protected:
	virtual void	DrawOn(BRect clip, BView* view);
};

/*************************************************************************
 * AM-PROGRAM-CHANGE-DATA-VIEW
 *************************************************************************/
class AmProgramChangeDataView : public AmTrackDataView
{
public:
	AmProgramChangeDataView(	BRect frame,
								AmSongRef songRef,
								AmTrackWinPropertiesI& trackWinProps,
								const AmViewPropertyI& viewProp,
								TrackViewType viewType);

	virtual ~AmProgramChangeDataView();

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual void MessageReceived(BMessage* msg);
	virtual	void ScrollTo(BPoint where);

protected:
	virtual void	PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView 	inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;
	ArpCRef<AmBankI>			mBank;

	void AddAsObserver();
};

#endif 
