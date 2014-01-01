/* AmControls.h
 * Copyright (c)1998-2000 by Angry Red Planet.
 * All rights reserved.
 *
 * This is an assortment of minor views that are used to allow precise
 * editing of the various properties of the MIDI events.
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
 * 05.17.00			hackborn
 * Moved over to the new MIDI domain.
 *
 * 05.08.00			hackborn
 * Updated all the text controls to be the now-standard int control.
 *
 * Dec 20, 1998		hackborn
 * 	Created this file.
 */


#ifndef AMKERNEL_AMEVENTCONTROLS_H
#define AMKERNEL_AMEVENTCONTROLS_H

#include <be/interface/MenuField.h>
#include <be/interface/View.h>
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmTrackRef.h"
#include "AmKernel/AmPhrase.h"
#include "ArpViews/ArpFloatControl.h"
#include "ArpViews/ArpIntControl.h"
class AmBankChange;
class AmChangeEventUndo;
class AmPhraseEvent;
class AmSignature;
class AmTrack;

enum {
	ARPMSG_TIME_VIEW_CHANGED	= 'aTVC'
};

/***************************************************************************
 * AM-UNDOABLE-INT-CONTROL
 * This just includes the wrapper behaviour that creates the undo state.
 * Subclasses are still responsible for noting when events change.
 ***************************************************************************/
class AmUndoableIntControl : public ArpIntControl
{
public:
	AmUndoableIntControl(	BRect frame,
						const char *name,
						const char *label,
						BMessage *message,
						uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
						uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	virtual ~AmUndoableIntControl();

	virtual	void	KeyUp(const char *bytes, int32 numBytes);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint pt);
	virtual	void	MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage *msg);

protected:
	AmChangeEventUndo*	mUndo;

	void			PrepareUndoState(	AmTrack* track,
										AmPhrase* phrase, AmEvent* event);
	virtual void	CommitUndo() = 0;
	void			LockedCommitUndo(AmSong* song);
	
private:
	typedef ArpIntControl	inherited;
};

/***************************************************************************
 * AM-UNDOABLE-FLOAT-CONTROL
 * This just includes the wrapper behaviour that creates the undo state.
 * Subclasses are still responsible for noting when events change.
 ***************************************************************************/
class AmUndoableFloatControl : public ArpFloatControl
{
public:
	AmUndoableFloatControl(	BRect frame,
							const char *name,
							const char *label,
							BMessage *message,
							uint32 rmask = B_FOLLOW_LEFT | B_FOLLOW_TOP,
							uint32 flags = B_WILL_DRAW | B_NAVIGABLE); 
	virtual ~AmUndoableFloatControl();

	virtual	void	KeyUp(const char *bytes, int32 numBytes);
	virtual	void	MouseDown(BPoint pt);
	virtual	void	MouseUp(BPoint pt);
	virtual	void	MouseMoved(	BPoint pt,
								uint32 code,
								const BMessage *msg);

protected:
	AmChangeEventUndo*	mUndo;

	void			PrepareUndoState(	AmTrack* track,
										AmPhrase* phrase, AmEvent* event);
	virtual void	CommitUndo() = 0;
	void			LockedCommitUndo(AmSong* song);
	
private:
	typedef ArpFloatControl	inherited;
};

/***************************************************************************
 * AM-TIME-VIEW
 * This class displays and edits time information in a friendly, user-
 * oriented fashion.  The format is MMMM.BB.CCC, where M = measure, B = beat,
 * C = clock.  It is somewhat modal depending upon what version of the
 * constructor you use:  Using either the signature or phrase constructors
 * means this class will store its own copy of whatever signatures are
 * supplied, and it will not be a valid AmSongObserver.  If the songRef and
 * trackRef constructor is used, then when building the display, the list
 * of signatures will be dynamically generated from the supplied track and
 * this object will be a full-fledged song observer, capable of locking its
 * song.
 ***************************************************************************/
