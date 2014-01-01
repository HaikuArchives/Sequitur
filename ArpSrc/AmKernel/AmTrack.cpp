/* AmTrack.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmGlobalsI.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmInputQueue.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmSong.h"

static const char*	MODE_FLAGS_STR		= "mode_flags";
//static const char*	THROUGH_MODE_STR	= "through_mode";
static const char*	RECORD_MODE_STR		= "record_mode";
static const char*	PHRASE_HEIGHT_STR	= "phrase_height";
static const char*	ARRANGE_VIEW_STR	= "arrange_view";
static const char*	PRI_VIEW_STR		= "pri_view";
static const char*	SEC_VIEW_STR		= "sec_view";
static const char*	WINDOW_STATE		= "window_state";
static const char*	GROUPS_STR			= "groups";

static const int32	INPUT_INDEX			= 0;
static const int32	OUTPUT_INDEX		= 1;

static int32 index_for_pipeline(AmPipelineType type, uint32* flags = NULL);

class AmTrackStateUndo : public BUndoOperation
{
public:
	AmTrackStateUndo(AmSong* song, AmTrack* track)
		: mSong(song), mTrack(track),
		  mHasTitle(false)
	{
		mTrack->AddRef();
	}
	
	virtual ~AmTrackStateUndo()
	{
		mTrack->RemoveRef();
	}
	
	virtual const void* Owner() const
	{
		return mTrack;
	}
	
	virtual bool HasData() const
	{
		return mHasTitle;
	}
	
	virtual bool MatchOwnerContext(const void* owner) const
	{
		return owner == mTrack;
	}
	
	virtual void Commit()
	{
	}
	
	virtual void Undo()
	{
		swap_data();
	}
	
	virtual void Redo()
	{
		swap_data();
	}

	void NoteSetTitle(const char* title)
	{
		if (!title) mTitle.Truncate(0, false);
		else mTitle = title;
		mHasTitle = true;
	}
	
private:
	void swap_data()
	{
		if (mHasTitle) {
			BString		titleSwap = mTrack->Title();
			mTrack->SetTitle(mTitle.String(), 0, 0);
			mTitle = titleSwap;
		}
	}
	
	AmSong* const	mSong;
	AmTrack* const	mTrack;
	
	bool			mHasTitle;
	BString			mTitle;					// if the title changes
};


/* FIX: This is an empty MIDI list that gets returned from the
 * Signatures() method if the track doesn't currently have a song.
 * I THINK that's an error condition, and maybe is preventable.  If
 * so, this should definitely go away.
 */
static const AmPhrase	_fake_sigs;

/***************************************************************************
 * AM-TRACK
 ****************************************************************************/
static AmPipelineType pipeline_for_index(int32 index)
{
	if (index == INPUT_INDEX) return INPUT_PIPELINE;
	if (index == OUTPUT_INDEX) return OUTPUT_PIPELINE;
	return NULLINPUTOUTPUT_PIPELINE;
}


AmTrack::AmTrack(AmSong* song, const char* title)
		: mRefCount(0), mSong(song),
		mSignatures(0), mGroups(0), mPhraseHeight(AM_DEFAULT_TRACK_HEIGHT),
		mModeFlags(0), mThroughMode(true), mRecordMode(RECORD_OFF_MODE), mFlags(0),
		mInputTarget(new AmInputTarget((track_id)this, song->InputQueue())),
		mRecording(false), mRecordingPhrase(NULL)
{
	mSong->AddRef();
	SetName( title ? title : SZ_UNTITLED_TRACK );
	mPhrases.SetParent(this);
	
	// Default signature and views for the arrange view.
	mArrangeView.SetSignature(DEFAULT_FACTORY_SIGNATURE);
	mArrangeView.SetName("Default Note");

	// For now, some default secondary views.
	AmViewProperty*	v_prop;
	if ( (v_prop = new AmViewProperty( DEFAULT_FACTORY_SIGNATURE, "Program Change")) != 0 )
		mSecViews.push_back( v_prop );
	if ( (v_prop = new AmViewProperty( DEFAULT_FACTORY_SIGNATURE, "Control Change")) != 0 ) {
		BMessage		cc07;
		cc07.AddInt32("ControlChange", 7);
		v_prop->SetConfiguration(&cc07);
		mSecViews.push_back( v_prop );
	}
	if ( (v_prop = new AmViewProperty( DEFAULT_FACTORY_SIGNATURE, "Control Change")) != 0 )
		mSecViews.push_back( v_prop );
	
	for (int32 i = 0; i < PIPELINE_SIZE; i++) {
		mPipelines[i].Setup(mSong, this, pipeline_for_index(i) );
	}
	
	// Now that we are all up and running, allow events to start
	// flowing in.
	mInputTarget->SetPerforming(mThroughMode);
}

AmTrack::~AmTrack()
{
#ifdef AM_LOGGING
	BString		str("\nDelete track ");
	str << Name() << "\n";
	AM_BLOG(str);
#endif
	if (mInputTarget) {
		mInputTarget->SetRecording(false);
		mInputTarget->SetPerforming(false);
	}
	
	DetachRecordingPhrase();
	
	mPhrases.DeleteEvents();
	mMotions.DeleteEvents();
	mRecordingPhrase = NULL;
	
	if (mSignatures) {
		mSignatures->DeleteEvents();
		delete mSignatures;
	}
	
	for (uint32 k=0; k<mSecViews.size(); k++) {
		delete mSecViews[k];
	}
	
	if (mSong) mSong->RemoveRef();
}

void AmTrack::AddRef() const
{
	AmTrack* me = const_cast<AmTrack*>(this);
	atomic_add(&me->mRefCount, 1);
}

void AmTrack::RemoveRef() const
{
	AmTrack* me = const_cast<AmTrack*>(this);
	if (atomic_add(&me->mRefCount, -1) == 1) {
		printf("AmTrack::RemoveRef() delete track %s\n", Name() );
		me->Delete();
	}
}

track_id AmTrack::Id() const
{
	return (void*)this;
}

const char* AmTrack::Title() const
{
	return Name();
}

void AmTrack::SetTitle(const char* title, void* sender)
{
	SetTitle( title, sender, mSong->UndoContext() );
}

const AmSong* AmTrack::Song() const
{
	return mSong;
}

AmSong* AmTrack::Song()
{
	return mSong;
}

int32 AmTrack::SongIndex() const
{
	if (!mSong) return -1;
	else return mSong->TrackIndex(Id() );
}

ArpCRef<AmDeviceI> AmTrack::Device() const
{
	AmFilterHolderI*	h = Filter(OUTPUT_PIPELINE);
	while (h) {
		if ( h->Filter() && h->Filter()->Device() )
			return h->Filter()->Device();
		h = h->NextInLine();
	}
	return NULL;
}

uint32 AmTrack::Groups() const
{
	return mGroups;
}

void AmTrack::SetGroups(uint32 groups)
{
	if (mGroups == groups) return;

	if (mSong) mSong->SetDirty();
	mGroups = groups;
	ReportChange(GROUP_CHANGE_OBS, BMessenger() );
	if (mSong) {
		BMessage	msg(GROUP_CHANGE_OBS);
		msg.AddPointer(SZ_TRACK_ID, Id() );
		msg.AddInt32("groups", mGroups);
		mSong->ReportMsgChange(&msg, BMessenger() );
	}
}

