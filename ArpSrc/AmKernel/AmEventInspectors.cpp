/* AmEventInspectors.cpp
 */
#include <stdio.h>
#include <be/interface/StringView.h>
#include <be/interface/Window.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmKernel/AmEventControls.h"
#include "AmKernel/AmEventInspectors.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmTrack.h"

/* This variable determines the width of the first column of all the
 * inspectors.  The first column is always the label used for the events
 * start time, but the label varies based on event.  This is used to
 * set the width to the widest label.
 */
static float		col1W		= 0;
static const char*	SZ_TIME		= "Time";
static const char*	SZ_START	= "Start";
static const char*	SZ_END		= "End";
static const char*	SZ_VALUE	= "Value";
static const char*	TEMPO_STR	= "Tempo";

/*************************************************************************
 * AM-EVENT-INSPECTOR
 *************************************************************************/
AmEventInspector::AmEventInspector(BRect frame,
								   const char *name)
		: inherited(frame,
					name,
					B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT,
					B_WILL_DRAW),
		mContainer(0), mEvent(0), mTimeView(0)
{
	if (col1W == 0) {
		float	timeW = StringWidth(SZ_TIME) + Prefs().Size(SPACE_X);
		float	startW = StringWidth(SZ_START) + Prefs().Size(SPACE_X);
		col1W = (timeW > startW) ? timeW : startW;
	}
}

AmEventInspector::~AmEventInspector()
{
	mTrackRef.RemoveObserverAll( this );
	if( mContainer ) mContainer->DecRefs();
	if( mEvent ) mEvent->DecRefs();
}

AmSongRef AmEventInspector::SongRef()
{
	return mSongRef;
}

void AmEventInspector::SetSongRef(AmSongRef songRef)
{
	mSongRef = songRef;
}

void AmEventInspector::SetTrackRef(AmTrackRef trackRef)
{
	mTrackRef = trackRef;
	if (mTimeView) mTimeView->SetTrackRef(trackRef);
}

void AmEventInspector::SetEvent(AmPhraseEvent* container, AmEvent* event)
{
	mTrackRef.RemoveObserverAll( this );
	if (mContainer) mContainer->DecRefs();
	mContainer = container;
	if (mContainer) mContainer->IncRefs();
	
	if (mEvent) mEvent->DecRefs();
	mEvent = event;
	if (mEvent) mEvent->IncRefs();

	if (mContainer && mEvent)
		mTrackRef.AddRangeObserver(	this,
									AmNotifier::CodeFor( mEvent->Type() ),
									mContainer->EventRange(mEvent) );
	Refresh();
}

void AmEventInspector::AddViews()
{
}

void AmEventInspector::AttachedToWindow()
{
	inherited::AttachedToWindow();
	if ( Parent() ) {
		SetViewColor( Parent()->ViewColor() );
		/* Resize this view to be the same width as my parent, so no one
		 * needs to keep track of whether or not I'm the right width.
		 */
		BRect	b = Bounds();
		float	height = b.Height();
		b = Parent()->Bounds();
		ResizeTo( b.Width(), height );
	}
	RangeChangeReceived();
}

void AmEventInspector::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mTrackRef.RemoveObserverAll( this );
}

void AmEventInspector::Draw(BRect clip)
{
	inherited::Draw(clip);
	BRect	b = Bounds();
	SetHighColor( 0, 0, 0 );
	StrokeLine( BPoint(clip.left, 1), BPoint(clip.right, 1) );
	StrokeLine( BPoint(clip.left, b.bottom), BPoint(clip.right, b.bottom) );
	SetHighColor( tint_color(ViewColor(), B_LIGHTEN_2_TINT) );
	StrokeLine( BPoint(clip.left, 2), BPoint(clip.right, 2) );
	SetHighColor( tint_color(ViewColor(), B_DARKEN_2_TINT) );
	StrokeLine( BPoint(clip.left, 0), BPoint(clip.right, 0) );
	StrokeLine( BPoint(clip.left, b.bottom - 1), BPoint(clip.right, b.bottom - 1) );
}

