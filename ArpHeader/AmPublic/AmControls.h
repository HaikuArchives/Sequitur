/* AmControls.h
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
 * 11.09.00		hackborn
 * Created this file
 */

#ifndef AMPUBLIC_AMCONTROLS_H
#define AMPUBLIC_AMCONTROLS_H

#include <interface/TextControl.h>
#include <support/SupportDefs.h>
#include "ArpLayout/ArpScrollArea.h"
#include "ArpLayout/ViewStubs.h"
#include "ArpViewsPublic/ArpIntToStringMapI.h"
#include "ArpViews/ArpIntControl.h"
#include "ArpViews/ArpRangeControl.h"
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmTimeConverter.h"
class AmActiveToolView;
class AmEditorTool;
class AmEditorToolBarView;
class AmFilterConfigLayout;
class AmMotionI;

/***************************************************************************
 * AM-KEY-CONTROL
 * This int control maps MIDI note numbers to friendly key names.
 ***************************************************************************/
class AmKeyControl : public ArpIntControl
{
public:
	AmKeyControl(	BRect frame,
					const char* name,
					const char* label,
					BMessage* message,
					uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	AmKeyControl(	const char* name,
					const char* label,
					BMessage* message);
	virtual ~AmKeyControl();

private:
	typedef ArpIntControl	inherited;
	size_t	_reserved_data[12];
};

/*************************************************************************
 * AM-KEY-MAP
 * A class that performs a mapping between ints and names for all the key
 * params.
 *************************************************************************/
class AmKeyMap : public ArpIntToStringMapI
{
public:
	AmKeyMap();
	virtual status_t IdForName(const char *name, int32 *answer) const;
	virtual status_t NameForId(int32 id, char **answer) const;
};

/***************************************************************************
 * AM-DURATION-CONTROL
 * This control lets users select duration from several common choices.
 ***************************************************************************/
#define AM_QUANTIZE_CONTROL_KEY_STR			"quantize key"
#define AM_EIGHTHS_CONTROL_KEY_STR			"eighths key"
#define AM_MULTIPLIER_CONTROL_KEY_STR		"multiplier key"

enum {
	AM_SHOW_DURATION_MULTIPLIER		= (1<<0),
	AM_SHOW_DURATION_TEXT			= (1<<1)
};

enum {
	DUR_QUANTIZE_FINISHED_MSG	= 'iqnf',
	DUR_EIGHTHS_FINISHED_MSG	= 'i8f_',
	DUR_MULT_FINISHED_MSG		= 'imlt',
	DUR_TEXT_FINISHED_MSG		= 'itcf'
};

class AmDurationControl : public ArpLayoutControl
{
public:
	AmDurationControl(	BPoint origin,
						const char* name,
						const char* label,
						uint32 initFlags = AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
						const char* quantizeKey = AM_QUANTIZE_CONTROL_KEY_STR,
						const char* eighthsKey = AM_EIGHTHS_CONTROL_KEY_STR,
						const char* multiplierKey = AM_MULTIPLIER_CONTROL_KEY_STR,
						uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	AmDurationControl(	const char* name,
						const char* label,
						AmFilterConfigLayout* target,
						const BMessage& initSettings,
						uint32 initFlags = AM_SHOW_DURATION_MULTIPLIER | AM_SHOW_DURATION_TEXT,
						const char* quantizeKey = AM_QUANTIZE_CONTROL_KEY_STR,
						const char* eighthsKey = AM_EIGHTHS_CONTROL_KEY_STR,
						const char* multiplierKey = AM_MULTIPLIER_CONTROL_KEY_STR);
	virtual ~AmDurationControl();

	static void			SplitTicks(AmTime ticks, int32* multiplier, AmTime* quant, int32* eighths);

	/* Answer my current value as a single AmTime.
	 */
	AmTime				RawTicks() const;
	void				GetSplitTime(int32* m, AmTime* v, int32* d) const;
	void				SetValue(AmTime value);
	void				SetValue(int32 multiplier, AmTime value, int32 divider);
	virtual	void		SetDivider(float dividing_line);
	/* This control can receive update messages of the same format as that
	 * supplied in the constructor (initSettings), and will update the display
	 * accordingly.
	 */
	void				Refresh(const BMessage& settings);
	
	ArpRangeControl*	QuantizeControl() const;
	ArpRangeControl*	EighthsControl() const;

	virtual	void		AttachedToWindow();
	virtual	void		GetPreferredSize(float *width, float *height);
	virtual void		MessageReceived(BMessage* msg);

protected:
	virtual void		ComputeDimens(ArpDimens& dimens);
	virtual void		LayoutView();

private:
	typedef ArpLayoutControl inherited;
	ArpRangeControl*		mQuantCtrl;
	ArpRangeControl*		mEighthsCtrl;
	ArpIntControl*			mMultiplierCtrl;
	BTextControl*			mTextCtrl;
	BString					mQuantKey;
	BString					mEighthsKey;
	BString					mMultiplierKey;
	float					mPrefW, mPrefH;
	float					mPrefBW;
	float					mDividingLine;
	AmFilterConfigLayout*	mTarget;

	// Set by the Refresh...() and Update...() functions,
	// used by the Send...() functions.
	int32					mMultiplier;
	AmTime					mValue;
	int32					mDivider;
	
	size_t					_reserved_data[12];
	
	void					AddViews(const char* label, const BMessage* initSettings, uint32 initFlags);
	void					UpdateSplitTime(AmTime time);
	void					UpdateSplitTime(int32 m, AmTime v, int32 d);
	void					UpdateRawTime(AmTime time);
	void					UpdateRawTime(int32 m, AmTime v, int32 d);
	
	void					RefreshSplitToRaw();
	void					RefreshRawToSplit();
	
	void					SendMultiplierUpdate();
	void					SendValueUpdate();
	void					SendDividerUpdate();
};

/***************************************************************************
 * AM-CONTROL-CHANGE-LIST-PANEL
 * This ArpScrollARea subclass displays all the active control changes.
 ***************************************************************************/
#define AM_CONTROL_CHANGE_KEY_STR			"control change key"

class AmControlChangeListPanel : public ArpScrollArea
{
public:
	AmControlChangeListPanel(	const char* name,
								AmFilterConfigLayout* target,
								const BMessage& initSettings,
								const char* settingsKey = AM_CONTROL_CHANGE_KEY_STR,
								list_view_type type = B_SINGLE_SELECTION_LIST); 
	virtual ~AmControlChangeListPanel();