AmTime AmTrack::LookaheadTime() const
{
	AmTime				t = 0;
	AmFilterHolderI*	h = Filter(OUTPUT_PIPELINE);
	while (h) {
		if (h->Filter()) {
			AmTime		t2 = h->Filter()->LookaheadTime();
			if (t2 > t) t = t2;
		}
		h = h->NextInLine();
	}
	return t;
}

const AmPhrase& AmTrack::Phrases() const
{
	return mPhrases;
}

const AmPhrase& AmTrack::Signatures() const
{
	if (mSignatures) return *mSignatures;
	if (!mSong) return (AmPhrase&)_fake_sigs;
	return mSong->Signatures();
}

static bool valid_beat_value(uint32 bv)
{
	return bv == 1 || bv == 2 || bv == 4 || bv == 8 || bv == 16 || bv == 32;
}

/* Perform a swap -- remove all the signatures from oldSignatures and
 * place them into newSignatures, while simultaneously giving them the
 * currect time values.  This is done because it is theoretically possible
 * for signatures to get our of order while they are being reassigned.
 * If they were to get out of order and you were operating on the same
 * list, the result would be corrupted data.
 */
static void straighten_out_signatures(AmPhrase& oldSignatures)
{
	AmPhrase		newSignatures;
	AmNode*			n = oldSignatures.HeadNode();
	AmNode*			nextN = NULL;
	AmSignature		currentSig;
	AmSignature*	prevSig = NULL;
	currentSig.Set(0, 1, 4, 4);
	AmTime			sigLength = currentSig.Duration();
	vector<AmSignature*>	removedSigs;
	while (n) {
		nextN = n->next;
		AmSignature*	sig = dynamic_cast<AmSignature*>( n->Event() );
		if (sig) {
			oldSignatures.Remove(sig);
			ArpASSERT( currentSig.Measure() <= sig->Measure() );
			while ( currentSig.Measure() < sig->Measure() ) {
				currentSig.Set( currentSig.StartTime() + sigLength,
								currentSig.Measure() + 1,
								currentSig.Beats(),
								currentSig.BeatValue() );
			}
			sig->Set( currentSig.StartTime(),
						sig->Measure(),
						sig->Beats(),
						sig->BeatValue() );
			currentSig.Set(*sig);
			sigLength = currentSig.Duration();
			/* Only add this sig into the list if it's different from
			 * the previous one.
			 */
			if ( !prevSig
					|| (prevSig->Beats() != sig->Beats())
					|| (prevSig->BeatValue() != sig->BeatValue()) ) {
				newSignatures.Add(sig);
				prevSig = sig;
			} else {
				removedSigs.push_back(sig);
			}
		}
		n = nextN;
	}
	n = newSignatures.HeadNode();
	while (n) {
		nextN = n->next;
		AmEvent*	event = n->Event();
		newSignatures.Remove(event);
		oldSignatures.Add(event);
		n = nextN;
	}
}

status_t AmTrack::SetSignature(int32 measure, uint32 beats, uint32 beatValue)
{
	if (!valid_beat_value(beatValue)) return B_ERROR;
	if (!mSignatures) {
		status_t	err = NewSignatures();
		if (err != B_OK) return err;
	}
	/* First see if there's an existing measure I can make use of
	 */
	AmNode*			n = mSignatures->HeadNode();
	AmTime			endTime = 0;
	while (n) {
		AmSignature*	sig = dynamic_cast<AmSignature*>( n->Event() );
		if (sig) {
			endTime = sig->EndTime() + 1;
			if (measure < sig->Measure()) {
				/* The time doesn't matter, since all the times for
				 * the measures will get straightened out later on.  All
				 * that matters is that the events be in the proper order.
				 */
				AmSignature*	newSig = new AmSignature();
				if (!newSig) return B_NO_MEMORY;
				newSig->Set(sig->StartTime() - 1, measure, beats, beatValue);
				mSignatures->Add(newSig);
				break;
			} else if (sig->Measure() == measure) {
				/* I need to remove this because setting the beats and
				 * beat values affects the sig's end time -- if I don't
				 * remove this, it might leave span events around.
				 */
				mSignatures->Remove(sig);
				sig->Set(sig->StartTime(), beats, beatValue);
				mSignatures->Add(sig);
				break;
			}
		}
		if (!(n->next)) {
			AmSignature*	newSig = new AmSignature();
			if (!newSig) return B_NO_MEMORY;
			newSig->Set(endTime, measure, beats, beatValue);
			mSignatures->Add(newSig);
			break;
		}
		n = n->next;
	}
	/* Now I know I have a signature and it's been added.  Run through
	 * and straighten out all the start and end times for the signatures.
	 */
//printf("BEFORE STRAIGHTENING TRACK: "); mSignatures->Print();
	straighten_out_signatures(*mSignatures);
//printf("AFTER STRAIGHTENING TRACK: "); mSignatures->Print();
	/* Now report on a change starting with the time of the measure
	 * supplied to this method.
	 */
	n = mSignatures->HeadNode();
	while (n) {
		AmSignature*	sig = dynamic_cast<AmSignature*>( n->Event() );
		if (sig) {
			if (sig->Measure() == measure) {
				AmRange		range( sig->StartTime(), sig->EndTime() );
				MergeRangeChange(range, range, sig);
				return B_OK;
			} else if (sig->Measure() > measure) {
				AmNode*	prev = n->prev;
				if (prev) {
					AmRange		range( prev->StartTime(), prev->EndTime() );
					MergeRangeChange(range, range, prev->Event() );
					return B_OK;
				}
			}
		}
		n = n->next;
	}
	AmSignature		fakeSig(0);
	AmRange			rng(0, 0);
	MergeRangeChange(rng, rng, &fakeSig);
	return B_OK;
}

status_t AmTrack::GetSignature(AmTime time, AmSignature& signature) const
{
	if (!mSignatures) {
		if (!mSong) return B_ERROR;
		return mSong->GetSignature(time, signature);
	}

	ArpASSERT(time >= 0);
	if (time < 0) return B_ERROR;
	AmNode*			node = mSignatures->HeadNode();
	ArpASSERT(node);
	if (!node) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	ArpASSERT(sig && sig->StartTime() == 0);
	if (!sig) return B_ERROR;
	if (sig->StartTime() != 0) return B_ERROR;
	AmSignature*	nextSig = NULL;
	AmNode*			nextNode = node->next;
	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	AmSignature		currentSig(*sig);
	AmTime			sigLength = currentSig.Duration();

	while (currentSig.EndTime() < time) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.Measure() == nextSig->Measure() ) ) {
			currentSig.Set(*nextSig);
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	/* Technically this check isn't necessary but I'm a safe guy.
	 */
	if ( time >= currentSig.StartTime() && time <= currentSig.EndTime() ) {
		signature.Set(currentSig);
		return B_OK;
	}
	debugger("Failed to find measure");
	return B_ERROR;
}

status_t AmTrack::GetSignatureForMeasure(int32 measure, AmSignature& signature) const
{
	if (!mSignatures) {
		if (!mSong) return B_ERROR;
		return mSong->GetSignatureForMeasure(measure, signature);
	}

	ArpASSERT(measure >= 1);
	if (measure < 1) return B_ERROR;
	AmNode*			node = mSignatures->HeadNode();
	ArpASSERT(node);
	if (!node) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	ArpASSERT(sig && sig->StartTime() == 0);
	if (!sig) return B_ERROR;
	if (sig->StartTime() != 0) return B_ERROR;
	AmSignature*	nextSig = NULL;
	AmNode*			nextNode = node->next;
	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
	AmSignature		currentSig(*sig);
	AmTime			sigLength = currentSig.Duration();

	if (currentSig.Measure() > measure) return B_ERROR;
	if (currentSig.Measure() == measure) {
		signature.Set(currentSig);
		return B_OK;
	}

	while (currentSig.Measure() < measure) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.Measure() == nextSig->Measure() ) ) {
			currentSig.Set(*nextSig);
			sigLength = currentSig.Duration();
			node = nextNode;
			nextNode = nextNode->next;
			if (nextNode != 0) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = 0;
		}
	}
	ArpASSERT(currentSig.Measure() == measure);
	signature.Set(currentSig);
	return B_OK;
}

