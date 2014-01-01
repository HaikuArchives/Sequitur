/* AmEventControls.cpp
 */
#include "AmKernel/AmEventControls.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <be/interface/MenuItem.h>
#include <be/interface/StringView.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpIntToStringMapI.h"
#include "ArpViews/ArpIntControlMotions.h"
#include "AmPublic/AmControls.h"
#include "AmPublic/AmDeviceI.h"
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmPrefsI.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTrack.h"

static const uint32	INT_FINISHED_MSG	= 'iIcf';

// FIX: The time view needs to hear about changes in the signature.
// FIX: Need a way to right-justify the int controls.

/***************************************************************************
 * AM-UNDOABLE-INT-CONTROL
 ***************************************************************************/
AmUndoableIntControl::AmUndoableIntControl(	BRect frame,
											const char *name,
											const char *label,
											BMessage *message,
											uint32 rmask,
											uint32 flags)
		: ArpIntControl(frame, name, label, message, rmask, flags),
		  mUndo(NULL)
{
}

AmUndoableIntControl::~AmUndoableIntControl()
{
	if (mUndo) delete mUndo;
}

void AmUndoableIntControl::KeyUp(const char *bytes, int32 numBytes)
{
	inherited::KeyUp(bytes, numBytes);
	CommitUndo();
}

void AmUndoableIntControl::MouseDown(BPoint pt)
{
	delete mUndo;
	mUndo = NULL;
	inherited::MouseDown(pt);
}

void AmUndoableIntControl::MouseUp(BPoint pt)
{
	CommitUndo();
	inherited::MouseUp(pt);
}

void AmUndoableIntControl::MouseMoved(	BPoint pt,
										uint32 code,
										const BMessage *msg)
{
	/* Sometimes, views might not receive a mouse up for
	 * whatever reason.  If it looks like that's happened,
	 * then behave as if I've had a mouse up.
	 */
	if (mUndo) {
		BPoint	pointypointy;
		uint32	button;
		GetMouse(&pointypointy, &button, false);
		if( button == 0 ) CommitUndo();
	}
	inherited::MouseMoved(pt, code, msg);
}

void AmUndoableIntControl::LockedCommitUndo(AmSong* song)
{
	ArpASSERT(song && mUndo);
	if (!mUndo) return;
	if ( !song->UndoContext() || !mUndo->HasChanges() ) {
		delete mUndo;
		mUndo = NULL;
		return;
	}
	song->UndoContext()->AddOperation(mUndo, BResEditor::B_ANY_UNDO_MERGE);
	BString		undoName("Change ");
	undoName << mUndo->StringContents();
	song->UndoContext()->SetUndoName( undoName.String() );
	song->UndoContext()->CommitState();
	mUndo = NULL;
}

void AmUndoableIntControl::PrepareUndoState(AmTrack* track,
											AmPhrase* phrase, AmEvent* event)
{
	if (mUndo) return;
	mUndo = new AmChangeEventUndo(track);
	if (!mUndo) return;
	mUndo->EventChanging(phrase, event);
}

// #pragma mark -

/***************************************************************************
 * AM-UNDOABLE-FLOAT-CONTROL
 ***************************************************************************/
AmUndoableFloatControl::AmUndoableFloatControl(	BRect frame,
												const char *name,
												const char *label,
												BMessage *message,
												uint32 rmask,
												uint32 flags)
		: inherited(frame, name, label, message, rmask, flags),
		  mUndo(NULL)
{
}

AmUndoableFloatControl::~AmUndoableFloatControl()
{
	if (mUndo) delete mUndo;
}

void AmUndoableFloatControl::KeyUp(const char *bytes, int32 numBytes)
{
	inherited::KeyUp(bytes, numBytes);
	CommitUndo();
}

void AmUndoableFloatControl::MouseDown(BPoint pt)
{
	delete mUndo;
	mUndo = NULL;
	inherited::MouseDown(pt);
}

void AmUndoableFloatControl::MouseUp(BPoint pt)
{
	CommitUndo();
	inherited::MouseUp(pt);
}

void AmUndoableFloatControl::MouseMoved(BPoint pt,
										uint32 code,
										const BMessage *msg)
{
	/* Sometimes, views might not receive a mouse up for
	 * whatever reason.  If it looks like that's happened,
	 * then behave as if I've had a mouse up.
	 */
	if (mUndo) {
		BPoint	pointypointy;
		uint32	button;
		GetMouse(&pointypointy, &button, false);
		if (button == 0) CommitUndo();
	}
	inherited::MouseMoved(pt, code, msg);
}

void AmUndoableFloatControl::LockedCommitUndo(AmSong* song)
{
	ArpASSERT(song && mUndo);
	if (!mUndo) return;
	if (!song->UndoContext() || !mUndo->HasChanges() ) {
		delete mUndo;
		mUndo = NULL;
		return;
	}
	song->UndoContext()->AddOperation(mUndo, BResEditor::B_ANY_UNDO_MERGE);
	BString		undoName("Change ");
	undoName << mUndo->StringContents();
	song->UndoContext()->SetUndoName( undoName.String() );
	song->UndoContext()->CommitState();
	mUndo = NULL;
}

void AmUndoableFloatControl::PrepareUndoState(	AmTrack* track,
												AmPhrase* phrase, AmEvent* event)
{
	if (mUndo) return;
	mUndo = new AmChangeEventUndo(track);
	if (!mUndo) return;
	mUndo->EventChanging(phrase, event);
}

// #pragma mark -

/*************************************************************************
 * AM-TIME-VIEW
 *************************************************************************/
AmTimeView::AmTimeView(const AmSignature& signature)
		: inherited(BRect(0, 0, 0, 0),
					"midi_time",
					B_FOLLOW_TOP | B_FOLLOW_LEFT,
					B_WILL_DRAW),
		  AmSongObserver( AmSongRef() ),
		  mSignature(new AmSignature), mTime(0),
		  mMeasureCtrl(0), mBeatCtrl(0), mClockCtrl(0)
{
	AmSignature*		sig = new AmSignature( signature );
	if( sig ) mSignatures.Add( sig );

	AddViews();
}

