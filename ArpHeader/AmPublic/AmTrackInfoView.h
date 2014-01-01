/* AmTrackInfoView.h
 * Copyright (c)1998-2000 by Eric Hackborn.
 * All rights reserved.z
 *
 * This class is the abstract base class for all views that
 * present information about an editable view on track data.  It is the
 * companion class to AmTrackDataView -- if you create a subclass
 * of that class, you will need to make a corresponding subclass of this
 * class, and vice versa.  Instances of this class can communicate with
 * instances of its companion class by sending messages through the
 * PostMessageToDataView() function.
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
 * 05.17.00		hackborn
 * Updated to new MIDI domain.
 *
 * 11.18.98		hackborn
 * Mutated this file from the original SeqTrackInfoView
 */

#ifndef AMPUBLIC_AMTRACKINFOVIEW_H
#define AMPUBLIC_AMTRACKINFOVIEW_H

#include <be/app/Message.h>
#include <be/interface/View.h>
#include "AmPublic/AmDefs.h"
#include "AmPublic/AmSongRef.h"
class BMenuItem;
class BPopUpMenu;
class ArpBackground;
class AmTrackWinPropertiesI;

enum {
	/* The info view sends this to its window if it wants to duplicate itself.
	 */
	DUPLICATE_INFO_MSG	= 'dupI',
	/* The info view sends this to its window if it wants to change to another view.
	 */
	CHANGE_INFO_MSG		= 'chnI',
	/* The info view sends this to its parent if it wants to remove itself.
	 */
	REMOVE_INFO_MSG		= 'remI'
};

#define AM_INFO_CHANGE_VIEW_STR		"Change View"

// I respond to this message:
#define S_DRAGGED_TRACK_DATA	'#Dtd'
	// STR_WINDOW, the window where the drag is coming from
	// STR_POSITION, the position where the drag is coming from

/*************************************************************************
 * AM-TRACK-INFO-VIEW
 * This simple class displays horizontal bars (like sheet music).
 *************************************************************************/
class AmTrackInfoView : public BView
{
public:
	AmTrackInfoView(BRect frame,
					const char* name,
					AmSongRef songRef,
					AmTrackWinPropertiesI& trackWinProps,
					TrackViewType viewType);
	virtual ~AmTrackInfoView();
	
	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void 	Draw(BRect clip);
	virtual void	MessageReceived(BMessage *msg);
	/* A general notice that a control has been activated --
	 * currently, it's only called when the property menu is
	 * pressed but this might expand.  Empty.
	 */
	virtual void	ControlActivated(int32 code);
	
	void			Highlight();
	void			Unhighlight();

protected:
	AmSongRef		mSongRef;
	AmTrackWinPropertiesI&	mTrackWinProps;
	TrackViewType	mViewType;
	rgb_color		mViewColor;

	/* Are these really needed in the info view?
	 */
	BString			mFactorySignature;
	BString			mViewName;

	ArpBackground*	mHeadBackground;
	/* Subclasses can plug into drawing on the background by
	 * supplying background objects.  This method adds the
	 * background as the tail of any existing backgrounds.
	 */
	void			AddBackground(ArpBackground* background);

	virtual void	DrawOn(BRect clip, BView* view);
	/* Subclasses are provided with the opportunity to draw either
	 * before or after the slice image.  The PreDraw is empty, the
	 * PostDraw displays a black line separating the info from the
	 * data view.
	 */
	virtual void	PreDrawSliceOn(BView* view, BRect clip);
	virtual void	PostDrawSliceOn(BView* view, BRect clip);
	/* Answer a new instance of the menu that will get used for the
	 * properties field.  If a subclass doesn't want a properties field,
	 * answer 0.  By default, a menu of simple actions, like removing and
	 * changing the view, is answered.
	 */
	virtual BPopUpMenu*	NewPropertiesMenu() const;

private:
	typedef BView		inherited;
	bool				highlight;

	void HandleDraggedTrackData(BMessage *msg);
	/* Construct a new menu item that has a submenu of all the views
	 * this view can be changed into.
	 */
	BMenuItem*		NewChangeViewItem() const;
	/* Add a small little widget that has a pull-down menu.
	 */
	void			AddPropertiesField();
};

#endif