void AmTrack::LockSignaturesToSong()
{
	if (mSignatures) {
		delete mSignatures;
		mSignatures = NULL;
		AmSignature		fakeSig(0);
		AmRange			range(0, 0);
		MergeRangeChange(range, range, &fakeSig);
	}
}

const AmPhrase& AmTrack::Motions() const
{
	return mMotions;
}

status_t AmTrack::SetMotion(int32 measure, const AmMotionI* motion)
{
	AmSignature		sig;
	status_t		err = GetSignatureForMeasure(measure, sig);
	if (err != B_OK) return err;

	/* First see if the measure already has a motion - if
	 * so, I be installin' a NEW ONE YEAH!
	 */
	AmNode*			n = mMotions.HeadNode();
	while (n) {
		AmMotionChange*		mc = dynamic_cast<AmMotionChange*>(n->Event() );
		if (mc && measure == mc->Measure() ) {
			mc->SetMotion(motion);
			AmRange		range(sig.StartTime(), sig.EndTime() );
			MergeRangeChange(range, range, &sig);
			return B_OK;
		}
		n = n->next;
	}
	/* Measure didn't exist, go ahead and add a new motion.
	 */
	AmMotionChange*	mc = new AmMotionChange(motion, measure, sig.StartTime() );
	if (!mc) return B_NO_MEMORY;
	err = mMotions.Add(mc);

	AmRange		range( sig.StartTime(), sig.EndTime() );
	MergeRangeChange(range, range, &sig);

	return err;
}

status_t AmTrack::ClearMotion(int32 measure)
{
	AmSignature		sig;
	status_t		err = GetSignatureForMeasure(measure, sig);
	if (err != B_OK) return err;

	AmNode*			n = mMotions.HeadNode();
	while (n) {
		AmMotionChange*		mc = dynamic_cast<AmMotionChange*>(n->Event() );
		if (mc && measure == mc->Measure() ) {
			if (mMotions.Remove(mc) == B_OK) {
				mc->Delete();
				AmRange			range(sig.StartTime(), sig.EndTime() );
				MergeRangeChange(range, range, &sig);
				return B_OK;
			}
		}
	}
	return B_ERROR;
}

AmTime AmTrack::EndTime2() const
{
	return mPhrases.EndTime();
}

uint32 AmTrack::ModeFlags() const
{
	return mModeFlags;
}

void AmTrack::SetModeFlags(uint32 flags)
{
	if (mModeFlags == flags) return;
	
	if (mSong) mSong->SetDirty();
	mModeFlags = flags;
	ReportChange(MODE_CHANGE_OBS, BMessenger() );
	if (mSong) {
		BMessage	msg(MODE_CHANGE_OBS);
		msg.AddPointer(SZ_TRACK_ID, Id() );
		msg.AddInt32("mode flags", mModeFlags);
		mSong->ReportMsgChange(&msg, BMessenger() );
	}
}

uint32 AmTrack::RecordMode() const
{
	return mRecordMode;
}

void AmTrack::SetRecordMode(uint32 mode)
{
	if (mRecordMode == mode) return;
	
	mRecordMode = mode;
	mInputTarget->SetRecording(mode != RECORD_OFF_MODE ? true : false);
	ReportChange( MODE_CHANGE_OBS, BMessenger() );
	
	if (mode == RECORD_OFF_MODE) {
		DetachRecordingPhrase();
	} else if (mRecording) {
		AttachRecordingPhrase();
	}
}

status_t AmTrack::GetSettings(BMessage* settings) const
{
	if (!settings) return B_ERROR;
	status_t	err;
	if ( (err = settings->AddInt32(MODE_FLAGS_STR, mModeFlags)) != B_OK) return err;
//	if ( (err = settings->AddInt32(THROUGH_MODE_STR, mThroughMode ? 1:0)) != B_OK) return err;
	if ( (err = settings->AddInt32(RECORD_MODE_STR, mRecordMode)) != B_OK) return err;
	if ( (err = settings->AddFloat(PHRASE_HEIGHT_STR, mPhraseHeight)) != B_OK) return err;
	if ( (err = settings->AddInt32(GROUPS_STR, int32(mGroups))) != B_OK) return err;

	BMessage	msg;
	if (mArrangeView.WriteTo(&msg) == B_OK) settings->AddMessage(ARRANGE_VIEW_STR, &msg);
	msg.MakeEmpty();
	if (mPriView.WriteTo(&msg) == B_OK) settings->AddMessage(PRI_VIEW_STR, &msg);
	msg.MakeEmpty();
	for (uint32 k = 0; k < mSecViews.size(); k++) {
		if( mSecViews[k]->WriteTo(&msg) == B_OK ) settings->AddMessage(SEC_VIEW_STR, &msg);
		msg.MakeEmpty();
	}

	if (!mConfiguration.IsEmpty() ) settings->AddMessage(WINDOW_STATE, &mConfiguration);

	AmNode*			n = mMotions.HeadNode();
	while (n) {
		AmMotionChange*		mc = dynamic_cast<AmMotionChange*>(n->Event() );
		if (mc) {
			BMessage	motionMsg;
			if (mc->GetAsMessage(motionMsg) == B_OK) msg.AddMessage("motion", &motionMsg);
		}
		n = n->next;
	}
	settings->AddMessage("motions", &msg);
	msg.MakeEmpty();

	return B_OK;
}

status_t AmTrack::PutSettings(const BMessage* settings)
{
	if (!settings) return B_ERROR;
	int32		i;
	float		f;
	BMessage	msg;
	if (settings->FindInt32(MODE_FLAGS_STR, &i) == B_OK) SetModeFlags(i);
//	if (settings->FindInt32(THROUGH_MODE_STR, &i) == B_OK) SetThroughMode(i ? true : false);
	if (settings->FindInt32(RECORD_MODE_STR, &i) == B_OK) SetRecordMode(i);
	if (settings->FindFloat(PHRASE_HEIGHT_STR, &f) == B_OK) mPhraseHeight = f;
	if (settings->FindInt32(GROUPS_STR, &i) == B_OK) mGroups = uint32(i);

	if (settings->FindMessage(ARRANGE_VIEW_STR, &msg) == B_OK) mArrangeView.ReadFrom(&msg);
	msg.MakeEmpty();
	if( settings->FindMessage(PRI_VIEW_STR, &msg) == B_OK ) mPriView.ReadFrom(&msg);
	msg.MakeEmpty();
	mSecViews.resize(0);
	for (int32 k = 0; settings->FindMessage(SEC_VIEW_STR, k, &msg) == B_OK; k++) {
		AmViewProperty*	v_prop = new AmViewProperty(&msg);
		if (v_prop) mSecViews.push_back(v_prop);
		msg.MakeEmpty();
	}

	if (settings->FindMessage(WINDOW_STATE, &msg) == B_OK) mConfiguration = msg;
	msg.MakeEmpty();

	mMotions.DeleteEvents();
	if (settings->FindMessage("motions", &msg) == B_OK) {
		BMessage			motionMsg;
		for (int32 k = 0; msg.FindMessage("motion", k, &motionMsg) == B_OK; k++) {
			AmMotionChange*	mc = new AmMotionChange(motionMsg);
			if (mc) mMotions.Add(mc);
			motionMsg.MakeEmpty();
		}
	}
	msg.MakeEmpty();
	
	return B_OK;
}

	/*---------------------------------------------------------
	 * EDITING
	 *---------------------------------------------------------*/