AmTimeView::AmTimeView(	const AmPhrase& signatures)
		: inherited(BRect(0, 0, 0, 0),
					"midi_time",
					B_FOLLOW_TOP | B_FOLLOW_LEFT,
					B_WILL_DRAW),
		  AmSongObserver( AmSongRef() ),
		  mSignature(new AmSignature), mTime(0),
		  mMeasureCtrl(0), mBeatCtrl(0), mClockCtrl(0)
{
	AmNode*		n = signatures.HeadNode();
	while( n ) {
		if( n->Event()->Type() == n->Event()->SIGNATURE_TYPE ) {
			AmSignature*	event = dynamic_cast<AmSignature*>( n->Event() );
			AmSignature*	sig;
			if( event && (sig = new AmSignature( *event )) ) {
				mSignatures.Add( sig );
			}
		}
		n = n->next;
	}

	AddViews();
}

AmTimeView::AmTimeView(AmSongRef songRef)
		: inherited(BRect(0, 0, 0, 0),
					"midi_time",
					B_FOLLOW_TOP | B_FOLLOW_LEFT,
					B_WILL_DRAW),
		  AmSongObserver(songRef),
		  mSignature(new AmSignature), mTime(0),
		  mMeasureCtrl(0), mBeatCtrl(0), mClockCtrl(0)
{
	AddViews();
}

AmTimeView::AmTimeView(	AmSongRef songRef,
						AmTrackRef trackRef)
		: inherited(BRect(0, 0, 0, 0),
					"midi_time",
					B_FOLLOW_TOP | B_FOLLOW_LEFT,
					B_WILL_DRAW),
		  AmSongObserver( songRef ), mTrackRef(trackRef),
		  mSignature(new AmSignature), mTime(0),
		  mMeasureCtrl(0), mBeatCtrl(0), mClockCtrl(0)
{
	AddViews();
}

AmTimeView::~AmTimeView()
{
	if( mMeasureCtrl )	mMeasureCtrl->StopWatching(this, ARPMSG_INT_CONTROL_CHANGED);
	if( mBeatCtrl )		mBeatCtrl->StopWatching(this, ARPMSG_INT_CONTROL_CHANGED);
	if( mClockCtrl )	mClockCtrl->StopWatching(this, ARPMSG_INT_CONTROL_CHANGED);
	mSignature->Delete();
	mSignatures.DeleteEvents();
}

status_t AmTimeView::InitializeDisplay()
{
	if( ConstructSignatureFromTime() != B_OK ) return B_ERROR;
	// Set the measure value
	if( mMeasureCtrl ) mMeasureCtrl->SetValue( mSignature->Measure() );
	// Calculate and set the beat value
	AmTime	time = DisplayTime() - mSignature->StartTime();
	AmTime	start = 0;
	AmTime	pulses = PPQN;
	int32		beat = 1;
	while ( (start + pulses) <= time) {
		start += pulses;
		beat++;
	}
	// Also need to set the range of values for the beat control here.
	if( mBeatCtrl ) {
		mBeatCtrl->SetLimits(1, mSignature->Beats() );
		mBeatCtrl->SetValue( beat );
	}
	// Calculate and set the clock value
	if( mClockCtrl ) mClockCtrl->SetValue( time - start );
	
	return B_OK;
}

void AmTimeView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if ( Parent() != 0 ) SetViewColor( Parent()->ViewColor() );
	InitializeDisplay();
}

void AmTimeView::GetPreferredSize(float* width, float* height)
{
	float	w = 0, h = 0;
	BView*	view;
	for( view = ChildAt(0); view != 0; view = view->NextSibling() ) {
		BRect	b = view->Frame();
		if( b.right > w ) w = b.right;
		if( b.bottom > h ) h = b.bottom;
	}
	*width = w;
	*height = h;
}