void AmEventInspector::MessageReceived(BMessage *msg)
{
	uint32		eventWhat = 0;

	if( mEvent ) eventWhat = AmNotifier::CodeFor( mEvent->Type() );
	if( msg->what == eventWhat ) {
		RangeChangeReceived();
		return;
	}

	switch (msg->what) {
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

void AmEventInspector::RangeChangeReceived()
{
	if( !mContainer || !mEvent ) return;
	if( mContainer->IsDeleted() || mEvent->IsDeleted() ) SetEvent( 0, 0 );
	else if( !mContainer->Phrase()->Includes( mEvent ) ) SetEvent( 0, 0 );
	else SetEvent( mContainer, mEvent );
	Refresh();
}

float AmEventInspector::AddTimeView(float left)
{
	if( !mTimeView ) {
		mTimeView = new AmEventTimeView(SongRef(),
										mTrackRef,
										BPoint(left, 7));
		AddChild( mTimeView );
	}
	if( mTimeView ) {
		if( mContainer && mEvent ) mTimeView->SetEvent( mContainer, mEvent );
		return mTimeView->Frame().right;
	}
	return left;
}


float AmEventInspector::AddLabel(const char *labelStr, float left)
{
	BStringView		*label;
	float			width = StringWidth(labelStr);
	
	BRect			b = Bounds();
	if ((label = new BStringView(BRect(left, b.top + 3,
			left + width, b.bottom - 7),
			0, labelStr)) != 0) {
		label->SetFontSize( 10 );
		AddChild(label);
		left = (left + width + Prefs().Size(SPACE_X) );
	}
	return left;
}

void AmEventInspector::Refresh(void)
{
	if( ( !mContainer || !mEvent ) && Parent() ) {
		Parent()->RemoveChild( this );
		return;
	}
	if( mTimeView ) mTimeView->SetEvent( mContainer, mEvent );
}

// #pragma mark -
 
/*************************************************************************
 * AM-BANK-CHANGE-INSPECTOR
 *************************************************************************/
AmBankChangeInspector::AmBankChangeInspector(BRect frame)
		: inherited(frame, "MidiBankChange"),
		  mBankChangeView(NULL), mProgramChangeView(NULL)
{
}

void AmBankChangeInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mBankChangeView) mBankChangeView->SetTrackRef(trackRef);
	if (mProgramChangeView) mProgramChangeView->SetTrackRef(trackRef);
}

void AmBankChangeInspector::AddViews()
{
	assert( mEvent );
	float	left;
	AddLabel(SZ_TIME, 0);
	left = AddTimeView(col1W);
	left = AddBankView(left + 10);
	AddProgramView(left + 10);
}

void AmBankChangeInspector::Refresh(void)
{
	inherited::Refresh();
	AmBankChange*		event = dynamic_cast<AmBankChange*>(mEvent);
	if (!mContainer || !event) return;

	if (mBankChangeView) mBankChangeView->SetEvent(mContainer, event);
	if (mProgramChangeView) mProgramChangeView->SetEvent(mContainer, event->ProgramChange() );
}

float AmBankChangeInspector::AddBankView(float left)
{
	left = AddLabel("Bank", left) + 5;
	BRect	b = Bounds();
	BRect	f(left, b.top + 3, left + 150, b.bottom - 2);
	mBankChangeView = new AmBankChangeView(f, mSongRef, mTrackRef);
	if (mBankChangeView) {
		float	w, h;
		mBankChangeView->GetPreferredSize(&w, &h);
		f.right = f.left + w;
		mBankChangeView->ResizeTo(f.Width(), f.Height() );
		AddChild(mBankChangeView);
	}
	return f.right;
}

float AmBankChangeInspector::AddProgramView(float left)
{
	left = AddLabel("Program", left) + 5;
	BRect	b = Bounds();
	float	right = left + 150;
	BRect	f(left, b.top + 3, right, b.bottom - 2);
	mProgramChangeView = new AmProgramChangeView(f, mSongRef, mTrackRef);
	if (mProgramChangeView){
		if (mBankChangeView) mProgramChangeView->SetBankControl(mBankChangeView);
		AddChild(mProgramChangeView);
	}
	return right;
}