status_t AmTrack::AddEvent(AmPhrase* phrase, AmEvent* event, const char* undoName)
{
	ArpASSERT(event);
	return AddEvent(phrase, event, mSong->UndoContext(), undoName);
}

status_t AmTrack::RemoveEvent(AmPhrase* phrase, AmEvent* event)
{
	ArpASSERT(event);
	return RemoveEvent(phrase, event, mSong->UndoContext() );
}

AmEventParent* AmTrack::Parent() const
{
	return NULL;
}

void AmTrack::SetParent(AmEventParent* parent)
{
}

void AmTrack::TimeRangeDirty()
{
}

void AmTrack::Invalidate(	AmEvent* changedEvent,
							AmRange oldRange, AmRange newRange)
{
	if (mSong) mSong->SetDirty();
	mFlags |= HAS_NOTIFICATION;
	MergeRangeChange(oldRange, newRange, changedEvent);
}

void AmTrack::ChangeEvent(	AmEvent* child,
							AmTime oldStart, AmTime newStart,
							AmTime oldEnd, AmTime newEnd)
{
	if (mSong) mSong->SetDirty();
	mFlags |= HAS_NOTIFICATION;
	MergeRangeChange(AmRange(oldStart, oldEnd), AmRange(newStart, newEnd), child);
}

status_t AmTrack::AddEvent(	AmPhrase* phrase, AmEvent* event,
							BUndoContext* undo, const char* undoName)
{
	ArpASSERT(event);
	if (!phrase) phrase = PhraseFor(event);
	if (!phrase) return B_ERROR;

	if (mSong) mSong->SetDirty();

	if (undo) {
		if (undoName) undo->SuggestUndoName(undoName);
		undo->AddOperation(new AmTrackEventUndo(this, true, phrase, event), BResEditor::B_ANY_UNDO_MERGE);
	}
	AmRange		oldRange = phrase->TimeRange();
	status_t	err = phrase->Add(event);
	if (err == B_OK) {
		event->AddedToPhrase();
		mFlags |= HAS_NOTIFICATION;
		/* If a whole phrase is being added, just add the entire thing
		 * to the change area.
		 */
		AmRange		range = event->TimeRange();
		if (event->Type() == event->PHRASE_TYPE) MergeRangeChange(range, range);
		else {
			AmRange		newRange = phrase->TimeRange();
			if (oldRange == newRange) MergeRangeChange(range, range, event);
			else {
				AmRange		r2( min(oldRange.start, newRange.start), max(oldRange.end, newRange.end) );
				MergeRangeChange(range, r2, event);
			}
		}
	}
	return err;
}

status_t AmTrack::RemoveEvent(	AmPhrase* phrase, AmEvent* event,
								BUndoContext* undo, const char* undoName)
{
	ArpASSERT(event);
	if (!phrase) phrase = PhraseFor(event);
	if (!phrase) return B_ERROR;
	if (mSong) mSong->SetDirty();

	if (undo) {
		if (undoName) undo->SuggestUndoName(undoName);
		undo->AddOperation(new AmTrackEventUndo(this, false, phrase, event), BResEditor::B_ANY_UNDO_MERGE);
	}
	AmRange		oldRange = phrase->TimeRange();
	event->RemovedFromPhrase();
	status_t	err = phrase->Remove(event);
	if (err == B_OK) {
		mFlags |= HAS_NOTIFICATION;
		/* If a whole phrase is being added, just add the entire thing
		 * to the change area.
		 */
		AmRange		range = event->TimeRange();
		if (event->Type() == event->PHRASE_TYPE) MergeRangeChange(range, range);
		else {
			AmRange		newRange = phrase->TimeRange();
			if (oldRange == newRange) MergeRangeChange(range, range, event);
			else {
				AmRange		r2( min(oldRange.start, newRange.start), max(oldRange.end, newRange.end) );
				MergeRangeChange(range, r2, event);
			}
		}
	}
	return err;
}

	/*---------------------------------------------------------
	 * USER INTERFACE
	 *---------------------------------------------------------*/

uint32 AmTrack::CountProperties(TrackViewType type) const
{
	if( type == ARRANGE_VIEW ) return 1;
	else if( type == PRI_VIEW ) return 1;
	else if( type == SEC_VIEW ) return mSecViews.size();
	else return 0;
}

const AmViewPropertyI* AmTrack::Property(	TrackViewType type,
												uint32 index) const
{
	if (type == ARRANGE_VIEW) return &mArrangeView;
	else if (type == PRI_VIEW) return &mPriView;
	else if (type == SEC_VIEW) {
		if ( index >= mSecViews.size() ) return 0;
		return mSecViews[index];
	}
	return 0;
}

void AmTrack::SetProperty(	const AmViewPropertyI* property,
							TrackViewType type,
							uint32 index)
{
	// If there's no property, this is an erase command.
	if ( !property ) {
		if (type == SEC_VIEW) {
			viewproperty_vec::iterator		i;
			if ( index >= mSecViews.size() )
				i = mSecViews.end();
			else
				i = mSecViews.begin() + index;
			delete (*i);
			mSecViews.erase(i);
		}
		return;
	}

	// Otherwise set myself to the property
	if (type == ARRANGE_VIEW) mArrangeView = *property;
	else if (type == PRI_VIEW) mPriView = *property;
	else if (type == SEC_VIEW) {
		if ( index >= mSecViews.size() ) {
			AmViewProperty*	prop = new AmViewProperty( *property );
			if ( prop ) mSecViews.push_back( prop );
		} else {
			*mSecViews[index] = *property;
		}
	}
}

status_t AmTrack::GetConfiguration(BMessage* config) const
{
	ArpASSERT(config);
	*config = mConfiguration;
	return B_OK;
}

status_t AmTrack::SetConfiguration(const BMessage* config)
{
	ArpASSERT(config);
	mConfiguration = *config;
	return B_OK;
}

float AmTrack::PhraseHeight() const
{
	return mPhraseHeight;
}

void AmTrack::SetPhraseHeight(float phraseHeight)
{
	mPhraseHeight = phraseHeight;
}

	/*---------------------------------------------------------
	 * FILTER MANIPULATION -- this needs to be updated, probably
	 * to a scheme similar to what the events are doing.
	 *---------------------------------------------------------*/
static int32 index_for_pipeline(AmPipelineType type, uint32* flags)
{
	if (flags) *flags = AM_PLAIN;
	if (type == INPUT_PIPELINE) return INPUT_INDEX;
	else if (type == OUTPUT_PIPELINE) return OUTPUT_INDEX;
	else if (type == SOURCE_PIPELINE) {
		if (flags) *flags = AM_SOURCE;
		return INPUT_INDEX;
	} else if (type == DESTINATION_PIPELINE) {
		if (flags) *flags = AM_DESTINATION;
		return OUTPUT_INDEX;
	}
	return -1;
}