void AmTimeView::MessageReceived(BMessage* msg)
{
	switch( msg->what ) {
		case B_OBSERVER_NOTICE_CHANGE:
			ObserverMessageReceived(msg);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmTimeView::SetEnabled(bool on)
{
	if (mMeasureCtrl) mMeasureCtrl->SetEnabled(on);
	if (mBeatCtrl) mBeatCtrl->SetEnabled(on);
	if (mClockCtrl) mClockCtrl->SetEnabled(on);
}

bool AmTimeView::IsEnabled() const
{
	if (mMeasureCtrl) return mMeasureCtrl->IsEnabled();
	if (mBeatCtrl) return mBeatCtrl->IsEnabled();
	if (mClockCtrl) return mClockCtrl->IsEnabled();
	return false;
}

AmTime AmTimeView::DisplayTime() const
{
	return mTime;
}


status_t AmTimeView::SetTime(AmTime time)
{
	ArpASSERT(time >= 0);
	mTime = time;
	if (ConstructSignatureFromTime() != B_OK) return B_ERROR;
	// Set the measure value
	if (mMeasureCtrl) mMeasureCtrl->SetValue( mSignature->Measure() );
	// Calculate and set the beat value
	AmTime	t = mTime - mSignature->StartTime();
	AmTime	start = 0;
	AmTime	ticks = mSignature->TicksPerBeat();
	int32	beat = 1;
	while ( (start + ticks) <= t) {
		start += ticks;
		beat++;
	}
	// Also need to set the range of values for the beat control here.
	if (mBeatCtrl) {
		mBeatCtrl->SetLimits(1, mSignature->Beats() );
		mBeatCtrl->SetValue(beat);
	}
	// Calculate and set the clock value
	if (mClockCtrl) mClockCtrl->SetValue(t - start);
	
	return B_OK;
}

void AmTimeView::SetDisplayTime(AmTime newTime)
{
	mTime = newTime;
	SendNotices(ARPMSG_TIME_VIEW_CHANGED);
}

void AmTimeView::ObserverMessageReceived(BMessage* msg)
{
	int32		change = 0;
	msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
	if (change == ARPMSG_INT_CONTROL_CHANGED) {
		if (ConstructSignatureFromControls() == B_OK) {
			if (!mBeatCtrl || !mClockCtrl) return;
			AmTime	time = mSignature->StartTime();
			time += ( (mBeatCtrl->Value() - 1) * PPQN );
			time += mClockCtrl->Value();
			SetDisplayTime(time);
		}
	} else if (change == ARPMSG_TIME_VIEW_CHANGED) {
		// Just force a redisplay
//		SetEvent( mContainer, mEvent );
	}
}

status_t AmTimeView::ConstructSignatureFromTime()
{
	if (!mSignature) return B_NO_MEMORY;
	status_t			err = B_ERROR;
	if ( SongRef().IsValid() ) {
		// READ TRACK BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("AmTimeView::ConstructSignatureFromTime() read lock\n");
		#endif
		const AmSong*	song = ReadLock();
		const AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
		if (track) err = track->GetSignature(DisplayTime(), *mSignature);
		else if (song) err = song->GetSignature(DisplayTime(), *mSignature);
		ReadUnlock(song);
		// END READ TRACK BLOCK
	} else {
		err = ConstructSignatureFromTime( mSignatures );
	}
	return err;
}

status_t AmTimeView::ConstructSignatureFromTime(const AmPhrase& signatures)
{
	debugger("This code doesn't work at all -- if anyone actually uses it, it needs to be rewritten");
	AmNode*			node = signatures.FindNode( DisplayTime(), BACKWARDS_SEARCH );
	if( !node ) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	if( !sig ) return B_ERROR;
	
	AmTime			start = sig->StartTime();
	AmTime			ticks = sig->TicksPerBeat();
	AmTime			displayTime = DisplayTime();
	int32			measure = sig->Measure();
	while ( (start + ticks) <= displayTime ) {
		start += ticks;
		measure++;
	}
	mSignature->Set( start, measure, sig->Beats(), sig->BeatValue() );
	return B_OK;
}

status_t AmTimeView::ConstructSignatureFromControls()
{
	status_t			err = B_ERROR;
	if ( SongRef().IsValid() ) {
		// READ TRACK BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("AmTimeView::ConstructSignatureFromControls() read lock\n");
		#endif
		const AmSong*	song = ReadLock();
		const AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
		if (track) err = ConstructSignatureFromControls( track->Signatures() );
		else if (song) err = ConstructSignatureFromControls( song->Signatures() );
		ReadUnlock(song);
		// END READ TRACK BLOCK
	} else {
		err = ConstructSignatureFromControls( mSignatures );
	}
	return err;
}

status_t AmTimeView::ConstructSignatureFromControls(const AmPhrase& signatures)
{
	if (!mMeasureCtrl) return B_ERROR;
	const AmSignature*	sig = GetSignatureBefore( signatures, mMeasureCtrl->Value() );
	if (!sig) return B_ERROR;
	
	int32		newMeasure = mMeasureCtrl->Value();
	if (sig->Measure() == newMeasure) {
		mSignature->Set( sig->StartTime(), newMeasure, sig->Beats(), sig->BeatValue() );
	} else {
		AmTime	start = sig->StartTime();
		AmTime	ticks = sig->Duration();
		int32	sigMeasure = sig->Measure();
		while (sigMeasure < newMeasure) {
			start += ticks;
			sigMeasure++;
		}
		mSignature->Set( start, newMeasure, sig->Beats(), sig->BeatValue() );
	}
	return B_OK;
}

const AmSignature* AmTimeView::GetSignatureBefore(const AmPhrase& signatures, int32 measure)
{
	AmNode*			node = signatures.HeadNode();
	if( !node ) return 0;
	
	while ( (node != 0) && (node->next != 0) ) {
		const AmSignature*	sig = dynamic_cast<const AmSignature*>( ((AmNode*)node->next)->Event() );
		if ( (sig != 0) && (sig->Measure() > measure) ) return sig;
		node = (AmNode*)node->next;
	}
	return dynamic_cast<const AmSignature*>( node->Event() );
}

void AmTimeView::AddViews()
{
	font_height		fheight;
	GetFontHeight( &fheight );
	// Add the measure control
	float	left = 0, top = 0;
	float	width = StringWidth("0000") + 5;
	float	height = fheight.ascent + fheight.descent + fheight.leading + 1;
	BRect	f(left, top, left + width, top + height);
	if( (mMeasureCtrl = new ArpIntControl(f, "measure", 0, 0)) != 0 ) {
		mMeasureCtrl->SetLimits(1, 9999);
		mMeasureCtrl->StartWatching(this, ARPMSG_INT_CONTROL_CHANGED);
		mMeasureCtrl->SetMotion( new ArpIntControlSmallMotion() );
		AddChild(mMeasureCtrl);
	}
	// Add a ":"
	left += width;
	float	dotW = StringWidth(":") + 2;
	BStringView	*sv = new BStringView(	BRect(left, top, left + dotW, top + height - 2),
										"dot", ":");
	if( sv ) AddChild(sv);
	// Add the beat control
	left += dotW;
	width = StringWidth("00") + 5;
	f.Set(left, top, left + width, top + height);
	if( (mBeatCtrl = new ArpIntControl(f, "beat", 0, 0)) != 0 ) {
		mBeatCtrl->StartWatching(this, ARPMSG_INT_CONTROL_CHANGED);
		AddChild(mBeatCtrl);
	}
	// Add a ":"
	left += width;
	sv = new BStringView(	BRect(left, top, left + dotW, top + height - 2),
							"dot", ":");
	if( sv ) AddChild(sv);
	// Add the clock control
	left += dotW;
	width = StringWidth("0000") + 5;
	f.Set(left, top, left + width, top + height);
	if ( (mClockCtrl = new ArpIntControl(f, "clock", 0, 0)) != 0 ) {
		mClockCtrl->SetLimits(0, PPQN - 1);
		mClockCtrl->StartWatching(this, ARPMSG_INT_CONTROL_CHANGED);
		AddChild(mClockCtrl);
	}
	ResizeTo(left + width, top + height);	
}

// #pragma mark -

/*************************************************************************
 * AM-EVENT-TIME-VIEW
 *************************************************************************/
/* This is pretty gross -- this ArpIntControl subclass exists solely to
 * inform an AmEventTimeView when the mouse down occurs.  This lets the
 * time view clear out its state.
 */
class _AmIntControlHack : public ArpIntControl
{
public:
	_AmIntControlHack(	BRect frame, const char* name, const char* label,
						BMessage* message)
			: ArpIntControl(frame, name, label, message),
			  mTimeView(NULL)
	{
	}

	void KeyDown(const char *bytes, int32 numBytes)
	{
		ArpIntControl::KeyDown(bytes, numBytes);
		if (mTimeView) mTimeView->PrepareUndoState();
	}
	
	void KeyUp(const char *bytes, int32 numBytes)
	{
		ArpIntControl::KeyUp(bytes, numBytes);
		if (mTimeView) mTimeView->IntControlFinished();
	}

	void SetTimeView(AmEventTimeView* timeView)
	{
		mTimeView = timeView;
	}
	
	virtual void MouseDown(BPoint where)
	{
		if (mTimeView) mTimeView->PrepareUndoState();
		ArpIntControl::MouseDown(where);
	}

private:
	AmEventTimeView*	mTimeView;
};

AmEventTimeView::AmEventTimeView(	AmSongRef songRef,
									AmTrackRef trackRef,
									BPoint leftTop)
		: inherited(BRect( leftTop, leftTop ),
					"midi_time",
					B_FOLLOW_TOP | B_FOLLOW_LEFT,
					B_WILL_DRAW),
		AmSongObserver(songRef),
		mTrackRef(trackRef),
		mContainer(0), mEvent(0),
		mSignature(new AmSignature),
		mMeasureCtrl(0), mBeatCtrl(0), mClockCtrl(0), mUndo(NULL)
{
	font_height		fheight;
	GetFontHeight( &fheight );
	// Add the measure control
	float	left = 0, top = 0;
	float	width = StringWidth("0000") + 5;
	float	height = fheight.ascent + fheight.descent + fheight.leading + 1;
	BRect	f(left, top, left + width, top + height);
	if( (mMeasureCtrl = new _AmIntControlHack(f, "measure", NULL, new BMessage(INT_FINISHED_MSG))) != 0 ) {
		((_AmIntControlHack*)mMeasureCtrl)->SetTimeView(this);
		mMeasureCtrl->SetLimits(1, 9999);
		mMeasureCtrl->StartWatching(this, ARPMSG_INT_CONTROL_CHANGED);
		mMeasureCtrl->SetMotion( new ArpIntControlSmallMotion() );
		AddChild(mMeasureCtrl);
	}
	// Add a ":"
	left += width;
	float	dotW = StringWidth(":") + 2;
	BStringView	*sv = new BStringView(	BRect(left, top, left + dotW, top + height - 2),
										"dot", ":");
	if (sv != 0) AddChild(sv);
	// Add the beat control
	left += dotW;
	width = StringWidth("00") + 5;
	f.Set(left, top, left + width, top + height);
	if ( (mBeatCtrl = new _AmIntControlHack(f, "beat", NULL, new BMessage(INT_FINISHED_MSG))) != 0 ) {
		((_AmIntControlHack*)mBeatCtrl)->SetTimeView(this);
		mBeatCtrl->StartWatching(this, ARPMSG_INT_CONTROL_CHANGED);
		AddChild(mBeatCtrl);
	}
	// Add a ":"
	left += width;
	sv = new BStringView(	BRect(left, top, left + dotW, top + height - 2),
							"dot", ":");
	if (sv != 0) AddChild(sv);
	// Add the clock control
	left += dotW;
	width = StringWidth("0000") + 5;
	f.Set(left, top, left + width, top + height);
	if ( (mClockCtrl = new _AmIntControlHack(f, "clock", NULL, new BMessage(INT_FINISHED_MSG))) != 0 ) {
		((_AmIntControlHack*)mClockCtrl)->SetTimeView(this);
		mClockCtrl->SetLimits(0, PPQN - 1);
		mClockCtrl->StartWatching(this, ARPMSG_INT_CONTROL_CHANGED);
		AddChild(mClockCtrl);
	}
	ResizeTo(left + width, top + height);
}

AmEventTimeView::~AmEventTimeView()
{
	mSignature->Delete();
	if (mMeasureCtrl != 0)
		mMeasureCtrl->StopWatching(this, ARPMSG_INT_CONTROL_CHANGED);
	if (mBeatCtrl != 0)
		mBeatCtrl->StopWatching(this, ARPMSG_INT_CONTROL_CHANGED);
	if (mClockCtrl != 0)
		mClockCtrl->StopWatching(this, ARPMSG_INT_CONTROL_CHANGED);
	delete mUndo;
	mUndo = NULL;
}

void AmEventTimeView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mEvent = NULL;
}

void AmEventTimeView::SetEvent(AmPhraseEvent* container, AmEvent* event)
{
	mContainer = container;
	mEvent = event;
	if (mContainer && mEvent) SetControls();
}

status_t AmEventTimeView::SetControls()
{
	if (ConstructSignatureFromTime() != B_OK) return B_ERROR;
	// Set the measure value
	if (mMeasureCtrl) mMeasureCtrl->SetValue( mSignature->Measure() );
	// Calculate and set the beat value
	AmTime	time = EventTime() - mSignature->StartTime();
	AmTime	start = 0;
	AmTime	ticks = mSignature->TicksPerBeat();
	int32	beat = 1;
	while ( (start + ticks) <= time) {
		start += ticks;
		beat++;
	}
	// Also need to set the range of values for the beat control here.
	if (mBeatCtrl) {
		mBeatCtrl->SetLimits(1, mSignature->Beats() );
		mBeatCtrl->SetValue(beat);
	}
	// Calculate and set the clock value
	if (mClockCtrl) mClockCtrl->SetValue(time - start);
	
	return B_OK;
}

void AmEventTimeView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if ( Parent() != 0 ) SetViewColor( Parent()->ViewColor() );
	if (mMeasureCtrl) mMeasureCtrl->SetTarget(this);
	if (mBeatCtrl) mBeatCtrl->SetTarget(this);
	if (mClockCtrl) mClockCtrl->SetTarget(this);
}