// #pragma mark -
 
/*************************************************************************
 * AM-NOTE-ON-INSPECTOR
 *************************************************************************/
#define STR_END_TIME_VIEW	"EndTimeView"
#define STR_NOTE			"Note"
#define STR_VEL				"Vel."
#define STR_REL_VEL			"Rel. vel."

AmNoteOnInspector::AmNoteOnInspector(BRect frame)
		: inherited(frame, "MidiNoteOn"),
		  mEndTimeView(0), mNoteView(0), mVelocityView(0),
		  mReleaseVelocityView(0)
{
}

void AmNoteOnInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mEndTimeView) mEndTimeView->SetTrackRef(trackRef);
	if (mNoteView) mNoteView->SetTrackRef(trackRef);
	if (mVelocityView) mVelocityView->SetTrackRef(trackRef);
	if (mReleaseVelocityView) mReleaseVelocityView->SetTrackRef(trackRef);
}

void AmNoteOnInspector::AddViews()
{
	assert( mEvent );
	float left;

	AddLabel( SZ_START, 0 );
	left = AddTimeView( col1W );
	left = AddLabel( SZ_END, left + 10 );
	left = AddEndTimeView( left );
	left = AddNoteView( left + 10 );
	left = AddVelocityView( left + 10 );
	AddReleaseVelocityView( left + 10 );
}

void AmNoteOnInspector::Refresh(void)
{
	inherited::Refresh();
	AmNoteOn*		event = dynamic_cast<AmNoteOn*>(mEvent);
	if( !mContainer || !event ) return;
	
	if( mEndTimeView ) mEndTimeView->SetEvent( mContainer, event );
	if( mNoteView ) mNoteView->SetEvent( mContainer, event );
	if( mVelocityView ) mVelocityView->SetEvent( mContainer, event );
	if( mReleaseVelocityView ) mReleaseVelocityView->SetEvent( mContainer, event );
}

float AmNoteOnInspector::AddEndTimeView(float left)
{
	if( !mEndTimeView ) {
		mEndTimeView = new AmEventEndTimeView(	SongRef(),
												mTrackRef,
												BPoint(left, 7));
		AddChild( mEndTimeView );
		if( mTimeView ) mTimeView->StartWatching( mEndTimeView, ARPMSG_TIME_VIEW_CHANGED );
	}
	if( mEndTimeView ) {
		mEndTimeView->SetEvent( mContainer, mEvent );
		return mEndTimeView->Frame().right;
	}
	return left;
}

float AmNoteOnInspector::AddNoteView(float left) {
	float	boxWidth = StringWidth("A#10") + 8;
	if (mNoteView == 0) {
		left = AddLabel(STR_NOTE, left);

		font_height		fheight;
		GetFontHeight( &fheight );
		float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
		BRect			rect(left, top, left + boxWidth, top + height);
		if ((mNoteView = new AmNoteView(rect, mSongRef, mTrackRef)) == 0)
			return left;
		AddChild(mNoteView);
	} else {
		AmNoteOn*		event = dynamic_cast<AmNoteOn*>(mEvent);
		if( mContainer && event ) mNoteView->SetEvent( mContainer, event );
	}
	return left + boxWidth;
}

float AmNoteOnInspector::AddVelocityView(float left)
{
	char	z[8];
	sprintf(z, "%d", 999);
	float	boxWidth = StringWidth(z) + 8;
	if( !mVelocityView ) {
		left = AddLabel(STR_VEL, left);
	
		font_height		fheight;
		GetFontHeight( &fheight );
		float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
		BRect			rect(left, top, left + boxWidth, top + height);
		if ((mVelocityView = new AmVelocityView(rect, mSongRef, mTrackRef)) == 0)
			return left;
		AddChild(mVelocityView);
	} else {
		AmNoteOn*		event = dynamic_cast<AmNoteOn*>(mEvent);
		if( mContainer && event ) mVelocityView->SetEvent( mContainer, event );
	}
	return left + boxWidth;
}