AmFilterHolderI* AmTrack::Filter(AmPipelineType type) const
{
	if (type == INPUT_PIPELINE) return mPipelines[INPUT_INDEX].Head();
	else if (type == OUTPUT_PIPELINE) return mPipelines[OUTPUT_INDEX].Head();
	else if (type == SOURCE_PIPELINE) return mPipelines[INPUT_INDEX].Source();
	else if (type == DESTINATION_PIPELINE) return mPipelines[OUTPUT_INDEX].Destination();
	else return NULL;
}

AmFilterHolderI* AmTrack::Filter(AmPipelineType type, filter_id id) const
{
	if (id == 0) return Filter(type);

	int32			index = index_for_pipeline(type);
	ArpASSERT(index < PIPELINE_SIZE);
	if (index < 0 || index >= PIPELINE_SIZE) return NULL;
	return mPipelines[index].Filter(id);
}

status_t AmTrack::InsertFilter(	AmFilterAddOn* addon,
								AmPipelineType type,
								int32 beforeIndex,
								const BMessage* config)
{
	if (mSong) mSong->SetDirty();
	
	ArpASSERT(addon);
	status_t		answer = B_ERROR;
	if (addon->Type() == AmFilterAddOn::SOURCE_FILTER || type == SOURCE_PIPELINE) {
		if (addon->Type() != AmFilterAddOn::SOURCE_FILTER ) return B_ERROR;
		if (type != SOURCE_PIPELINE && type != INPUT_PIPELINE) return B_ERROR;
		answer = mPipelines[INPUT_INDEX].SetSource(addon, config);
	} else if (addon->Type() == AmFilterAddOn::DESTINATION_FILTER || type == DESTINATION_PIPELINE) {
		if (addon->Type() != AmFilterAddOn::DESTINATION_FILTER || type != DESTINATION_PIPELINE) return B_ERROR;
		answer = mPipelines[OUTPUT_INDEX].SetDestination(addon, config);
	} else {
		int32		index = index_for_pipeline(type);
		ArpASSERT(index < PIPELINE_SIZE);
		if (index >= 0 && index < PIPELINE_SIZE)
			answer = mPipelines[index].InsertFilter(addon, beforeIndex, config);
	}
	if (answer == B_OK) MergePipelineChange(Id(), type);
	return answer;
}

static bool head_is_dest(AmPipelineSegment& segment)
{
	AmFilterHolderI*	head = segment.Head();
	return head && head->Filter() && head->Filter()->AddOn() && head->Filter()->AddOn()->Type() == AmFilterAddOn::DESTINATION_FILTER;
}

status_t AmTrack::ReplaceFilter(AmFilterAddOn* addon,
								AmPipelineType type,
								int32 atIndex,
								const BMessage* config)
{
	assert( addon );
	if (mSong) mSong->SetDirty();
	status_t		answer = B_ERROR;
	/* Little bit of a hack.  The source pipeline and input pipeline are merged
	 * together as a single view, so currently this method will receive replace
	 * commands for the input pipeline, when technically it should be receiving
	 * them for the source pipeline.
	 */
	if (addon->Type() == AmFilterAddOn::SOURCE_FILTER || type == SOURCE_PIPELINE) {
		AmPipelineType	realType = (type == INPUT_PIPELINE) ? SOURCE_PIPELINE : type;
		if( addon->Type() != AmFilterAddOn::SOURCE_FILTER || realType != SOURCE_PIPELINE) return B_ERROR;
		answer = mPipelines[INPUT_INDEX].SetSource(addon, config);
	} else if (addon->Type() == AmFilterAddOn::DESTINATION_FILTER || type == DESTINATION_PIPELINE ) {
		if (addon->Type() != AmFilterAddOn::DESTINATION_FILTER || type != DESTINATION_PIPELINE) return B_ERROR;
		answer = mPipelines[OUTPUT_INDEX].SetDestination(addon, config);
	/* A little bit of a hack.  This method receives replace commands for filters
	 * at the start of the output pipeline, but since the output pipeline and
	 * the destination pipeline are the same, the view might not know to check
	 * and see if there's actually a destination filter at the start.  In that
	 * case, I just want to do an insert.
	 */
	} else if (atIndex == 0 && type == OUTPUT_PIPELINE && head_is_dest(mPipelines[type]) ) {
		return InsertFilter(addon, type, 0, config);
	} else {
		int32		index = index_for_pipeline(type);
		ArpASSERT(index < PIPELINE_SIZE);
		if (index >= 0 && index < PIPELINE_SIZE)
			answer = mPipelines[index].ReplaceFilter(addon, atIndex, config);
	}
	if (answer == B_OK) MergePipelineChange(Id(), type);
	return answer;
}

status_t AmTrack::RemoveFilter(AmPipelineType type, filter_id id)
{
	if (mSong) mSong->SetDirty();

	int32			index = index_for_pipeline(type);
	ArpASSERT(index < PIPELINE_SIZE);
	if (index < 0 || index >= PIPELINE_SIZE) return B_ERROR;
	status_t		answer = mPipelines[index].RemoveFilter(id);
	if (answer == B_OK) MergePipelineChange(Id(), type);
	return answer;
}

status_t AmTrack::FlattenFilters(BMessage* into, AmPipelineType type) const
{
	uint32			flags = AM_PLAIN;
	int32			index = index_for_pipeline(type, &flags);
	ArpASSERT(index < PIPELINE_SIZE);
	if (index < 0 || index >= PIPELINE_SIZE) return B_ERROR;
	return mPipelines[index].FlattenFilters(into, flags);
}

status_t AmTrack::UnflattenFilters(const BMessage* from,
								   AmPipelineType type, bool append)
{
	if (mSong) mSong->SetDirty();
	
	uint32			flags = AM_PLAIN;
	int32			index = index_for_pipeline(type, &flags);
	ArpASSERT(index < PIPELINE_SIZE);
	if (index < 0 || index >= PIPELINE_SIZE) return B_ERROR;
	return mPipelines[index].UnflattenFilters(from, flags|(append?AM_PLAIN:0) );
}

	/*---------------------------------------------------------
	 * PLAYBACK
	 *---------------------------------------------------------*/

ArpRef<AmInputTarget> AmTrack::InputTarget() const
{
	return mInputTarget;
}

