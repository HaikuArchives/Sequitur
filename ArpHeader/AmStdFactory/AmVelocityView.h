/* AmVelocityView.h
 * Copyright (c)2001 by Eric Hackborn.
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
 * 2001.01.08		hackborn
 * Mutated this file from AmControlChangeView.h.
 */

#ifndef AMSTDFACTORY_AMVELOCITYVIEW_H
#define AMSTDFACTORY_AMVELOCITYVIEW_H

#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
class AmTrackMeasureBackground;
class AmViewPropertyI;

/**********************************************************************
 * AM-VELOCITY-INFO-VIEW
 **********************************************************************/
class AmVelocityInfoView : public AmTrackInfoView
{
public:
	AmVelocityInfoView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual void	SetConfiguration(const BMessage* config);

	virtual	void	GetPreferredSize(float *width, float *height);
	virtual void	MessageReceived(BMessage *msg);

protected:
	virtual void	DrawOn(BRect clip, BView* view);
	/* Annotate the properties menu with my list of control changes.
	 */
	virtual BPopUpMenu*	NewPropertiesMenu() const;

private:
	typedef 	AmTrackInfoView inherited;

	uint32		mDisplayFlags;
	mutable BMenu* mCachedVelocityMenu;

	BMenu*		NewVelocityMenu() const;
	BMenu*		VelocityMenu() const;
};

/**********************************************************************
 * AM-VELOCITY-DATA-VIEW
 **********************************************************************/
class AmVelocityDataView : public AmTrackDataView
{
public:
	AmVelocityDataView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI& viewProp,
						TrackViewType viewType);

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual void MessageReceived(BMessage *msg);
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual	void ScrollTo(BPoint where);
	virtual BMessage* ConfigurationData();

protected:
	/* It's expected that this is being set from the info view,
	 * so the info view already displays the correct display flags.
	 * I.e., all I have to do is update myself and my target.
	 */
	void			SetDisplayFlags(uint32 displayFlags);
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView 	inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;

	BPoint			mPt1, mPt2;	// cached for drawing purposes
	float			mScale;		// cached for drawing purposes
	uint32			mDisplayFlags;
	
	// Configure myself with a configuration message I generated in
	// ConfigurationData().
	void Configure(const BMessage* msg);

	void AddAsObserver();
};


#endif 