float AmNoteOnInspector::AddReleaseVelocityView(float left)
{
	char	z[8];
	sprintf(z, "%d", 999);
	float	boxWidth = StringWidth(z) + 8;
	if( !mReleaseVelocityView ) {
		left = AddLabel(STR_REL_VEL, left);
	
		font_height		fheight;
		GetFontHeight( &fheight );
		float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
		BRect			rect(left, top, left + boxWidth, top + height);
		if ((mReleaseVelocityView = new AmReleaseVelocityView(rect, mSongRef, mTrackRef)) == 0)
			return left;
		AddChild(mReleaseVelocityView);
	} else {
		AmNoteOn*		event = dynamic_cast<AmNoteOn*>(mEvent);
		if( mContainer && event ) mReleaseVelocityView->SetEvent( mContainer, event );
	}
	return left + boxWidth;
}

// #pragma mark -
 
/*************************************************************************
 * AM-CC-INSPECTOR
 *************************************************************************/
AmCcInspector::AmCcInspector(BRect frame)
		: inherited(frame, "MidiCC"),
		mCcView(0)
{
}

void AmCcInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mCcView) mCcView->SetTrackRef(trackRef);
}

void AmCcInspector::AddViews()
{
	assert( mEvent );
	float left;
	AddLabel(SZ_TIME, 0);
	left = AddTimeView(col1W);
	AddControlChangeView(left + 10);
}

void AmCcInspector::Refresh(void)
{
	inherited::Refresh();
	AmControlChange*	event = dynamic_cast<AmControlChange*>( mEvent );
	if( !mContainer || !event ) return;
	
	if( mCcView ) mCcView->SetEvent( mContainer, event );
}

float AmCcInspector::AddControlChangeView(float left)
{
	left = AddLabel(SZ_VALUE, left);
	char	z[8];
	sprintf(z, "%d", 999);
	float	boxWidth = StringWidth(z) + 8;
	
	font_height		fheight;
	GetFontHeight( &fheight );
	float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
	BRect			rect(left, top, left + boxWidth, top + height);
	if ((mCcView = new AmCcTextView(rect, mSongRef, mTrackRef)) == 0)
		return left;
	
	AddChild(mCcView);
	return left + boxWidth;
}

// #pragma mark -
 
/*************************************************************************
 * AM-CHANNEL-PRESSURE-INSPECTOR
 *************************************************************************/
AmChannelPressureInspector::AmChannelPressureInspector(BRect frame)
		: inherited(frame, "MIDI Channel Pressure"),
		  mCpCtrl(0)
{
}

void AmChannelPressureInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mCpCtrl) mCpCtrl->SetTrackRef(trackRef);
}

void AmChannelPressureInspector::AddViews()
{
	assert( mEvent );
	float left;
	AddLabel(SZ_TIME, 0);
	left = AddTimeView(col1W);
	AddChannelPressureControl(left + 10);
}

void AmChannelPressureInspector::Refresh(void)
{
	inherited::Refresh();
	AmChannelPressure*	event = dynamic_cast<AmChannelPressure*>(mEvent);
	if (!mContainer || !event) return;
	
	if (mCpCtrl) mCpCtrl->SetEvent( mContainer, event );
}

float AmChannelPressureInspector::AddChannelPressureControl(float left)
{
	left = AddLabel(SZ_VALUE, left);
	char	z[8];
	sprintf(z, "%d", 999);
	float	boxWidth = StringWidth(z) + 8;
	
	font_height		fheight;
	GetFontHeight( &fheight );
	float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
	BRect			rect(left, top, left + boxWidth, top + height);
	if ((mCpCtrl = new AmChannelPressureControl(rect, mSongRef, mTrackRef)) == 0)
		return left;
	
	AddChild(mCpCtrl);
	return left + boxWidth;
}

// #pragma mark -
 
/*************************************************************************
 * AM-PITCH-BEND-INSPECTOR
 *************************************************************************/
AmPitchBendInspector::AmPitchBendInspector(BRect frame)
		: inherited(frame, "AmPitchBendInspector"),
		mPitchView(0)
{
}

void AmPitchBendInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mPitchView) mPitchView->SetTrackRef(trackRef);
}

void AmPitchBendInspector::AddViews()
{
	assert( mEvent );
	float	left;
	AddLabel(SZ_TIME, 0);
	left = AddTimeView(col1W);
	AddPitchBendView(left + 10);
}

void AmPitchBendInspector::Refresh(void)
{
	inherited::Refresh();
	if( !mContainer || !mEvent ) return;
	if( mPitchView ) mPitchView->SetEvent( mContainer, dynamic_cast<AmPitchBend*>(mEvent) );
}

float AmPitchBendInspector::AddPitchBendView(float left)
{
	left = AddLabel("Pitch bend", left);
	char	z[8];
	sprintf(z, "%d", -9999);
	float	boxWidth = StringWidth(z) + 8;
	
	font_height		fheight;
	GetFontHeight( &fheight );
	float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
	BRect			rect(left, top, left + boxWidth, top + height);
	if ((mPitchView = new AmPitchBendView( rect, mSongRef, mTrackRef )) == 0)
		return left;
	
	AddChild(mPitchView);
	return left + boxWidth;
}

// #pragma mark -
 
/*************************************************************************
 * AM-PROGRAM-CHANGE-INSPECTOR
 *************************************************************************/
AmProgramChangeInspector::AmProgramChangeInspector(BRect frame)
		: inherited(frame, "MidiProgamChange"),
		  mProgramChangeView(0)
{
}

void AmProgramChangeInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mProgramChangeView) mProgramChangeView->SetTrackRef(trackRef);
}

void AmProgramChangeInspector::AddViews()
{
	assert( mEvent );
	float	left;
	AddLabel(SZ_TIME, 0);
	left = AddTimeView(col1W);
	AddProgramView( left + 10 );
}

void AmProgramChangeInspector::Refresh(void)
{
	inherited::Refresh();
	AmProgramChange*		event = dynamic_cast<AmProgramChange*>(mEvent);
	if( !mContainer || !event ) return;
	
	if( mProgramChangeView ) mProgramChangeView->SetEvent( mContainer, event );
}

float AmProgramChangeInspector::AddProgramView(float left)
{
	left = AddLabel("Program", left) + 5;
	BRect	b = Bounds();
	float	right = left + 150;
	BRect	f(left, b.top + 3, right, b.bottom - 2);
	mProgramChangeView = new AmProgramChangeView( f, mSongRef, mTrackRef );
	if( mProgramChangeView ) AddChild( mProgramChangeView );
	return right;
}

// #pragma mark -
 
/*************************************************************************
 * AM-TEMPO-CHANGE-INSPECTOR
 *************************************************************************/
AmTempoChangeInspector::AmTempoChangeInspector(BRect frame)
		: inherited(frame, "MidiTempoChange"),
		  mTempoView(0)
{
}

void AmTempoChangeInspector::SetTrackRef(AmTrackRef trackRef)
{
	inherited::SetTrackRef(trackRef);
	if (mTempoView) mTempoView->SetTrackRef(trackRef);
}

void AmTempoChangeInspector::AddViews()
{
	ArpASSERT(mEvent);
	float left;
	AddLabel(SZ_TIME, 0);
	left = AddTimeView(col1W);
	AddTempoChangeView(left + 10);
}

void AmTempoChangeInspector::Refresh(void)
{
	inherited::Refresh();
	AmTempoChange*	event = dynamic_cast<AmTempoChange*>( mEvent );
	if (!mContainer || !event) return;
	
	if (mTempoView) mTempoView->SetEvent( mContainer, event );
}

float AmTempoChangeInspector::AddTempoChangeView(float left)
{
	left = AddLabel(TEMPO_STR, left);
	char	z[32];
	sprintf(z, "%f", 999.9999);
	float	boxWidth = StringWidth(z) + 8;
	
	font_height		fheight;
	GetFontHeight( &fheight );
	float			top = 7, height = fheight.ascent + fheight.descent + fheight.leading + 1;
	BRect			rect(left, top, left + boxWidth, top + height);
	if ((mTempoView = new AmTempoTextView(rect, mSongRef, mTrackRef)) == 0)
		return left;
	
	AddChild(mTempoView);
	return left + boxWidth;
}