void AmTrack::RecordEvents(AmEvent* events)
{
	ArpD(if (events) printf("Recording event: "); events->Print());
	
	if (mRecordMode == RECORD_OFF_MODE || !mRecordingPhrase) {
		ArpD(printf("Not recording!\n"));
		//printf("Dropping input:\n"); events->PrintChain();
		if (events) events->DeleteChain();
		return;
	}
	
	AmRange range;
	
	if (mLastRecordedTime >= 0) {
		range += AmRange(mLastRecordedTime, mLastRecordedTime);
	}
	
	const bool firstEvent = mRecordingPhrase->IsEmpty();
	
	AmRange			r;
	if (mSong) r = mSong->RecordRange();
	while (events) {
		AmEvent* next = events->RemoveEvent();
		if (events->StartTime() < 0) events->DeleteChain();
		else {
			range += AmRange(events->StartTime(), events->EndTime());
			mLastRecordedTime = events->StartTime();
			if (r.IsValid() && (events->StartTime() < r.start || events->StartTime() > r.end) ) {
				events->Delete();
			} else if (events->Type() == AmEvent::NOTEOFF_TYPE && mRecordingPhrase->Phrase() ) {
				// Don't record note off events; instead, use them
				// to set the duration of a previous note-on.
				AmNoteOff* noteOff = dynamic_cast<AmNoteOff*>(events);
				AmNode* node = mRecordingPhrase->Phrase()->FindNode(
						events->StartTime(), BACKWARDS_SEARCH);
				while (noteOff && node) {
					AmNoteOn* noteOn = dynamic_cast<AmNoteOn*>(
							node->Event());
					if (noteOn && noteOn->Note() == noteOff->Note()) {
						range += AmRange(noteOn->StartTime(), noteOn->StartTime());
						noteOn->SetNote(noteOff->Note());
						noteOn->SetReleaseVelocity(noteOff->Velocity());
						noteOn->SetHasDuration(true);
						mRecordingPhrase->Phrase()->SetEventEndTime(noteOn, noteOff->StartTime());
						ArpD(printf("Modified: "); noteOn->Print());
						break;
					}
					node = node->prev;
				}
				events->Delete();
			} else {
				if ( mRecordingPhrase->Phrase() )
					mRecordingPhrase->Phrase()->Add(events);
			}
		}
		events = next;
	}
	
	if (firstEvent && !mRecordingPhrase->IsEmpty()) {
		AddEvent(NULL, mRecordingPhrase, "Record Events");
	}
	
	if (range.IsValid()) {
		mSong->SetDirty();
		MergeRangeChange(range, range);
		FlushChanges();
		if (mSong) mSong->FlushChanges();
		if (range.end > mSong->LastEndTime()) {
			mSong->EndTimeChangeNotice(range.end);
		}
	}
}

void AmTrack::StartRecordingPhrase()
{
	StopRecordingPhrase();
	mRecording = true;
	mLastRecordedTime = -1;
	if (mRecordMode != RECORD_OFF_MODE) {
		AttachRecordingPhrase();
	}
}

bool AmTrack::StopRecordingPhrase()
{
	mRecording = false;
	return DetachRecordingPhrase();
}

status_t AmTrack::NewSignatures() const
{
	if (!mSignatures) {
		AmSignature*	sig = new AmSignature(0);
		if (!sig) return B_NO_MEMORY;
		if (mSong) {
			const AmPhrase&	songSigs = mSong->Signatures();
			AmNode*		n = songSigs.HeadNode();
			AmSignature* headSig;
			if ( n && (headSig = dynamic_cast<AmSignature*>( n->Event() )) ) {
				sig->SetBeats( headSig->Beats() );
				sig->SetBeatValue( headSig->BeatValue() );
			}
		}
		mSignatures = new AmPhrase();
		if (!mSignatures) {
			sig->Delete();
			return B_NO_MEMORY;
		}
		mSignatures->SetParent( const_cast<AmTrack*>(this) );
		mSignatures->Add(sig);
	}
	return B_OK;
}

void AmTrack::AttachRecordingPhrase()
{
	DetachRecordingPhrase();
	mRecordingPhrase = new AmRootPhraseEvent;
	mRecordingPhrase->IncRefs();
	//Wait on adding until first event is placed into it.
	//AddEvent(mRecordingPhrase);
}

bool AmTrack::DetachRecordingPhrase()
{
	if (mRecordingPhrase == NULL) return false;
	bool		answer = false;
	if (mRecordingPhrase->IsEmpty()) {
//		RemoveEvent(NULL, mRecordingPhrase);
//		mRecordingPhrase->Delete();
		mRecordingPhrase->DecRefs();
	
	} else {
		answer = true;
		// Clean up any note-on events that don't
		// have corresponding note-offs.
		AmNode*	node = NULL;
		if ( mRecordingPhrase->Phrase() ) node = mRecordingPhrase->Phrase()->HeadNode();
		while (node) {
			AmNoteOn* noteOn = dynamic_cast<AmNoteOn*>(node->Event());
			if (noteOn && !noteOn->HasDuration()) {
				printf("*** Missing note off: "); noteOn->Print();
				noteOn->SetHasDuration(true);
				mRecordingPhrase->Phrase()->SetEndTime(noteOn, noteOn->StartTime()+PPQN, noteOn->StartTime());
			}
			node = node->next;
		}
	}
	
	mRecordingPhrase = NULL;
	return answer;
}

AmEvent* AmTrack::PlaybackList(	AmTime startTime, AmTime stopTime,
								uint32 flags) const
{
	if( !(flags&PLAYBACK_IGNORE_MUTE)
			&& !(ModeFlags()&SOLO_MODE)
			&& (ModeFlags()&MUTE_MODE) )
		return 0;

	AmEvent* e = NULL;
	
	if ((flags&PLAYBACK_NO_TEMPO) == 0) {
		mSong->MergeTemposInto(&e, startTime, stopTime, flags);
	}
	
	if ((flags&PLAYBACK_NO_SIGNATURE) == 0) {
		mSong->MergeSignaturesInto(&e, startTime, stopTime, flags);
	}
	
	if ((flags&PLAYBACK_NO_PERFORMANCE) == 0) {
		AmEvent* pe = RawPlaybackList(startTime, stopTime, flags);
		if (pe) {
			if (e == 0) e = pe;
			else e->MergeList(pe, true);
		}
	}
	
	if (e) e = e->HeadEvent();
	return e;
}

AmEvent* AmTrack::RawPlaybackList(	AmTime startTime, AmTime stopTime,
									uint32 flags) const
{
	if( !(flags&PLAYBACK_IGNORE_MUTE)
			&& !(ModeFlags()&SOLO_MODE)
			&& (ModeFlags()&MUTE_MODE) )
		return 0;

	AmEvent*		e = 0;
	AmNode*			n = mPhrases.HeadNode();
	AmPhraseEvent*	topPhrase;
	while( n && (stopTime < 0 || n->StartTime() <= stopTime) ) {
		if( (topPhrase = dynamic_cast<AmPhraseEvent*>( n->Event() ))
				&& topPhrase->EndTime() >= startTime
				&& topPhrase->Phrase() ) {
			topPhrase->Phrase()->MergeInto(&e, topPhrase, Filter(OUTPUT_PIPELINE),
										   startTime, stopTime);
		}
		n = n->next;
	}
	if (!e) return NULL;
	return e->HeadEvent();
//	return e;
}

status_t AmTrack::AddPulseFilters(AmSong* song)
{
	ArpASSERT(song);
	for (int32 k = 0; k < PIPELINE_SIZE; k++) {
		AmFilterHolderI*	h = mPipelines[k].Head();
		while (h) {
			AmFilterI*		f = h->Filter();
			if (f && f->Flags()&f->OSCILLATOR_FLAG) {
				status_t	err = song->AddPulseFilter(h);
				if (err != B_OK) return err;
			}
			h = h->NextInLine();
		}
	}
	return B_OK;
}

AmPhrase* AmTrack::PhraseFor(AmEvent* event) const
{
	if( event->Type() == event->PHRASE_TYPE ) {
		return (AmPhrase*)&mPhrases;
	} else if( event->Type() == event->SIGNATURE_TYPE ) {
		if( !mSignatures ) {
			status_t	err = NewSignatures();
			if (err != B_OK) return NULL;
		}
		return mSignatures;
	} else {
		/* This used to return 0, but since some tracks, like the
		 * tempo track, will allow events that aren't in phrases...
		 */
		return (AmPhrase*)&mPhrases;
	}
}