class AmTimeView : public BView,
				   public AmSongObserver
{
public:
	/* Use this form if the control is just being used to set a time value
	 * with no context.  Provide the most appropriate signature you can.
	 */
	AmTimeView(const AmSignature& signature);
	/* Use this form if the user is selecting a time value from within the
	 * context of a series of signature changes.
	 */
	AmTimeView(const AmPhrase& signatures);
	/* Use this form if the user is selecting a time value from within the
	 * context of a song, and you want the signature list rebuilt as
	 * necessary.
	 */
	AmTimeView(AmSongRef songRef);
	/* Use this form if the user is selecting a time value from within the
	 * context of a series of signature changes, and you want the signature
	 * list rebuilt dynamically based on the supplied track.
	 */
	AmTimeView(	AmSongRef songRef,
				AmTrackRef trackRef);
	virtual ~AmTimeView();

	/* Set the values of all my int controls, based on my current display
	 * time.  This method will fail with B_ERROR if a valid signature could
	 * not be constructed from mTime.
	 */
	status_t		InitializeDisplay();
	virtual	void	AttachedToWindow();
	virtual	void	GetPreferredSize(float* width, float* height);
	virtual void	MessageReceived(BMessage* msg);

	virtual void	SetEnabled(bool on);
	bool			IsEnabled() const;

	/* Answer my currently displayed value as an AmTime.
	 */
	virtual AmTime		DisplayTime() const;
	virtual status_t	SetTime(AmTime time);
	
protected:
	/* This will only be valid if the client used the form of the
	 * constructor that supplies a song and track ref.
	 */
	AmTrackRef			mTrackRef;

	/* Set my current mTime.the new time appropriately for my event.  Subclasses can
	 * reimplement if they should be calling a method other than track->SetTime().
	 * After its finished setting the time, it sends out a notice of
	 * ARPMSG_TIME_VIEW_CHANGED to any observers, in case anyone is
	 * dependent on changes here.  (This means you, AmEventEndTimeView.)
	 */
	virtual void		SetDisplayTime(AmTime newTime);

	virtual void		ObserverMessageReceived(BMessage* msg);
	
private:
	typedef BView		inherited;
	/* This signature is constructed whenever the time
	 * is set or changed.  It contains the signature
	 * information for the given time.
	 */
	AmSignature*		mSignature;
	/* My list of signatures.
	 */
	AmPhrase			mSignatures;
	/* The currently displayed time value, cached as a
	 * friendly AmTime.
	 */
	AmTime				mTime;
	
	ArpIntControl*		mMeasureCtrl;
	ArpIntControl*		mBeatCtrl;
	ArpIntControl*		mClockCtrl;

	/* Populate my mSignature object from my mTime.  Answer
	 * with B_OK if the signature was constructed properly,
	 * otherwise B_ERROR.  An error probably indicates there's
	 * something wrong with getting the signature list.  The first form
	 * checks to see if I have a valid song and track ref, and uses
	 * the list based on that.  If I don't, my mSignatures phrase is used.
	 */
	virtual status_t	ConstructSignatureFromTime();
	status_t			ConstructSignatureFromTime(const AmPhrase& signatures);
	/* Populate mSignature according to the values that are
	 * currently in my int controls.  Answer B_ERROR if it
	 * couldn't be populated for some reason.
	 */
	virtual status_t	ConstructSignatureFromControls();
	status_t			ConstructSignatureFromControls(const AmPhrase& signatures);
	/* Answer the signature with the given measure, or the
	 * first signature before it.
	 */
	const AmSignature*	GetSignatureBefore(const AmPhrase& signatures, int32 measure);

	void				AddViews();
};

/***************************************************************************
 * AM-EVENT-TIME-VIEW
 * This class displays and edits time information in a friendly, user-
 * oriented fashion.  The format is MMMM.BB.CCC, where M = measure, B = beat,
 * C = clock.
 ***************************************************************************/
