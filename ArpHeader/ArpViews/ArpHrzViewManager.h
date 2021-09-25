/* ArpHrzViewManager.h
 * Copyright (c)1998 by Eric Hackborn.
 * All rights reserved.
 *
 * This class stores a list of views (they currently HAVE to be ArpViews,
 * but I'd like to remove this restriction.  It's there because it makes
 * use of the PreferredHeight that an ArpView supplies.  One option is to
 * get this information view a message send -- the default ArpView would
 * provide its current frame height info, but subclasses could provide a
 * hardcoded preferred height if they liked), stacked horizontally.  It
 * contains behaviour for operating on these views -- adding new ones at
 * a given position, removing them, etc.  It is intended to be subclassed.
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
 * 11.08.98		hackborn
 * The InsertMiniView() rules have changed slightly.  See comments above
 * that func for how new views get positioned in the list.
 *
 * 11.08.98		hackborn
 * Changed InsertMiniView() to make use of the new Be-function GetPreferredSize()
 * when adding views.  This means we can now remove our dependecy on using ArpViews,
 * and make BViews the basis of this class.
 *
 * 11.08.98		hackborn
 * Mutated this file from SeqMiniViewManager
 */

#ifndef ARPVIEWS_ARPHRZVIEWMANAGER_H
#define ARPVIEWS_ARPHRZVIEWMANAGER_H

#include <app/Message.h>
#include <interface/View.h>
#include <support/List.h>

#include "ArpViewsPublic/ArpViewDefs.h"
#include "ArpViews/ArpMultiScrollBar.h"


class ArpHrzViewManager : public BView
{
public:
	ArpHrzViewManager(BRect frame, const char* name, uint32 resizingMode);
	virtual ~ArpHrzViewManager();

	virtual void FrameResized(float new_width, float);
	virtual	void AttachedToWindow();

	void SetDefaultViewHeight(float height)		{ mDefaultViewHeight = height; }
	/* This sets how much blank space will be shown between each of the views
	 * I display.
	 */
	void SetSeparation(float separation)		{ mSeparation = separation; }
	void SetHorizontalScrollBar(ArpMultiScrollBar*);
	void InitializeHorizontalScrollBar();
	virtual BView* AddMiniView(BView *view);
	/* This method is used to add the view to the manager.  posArg is used to
	 * determine where to put it:  values of less than 0 and greater than the
	 * current number of views will be placed at the tail, all other values will
	 * be placed at the given position (a value of 0 places it at the head, for
	 * example).  topArg can be used to determine the top pixel of the view, if
	 * it's already known, otherwise sending B_ERROR will find this value dynamically.
	 * The usefulness of this feature is questionable; it's likely that the ability
	 * to pass in the topArg will be removed from a future release.
	 */
	BView* InsertMiniView(BView *view, int32 posArg, float topArg = B_ERROR);
	virtual bool ReplaceMiniView(BView *view, int32 position);
	void RemoveMiniView( view_id id );
	
	/* If I contain the view whose ID has been supplied, answer its index in
	 * my list, otherwise answer -1.
	 */
	int32 IndexOf( view_id id );
	/* Answer the BView at the given index or 0 if the index is not valid.
	 */
	BView* ItemAt(int32 index) const;

	void InvalidateAll();

protected:
	// A list of all views I am displaying.
	BList				mViewList;
	// A value that can be set, which will be used if mini views do not supply
	// their own view height.
	float				mDefaultViewHeight;
	// A scroll bar that controls scrolling all the views I contain horizontally.
	// This is completely optional.
	ArpMultiScrollBar*	mHsb;

	BView*	ViewForId( view_id id ) const;

	int32 do_RemoveMiniView(view_id id);
		
	// Update messages that notify subclasses about what's going on.
	virtual void PreInsertMiniView(BView* /*view*/)		{ }
	virtual void PostInsertMiniView(BView *view);
	virtual void PreRemoveMiniView(BView* /*view*/)		{ }
	virtual void PostRemoveMiniView(BView *view);
	/* This hook function is called whenever a complete operation is
	 * finished.
	 */
	enum {
		INSERT_OP,
		REMOVE_OP
	};
	virtual void ManagerOperationFinished(uint32 op);

	/* Return the horizontal width of our data area for our horizontal scroll
	 * bar -- this means all the area that's viewable, and all that isn't
	 * but includes data.  Default to returning our bounds width, subclasses
	 * should override this.
	 */
	virtual float HorizontalWidth()						{ return Bounds().Width(); }
	virtual int32 BigHorizontalStep()					{ return (int32)Bounds().Width(); }
	virtual int32 SmallHorizontalStep()					{ return 10; }

	virtual void SetHSBRange();
	virtual void SetHSBSteps();

private:
	typedef BView		inherited;
	// This is how much space will be used to separate each of the views I show.
	float				mSeparation;
	
	void RepositionMiniViewsFrom(int32 position);

	int32 ListTopAt(int32 position);
	bool ListHeadConditions(int32 position);
	bool ListTailConditions(int32 position);
};


#endif 
