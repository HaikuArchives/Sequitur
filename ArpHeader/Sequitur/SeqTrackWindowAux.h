/* TrackWindowAux.h
 * Copyright (c)1996 - 2001 by Eric Hackborn.
 * All rights reserved.
 *
 * This file is just helper classes for the track window.
 * There was getting to be enough that I pulled it into a
 * separate file.
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
 * 2001.04.18		hackborn@angryredplanet.com
 * Mutated this file from its original incarnation.
 */
#ifndef SEQUITUR_SEQTRACKWINDOWAUX_H
#define SEQUITUR_SEQTRACKWINDOWAUX_H

#include <InterfaceKit.h>
#include "ArpKernel/ArpBitmapCache.h"
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
#include "BeExp/ToolTip.h"
class ArpIntControl;
class AmKeyControl;
class AmMotion;
class AmNoteOn;
class AmSelectionsI;
class AmSong;
class AmTrackWinPropertiesI;

/* This shadows the AM_PRIMARY_TRACK_MSG, but I use a different what because
 * that message gets posted to all of my views when the pri track changes --
 * and of course, if one of the views doesn't handle it, then I'd receive it
 * and go into a loop.
 */
enum {
	TW_PRIMARY_TRACK_MSG			= 'twPT',
	TW_CLEAR_ORDERED_TRACK_MSG		= 'twCO',
	TW_SET_ORDERED_TRACK_MSG		= 'twSO',
	
	TW_ORDERED_SAT_MSG				= 'twOS',
	TW_SHADOW_SAT_MSG				= 'twSS'
};

extern const char*		ACTIVE_TOOL_VIEW_NAME;

/*************************************************************************
 * _TW-ACTIVE-TOOL-VIEW
 * This class displays which tools are mapped to the mouse buttons.
 *************************************************************************/
class _TwActiveToolView : public BView
{
public:
	_TwActiveToolView(BPoint origin);
	virtual ~_TwActiveToolView();
	
	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void	Draw(BRect clip);
	virtual void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage* msg);

protected:
	void			DrawOn(BView*, BRect clip);

private:
	typedef BView		inherited;
	int32				mMouseType;
	const BBitmap*		mMouseBitmap;
	BBitmap*			mPrimaryIcon;
	BBitmap*			mSecondaryIcon;
	BBitmap*			mTertiaryIcon;
	rgb_color			mViewC;
	float				mLeftOverhang;
	float				mRightOverhang;
	float				mTopOverhang;

	BBitmap*			NewIcon(uint32 button, uint32 mods);
	void				Uncache();
};

/*************************************************************************
 * _TW-CHOOSE-TRACK-MENU
 * This class dynamically builds a list of tracks for selections.
 *************************************************************************/
class _TwChooseTrackMenu : public BMenu
{
public:
	_TwChooseTrackMenu(	const char* name, AmSongRef songRef,
						AmTrackWinPropertiesI& trackWinProps,
						const BMessenger& target);

	void			EmptyState();

protected:
	virtual bool	AddDynamicItem(add_state state);

private:
	typedef BMenu		inherited;
	AmSongRef			mSongRef;
	AmTrackWinPropertiesI& mTrackWinProps;
	BMessenger			mTarget;
	bool				mMenuBuilt;
	vector<BMenuItem*>	mItems;
	
	bool	StartBuildingItemList();
	bool	AddNextItem(const AmSong* song);
	void	DoneBuildingItemList();
	void	ClearMenuBuildingState();
	int32	IndexOf(track_id tid) const;
};

/*************************************************************************
 * _TW-CREATE-RHTYHM-WIN
 * This window creates a new AmRhythm from the supplied selections.
 *************************************************************************/
class TwCreateMotionWin : public BWindow
{
public:
	TwCreateMotionWin(	AmSongRef songRef,
						BMessenger trackWin,
						uint8 initialNote);
	virtual ~TwCreateMotionWin();

	void				MessageReceived(BMessage* msg);

private:
	typedef BWindow		inherited;
	AmSongRef			mSongRef;
	BMessenger			mTrackWin;
	BTextControl*		mNameCtrl;
	AmKeyControl*		mInitialNoteCtrl;
	ArpIntControl*		mCenterCtrl;
	enum {
		PROGRESSION_FLAG		= 0x00000001,
		RHYTHM_FLAG				= 0x00000002,
		PITCH_ENVELOPE_FLAG		= 0x00000004,
		VELOCITY_ENVELOPE_FLAG	= 0x00000008
	};
	uint32				mFlags;
	
	bool				CreateMotion();
	void				CreateMotion(AmTrackWinPropertiesI& props, const AmSong& song);
	void				AddHit(AmMotion* motion, AmNoteOn* noteOn) const;
	void				AddViews();
};

/*************************************************************************
 * _TW-SATURATION-WIN
 * Set the saturation values for the supplied window.
 *************************************************************************/
class _TwSaturationWin : public BWindow,
						 public ArpBitmapCache,
						 public BToolTipFilter
{
public:
	_TwSaturationWin(	BMessenger target,
						float orderedSat,
						float shadowSat);

private:
	typedef BWindow		inherited;

	void				AddViews(	BMessenger target,
									float orderedSat,
									float shadowSat);
};

#endif