// #pragma mark -
 
/********************************************************
 * AM-INSPECTOR-FACTORY
 ********************************************************/
AmInspectorFactory::AmInspectorFactory(	AmSongRef songRef,
										AmTrackRef trackRef,
										BView* backgroundView,
										BRect bounds)
		: mSongRef(songRef), mTrackRef(trackRef), mBackgroundView(backgroundView),
		mBounds(bounds)
{
	assert( mBackgroundView != 0 );
}

AmInspectorFactory::~AmInspectorFactory()
{
	for (uint32 k=0; k<mViews.size(); k++) {
		if (mViews[k]->Parent() == 0) delete mViews[k];
	}
}

bool AmInspectorFactory::SetTrackRef(AmTrackRef trackRef, AmSelectionsI* selections = NULL)
{
	mTrackRef = trackRef;
	for (uint32 k = 0; k < mViews.size(); k++) {
		AmEventInspector*	inspector = dynamic_cast<AmEventInspector*>(mViews[k]);
		if (inspector) inspector->SetTrackRef(trackRef);
	}
	return InstallViewFor(selections);
}

bool AmInspectorFactory::InstallViewFor(AmSelectionsI* selections)
{
	if (!selections) return EmptyView();
	BView*		view = GetView(selections);
	if (!view) return false;
	
	// If the selected view is currently installed, don't need to
	// do anything else.
	if (view->Parent() ) return true;
	// Otherwise uninstall the current view and install the new one.
	for (uint32 k=0; k<mViews.size(); k++) {
		if (mViews[k]->Parent() != NULL) mBackgroundView->RemoveChild(mViews[k]);
	}

	mBackgroundView->AddChild(view);
	return true;
}

BView* AmInspectorFactory::GetView(AmSelectionsI* selections)
{
	ArpASSERT(selections);
	uint32	count = selections->CountEvents();
	if (count < 1) {
		EmptyView();
		return NULL;
	}
	if (count == 1) {
		AmPhraseEvent*	topPhrase = NULL;
		AmEvent*		e = NULL;
		track_id		tid;
		if (selections->EventAt(0, 0, &tid, &topPhrase, &e) == B_OK ) {
			if (tid != mTrackRef.TrackId() ) {
				EmptyView();
				return NULL;
			}
			return GetViewForEvent(topPhrase, e);
		}			
		return NULL;
	}
	return GetViewForEvents(selections);
}

BView* AmInspectorFactory::GetViewForEvent(AmPhraseEvent* container, AmEvent* event)
{
	uint32	index = 0;
	for (uint32 k=0; k<mTypes.size(); k++) {
		if (mTypes[k] == event->Type() ) break;
		index++;
	}

	if ( index < mViews.size() ) {
		// Populate the newly created view.
		AmEventInspector*	ei = dynamic_cast<AmEventInspector*>( mViews[index] );
		if( !ei ) return 0;
		ei->SetEvent( container, event );
		return mViews[index];
	}
	
	BView*	newView = event->NewView(event->INSPECTOR_VIEW, mBounds);
	// Populate the newly created view.
	AmEventInspector*	ei = dynamic_cast<AmEventInspector*>( newView );
	if( !ei ) {
		delete newView;
		return 0;
	}
	ei->SetSongRef( mSongRef );
	ei->SetTrackRef( mTrackRef );
	ei->SetEvent( container, event );
	ei->AddViews();
	// Add the new view to my impromptu dictionary.
	mTypes.push_back( event->Type() );
	mViews.push_back( newView );
	return newView;
}

BView* AmInspectorFactory::GetViewForEvents(AmSelectionsI* selections)
{
	EmptyView();
	return NULL;
}

bool AmInspectorFactory::EmptyView()
{
	for (uint32 k=0; k<mViews.size(); k++) {
		if (mViews[k]->Parent() != 0) mBackgroundView->RemoveChild( mViews[k] );
	}
	return true;
}
