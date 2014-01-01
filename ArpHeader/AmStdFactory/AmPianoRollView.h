/* AmPianoRollView.h
 * Copyright (c)2000 by Eric Hackborn.
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
 * 07.10.00		hackborn
 * Mutated this file from the hybrid view.
 */
 
#ifndef AMSTDFACTORY_AMPIANOROLLVIEW_H
#define AMSTDFACTORY_AMPIANOROLLVIEW_H

#include "AmPublic/AmTrackDataView.h"
#include "AmPublic/AmTrackInfoView.h"
#include "AmPublic/AmViewPropertyI.h"
class AmTrackMeasureBackground;
class _AprBackground;

/*************************************************************************
 * AM-PIANO-ROLL-DATA-VIEW
 * An editable view of note data.  Displayed in a piano roll view.
 *************************************************************************/
class AmPianoRollDataView : public AmTrackDataView
{
public:
	AmPianoRollDataView(	BRect frame,
							AmSongRef songRef,
							AmTrackWinPropertiesI& trackWinProps,
							const AmViewPropertyI& viewProp,
							TrackViewType viewType);
	virtual ~AmPianoRollDataView();

	virtual	void		AttachedToWindow();
	virtual	void		DetachedFromWindow();
	virtual	void		FrameResized(float new_width, float new_height);
	virtual	void		GetPreferredSize(float* width, float* height);
	virtual void		MessageReceived(BMessage* msg);
	virtual	void		MouseMoved(	BPoint where,
									uint32 code,
									const BMessage* msg);
	virtual	void		ScrollTo(BPoint where);

	virtual BMessage*	ConfigurationData();

protected:
	virtual void	DrawEvent(	BView* view, const AmPhraseEvent& topPhrase,
								const AmEvent* event, AmRange eventRange, int32 properties);
	void			SetNoteHeight(float noteHeight);
	
private:
	typedef AmTrackDataView inherited;
	float			mNoteHeight;
	int32			mHighlightedNote;
	/* This is my initial y scroll value.  It's pulled out of the
	 * configuration supplied in the constructor.
	 */
	float			mInitialTop;
	_AprBackground*	mAprBg;
	AmTrackMeasureBackground*	mMeasureBg;
	
	void AddAsObserver();
};

/*************************************************************************
 * AM-PIANO-ROLL-INFO-VIEW
 *************************************************************************/
class AmPianoRollInfoView : public AmTrackInfoView
{
public:
	AmPianoRollInfoView(BRect frame,
						AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const AmViewPropertyI* property,
						TrackViewType viewType);

	virtual	void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage* msg);
		
protected:
	virtual void	DrawOn(BRect clip, BView* view);
	virtual BPopUpMenu* NewPropertiesMenu() const	{ return 0; }

private:
	typedef 	AmTrackInfoView	inherited;
	float		mNoteHeight;
	int32		mHighlightNote;
};

#endif
