/* AmEventInspectors.h
 * Copyright (c)1998-2000 by Angry Red Planet.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * An inspector is simply a BView, but it's one that's intended to allow
 * the user to edit the actual numerical values available in each of the
 * MIDI events.  This file is a collection of inspectors for each type
 * of MIDI event, plus a factory for generating and reusing them.
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
 * 05.09.00			hackborn
 * Updated to make use of the new time view that's based on int controls.
 *
 * Dec 20, 1998		hackborn
 * 	Created this file.
 */


#ifndef AMKERNEL_AMEVENTINSPECTORS_H
#define AMKERNEL_AMEVENTINSPECTORS_H

#include <vector>
#include <interface/View.h>
#include "AmPublic/AmSongRef.h"
#include "AmPublic/AmTrackRef.h"
class AmBankChangeView;
class AmCcTextView;
class AmChannelPressureControl;
class AmEvent;
class AmEventTimeView;
class AmNoteView;
class AmPhraseEvent;
class AmPitchBendView;
class AmProgramChangeView;
class AmReleaseVelocityView;
class AmSelectionsI;
class AmTempoTextView;
class AmVelocityView;

/********************************************************
 * AM-EVENT-INSPECTOR
 * This abstract superclass for the inspectors provides
 * a small amount of convenience functionality, like
 * handling the messages that inspectors receive and
 * translating them to method sends.
 ********************************************************/
class AmEventInspector : public BView
{
public:
	AmEventInspector(BRect frame,
					 const char *name);
	virtual ~AmEventInspector();

	AmSongRef		SongRef();
	void			SetSongRef(AmSongRef songRef);
	virtual void	SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmEvent* event);
	/* Called by the factory when the view is first constructed,
	 * this is overriden by subclasses to add all the child views.
	 */
	virtual void	AddViews();
	
	virtual	void	AttachedToWindow();
	virtual	void	DetachedFromWindow();
	virtual void	Draw(BRect clip);
	virtual void	MessageReceived(BMessage *msg);

protected:
	/* This is temporary while the event inspector is not a
	 * subclass of AmSongObserver.
	 */
	AmSongRef			mSongRef;
	/* The track that will receive notification about changes I
	 * make to my event.
	 */
	AmTrackRef			mTrackRef;
	/* My currently installed event.  It's a pretty bad idea to
	 * be holding onto an event pointer, altough they will probably
	 * soon be reference counted and it won't be any big deal.
	 * For now, the mEventId is used to prevent problems.
	 */
	AmPhraseEvent*		mContainer;
	AmEvent*			mEvent;
	/* A time view to display and edit the start time of my event.
	 */
	AmEventTimeView*	mTimeView;

	void	RangeChangeReceived();
	
	/* Every MIDI event should have a start time, so every MIDI event should
	 * call this initialize method.  This initializes mTimeView appropriately.
	 */
	float AddTimeView(float left);
	/* Subclasses can use this as a convenience for adding a label.  It will
	 * be added at the proper y value and the supplied x (left) value, and the
	 * resulting far-right pixel will be answered.
	 */
	float AddLabel(const char *labelStr, float left);

	virtual void Refresh(void);

private:
	typedef BView	inherited;
};

/***************************************************************************
 * AM-BANK-CHANGE-INSPECTOR
 ***************************************************************************/
class AmBankChangeInspector : public AmEventInspector
{
public:
	AmBankChangeInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);

private:
	typedef AmEventInspector	inherited;
	AmBankChangeView*			mBankChangeView;
	AmProgramChangeView*		mProgramChangeView;

	float			AddBankView(float left);
	float			AddProgramView(float left);
};

/***************************************************************************
 * AM-NOTE-ON-INSPECTOR
 *************************************************************************/
class AmNoteOnInspector : public AmEventInspector
{
public:
	AmNoteOnInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);
	