class AmEventTimeView : public BView,
						public AmSongObserver
{
public:
	AmEventTimeView(AmSongRef songRef,
					AmTrackRef trackRef,
					BPoint leftTop);
	virtual ~AmEventTimeView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmEvent* event);
	/* Set the values of all my int controls, based on my current track and
	 * time.  This method will fail with B_ERROR if a valid signature could
	 * not be constructed from mTime.
	 */
	status_t		SetControls();
	virtual	void	AttachedToWindow();
	virtual void	MessageReceived(BMessage* msg);

	/* A hook my int controls call.  Pretty much a hack.
	 */
	void 			PrepareUndoState();
	void			IntControlFinished();

protected:
	AmTrackRef		mTrackRef;
	AmPhraseEvent*	mContainer;
	AmEvent*		mEvent;

	void ObserverMessageReceived(BMessage* msg);
	/* Answer the Time() of my current event.  Subclasses can override if
	 * they should be answering some other time value in the current event.
	 */
	virtual AmTime EventTime() const;
	/* Set the new time appropriately for my event.  Subclasses can
	 * reimplement if they should be calling a method other than track->SetTime().
	 * After its finished setting the time, it sends out a notice of
	 * ARPMSG_TIME_VIEW_CHANGED to any observers, in case anyone is
	 * dependent on changes here.  (This means you, AmEventEndTimeView.)
	 */
	virtual void	SetNewTime(AmTime newTime);
	void			LockedIntControlFinished(AmSong* song);
	virtual const char* UndoName() const;
	
private:
	typedef BView		inherited;
	/* This signature is constructed whenever the time
	 * is set or changed.  It contains the signature
	 * information for the given time.
	 */
	AmSignature*		mSignature;
	
	ArpIntControl*		mMeasureCtrl;
	ArpIntControl*		mBeatCtrl;
	ArpIntControl*		mClockCtrl;
	/* An undo object that gets modified as the user drags.
	 */
	AmChangeEventUndo*	mUndo;

	/* Populate my mSignature object from my mTime.  Answer
	 * with B_OK if the signature was constructed properly,
	 * otherwise B_ERROR.  An error probably indicates there's
	 * something wrong with getting the signature list.
	 */
	status_t ConstructSignatureFromTime();
	/* Populate mSignature according to the values that are
	 * currently in my int controls.  Answer B_ERROR if it
	 * couldn't be populated for some reason.
	 */
	status_t ConstructSignatureFromControls();
};

/***************************************************************************
 * AM-EVENT-END-TIME-VIEW
 * This subclass of AmEventTimeView is almost identical, with just some
 * slight adjustments to work on the EndTime() property of a note on event.
 ***************************************************************************/
class AmEventEndTimeView : public AmEventTimeView
{
public:
	AmEventEndTimeView(	AmSongRef songRef,
						AmTrackRef trackRef,
						BPoint leftTop);
	virtual ~AmEventEndTimeView();

protected:
	virtual AmTime	EventTime() const;
	virtual void	SetNewTime(AmTime newTime);
	virtual const char* UndoName() const;

private:
	typedef AmEventTimeView		inherited;
};

/***************************************************************************
 * AM-NOTE-VIEW
 * This class displays and edits note information in a friendly, user-
 * oriented way.  For example, C#5 is displayed instead of 61.
 ***************************************************************************/
class AmNoteView : public AmUndoableIntControl,
				   public AmSongObserver
{
public:
	AmNoteView(	BRect frame,
				AmSongRef songRef,
				AmTrackRef trackRef);
	virtual ~AmNoteView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmNoteOn* noteOn);
	virtual void	NotifyHook(int32 newValue);
	
protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableIntControl inherited;
	AmPhraseEvent*			mContainer;
	AmNoteOn*				mNoteOn;
	AmTrackRef				mTrackRef;
};

