/*
 * SsAllTrackInfoView.h: A container for all the TrackInfoViews.
 *
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
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	• None.  Ha, ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 12.07.97		hackborn
 * In ControlChangeMenu(), changed the Other CC... menu item to
 * read Other Control Change...
 */
 

#ifndef SEQUITUR_SEQTRACKHRZSECONDARYINFO_H
#define SEQUITUR_SEQTRACKHRZSECONDARYINFO_H

#include <be/app/Message.h>
#include <be/interface/PopUpMenu.h>

#include "ArpViews/ArpHrzViewManager.h"
#include "AmPublic/AmSongObserver.h"
#include "AmKernel/AmTrack.h"
#include "AmPublic/AmTrackRef.h"

class ArpMultiScrollBar;
class AmTrackWinPropertiesI;

/******************************************************************
 * SEQ-TRACK-HRZ-SECONDARY-INFO
 ******************************************************************/
class SeqTrackHrzSecondaryInfo : public ArpHrzViewManager,
								 public AmSongObserver
{
public:
	SeqTrackHrzSecondaryInfo(	AmSongRef songRef,
								AmTrackWinPropertiesI& trackWinProps,
								BRect frame,
								BString factorySignature,
								float separation = 0);
	virtual ~SeqTrackHrzSecondaryInfo();
		
	void			SetVerticalScrollBar(ArpMultiScrollBar *sb);

	virtual	void	AttachedToWindow();
	virtual void	FrameResized(float new_width, float new_height);

private:
	typedef ArpHrzViewManager	inherited;
	AmTrackWinPropertiesI&		mTrackWinProps;
	BString						mFactorySignature;
	ArpMultiScrollBar*			mVsb;

	void InitializeViews();
	BView* NewSecondaryView(const AmViewPropertyI* prop);
	// Answer a new BView with nothing but a StringView describing a problem
	BView* NoSecondaryView(const char *problem);

	/* I overwrite this method to make sure I always have an empty info view
	 * at the bottom, one with a simple property menu.
	 */
	virtual void ManagerOperationFinished(uint32 op);
	void AddEmptyView();
	/* This is set to true during the IntializeViews() method.  It's a flag
	 * to keep myself from doing anything in the PostInsertMiniView() method.
	 */
	bool						mInitializing;

	void	SetVsb();
	float	VerticalHeight() const;
};

#endif 
