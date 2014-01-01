/* ArpControlChangeView.h
 *
 * This file contains both views needed (an info view and a data view)
 * to display and edit control change MIDI events.
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
 *	• When moving notes with the MoveSelectedBy() func, playback isn't
 * occurring.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 11.21.98		hackborn
 * Mutated from the original SeqControlChangeView file.
 */

#ifndef AMSTDFACTORY_AMCONTROLCHANGEVIEW_H
#define AMSTDFACTORY_AMCONTROLCHANGEVIEW_H

#include "AmPublic/AmDeviceI.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmStdFactory/AmStdViewFactoryAux.h"
class BMenu;
class AmTrackMeasureBackground;
class AmViewPropertyI;
class _AmControlMenu;

#define STR_CONTROL_CHANGE_INFO		"ControlChangeInfo"
#define STR_CONTROL_CHANGE_DATA		"ControlChangeData"


/***************************************************************************
 * AM-CONTROL-CHANGE-INFO-VIEW
 ***************************************************************************/
class AmControlChangeInfoView : public AmTrackInfoView
{
public:
	AmControlChangeInfoView(BRect frame,
							AmSongRef songRef,
							AmTrackWinPropertiesI& trackWinProps,
							const AmViewPropertyI* property,
							TrackViewType viewType);

	virtual void	SetConfiguration(const BMessage* config);

	virtual	void	AttachedToWindow();
	virtual	void	GetPreferredSize(float *width, float *height);
	virtual void	MessageReceived(BMessage *msg);

	virtual void	ControlActivated(int32 code);

protected:
	virtual void	DrawOn(BRect clip, BView* view);
	/* Annotate the properties menu with my list of control changes.
	 */
	virtual BPopUpMenu*	NewPropertiesMenu() const;

private:
	typedef AmTrackInfoView	inherited;

	uchar					mCc;
	/* NewControlMenu() is called to create the control menu, which is
	 * then added to the property menu.  However, as soon as it's assigned
	 * to the property menu I can't get access to it, so I cache it.
	 */
	mutable _AmControlMenu*	mCachedControlMenu;

	_AmControlMenu*			NewControlMenu() const;
	/* Answer a string for the control value in the current menu.
	 */
	BString					MenuStringForControl(uint32 control) const;
	/* Answer my current control menu, or 0 if none.
	 */
	BMenu*					ControlMenu() const;
};

/***************************************************************************
 * AM-CONTROL-CHANGE-DATA-VIEW
 ***************************************************************************/
class AmControlChangeDataView : public AmTrackDataView
{
public:
	AmControlChangeDataView(BRect frame,
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
	 * so the info view already displays the correct control number.
	 */
	void SetControlNumber(uchar controlNumber);

	virtual void	PreDrawEventsOn(BRect clip, BView* view, const AmTrack* track);
	virtual void	DrawShadowEvents(BRect clip, BView* view, const AmSong* song)	{ }
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);

private:
	typedef AmTrackDataView inherited;
	AmTrackRef					mCachedPrimaryTrack;
	AmTrackMeasureBackground*	mMeasureBg;

	BPoint	mPt1, mPt2;	// cached for drawing purposes
	float	mScale;		// cached for drawing purposes
	uchar	mCc;		// The control change number we're
							// responsible for displaying
	
	// Configure myself with a configuration message I generated in
	// ConfigurationData().
	void Configure(const BMessage* msg);

	void AddAsObserver();
};

/***************************************************************************
 * AM-CONTROL-CHANGE-PREF-VIEW
 ***************************************************************************/
class AmControlChangePrefView : public AmStdFactoryPrefView
{
public:
	AmControlChangePrefView(BRect f, BMessage* prefs,
							const BString& facSig,
							const BString& facKey);

	virtual	void			AttachedToWindow();
	virtual void			MessageReceived(BMessage* msg);

	virtual void			AddViews();

private:
	typedef AmStdFactoryPrefView inherited;
	BCheckBox*				mRunReport;
};

#endif 