/***************************************************************************
 * AM-VELOCITY-VIEW
 * This class displays and edits velocity information.
 ***************************************************************************/
class AmVelocityView : public AmUndoableIntControl,
					   public AmSongObserver
{
public:
	AmVelocityView(	BRect frame, 
					AmSongRef songRef,
					AmTrackRef trackRef);
	virtual ~AmVelocityView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmNoteOn* noteOn);
	virtual void	NotifyHook(int32 newValue);

protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableIntControl inherited;
	AmPhraseEvent*			mContainer;
	AmNoteOn*				mNoteOn;
	AmTrackRef				mTrackRef;
};

/***************************************************************************
 * AM-RELEASE-VELOCITY-VIEW
 * This class displays and edits release velocity information.
 ***************************************************************************/
class AmReleaseVelocityView : public AmUndoableIntControl,
							  public AmSongObserver
{
public:
	AmReleaseVelocityView(	BRect frame, 
							AmSongRef songRef,
							AmTrackRef trackRef);
	virtual ~AmReleaseVelocityView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmNoteOn* noteOn);
	virtual void	NotifyHook(int32 newValue);

protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableIntControl inherited;
	AmPhraseEvent*			mContainer;
	AmNoteOn*				mNoteOn;
	AmTrackRef				mTrackRef;
};

/***************************************************************************
 * AM-BANK-CHANGE-VIEW
 * This menu field presents a list of programs for the current bank.
 ***************************************************************************/
class AmBankChangeView : public BMenuField
{
public:
	AmBankChangeView(	BRect frame,
						AmSongRef songRef,
						AmTrackRef trackRef);
	virtual ~AmBankChangeView();

	virtual	void		AttachedToWindow();
	virtual	void		GetPreferredSize(float *width, float *height);
	virtual	void		MessageReceived(BMessage* msg);
	/* This method is used to build the menu.
	 */
	void				SetTrackRef(AmTrackRef trackRef);
	void				SetEvent(AmPhraseEvent* container, AmBankChange* pc);
	int32				BankNumber() const;
	AmProgramChange*	ProgramChange() const;

private:
	typedef BMenuField		inherited;
	AmSongRef				mSongRef;
	AmTrackRef				mTrackRef;
	AmPhraseEvent*			mContainer;
	AmBankChange*			mBankChange;
	/* Everytime I redraw my display I cache the currently displayed
	 * number, so I don't have to redisplay it if I get a change to
	 * my event but the number hasn't changed.
	 */
	int32					mCachedBankNumber;
	/* Build the preferred width as I'm building the menu and cache it.
	 */
	mutable float			mPreferredWidth;

	/* Do all the work of creating the undo state and setting the
	 * bank number.
	 */
	void					LockedSetBankNumber(AmSong* song, uint8 number);

	/* Given a track and a menu, fill the menu in with an item for each
	 * program change in the track's device, or 0 - 127 if the track
	 * has no instrument.  Assume the supplied menu is empty.
	 */
	void					BuildMenu(const AmTrack* track, BMenu* menu) const;
};

/***************************************************************************
 * AM-CC-TEXT-VIEW
 * This class displays and edits control change information.
 ***************************************************************************/
class AmCcTextView : public AmUndoableIntControl,
					 public AmSongObserver
{
public:
	AmCcTextView(	BRect frame,
					AmSongRef songRef,
					AmTrackRef trackRef);
	virtual ~AmCcTextView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmControlChange* cc);
	virtual void	NotifyHook(int32 newValue);

protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableIntControl inherited;
	AmPhraseEvent*			mContainer;
	AmControlChange*		mCc;
	AmTrackRef				mTrackRef;
};

/***************************************************************************
 * AM-CHANNEL-PRESSURE-CONTROL
 * This class displays and edits channel pressure values.
 ***************************************************************************/