void AmTrack::AnnotateMessage(BMessage& msg) const
{
	msg.AddPointer( SZ_TRACK_ID, Id() );
	if (msg.what == TITLE_CHANGE_OBS) {
		if( Name() ) msg.AddString( SZ_TRACK_TITLE, Name() );
	} else if (msg.what == MODE_CHANGE_OBS) {
		msg.AddInt32("mode flags", mModeFlags);
		msg.AddInt32("record mode", mRecordMode);
	} else if (msg.what == GROUP_CHANGE_OBS) {
		msg.AddInt32("groups", mGroups);
	} else if (msg.what == AmNotifier::PIPELINE_CHANGE_OBS) {
		if (mSong) msg.AddPointer(MATRIX_ID_STR, mSong->Id() );
		msg.AddInt32("pipeline_index", SongIndex() );
	} else if (msg.what == AmNotifier::FILTER_CHANGE_OBS) {
		if (mSong) msg.AddPointer(MATRIX_ID_STR, mSong->Id() );
		msg.AddInt32("pipeline_index", SongIndex() );
	}
}

void AmTrack::MergeRangeChange(	AmRange oldRange, AmRange newRange,
								AmEvent* event)
{
	AmNotifier::MergeRangeChange(oldRange, newRange, event);
	if (mSong) {
		mSong->SetChangedTrack(Id() );
		mSong->MergeRangeChange(oldRange, newRange, event);
	}
}

void AmTrack::MergePipelineChange(AmPipelineSegment* pipeline)
{
	if (&(mPipelines[INPUT_INDEX]) == pipeline)
		MergePipelineChange(Id(), INPUT_PIPELINE);
	else if (&(mPipelines[OUTPUT_INDEX]) == pipeline)
		MergePipelineChange(Id(), OUTPUT_PIPELINE);
	else
		MergePipelineChange(Id(), _NUM_PIPELINE);
}

void AmTrack::MergePipelineChange(pipeline_id id, AmPipelineType type)
{
	mFlags |= HAS_NOTIFICATION;
	AmNotifier::MergePipelineChange(id, type);
	if (mSong) {
		mSong->SetChangedTrack(Id() );
		mSong->MergePipelineChange(id, type);	
	}
}

void AmTrack::MergeFilterChange(pipeline_id id, AmPipelineType type)
{
	mFlags |= HAS_NOTIFICATION;
	AmNotifier::MergeFilterChange(id, type);
	if (mSong) {
		mSong->SetChangedTrack(Id() );
		mSong->MergeFilterChange(id, type);	
	}
}

void AmTrack::FlushChanges()
{
	if (!(mFlags&HAS_NOTIFICATION)) return;
	mFlags &= ~HAS_NOTIFICATION;
	AmNotifier::FlushChanges();
}

void AmTrack::SetTitle(const char* title, void* sender, BUndoContext* undo)
{
	if (mSong) mSong->SetDirty();
	/* Setup the undo state.
	 */
	if (undo) {
		undo->SuggestUndoName("Set Track Title");
		AmTrackStateUndo* tu = dynamic_cast<AmTrackStateUndo*>(undo->LastOperation( Id(), B_UNIQUE_UNDO_MERGE ));
		if (!tu) {
			tu = new AmTrackStateUndo(mSong, this);
			undo->AddOperation(tu);
		}
		if (tu) tu->NoteSetTitle( Name() );
	}
	/* Change the title.
	 */
	SetName( title ? title : SZ_UNTITLED_TRACK );
	ReportChange(TITLE_CHANGE_OBS, BMessenger() );
	if (mSong) {
		BMessage	msg(TITLE_CHANGE_OBS);
		msg.AddPointer(SZ_TRACK_ID, Id() );
		if (Name() ) msg.AddString(SZ_TRACK_TITLE, Name() );
		mSong->ReportMsgChange(&msg, BMessenger() );
	}
}

AmPipelineSegment* AmTrack::PipelineSegment(AmPipelineType type)
{
	int32			index = index_for_pipeline(type);
	ArpASSERT(index < PIPELINE_SIZE);
	if (index < 0 || index >= PIPELINE_SIZE) return NULL;
	return &mPipelines[index];
}

// #pragma mark -

/***************************************************************************
 * AM-TRACK-EVENT-UNDO
 ****************************************************************************/
AmTrackEventUndo::AmTrackEventUndo(	AmTrack* track, bool added,
									AmPhrase* phrase, AmEvent* event)
		: mTrack(track), mPhrase(phrase), mEvent(event),
		  mAdded(added)
{
	mTrack->AddRef();
	mEvent->IncRefs();
}
	
AmTrackEventUndo::~AmTrackEventUndo()
{
	if (!mAdded) mEvent->Delete();
	mTrack->RemoveRef();
	mEvent->DecRefs();
}
	
const void* AmTrackEventUndo::Owner() const
{
	return mTrack;
}
	
bool AmTrackEventUndo::MatchOwnerContext(const void* owner) const
{
	return owner == mTrack;
}
	
void AmTrackEventUndo::Commit()
{
}
	
void AmTrackEventUndo::Undo()
{
	swap_data();
}
	
void AmTrackEventUndo::Redo()
{
	swap_data();
}

void AmTrackEventUndo::Print() const
{
	const char*		s = "added";
	if (!mAdded) s = "removed";
	printf("Track (%p) EventUndo %s event ", mTrack, s); mEvent->Print();
}

void AmTrackEventUndo::swap_data()
{
	if (mAdded) mTrack->RemoveEvent(mPhrase, mEvent, NULL);
	else {
		mEvent->Undelete();
		mTrack->AddEvent(mPhrase, mEvent, (BUndoContext*)NULL);
	}
	mAdded = !mAdded;
}

// #pragma mark -

/***************************************************************************
 * AM-CHANGE-EVENT-UNDO
 ****************************************************************************/
class _AmCeoEntry
{
public:
	_AmCeoEntry();
	_AmCeoEntry(AmPhrase* phrase, AmEvent* event);
	~_AmCeoEntry();
	
	_AmCeoEntry&	operator=(const _AmCeoEntry &e);

	void			Swap();
	bool			HasChanges() const;
	void			EventChanged();
	bool			Matches(event_id id) const;
	void			Print() const;

	event_id		mId;
	AmPhrase*		mPhrase;
	AmEvent*		mEvent;
	AmEvent*		mOriginalEvent;
	AmEvent*		mChangedEvent;

private:
	bool			mUndo;
};

AmChangeEventUndo::AmChangeEventUndo(const AmTrack* track)
		: mTrack(track)
{
	mTrack->AddRef();
}
	
AmChangeEventUndo::~AmChangeEventUndo()
{
	mTrack->RemoveRef();
}
	
const void* AmChangeEventUndo::Owner() const
{
	return mTrack;
}
	
bool AmChangeEventUndo::MatchOwnerContext(const void* owner) const
{
	return owner == mTrack;
}
	
void AmChangeEventUndo::Commit()
{
}
	
void AmChangeEventUndo::Undo()
{
	swap_data();
}
	
void AmChangeEventUndo::Redo()
{
	swap_data();
}

track_id AmChangeEventUndo::TrackId() const
{
	ArpASSERT(mTrack);
	return mTrack->Id();
}

int32 AmChangeEventUndo::CountEvents() const
{
	return mEntries.CountItems();
}

AmEvent* AmChangeEventUndo::EventAt(int32 index, AmPhrase** outContainer)
{
	_AmCeoEntry*	entry = (_AmCeoEntry*)(mEntries.ItemAt(index));
	if (!entry) return NULL;
	if (outContainer) *outContainer = entry->mPhrase;
	return entry->mEvent;
}