void AmEventTimeView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			ObserverMessageReceived(msg);
			break;
		case INT_FINISHED_MSG: {
			IntControlFinished();
			} break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmEventTimeView::PrepareUndoState()
{
	if (mUndo) delete mUndo;
	mUndo = NULL;
	if (!mContainer || !mEvent) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		mUndo = new AmChangeEventUndo(track);
		mUndo->EventChanging(mContainer->Phrase(), mEvent);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmEventTimeView::ObserverMessageReceived(BMessage* msg)
{
	int32		change;
	msg->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
	if (change == ARPMSG_INT_CONTROL_CHANGED) {
		if ( ConstructSignatureFromControls() == B_OK ) {
			if ( (mBeatCtrl == 0) || (mClockCtrl == 0) ) return;
			AmTime	time = mSignature->StartTime();
			time += ( (mBeatCtrl->Value() - 1) * mSignature->TicksPerBeat() );
			time += mClockCtrl->Value();
			SetNewTime(time);
		}
	} else if (change == ARPMSG_TIME_VIEW_CHANGED) {
		// Just force a redisplay
		SetEvent( mContainer, mEvent );
	}
}

AmTime AmEventTimeView::EventTime() const
{
	if (!mContainer || !mEvent) return 0;
	return mContainer->EventRange(mEvent).start;
}

void AmEventTimeView::SetNewTime(AmTime newTime)
{
	if (!mContainer || !mEvent) return;
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) mContainer->SetEventStartTime(mEvent, newTime);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
	SendNotices(ARPMSG_TIME_VIEW_CHANGED);
}