private:
	typedef AmEventInspector	inherited;
	AmEventTimeView*			mEndTimeView;
	AmNoteView*					mNoteView;
	AmVelocityView*				mVelocityView;
	AmReleaseVelocityView*		mReleaseVelocityView;
	
	float AddEndTimeView(float left);
	float AddNoteView(float left);
	float AddVelocityView(float left);
	float AddReleaseVelocityView(float left);
};

/***************************************************************************
 * AM-CC-INSPECTOR
 *************************************************************************/
class AmCcInspector : public AmEventInspector
{
public:
	AmCcInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);

private:
	typedef AmEventInspector	inherited;
	AmCcTextView*				mCcView;
		
	float AddControlChangeView(float left);
};

/***************************************************************************
 * AM-CHANNEL-PRESSURE-INSPECTOR
 *************************************************************************/
class AmChannelPressureInspector : public AmEventInspector
{
public:
	AmChannelPressureInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);

private:
	typedef AmEventInspector	inherited;
	AmChannelPressureControl*	mCpCtrl;
		
	float AddChannelPressureControl(float left);
};

/***************************************************************************
 * AM-PITCH-BEND-INSPECTOR
 ***************************************************************************/
class AmPitchBendInspector : public AmEventInspector
{
public:
	AmPitchBendInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);

private:
	typedef AmEventInspector	inherited;
	AmPitchBendView*			mPitchView;
		
	float AddPitchBendView(float left);
};

/***************************************************************************
 * AM-PROGRAM-CHANGE-INSPECTOR
 ***************************************************************************/
class AmProgramChangeInspector : public AmEventInspector
{
public:
	AmProgramChangeInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);

private:
	typedef AmEventInspector	inherited;
	AmProgramChangeView*		mProgramChangeView;

	float			AddProgramView(float left);
};

/***************************************************************************
 * AM-TEMPO-CHANGE-INSPECTOR
 *************************************************************************/
class AmTempoChangeInspector : public AmEventInspector
{
public:
	AmTempoChangeInspector(BRect frame);

	virtual void	SetTrackRef(AmTrackRef trackRef);
	virtual void	AddViews();

protected:
	virtual void	Refresh(void);

private:
	typedef AmEventInspector	inherited;
	AmTempoTextView*			mTempoView;
		
	float AddTempoChangeView(float left);
};

/***************************************************************************
 * AM-INSPECTOR-FACTORY
 * This is a convenience utility for generating inspectors of MIDI events.
 * Provided a bounding box and a view to add BViews to, this class
 * makes sure the proper inspector is installed based on whatever MIDI
 * events are supplied through the InstallViewFor() method.
 ***************************************************************************/
typedef std::vector<int32>		int32_vec;
typedef std::vector<BView*>		view_vec;

class AmInspectorFactory
{
public:
	AmInspectorFactory(	AmSongRef songRef,
						AmTrackRef trackRef,
						BView* backgroundView,
						BRect bounds);
	virtual ~AmInspectorFactory();

	bool	SetTrackRef(AmTrackRef trackRef, AmSelectionsI* selections = NULL);
	bool	InstallViewFor(AmSelectionsI* selections);
	void	SetRight(float r)		{ mBounds.right = r; }
		
private:
	AmSongRef			mSongRef;
	AmTrackRef			mTrackRef;
	// This is the view that the inspectors will be added to.
	BView*				mBackgroundView;
	/* These vectors store MIDI event types and the corresponding
	 * inspector views they create.  Both vecs are always the same size.
	 */
	int32_vec			mTypes;
	view_vec			mViews;
	BRect				mBounds;
	
	/* The following methods are responsible for answering an editing
	 * view for an event or list.  Since I always receive a list of
	 * events, even if only a single event is currently selected.
	 * GetView() determines whether or not the list has just one view
	 * in it.  If it does, then it uses the GetViewForEvent() method,
	 * otherwise it uses GetViewForPhrase().
	 */
	BView* GetView(AmSelectionsI* selections);
	BView* GetViewForEvent(AmPhraseEvent* container, AmEvent* event);
	BView* GetViewForEvents(AmSelectionsI* selections);

	/* If any view is installed, detach it from the background.
	 */
	bool EmptyView();
};

#endif 