	/* Obviously, this class isn't the real list view, just a wrapper around
	 * it.  Here's a way to get at the actual list view, in case clients want
	 * to set selection and invocation messages.
	 */
	BListView*	ListView() const;

private:
	typedef ArpScrollArea	inherited;
	BListView*				mListView;
	size_t					_reserved_data[4];
};

/***************************************************************************
 * AM-VELOCITY-CONTROL
 * This control lets users select a velocity from several common choices.
 ***************************************************************************/
class AmVelocityControl : public ArpRangeControl
{
public:
	AmVelocityControl(	BPoint origin, const char* name,
						uint32 resizeMask, BMessage* finishedMsg = NULL);

	static uint8	InitialVelocity();

	uint8			Velocity() const;

private:
	typedef ArpRangeControl	inherited;
	size_t			_reserved_data[4];
};

/*************************************************************************
 * AM-MOTION-EDITOR
 * This view creates an editor for Motions.
 *************************************************************************/
class _AmMotionEditorData;
class _AmPlugView;

class AmMotionEditor : public ArpLayoutControl
{
public:
	AmMotionEditor(BRect frame, const char* name, uint32 resizeMask);
	AmMotionEditor(	const char* name,
					AmFilterConfigLayout* target,
					const BMessage& initSettings,
					const char* settingsKey = "motion",
					int32 settingsIndex = 0);
	virtual ~AmMotionEditor();

	void			SetMotion(const AmMotionI* motion);
	status_t		AddHitsTo(AmMotionI* motion) const;

	virtual void	AttachedToWindow();
	virtual void	AllAttached();
	virtual void	DetachedFromWindow();
	virtual	void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage* msg);
	/* If this class is owned by a filter settings window, and that
	 * window changes the motion, someone needs to notify me.
	 */
	void			Refresh(const BMessage& settings);

protected:
	virtual void	ComputeDimens(ArpDimens& dimens);
	virtual void	LayoutView();

private:
	typedef ArpLayoutControl	inherited;
	_AmMotionEditorData*		mData;
	BMenuField*					mViewAsField;
	AmActiveToolView*			mActiveTools;
	AmEditorToolBarView*		mToolBar;
	AmEditorTool*				mPencilTool;
	AmEditorTool*				mWandTool;
	AmEditorTool*				mEraserTool;
	AmDurationControl*			mGridCtrl;
	_AmPlugView*				mInfoPlug;
	_AmPlugView*				mDataPlug;
	BScrollBar*					mScrollV;
	BRect						mScrollVF;
	BScrollBar*					mScrollH;
	BRect						mScrollHF;
	float						mPrefW;
	
	AmFilterConfigLayout*		mTarget;
	BString						mSettingsKey;
	int32						mSettingsIndex;
	
	size_t						_reserved_data[12];
	
	void						ReplugViews();
	void						AddViews(BRect bounds);	
	void						Init();
};

/***************************************************************************
 * Some common formatters
 ***************************************************************************/
// Adds a percent sign to the number
ArpIntFormatterI* arp_new_percent_formatter();
// >= 100 equals Always, <= 0 equals Never, all others have a percent sign
ArpIntFormatterI* arp_new_frequency_formatter();

#endif 