bool AmChangeEventUndo::HasChanges() const
{
	int32	size = mEntries.CountItems();
	for (int32 k = 0; k < size; k++) {
		_AmCeoEntry*	entry = (_AmCeoEntry*)( mEntries.ItemAt(k) );
		if ( entry && entry->HasChanges() ) return true;
	}
	return false;
}

void AmChangeEventUndo::EventChanging(AmPhrase* phrase, AmEvent* event)
{
	ArpASSERT(event);
	_AmCeoEntry*	entry = EntryForId( event->Id() );
	if (entry) return;
	entry = new _AmCeoEntry(phrase, event);
	if (!entry) return;
	mEntries.AddItem( (void*)entry );
}

void AmChangeEventUndo::EventChanged(AmEvent* event)
{
	ArpASSERT(event);
	_AmCeoEntry*	entry = EntryForId( event->Id() );
	if (entry) entry->EventChanged();
}

void AmChangeEventUndo::RemoveEvent(AmEvent* event)
{
	ArpASSERT(event);
	int32	size = mEntries.CountItems();
	for (int32 k = 0; k < size; k++) {
		_AmCeoEntry*	entry = (_AmCeoEntry*)( mEntries.ItemAt(k) );
		if ( entry && entry->Matches(event->Id() ) ) {
			mEntries.RemoveItem(k);
			delete entry;
			return;
		}
	}
}

static bool scrub_one(BList* entries)
{
	int32	size = entries->CountItems();
	for (int32 k = 0; k < size; k++) {
		_AmCeoEntry*	entry = (_AmCeoEntry*)( entries->ItemAt(k) );
		if (entry && !entry->HasChanges() ) {
			entries->RemoveItem(k);
			delete entry;
			return true;
		}
	}
	return false;
}

void AmChangeEventUndo::ScrubUnchanged()
{
	while (scrub_one(&mEntries) ) ;
}

BString AmChangeEventUndo::StringContents() const
{
	AmEvent::EventType	type = AmEvent::_NUM_TYPE;
	int32	size = mEntries.CountItems();
	for (int32 k = 0; k < size; k++) {
		if (type != AmEvent::_NUM_TYPE + 1) {
			_AmCeoEntry*	entry = (_AmCeoEntry*)( mEntries.ItemAt(k) );
			if (entry) {
				if (type == AmEvent::_NUM_TYPE) type = entry->mEvent->Type();
				else if (type != entry->mEvent->Type()) type = (AmEvent::EventType)(AmEvent::_NUM_TYPE + 1);
			}
		}
	}
	BString		answer;
	answer << size << " ";
	string_for_event_type(answer, type, AmEvent::NO_SUBTYPE, (size == 1) ? false : true);
	return answer;
}

void AmChangeEventUndo::Print() const
{
	int32	size = mEntries.CountItems();
	printf("AmChangeEventUndo with %ld entries:\n", size);
	for (int32 k = 0; k < size; k++) {
		_AmCeoEntry*	entry = (_AmCeoEntry*)( mEntries.ItemAt(k) );
		if (entry) {
			printf("\t%ld: ", k);
			entry->Print();
		}
	}
}

void AmChangeEventUndo::swap_data()
{
	int32	size = mEntries.CountItems();
	for (int32 k = 0; k < size; k++) {
		_AmCeoEntry*	entry = (_AmCeoEntry*)( mEntries.ItemAt(k) );
		if (entry) entry->Swap();
	}
}

_AmCeoEntry* AmChangeEventUndo::EntryForId(event_id id) const
{
	int32	size = mEntries.CountItems();
	for (int32 k = 0; k < size; k++) {
		_AmCeoEntry*	entry = (_AmCeoEntry*)( mEntries.ItemAt(k) );
		if ( entry && entry->Matches(id) ) return entry;
	}
	return NULL;
}

// #pragma mark -

/***************************************************************************
 * _AM-CEO-ENTRY
 ****************************************************************************/
_AmCeoEntry::_AmCeoEntry()
		: mId(NULL), mPhrase(NULL), mEvent(NULL),
		  mOriginalEvent(NULL), mChangedEvent(NULL),
		  mUndo(true)
{
}

_AmCeoEntry::_AmCeoEntry(AmPhrase* phrase, AmEvent* event)
		: mId( event->Id() ), mPhrase(phrase), mEvent(event),
		  mOriginalEvent( event->Copy() ), mChangedEvent(NULL),
		  mUndo(true)
{
}

_AmCeoEntry::~_AmCeoEntry()
{
	if (mOriginalEvent) mOriginalEvent->Delete();
	if (mChangedEvent) mChangedEvent->Delete();
}
	
_AmCeoEntry& _AmCeoEntry::operator=(const _AmCeoEntry &e)
{
	mId = e.mId;
	mPhrase = e.mPhrase;
	mEvent = e.mEvent;
	mOriginalEvent = NULL;
	mChangedEvent = NULL;
	if (e.mOriginalEvent) mOriginalEvent = e.mOriginalEvent->Copy();
	if (e.mChangedEvent) mChangedEvent = e.mChangedEvent->Copy();
	return *this;
}

void _AmCeoEntry::Swap()
{
	ArpASSERT(mEvent && mOriginalEvent && mChangedEvent);
	if (!mEvent || !mOriginalEvent || !mChangedEvent) return;

	if (mUndo) {
		mEvent->SetTo(mOriginalEvent);
		if (mPhrase) {
			if (mOriginalEvent->StartTime() != mEvent->StartTime() ) mPhrase->SetEventStartTime(mEvent, mOriginalEvent->StartTime() );
			if (mOriginalEvent->EndTime() != mEvent->EndTime() ) mPhrase->SetEventEndTime(mEvent, mOriginalEvent->EndTime() );
		} else {
			if (mOriginalEvent->StartTime() != mEvent->StartTime() ) mEvent->SetStartTime(mOriginalEvent->StartTime() );
			if (mOriginalEvent->EndTime() != mEvent->EndTime() ) mEvent->SetEndTime(mOriginalEvent->EndTime() );
		}
	} else {
		mEvent->SetTo(mChangedEvent);
		if (mPhrase) {
			if (mChangedEvent->StartTime() != mEvent->StartTime() ) mPhrase->SetEventStartTime(mEvent, mChangedEvent->StartTime() );
			if (mChangedEvent->EndTime() != mEvent->EndTime() ) mPhrase->SetEventEndTime(mEvent, mChangedEvent->EndTime() );
		} else {
			if (mChangedEvent->StartTime() != mEvent->StartTime() ) mEvent->SetStartTime(mChangedEvent->StartTime() );
			if (mChangedEvent->EndTime() != mEvent->EndTime() ) mEvent->SetEndTime(mChangedEvent->EndTime() );
		}
	}
	mUndo = !mUndo;
}

bool _AmCeoEntry::HasChanges() const
{
	if (!mOriginalEvent || !mChangedEvent) return false;
	return !(mOriginalEvent->Equals(mChangedEvent));
}

void _AmCeoEntry::EventChanged()
{
	if (mChangedEvent) mChangedEvent->Delete();
	mChangedEvent = NULL;
	if (mEvent) mChangedEvent = mEvent->Copy();
}

bool _AmCeoEntry::Matches(event_id id) const
{
	return mId == id;
}

void _AmCeoEntry::Print() const
{
	printf("For event %p: ", mEvent);
	printf("%p (orig) ", mId);
	if (!mOriginalEvent) printf("NULL EVENT\n");
	else mOriginalEvent->Print();
	printf("\t\tbecomes (changed) ");
	if (!mChangedEvent) printf("NULL EVENT\n");
	else mChangedEvent->Print();
}
