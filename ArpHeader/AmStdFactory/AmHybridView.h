/* AmHybridView.h.
 * Copyright (c)1997 - 2000 by Eric Hackborn.
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
 *	- When moving notes with the MoveSelectedBy() func, playback isn't
 * occurring.
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 05.17.00		hackborn
 * Updated to the new MIDI domain.
 *
 * 11.15.98		hackborn
 * Mututated this file from the originals.
 */
 
#ifndef AMSTDFACTORY_AMHYBRIDVIEW_H
#define AMSTDFACTORY_AMHYBRIDVIEW_H

#include "ArpViews/ArpBackground.h"
#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewPropertyI.h"
class AmTrackMeasureBackground;

class _AmHybridSharedData
{
public:
	_AmHybridSharedData(float noteHeight, int32 octave, int32 notesBetweenClefs);
	
	float	mNoteHeight;
	int32	mOctave;
	int32	mNotesBetweenClefs;

	/* Answer the number of notes visible on the screen.
	 */
	int32	NotesOnScreen() const;
};

/*************************************************************************
 * AM-HBYRID-BACKGROUND
 * This simple class displays horizontal bars (like sheet music).
 *************************************************************************/
class AmHybridBackground : public ArpBackground
{
public:
	AmHybridBackground(_AmHybridSharedData& shared);

protected:
	_AmHybridSharedData&	mShared;

	virtual void DrawOn(BView* view, BRect clip);
};

/*************************************************************************
 * AM-HYBRID-DATA-VIEW
 * An editable view of note data.  Displayed in a hybrid view, ala
 * bars&pipes.
 *************************************************************************/
class AmHybridDataView : public AmTrackDataView
{
public:
	AmHybridDataView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI& viewProp,
						TrackViewType viewType);
	virtual ~AmHybridDataView();

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();
	virtual	void FrameResized(float new_width, float new_height);
	virtual	void GetPreferredSize(float *width, float *height);
	virtual	void ScrollTo(BPoint where);
	virtual void MessageReceived(BMessage*);
	virtual BMessage* ConfigurationData();

protected:
	void			SetNoteHeight(float noteHeight);
	void			SetOctave(int32 octave);
	void			SetNotesBetweenClefs(int32 notesBetweenClefs);
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);
	virtual void	PostDrawEventsOn(BRect clip, BView* view, const AmTrack* track);

private:
	typedef AmTrackDataView 	inherited;
	_AmHybridSharedData			mShared;
	AmHybridBackground*			mBg;
	AmTrackMeasureBackground*	mMeasureBg;

	void Configure(BMessage *msg);
	void AddAsObserver();
};

/*************************************************************************
 * AM-HYBRID-INFO-VIEW
 *************************************************************************/
class AmHybridInfoView : public AmTrackInfoView
{
public:
	AmHybridInfoView(	BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual	void	AttachedToWindow();
	virtual	void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage *msg);
		
protected:
	void			SetNoteHeight(float noteHeight);
	void			SetOctave(int32 octave);
	void			SetNotesBetweenClefs(int32 notesBetweenClefs);
	virtual void	PreDrawSliceOn(BView* view, BRect clip);
	virtual void	PostDrawSliceOn(BView* view, BRect clip);
	virtual void	AddOctaveMenu();

	virtual BPopUpMenu*	NewPropertiesMenu() const;

private:
	typedef AmTrackInfoView	inherited;
	_AmHybridSharedData		mShared;
	AmHybridBackground*		mBg;
	
	void		Configure(const BMessage* config);

	void		AddOctaveItem(BPopUpMenu* toMenu, int32 octaveNum, const char* label, bool enable = true);
	const char*	StringForOctave(int32 octave);
	BPoint		OctaveMenuLeftTop() const;
	BMenu*		NewHybridMenu() const;
};

#endif
