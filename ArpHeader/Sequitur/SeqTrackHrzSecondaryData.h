/* SeqTrackHrzSecondaryData.h
 *
 * This class is a collection of views, stacked horizontally.  Each view
 * is something given to me by a subclass of AmViewFactory.  I
 * simply manage this list.
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
 * 11.22.98		hackborn
 * Mutated from SsAllTrackDataView
 */
 

#ifndef SEQUITUR_SEQTRACKHRZSECONDARYDATA_H
#define SEQUITUR_SEQTRACKHRZSECONDARYDATA_H

#include <be/app/Message.h>
#include <be/interface/PopUpMenu.h>

#include "ArpViews/ArpHrzViewManager.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmTimeConverter.h"
#include "AmKernel/AmTrack.h"
#include "AmPublic/AmTrackRef.h"

class SeqTrackHrzSecondaryData : public ArpHrzViewManager,
								 public AmSongObserver
{
public:
	SeqTrackHrzSecondaryData(	AmSongRef songRef,
								AmTrackWinPropertiesI& trackWinProps,
								BRect frame,
							 	float separation = 0);
	virtual ~SeqTrackHrzSecondaryData();

	virtual	void	AttachedToWindow();

	/* Iterate through my list of views, and store each one as a view property
	 * in the supplied track.
	 */
	void			StoreViewProperties(AmTrack* track);

protected:
	/* Turn these guys off.  This data manager shouldn't be messing
	 * with the scrollbar, the window takes care of that.
	 */
	virtual void	SetHSBRange()		{ }
	virtual void	SetHSBSteps()		{ }

private:
	typedef ArpHrzViewManager	inherited;
	AmTrackWinPropertiesI&		mTrackWinProps;

	void	InitializeViews();
	BView*	NewSecondaryView(const AmViewPropertyI* prop);
	// Answer a new BView with nothing but a StringView describing a problem
	BView*	NoSecondaryView(const char *problem);
};

#endif 