void AmEventTimeView::IntControlFinished()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedIntControlFinished(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

void AmEventTimeView::LockedIntControlFinished(AmSong* song)
{
	ArpASSERT(song);
	if ( !song || !mUndo || !mEvent || !song->UndoContext() ) {
		delete mUndo;
		mUndo = NULL;
		return;
	}
	mUndo->EventChanged(mEvent);
	if ( !mUndo->HasChanges() ) {
		delete mUndo;
		mUndo = NULL;
		return;
	}
	song->UndoContext()->AddOperation(mUndo, BResEditor::B_ANY_UNDO_MERGE);
	BString		undoName( UndoName() );
	undoName << mUndo->StringContents();
	song->UndoContext()->SetUndoName( undoName.String() );
	song->UndoContext()->CommitState();
	mUndo = NULL;
}

const char* AmEventTimeView::UndoName() const
{
	return "Move ";
}

status_t AmEventTimeView::ConstructSignatureFromTime()
{
	if (!mEvent) return B_ERROR;
	status_t		err = B_ERROR;
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmEventTimeView::ConstructSignatureFromTime() read lock\n");
	#endif
	const AmSong*	song = ReadLock();
	const AmTrack*	track = song ? song->Track( mTrackRef ) : 0;
	if (track) {
		AmSignature		sig;
		if ( (err = track->GetSignature(EventTime(), sig)) == B_OK ) {
			mSignature->Set(sig);
		}
	}
	ReadUnlock( song );
	// END READ TRACK BLOCK
	return err;
}

status_t AmEventTimeView::ConstructSignatureFromControls()
{
	if (!mMeasureCtrl) return B_ERROR;
	status_t		err = B_ERROR;
	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmEventTimeView::ConstructSignatureFromControls() read lock\n");
	#endif
	const AmSong*	song = ReadLock();
	const AmTrack*	track = song ? song->Track( mTrackRef ) : 0;
	if (track) {
		AmSignature		sig;
		err = track->GetSignatureForMeasure(mMeasureCtrl->Value(), sig);
		if (err == B_OK) mSignature->Set(sig);
	}
	ReadUnlock( song );
	// END READ TRACK BLOCK
	return err;
}

// #pragma mark -

/***************************************************************************
 * AM-EVENT-END-TIME-VIEW
 ***************************************************************************/
AmEventEndTimeView::AmEventEndTimeView(	AmSongRef songRef,
										AmTrackRef trackRef,
										BPoint leftTop)
		: inherited(songRef, trackRef, leftTop)
{
}

AmEventEndTimeView::~AmEventEndTimeView()
{
}

AmTime AmEventEndTimeView::EventTime() const
{
	if (!mContainer || !mEvent) return 0;
	return mContainer->EventRange(mEvent).end;
}

void AmEventEndTimeView::SetNewTime(AmTime newTime)
{
	if (!mContainer || !mEvent) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	if (song) mContainer->SetEventEndTime(mEvent, newTime);
	WriteUnlock( song );
	// END WRITE TRACK BLOCK
}

const char* AmEventEndTimeView::UndoName() const
{
	return "Change ";
}

// #pragma mark -

/*************************************************************************
 * AM-NOTE-VIEW
 *************************************************************************/
AmNoteView::AmNoteView(	BRect frame,
						AmSongRef songRef,
						AmTrackRef trackRef)
		: inherited(frame, "AmNoteView", NULL, NULL,
				B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		AmSongObserver(songRef),
		mContainer(0), mNoteOn(0), mTrackRef(trackRef)
{
	SetLimits(0, 127);
	SetStringMap( new AmKeyMap() );
}

AmNoteView::~AmNoteView()
{
}

void AmNoteView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mNoteOn = NULL;
}

void AmNoteView::SetEvent(AmPhraseEvent* container, AmNoteOn* noteOn)
{
	mContainer = container;
	mNoteOn = noteOn;
	if( mNoteOn ) SetValue( mNoteOn->Note() );
	Draw( Bounds() );
}

void AmNoteView::NotifyHook(int32 newValue)
{
	if (!mContainer || !mNoteOn) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mNoteOn);
		mNoteOn->SetNote(newValue);
		if (mUndo) mUndo->EventChanged(mNoteOn);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmNoteView::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedCommitUndo(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

// #pragma mark -

/*************************************************************************
 * AM-VELOCITY-VIEW
 *************************************************************************/
AmVelocityView::AmVelocityView(	BRect frame,
								AmSongRef songRef,
								AmTrackRef trackRef)
		: inherited(frame, "AmVelocityView", NULL, NULL,
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		  AmSongObserver(songRef),
		  mContainer(0), mNoteOn(0), mTrackRef(trackRef)
{
	SetLimits(0, 127);
}

AmVelocityView::~AmVelocityView()
{
}

void AmVelocityView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mNoteOn = NULL;
}

void AmVelocityView::SetEvent(AmPhraseEvent* container, AmNoteOn* noteOn)
{
	mContainer = container;
	mNoteOn = noteOn;
	if( mNoteOn ) SetValue( mNoteOn->Velocity() );
	Draw( Bounds() );
}

void AmVelocityView::NotifyHook(int32 newValue)
{
	if (!mContainer || !mNoteOn) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mNoteOn);
		mNoteOn->SetVelocity(newValue);
		if (mUndo) mUndo->EventChanged(mNoteOn);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmVelocityView::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedCommitUndo(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

// #pragma mark -

/*************************************************************************
 * AM-RELEASE-VELOCITY-VIEW
 *************************************************************************/
AmReleaseVelocityView::AmReleaseVelocityView(	BRect frame,
												AmSongRef songRef,
												AmTrackRef trackRef)
		: inherited(frame, "AmVelocityView", NULL, NULL,
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		AmSongObserver(songRef),
		mContainer(0), mNoteOn(0), mTrackRef(trackRef)
{
	SetLimits(0, 127);
}

AmReleaseVelocityView::~AmReleaseVelocityView()
{
}

void AmReleaseVelocityView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mNoteOn = NULL;
}

void AmReleaseVelocityView::SetEvent(AmPhraseEvent* container, AmNoteOn* noteOn)
{
	mContainer = container;
	mNoteOn = noteOn;
	if( mNoteOn ) SetValue( mNoteOn->ReleaseVelocity() );
	Draw( Bounds() );
}

void AmReleaseVelocityView::NotifyHook(int32 newValue)
{
	if (!mContainer || !mNoteOn) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mNoteOn);
		mNoteOn->SetReleaseVelocity(newValue);
		if (mUndo) mUndo->EventChanged(mNoteOn);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmReleaseVelocityView::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedCommitUndo(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

// #pragma mark -

/*************************************************************************
 * AM-BANK-CHANGE-VIEW
 *************************************************************************/
static const uint32			BANK_MSG		= 'ibnk';

static BMenu* new_bank_menu()
{
	BMenu*	m = new BMenu("bank menu");
	if( !m ) return 0;
	m->SetLabelFromMarked( true );
	return m;
}

AmBankChangeView::AmBankChangeView(	BRect frame,
									AmSongRef songRef,
									AmTrackRef trackRef)
		: inherited(frame, "AmBankChangeView", 0, new_bank_menu(),
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		  mSongRef(songRef),
		  mContainer(NULL), mBankChange(NULL), mCachedBankNumber(-1),
		  mPreferredWidth(20)
{
	SetTrackRef(trackRef);
}

AmBankChangeView::~AmBankChangeView()
{
}

void AmBankChangeView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (Menu() ) Menu()->SetTargetForItems(this);
}

void AmBankChangeView::GetPreferredSize(float *width, float *height)
{
	*width = mPreferredWidth;
	*height = Prefs().Size(MENUFIELD_Y);
}

void AmBankChangeView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case BANK_MSG: {
			int32	number;
			if (mBankChange && msg->FindInt32("number", &number) == B_OK) {
				// WRITE SONG BLOCK
				AmSong*		song = mSongRef.WriteLock();
				if (song) LockedSetBankNumber(song, number);
				mSongRef.WriteUnlock(song);
				// END WRITE SONG BLOCK
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void AmBankChangeView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	BMenu*	menu = Menu();
	if (!menu) return;
	if (menu->RemoveItems(0, menu->CountItems(), true) ) return;
	if (!mTrackRef.IsValid() ) return;

	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmProgramChangeView::SetTrackRef() read lock\n");
	#endif
	const AmSong*	song = mSongRef.ReadLock();
	const AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) BuildMenu(track, menu);
	mSongRef.ReadUnlock(song);
	// END READ TRACK BLOCK
}

void AmBankChangeView::SetEvent(AmPhraseEvent* container, AmBankChange* bc)
{
	ArpASSERT(container && bc);
	mContainer = container;
	mBankChange = bc;
	if (mCachedBankNumber != int32(mBankChange->BankNumber()) ) {
		BMenu*		m = Menu();
		if (m) {
			int32		count = m->CountItems();
			for (int32 k = 0; k < count; k++) {
				BMenuItem*	item = m->ItemAt(k);
				if (item) {
					if (bc && k == int32(bc->BankNumber() ) ) item->SetMarked(true);
					else item->SetMarked(false);
				}
			}
		}
	}
	mCachedBankNumber = mBankChange->BankNumber();	
}

int32 AmBankChangeView::BankNumber() const
{
	return mCachedBankNumber;
}

AmProgramChange* AmBankChangeView::ProgramChange() const
{
	if (!mBankChange) return NULL;
	return mBankChange->ProgramChange();
}

void AmBankChangeView::LockedSetBankNumber(AmSong* song, uint8 number)
{
	ArpASSERT(song);
	if (!mContainer || !mBankChange) return;
	AmTrack*	track = song->Track(mTrackRef);
	if (!track) return;
	ArpCRef<AmDeviceI>	device = track->Device();
	if (!device) return;
	uint32		count = device->CountBanks();
	if (number == mCachedBankNumber || number >= count) return;
	ArpCRef<AmBankI>	bank = device->Bank((uint32)number);
	if (!bank) return;

	AmChangeEventUndo* undo = NULL;
	if (song->UndoContext() ) undo = new AmChangeEventUndo(track);
	if (undo) undo->EventChanging(mContainer->Phrase(), mBankChange);
	
	mBankChange->SetTo(bank);
	mCachedBankNumber = number;
	
	if (undo) {
		undo->EventChanged(mBankChange);
		song->UndoContext()->AddOperation(undo, BResEditor::B_ANY_UNDO_MERGE);
		song->UndoContext()->SetUndoName("Bank Change");
		song->UndoContext()->CommitState();
	}
}

static void get_bank_label(ArpCRef<AmBankI> bank, uint32 bankNumber, BString& out)
{
	if (bank) out = bank->Name().String();
	if (out.Length() < 1) out << "Bank " << bankNumber;
}

void AmBankChangeView::BuildMenu(const AmTrack* track, BMenu* menu) const
{
	mPreferredWidth = 20;
	
	AmFilterHolderI*				holder = track->Filter(DESTINATION_PIPELINE);
	if (holder && holder->Filter() ) {
		ArpCRef<AmDeviceI>			device = holder->Filter()->Device();
		if (device) {
			uint32					count = device->CountBanks();
			for (uint32 k = 0; k < count; k++) {
				ArpCRef<AmBankI>	bank = device->Bank(k);
				if (bank) {
					BString			label;
					get_bank_label(bank, k, label);
					BMessage*		msg = new BMessage(BANK_MSG);
					BMenuItem*		item = new BMenuItem(label.String(), msg);
					if (msg && item) {
						msg->AddInt32("number", k);
						menu->AddItem(item);
						float		w = StringWidth(label.String() ) + 20;
						if (w > mPreferredWidth) mPreferredWidth = w;
					}
				}
			}
		}
	}
}

// #pragma mark -

/*************************************************************************
 * AM-CC-TEXT-VIEW
 *************************************************************************/
AmCcTextView::AmCcTextView(	BRect frame,
							AmSongRef songRef,
							AmTrackRef trackRef)
		: inherited(frame, "AmCcTextView", NULL, NULL,
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		AmSongObserver(songRef),
		mContainer(0), mCc(0), mTrackRef(trackRef)
{
	SetLimits(0, 127);
}

AmCcTextView::~AmCcTextView()
{
}

void AmCcTextView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mCc = NULL;
}

void AmCcTextView::SetEvent(AmPhraseEvent* container, AmControlChange* cc)
{
	mContainer = container;
	mCc = cc;
	if (mCc) SetValue( mCc->ControlValue() );
	Draw( Bounds() );
}

void AmCcTextView::NotifyHook(int32 newValue)
{
	if (!mContainer || !mCc) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mCc);
		mCc->SetControlValue(newValue);
		if (mUndo) mUndo->EventChanged(mCc);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmCcTextView::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedCommitUndo(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

// #pragma mark -

/*************************************************************************
 * AM-CHANNEL-PRESSURE-CONTROL
 *************************************************************************/
AmChannelPressureControl::AmChannelPressureControl(	BRect frame,
													AmSongRef songRef,
													AmTrackRef trackRef)
		: inherited(frame, "AmChannelPressureControl", NULL, NULL,
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		AmSongObserver(songRef),
		mContainer(NULL), mChannelPressure(NULL), mTrackRef(trackRef)
{
	SetLimits(0, 127);
}

AmChannelPressureControl::~AmChannelPressureControl()
{
}

void AmChannelPressureControl::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mChannelPressure = NULL;
}

void AmChannelPressureControl::SetEvent(AmPhraseEvent* container, AmChannelPressure* cp)
{
	mContainer = container;
	mChannelPressure = cp;
	if (mChannelPressure) SetValue( mChannelPressure->Pressure() );
	Draw( Bounds() );
}

void AmChannelPressureControl::NotifyHook(int32 newValue)
{
	if (!mContainer || !mChannelPressure) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mChannelPressure);
		mChannelPressure->SetPressure(newValue);
		if (mUndo) mUndo->EventChanged(mChannelPressure);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmChannelPressureControl::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedCommitUndo(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

// #pragma mark -

/*************************************************************************
 * AM-PITCH-BEND-TEXT-VIEW
 *************************************************************************/
AmPitchBendView::AmPitchBendView(	BRect frame,
									AmSongRef songRef,
									AmTrackRef trackRef)
		: inherited(frame, "AmPitchBendView", NULL, NULL,
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		AmSongObserver(songRef),
		mContainer(0), mPitchBend(0), mTrackRef(trackRef)
{
	SetLimits(AM_PITCH_MIN, AM_PITCH_MAX);
	SetMotion( new ArpIntControlMediumMotion() );
}

AmPitchBendView::~AmPitchBendView()
{
}

void AmPitchBendView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mPitchBend = NULL;
}

void AmPitchBendView::SetEvent(AmPhraseEvent* container, AmPitchBend* pb)
{
	mContainer = container;
	mPitchBend = pb;
	if( mPitchBend ) SetValue( mPitchBend->Value() );
	Draw( Bounds() );
}

void AmPitchBendView::NotifyHook(int32 newValue)
{
	if (!mContainer || !mPitchBend) return;
	// WRITE TRACK BLOCK
	AmSong*		song = WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mPitchBend);
		mPitchBend->SetValue(newValue);
		if (mUndo) mUndo->EventChanged(mPitchBend);
	}
	WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmPitchBendView::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = WriteLock();
	if (song) LockedCommitUndo(song);
	WriteUnlock(song);
	// END WRITE SONG BLOCK
}

// #pragma mark -

/*************************************************************************
 * AM-PROGRAM-CHANGE-VIEW
 *************************************************************************/
static const uint32			PROGRAM_MSG		= 'iprg';

static BMenu* new_menu()
{
	BMenu*	m = new BMenu("program menu");
	if( !m ) return 0;
	m->SetLabelFromMarked( true );
	return m;
}

AmProgramChangeView::AmProgramChangeView(	BRect frame,
											AmSongRef songRef,
											AmTrackRef trackRef)
		: inherited(frame, "AmProgramChangeView", 0, new_menu(),
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		  AmSongObserver(songRef),
		  mContainer(NULL), mProgramChange(NULL),
		  mCachedProgramNumber(-1), mCachedBankNumber(-1),
		  mBankCtrl(NULL)
{
	SetTrackRef(trackRef);
}

AmProgramChangeView::~AmProgramChangeView()
{
}

void AmProgramChangeView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	BMenu*	menu = Menu();
	if (!menu) return;
	int32			oldIndex = -1;
	BMenuItem*		item = menu->FindMarked();
	if (item) oldIndex = menu->IndexOf(item);
	menu->RemoveItems(0, menu->CountItems(), true);
	if (!mTrackRef.IsValid() ) return;

	// READ TRACK BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmProgramChangeView::SetTrackRef() read lock\n");
	#endif
	const AmSong*	song = ReadLock();
	const AmTrack*	track = song ? song->Track( mTrackRef ) : 0;
	if (track) BuildMenu(track, menu, oldIndex);
	ReadUnlock(song);
	// END READ TRACK BLOCK
}

void AmProgramChangeView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if (Menu() ) Menu()->SetTargetForItems(this);
}

void AmProgramChangeView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case PROGRAM_MSG:
			{
				int32	number;
				if (msg->FindInt32("number", &number) == B_OK) {
					// WRITE SONG BLOCK
					AmSong*		song = WriteLock();
					if (song) LockedSetProgramNumber(song, number);
					WriteUnlock(song);
					// END WRITE SONG BLOCK
				}
			}
			break;
		default:
			inherited::MessageReceived( msg );
	}
}

void AmProgramChangeView::SetEvent(AmPhraseEvent* container, AmProgramChange* pc)
{
	ArpASSERT(container && pc);
	mContainer = container;
	bool		updateAll = false;
	if (mProgramChange != pc) updateAll = true;
	mProgramChange = pc;
	if (updateAll || mCachedBankNumber != int32(BankNumber()) )
		SetTrackRef(mTrackRef);
	if (updateAll || mCachedProgramNumber != int32(mProgramChange->ProgramNumber()) ) {
		BMenu*		m = Menu();
		if (m) {
			int32		count = m->CountItems();
			for( int32 k = 0; k < count; k++ ) {
				BMenuItem*	item = m->ItemAt( k );
				if (item) {
					if( pc && k == pc->ProgramNumber() ) item->SetMarked( true );
					else item->SetMarked(false);
				}
			}
		}
	}
	mCachedProgramNumber = mProgramChange->ProgramNumber();
	mCachedBankNumber = BankNumber();
}

void AmProgramChangeView::SetBankControl(AmBankChangeView* bv)
{
	mBankCtrl = bv;
}

void AmProgramChangeView::LockedSetProgramNumber(AmSong* song, uint8 number)
{
	ArpASSERT(song);
	AmProgramChange*	programChange = ProgramChange();
	if (!mContainer || !programChange) return;
	AmTrack*	track = song->Track(mTrackRef);
	if (!track) return;
	AmChangeEventUndo* undo = NULL;
	if (song->UndoContext() ) undo = new AmChangeEventUndo(track);

	if (undo) undo->EventChanging(mContainer->Phrase(), programChange);
	programChange->SetProgramNumber(number);
	if (undo) {
		undo->EventChanged(programChange);
		song->UndoContext()->AddOperation(undo, BResEditor::B_ANY_UNDO_MERGE);
		BString		undoName("Change ");
		undoName << undo->StringContents();
		song->UndoContext()->SetUndoName( undoName.String() );
		song->UndoContext()->CommitState();
	}
}

void AmProgramChangeView::BuildMenu(const AmTrack* track, BMenu* menu, int32 oldIndex) const
{
	ArpCRef<AmBankI>		bank = 0;
	AmFilterHolderI*		holder = track->Filter(DESTINATION_PIPELINE);
	uint32					count = 128;
	uint32					firstNumber = 0;
	if (holder && holder->Filter() ) {
		ArpCRef<AmDeviceI>	device = holder->Filter()->Device();
		if (device) {
			bank = device->Bank(BankNumber() );
			if (bank) {
				count = bank->CountPatches();
				firstNumber = bank->FirstPatchNumber();
			}
		}
	}
	for (uint32 k = 0; k < count; k++) {
		BMessage*			msg = new BMessage(PROGRAM_MSG);
		BString				label;
		BString				patchName;
		label << k + firstNumber;
		if (bank) patchName = bank->PatchName(k);
		if (patchName.Length() > 0) label << " - " << patchName.String();
		BMenuItem*			item = new BMenuItem(label.String(), msg);
		if (msg && item) {
			msg->AddInt32("number", (int32)k);
			menu->AddItem(item);
			if (int32(k) == oldIndex) item->SetMarked(true);
		}
	}
	menu->SetTargetForItems(this);
}

uint32 AmProgramChangeView::BankNumber() const
{
	if (mBankCtrl) return mBankCtrl->BankNumber();
	return 0;
}

AmProgramChange* AmProgramChangeView::ProgramChange() const
{
	if (mBankCtrl) return mBankCtrl->ProgramChange();
	return mProgramChange;
}

// #pragma mark -

/*************************************************************************
 * AM-TEMPO-TEXT-VIEW
 *************************************************************************/
AmTempoTextView::AmTempoTextView(	BRect frame,
									AmSongRef songRef,
									AmTrackRef trackRef)
		: inherited(frame, "AmTempoTextView", 0, 0,
					B_FOLLOW_TOP | B_FOLLOW_LEFT, B_WILL_DRAW),
		  mSongRef(songRef), mTrackRef(trackRef),
		  mContainer(NULL), mTempoChange(NULL)
{
	SetLimits(1, 400);
	SetSteps(1);
}

AmTempoTextView::~AmTempoTextView()
{
}

void AmTempoTextView::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	mContainer = NULL;
	mTempoChange = NULL;
}

void AmTempoTextView::SetEvent(AmPhraseEvent* container, AmTempoChange* tempoChange)
{
	mContainer = container;
	mTempoChange = tempoChange;
	if (mTempoChange) SetValue(mTempoChange->Tempo() );
	Draw( Bounds() );
}

void AmTempoTextView::NotifyHook(float newValue)
{
	if (!mContainer || !mTempoChange) return;
	// WRITE TRACK BLOCK
	AmSong*		song = mSongRef.WriteLock();
	AmTrack*	track = song ? song->Track(mTrackRef) : NULL;
	if (track) {
		if (!mUndo) PrepareUndoState(track, mContainer->Phrase(), mTempoChange);
		mTempoChange->SetTempo(newValue);
		if (mUndo) mUndo->EventChanged(mTempoChange);
	}
	mSongRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
}

void AmTempoTextView::CommitUndo()
{
	// WRITE SONG BLOCK
	AmSong*		song = mSongRef.WriteLock();
	if (song) LockedCommitUndo(song);
	mSongRef.WriteUnlock(song);
	// END WRITE SONG BLOCK
}