class AmChannelPressureControl : public AmUndoableIntControl,
								 public AmSongObserver
{
public:
	AmChannelPressureControl(	BRect frame,
								AmSongRef songRef,
								AmTrackRef trackRef);
	virtual ~AmChannelPressureControl();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmChannelPressure* cp);
	virtual void	NotifyHook(int32 newValue);

protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableIntControl inherited;
	AmPhraseEvent*			mContainer;
	AmChannelPressure*		mChannelPressure;
	AmTrackRef				mTrackRef;
};

/***************************************************************************
 * AM-PITCH-BEND-TEXT-VIEW
 * This class displays and edits pitch information.
 ***************************************************************************/
class AmPitchBendView : public AmUndoableIntControl,
						public AmSongObserver
{
public:
	AmPitchBendView(BRect frame,
					AmSongRef songRef,
					AmTrackRef trackRef);
	virtual ~AmPitchBendView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmPitchBend* pb);
	virtual void	NotifyHook(int32 newValue);

protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableIntControl inherited;
	AmPhraseEvent*			mContainer;
	AmPitchBend*			mPitchBend;
	AmTrackRef				mTrackRef;
};

/***************************************************************************
 * AM-PROGRAM-CHANGE-VIEW
 * This menu field presents a list of programs for the current bank.
 ***************************************************************************/
class AmProgramChangeView : public BMenuField,
							public AmSongObserver
{
public:
	AmProgramChangeView(BRect frame,
						AmSongRef songRef,
						AmTrackRef trackRef);
	virtual ~AmProgramChangeView();

	virtual	void	AttachedToWindow();
	virtual	void	MessageReceived(BMessage* msg);
	/* This method is used to build the menu.
	 */
	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmProgramChange* pc);
	/* If I am given a bank control, then I grab the
	 * current bank number from the control.
	 */
	void			SetBankControl(AmBankChangeView* bv);
		
private:
	typedef BMenuField		inherited;
	AmPhraseEvent*			mContainer;
	AmProgramChange*		mProgramChange;
	AmTrackRef				mTrackRef;
	/* Everytime I redraw my display I cache the currently displayed
	 * number, so I don't have to redisplay it if I get a change to
	 * my event but the number hasn't changed.
	 */
	int32					mCachedProgramNumber;
	int32					mCachedBankNumber;
	/* This is the bank I use to get my list of program change names.
	 */
	AmBankChangeView*		mBankCtrl;
	/* Do all the work of creating the undo state and setting the
	 * program number.
	 */
	void					LockedSetProgramNumber(AmSong* song, uint8 number);

	/* Given a track and a menu, fill the menu in with an item for each
	 * program change in the track's device, or 0 - 127 if the track
	 * has no instrument.  Assume the supplied menu is empty.
	 */
	void					BuildMenu(	const AmTrack* track,
										BMenu* menu, int32 oldIndex) const;

	/* Answer the bank number based on my bank change, if any, or none.
	 */
	uint32					BankNumber() const;
	/* Answer the program change based on my bank change, if any, or mProgramChange.
	 */
	AmProgramChange*		ProgramChange() const;
};

/***************************************************************************
 * AM-TEMPO-TEXT-VIEW
 * This class displays and edits tempo change information.
 ***************************************************************************/
class AmTempoTextView : public AmUndoableFloatControl
{
public:
	AmTempoTextView(BRect frame,
					AmSongRef songRef,
					AmTrackRef trackRef);
	virtual ~AmTempoTextView();

	void			SetTrackRef(AmTrackRef trackRef);
	void			SetEvent(AmPhraseEvent* container, AmTempoChange* tempoChange);
	virtual void	NotifyHook(float newValue);

protected:
	virtual void	CommitUndo();

private:
	typedef	AmUndoableFloatControl inherited;
	AmSongRef				mSongRef;
	AmTrackRef				mTrackRef;
	AmPhraseEvent*			mContainer;
	AmTempoChange*			mTempoChange;
};

#endif 
