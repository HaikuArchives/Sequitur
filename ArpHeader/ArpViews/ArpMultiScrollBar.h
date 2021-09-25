/* ArpMultiScrollBar.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * This class is a type of scrollbar that can control an arbitrary number of
 * views.
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
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 *
 * 11.08.98		hackborn
 * Changed the AddTarget() and RemoveTarget() functions to work on BViews.
 */

#ifndef ARPVIEWS_ARPMULTISCROLLBAR_H
#define ARPVIEWS_ARPMULTISCROLLBAR_H

#include <interface/ScrollBar.h>
#include <interface/View.h>
#include <support/List.h>
#include <support/SupportDefs.h>

/***************************************************************************
 * ARP-MULTI-SCROLL-BAR
 * This subclass of BScrollBar allows multiple views to be targeted.
 ***************************************************************************/
class ArpMultiScrollBar : public BScrollBar
{
public:
	ArpMultiScrollBar(	BRect frame, const char* name, BView* target,
						int32 min, int32 max, orientation direction);
	virtual ~ArpMultiScrollBar();
		
	virtual void ValueChanged(float newValue);
		
	void AddTarget(BView *target);
	bool RemoveTarget(BView *target);
	void ClearTargets();

private:
	typedef BScrollBar		inherited;
	BList					mTargetList;
};

#endif
