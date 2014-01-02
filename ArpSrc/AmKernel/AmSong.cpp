/* AmSong.cpp
 */
#define _BUILDING_AmKernel 1

#include "AmKernel/AmSong.h"

#include <stdio.h>
#include <stdlib.h>
#include <support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmInputQueue.h"
#include "AmKernel/AmTrack.h"
#include "AmKernel/AmTransport.h"
#include "AmKernel/AmViewProperty.h"

#include "AmKernel/AmFilterRoster.h"

static const int32	_DEFAULT_TEMPO		= 120;

enum {
	_AM_SUPPRESS_UNDO	= 1<<0
};


class AmSongTrackUndo : public BUndoOperation
{
public:
	AmSongTrackUndo(AmSong* song, AmTrack* track, bool added, int32 index)
		: mSong(song), mTrack(track),
		  mAdded(added), mIndex(index)
	{
		mTrack->AddRef();
	}
	
	virtual ~AmSongTrackUndo()
	{
		mTrack->RemoveRef();
	}
	
	virtual const void* Owner() const
	{
		return mSong;
	}
	
	virtual bool MatchOwnerContext(const void* owner) const
	{
		return owner == mSong;
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

private:
	void swap_data()
	{
		if (mAdded) mSong->RemoveTrack(mIndex, NULL);
		else mSong->AddTrack(mTrack, NULL, mIndex);
		mAdded = !mAdded;
	}
	
	AmSong*			mSong;
	AmTrack* const	mTrack;
	bool			mAdded;
	int32			mIndex;
};

class AmSongTrackMoveUndo : public BUndoOperation
{
public:
	AmSongTrackMoveUndo(AmSong* song, track_id tid, int32 origPos, int32 newPos)
		: mSong(song), mTrackId(tid), mOrigPos(origPos), mNewPos(newPos),
		  mSwapped(true)
	{
	}
	
	virtual ~AmSongTrackMoveUndo()
	{
	}
	
	virtual const void* Owner() const
	{
		return mSong;
	}
	
	virtual bool MatchOwnerContext(const void* owner) const
	{
		return owner == mSong;
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

private:
	void swap_data()
	{
		if (mSwapped) mSong->MoveTrackBy(mTrackId, mOrigPos - mNewPos, NULL);
		else mSong->MoveTrackBy(mTrackId, mNewPos - mOrigPos, NULL);
		mSwapped = !mSwapped;
	}
	
	AmSong*			mSong;
	track_id		mTrackId;
	int32			mOrigPos, mNewPos;
	bool			mSwapped;
};

class _AmPulseFilters
{
public:
	vector<AmFilterHolderI*>		holders;

	_AmPulseFilters();
	~_AmPulseFilters();

	status_t		Add(AmFilterHolderI* holder);
};

/**********************************************************************
 * AM-SONG
 **********************************************************************/
AmSong::AmSong(const char* title)
		: mRefCount(0), mTempoTrack(NULL), mTransport(new AmTransport(Id(), true)),
		  mDirty(false), mFlags(0), mChangedTrack(0), mLastEndTime(0),
		  mInputQueue(new AmInputQueue(Id())),
		  mPulseFilters(0), mPulseFiltersChanged(true), mPulseThread(0)
{
	SetName(title);
	Clear();
	
	// Set myself up with a default signature
//	AmSignature		*ms;
//	if ( (ms = new AmSignature(0)) != 0 ) {
//		ms->Set(0, 1, 4, 4);
//		AddEvent( ms );
//	}
//	if ( (ms = new AmSignature()) != 0 ) {
//		ms->Set(1536, 5, 2, 4);
//		AddEvent( ms );
//	}
	// Now that we are all up and running, start the real-time
	// performance thread.
	mInputQueue->StartPerforming();
}

AmSong::~AmSong()
{
//printf("Delete song\n"); fflush(stdout);
	{
		BAutolock		l(mPulseAccess);
		delete mPulseFilters;
		mPulseFilters = 0;
	}
		
	DoStopTransport(AmFilterI::TRANSPORT_CONTEXT);
	if (mInputQueue) {
		mInputQueue->StopRecording();
		mInputQueue->StopPerforming();
	}
	if (mTempoTrack) mTempoTrack->RemoveRef();
	mSignatures.DeleteEvents();
	mTrackRefs.resize(0);
	delete mTransport;
}

void AmSong::AddRef() const
{
	AmSong* me = const_cast<AmSong*>(this);
	atomic_add(&me->mRefCount, 1);
}

void AmSong::RemoveRef() const
{
	AmSong* me = const_cast<AmSong*>(this);
	if( atomic_add(&me->mRefCount, -1) == 1 ) {
		printf("AmSong::RemoveRef() delete song %s\n", Name() );
		me->Delete();
	}
}

bool AmSong::ReadLock() const
{
	return mLock.ReadLock();
}

bool AmSong::WriteLock(const char* name)
{
	mChangedTrack = 0;
	bool result = mLock.WriteLock();
	if (result) {
		mUndoContext.StartUpdate(name);
	}
	return result;
}

bool AmSong::ReadUnlock() const
{
	return mLock.ReadUnlock();
}

bool AmSong::WriteUnlock()
{
	if (mUndoContext.UpdateNestingLevel() == 1) {
		for (uint32 k=0; k<mTrackRefs.size(); k++) {
			mTrackRefs[k].mTrack->FlushChanges();
		}
		if (mTempoTrack) mTempoTrack->FlushChanges();
		FlushChanges();
	}
	mUndoContext.EndUpdate();
	mChangedTrack = 0;

	/* After every write, check to see if my filters changed --
	 * if so, restart the pulsing thread.
	 */
	{
		BAutolock		l(mPulseAccess);
		if (mPulseFiltersChanged) {
			if (mPulseThread > 0) kill_thread(mPulseThread);
			mPulseThread = 0;
			CachePulseFilters();
			if (mPulseFilters && mPulseFilters->holders.size() > 0) {
				mPulseThread = spawn_thread(PulseThreadEntry,
											"ARP Oscillator Pulse",
											B_NORMAL_PRIORITY,
											this);
				if (mPulseThread > 0) resume_thread(mPulseThread);
			}
		}
	}

	return mLock.WriteUnlock();
}

song_id AmSong::Id() const
{
	return (void*)this;
}

uint32 AmSong::CountPipelines() const
{
	return mTrackRefs.size();
}

pipeline_id AmSong::PipelineId(uint32 pipelineIndex) const
{
	if (pipelineIndex >= mTrackRefs.size() ) return 0;
	return mTrackRefs[pipelineIndex].TrackId();
}

status_t AmSong::PipelineHeight(uint32 pipelineIndex, float* outHeight) const
{
	if (pipelineIndex >= mTrackRefs.size() ) return B_BAD_INDEX;
	*outHeight = mTrackRefs[pipelineIndex].mTrack->PhraseHeight();
	return B_OK;
}

AmFilterHolderI* AmSong::Filter(pipeline_id id,
								AmPipelineType type,
								filter_id filterId) const
{
	const AmTrack*	track = TrackForPipeline(id);
	if (!track) return NULL;
	else return track->Filter(type, filterId);
}

status_t AmSong::InsertFilter(	AmFilterAddOn* addon,
								pipeline_id id,
								AmPipelineType type,
								int32 beforeIndex,
								const BMessage* config)
{
	AmTrack*			track = TrackForPipeline(id);
	if (!track) return B_ERROR;
	{
		BAutolock		l(mPulseAccess);
		delete mPulseFilters;
		mPulseFilters = 0;
		mPulseFiltersChanged = true;
	}
	return track->InsertFilter(addon, type, beforeIndex, config);
}

status_t AmSong::ReplaceFilter(	AmFilterAddOn* addon,
								pipeline_id id,
								AmPipelineType type,
								int32 atIndex,
								const BMessage* config)
{
	AmTrack*			track = TrackForPipeline(id);
	if (!track) return B_ERROR;
	{
		BAutolock		l(mPulseAccess);
		delete mPulseFilters;
		mPulseFilters = 0;
		mPulseFiltersChanged = true;
	}
	return track->ReplaceFilter(addon, type, atIndex, config);
}

status_t AmSong::RemoveFilter(	pipeline_id id,
								AmPipelineType type,
								filter_id filterId)
{
	AmTrack*			track = TrackForPipeline(id);
	if (!track) return B_ERROR;
	{
		BAutolock		l(mPulseAccess);
		delete mPulseFilters;
		mPulseFilters = 0;
		mPulseFiltersChanged = true;
	}
	return track->RemoveFilter(type, filterId);
}

status_t AmSong::MakeConnection(pipeline_id fromPid,
								AmPipelineType fromType,
								filter_id fromFid,
								pipeline_id toPid,
								AmPipelineType toType,
								filter_id toFid)
{
	AmPipelineSegment*	fromSegment = PipelineSegment(fromPid, fromType);
	AmPipelineSegment*	toSegment = PipelineSegment(toPid, toType);
	if (!fromSegment || !toSegment) return B_ERROR;
	return fromSegment->MakeConnection(	this, fromPid, fromType, fromFid,
										toSegment, toFid, this, UndoContext() );
}

status_t AmSong::BreakConnection(	pipeline_id fromPid,
									AmPipelineType fromType,
									filter_id fromFid,
									pipeline_id toPid,
									AmPipelineType toType,
									filter_id toFid)
{
	AmPipelineSegment*	fromSegment = PipelineSegment(fromPid, fromType);
	AmPipelineSegment*	toSegment = PipelineSegment(toPid, toType);
	if (!fromSegment || !toSegment) return B_ERROR;
	return fromSegment->BreakConnection(this, fromPid, fromType, fromFid,
										toSegment, toFid, this, UndoContext() );
}

void AmSong::PipelineChanged(pipeline_id id, AmPipelineType type)
{
	AmTrack*	track = TrackForPipeline(id);
	if (track) track->MergePipelineChange(id, type);
}

void AmSong::FilterChanged(pipeline_id id, AmPipelineType type)
{
	AmTrack*	track = TrackForPipeline(id);
	if (track) track->MergeFilterChange(id, type);
}

status_t AmSong::FlattenConnections(BMessage* into, AmPipelineType type) const
{
	ArpASSERT(into);
	for (uint32 pi = 0; pi < mTrackRefs.size(); pi++) {
		AmFilterHolderI*	h = mTrackRefs[pi].mTrack->Filter(type);
		uint32				fi = 0;
		while (h) {
			uint32			count = h->CountConnections();
			for (uint32 k = 0; k < count; k++) {
				AmFilterHolderI*	connection = h->ConnectionAt(k);
				if (connection && h->PipelineId() != connection->PipelineId() ) {
					BMessage	msg;
					if (AddConnectionInfo(connection, msg, type) == B_OK) {
						msg.AddInt32("source_pi", pi);
						msg.AddInt32("source_fi", fi);
						into->AddMessage("entry", &msg);
					}
				}
			}
			h = h->NextInLine();
			fi++;
		}
	}
	return B_OK;
}

status_t AmSong::AddConnectionInfo(	AmFilterHolderI* connection, BMessage& msg,
									AmPipelineType type) const
{
	for (uint32 pi = 0; pi < mTrackRefs.size(); pi++) {
		if (mTrackRefs[pi].TrackId() == connection->PipelineId() ) {
			AmFilterHolderI*	h = mTrackRefs[pi].mTrack->Filter(type);
			uint32				fi = 0;
			while (h) {
				if (h && h->Filter() && h->Filter()->Id() == connection->Filter()->Id() ) {
					msg.AddInt32("dest_pi", pi);
					msg.AddInt32("dest_fi", fi);
					return B_OK;
				}
				h = h->NextInLine();
				fi++;
			}
		}
	}
	return B_ERROR;
}

status_t AmSong::UnflattenConnections(const BMessage* into, AmPipelineType type)
{
	ArpASSERT(into);
	BMessage		entry;
	for (int32 k = 0; into->FindMessage("entry", k, &entry) == B_OK; k++) {
		int32		source_pi, source_fi, dest_pi, dest_fi;
		if (entry.FindInt32("source_pi", &source_pi) == B_OK
				&& entry.FindInt32("source_fi", &source_fi) == B_OK
				&& entry.FindInt32("dest_pi", &dest_pi) == B_OK
				&& entry.FindInt32("dest_fi", &dest_fi) == B_OK) {
			UnflattenConnection(source_pi, source_fi, dest_pi, dest_fi, type);
		}
		entry.MakeEmpty();
	}
	return B_OK;
}

status_t AmSong::UnflattenConnection(	int32 source_pi, int32 source_fi,
										int32 dest_pi, int32 dest_fi,
										AmPipelineType type)
{
	if (source_pi >= (int32)mTrackRefs.size() || dest_pi >= (int32)mTrackRefs.size() )
		return B_ERROR;
	AmFilterHolderI*	source = mTrackRefs[source_pi].mTrack->Filter(type);
	AmFilterHolderI*	dest = mTrackRefs[dest_pi].mTrack->Filter(type);
	int32		k = 0;
	while (source) {
		if (k == source_fi) break;
		source = source->NextInLine();
		k++;
	}
	k = 0;
	while (dest) {
		if (k == dest_fi) break;
		dest = dest->NextInLine();
		k++;
	}
	AmFilterHolder*		s = dynamic_cast<AmFilterHolder*>(source);
	AmFilterHolder*		d = dynamic_cast<AmFilterHolder*>(dest);
	if (!s || !d) return B_ERROR;
	return s->AddConnection(d);
}

status_t AmSong::AddMatrixPipelineObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0) {
		return AddObserver(handler, AmNotifier::PIPELINE_CHANGE_OBS);
	}
	AmTrack*	track = TrackForPipeline(id);
	if (!track) return B_ERROR;
	else return track->AddObserver(handler, AmNotifier::PIPELINE_CHANGE_OBS);
}

status_t AmSong::AddMatrixFilterObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0) {
		return AddObserver(handler, AmNotifier::FILTER_CHANGE_OBS);
	}
	AmTrack*	track = TrackForPipeline(id);
	if (!track) return B_ERROR;
	else return track->AddObserver(handler, AmNotifier::FILTER_CHANGE_OBS);
}

status_t AmSong::RemoveMatrixObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0) {
		return RemoveObserverAll(handler);
	}
	AmTrack*	track = TrackForPipeline(id);
	if (!track) return B_ERROR;
	else return track->RemoveObserverAll(handler);
}

	/*---------------------------------------------------------
	 * CHANGE NOTIFICATION
	 *---------------------------------------------------------*/

void AmSong::EndTimeChangeNotice(AmTime endTime)
{
	AmTime		t = (endTime < 0) ? CountEndTime() : endTime;
	mLastEndTime = t;
	BMessage	msg(END_TIME_CHANGE_OBS);
	add_time(msg, "end_time", t );
	ReportMsgChange(&msg, BMessenger());
}

AmTime AmSong::LastEndTime() const
{
	return mLastEndTime;
}

bool AmSong::IsDirty() const
{
	return mDirty;
}

void AmSong::SetDirty(bool dirty)
{
	if (mDirty != dirty) {
		printf("Changing dirty to %d\n", dirty);
	}
	mDirty = dirty;
}

void AmSong::ClearDirty() const
{
	if (mDirty != false) {
		printf("Changing dirty to %d\n", false);
	}
	mDirty = false;
}

	/*---------------------------------------------------------
	 * ACCESSING
	 *---------------------------------------------------------*/
float AmSong::BPM() const
{
	if (!mTempoTrack) return _DEFAULT_TEMPO;
	AmNode*			n = mTempoTrack->Phrases().HeadNode();
	if (!n) return _DEFAULT_TEMPO;
	AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
	if (!pe || !pe->Phrase() ) return _DEFAULT_TEMPO;
	n = pe->Phrase()->HeadNode();
	if (!n) return _DEFAULT_TEMPO;
	AmTempoChange*	event = dynamic_cast<AmTempoChange*>( n->Event() );
	if (!event) return _DEFAULT_TEMPO;
	return event->Tempo();
}

void AmSong::SetBPM(float BPM)
{
	if (!mTempoTrack) return;
	SetDirty();
	
	AmPhrase*		tempoPhrase = const_cast<AmPhrase*>( &(mTempoTrack->Phrases()) );
	AmNode*			n = tempoPhrase->HeadNode();
	AmPhraseEvent*	pe = NULL;
	if (!n) {
		pe = new AmRootPhraseEvent();
		if (pe) tempoPhrase->Add(pe);
	} else {
		pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
	}
	if (!pe || !pe->Phrase() ) return;
	
	n = pe->Phrase()->HeadNode();
	AmTempoChange*	event = 0;
	if (n) event = dynamic_cast<AmTempoChange*>( n->Event() );
	if (!event) {
		AmTempoChange*	tc = new AmTempoChange(BPM, 0);
		if (tc) {
			pe->Phrase()->Add(tc);
			AmRange		range( tc->StartTime(), tc->EndTime() );
			mTempoTrack->MergeRangeChange(range, range, tc);
		}
	} else {
		event->SetTempo( BPM );
		AmRange		range( event->StartTime(), event->EndTime() );
		mTempoTrack->MergeRangeChange(range, range, event);
	}
}

const AmPhrase& AmSong::Signatures() const
{
	return mSignatures;
}

static bool valid_beat_value(uint32 bv)
{
	return bv == 1 || bv == 2 || bv == 4 || bv == 8 || bv == 16 || bv == 32;
}

/* Perform a swap -- remove all the signatures from oldSignatures and
 * place them into newSignatures, while simultaneously giving them the
 * currect time values.  This is done because it is theoretically possible
 * for signatures to get of of order while they are being reassigned.
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

status_t AmSong::GetSignature(AmTime time, AmSignature& signature) const
{
	ArpASSERT(time >= 0);
	if (time < 0) return B_ERROR;
	AmNode*			node = mSignatures.HeadNode();
	ArpASSERT(node);
	if (!node) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	ArpASSERT(sig && sig->Time() == 0);
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
//	debugger("Failed to find measure");
	return B_ERROR;
}

status_t AmSong::GetSignatureForMeasure(int32 measure, AmSignature& signature) const
{
	ArpASSERT(measure >= 1);
	if (measure < 1) return B_ERROR;
	AmNode*			node = mSignatures.HeadNode();
	ArpASSERT(node);
	if (!node) return B_ERROR;
	AmSignature*	sig = dynamic_cast<AmSignature*>( node->Event() );
	ArpASSERT(sig && sig->Time() == 0);
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

status_t AmSong::SetSignature(int32 measure, uint32 beats, uint32 beatValue)
{
	if (!valid_beat_value(beatValue)) return B_ERROR;
	/* First see if there's an existing measure I can make use of
	 */
	AmNode*			n = mSignatures.HeadNode();
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
				mSignatures.Add(newSig);
				break;
			} else if (sig->Measure() == measure) {
				/* I need to remove this because setting the beats and
				 * beat values affects the sig's end time -- if I don't
				 * remove this, it might leave span events around.
				 */
				mSignatures.Remove(sig);
				sig->Set(sig->StartTime(), beats, beatValue);
				mSignatures.Add(sig);
				break;
			}
		}
		if (!(n->next)) {
			AmSignature*	newSig = new AmSignature();
			if (!newSig) return B_NO_MEMORY;
			newSig->Set(endTime, measure, beats, beatValue);
			mSignatures.Add(newSig);
			break;
		}
		n = n->next;
	}
	/* Now I know I have a signature and it's been added.  Run through
	 * and straighten out all the start and end times for the signatures.
	 */
//printf("BEFORE STRAIGHTENING: "); mSignatures.Print();
	straighten_out_signatures(mSignatures);
//printf("AFTER STRAIGHTENING: "); mSignatures.Print();
	/* Now report on a change starting with the time of the measure
	 * supplied to this method.
	 */
	n = mSignatures.HeadNode();
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
	MergeRangeChange(AmRange(0, 0), AmRange(0, 0), &fakeSig);
	return B_OK;
}

/* FIX:  THIS DOESN'T WORK!
 */
status_t AmSong::OffsetSignatures(AmTime start, AmTime offset)
{
	AmRange				r;
	/* Delete if necessary.
	 */
	if (offset < 0) {
		r.Set(start + offset, start);
//printf("\tcontract %lld to %lld\n", r.start, r.end);
		AmNode*			n = mSignatures.HeadNode();
		AmNode*			next = 0;
		while (n) {
			next = n->next;
			if (n->Event() && r.Overlaps(n->Event()->TimeRange()))
				mSignatures.Remove(n->Event());
			n = next;
		}
	} else {
		r.Set(start, start + offset);
	}
	/* Offset everyone and caching the starting info.
	 */
	uint32				beats = 4, beatValue = 4;
	vector<AmEvent*>	moved;
	AmNode*				n = mSignatures.HeadNode();
	if (n) {
		AmSignature*	sig = dynamic_cast<AmSignature*>(n->Event());
		if (sig) {
			beats = sig->Beats();
			beatValue = sig->BeatValue();
		}
	}
	while (n) {
		if (n->Event() && r.Overlaps(n->Event()->TimeRange()))
			moved.push_back(n->Event());
		n = n->next;
	}
	for (uint32 k = 0; k < moved.size(); k++) {
		mSignatures.SetEventStartTime(moved[k], moved[k]->StartTime() + offset);
	}
	/* Now align everyone left to the proper measure.  If there isn't
	 * anyone starting at 0, add a default.
	 */
	n = mSignatures.HeadNode();
	if (!n || n->StartTime() != 0) {
		AmSignature*	newSig = new AmSignature(0, 1, beats, beatValue);
		if (!newSig) return B_NO_MEMORY;
		mSignatures.Add(newSig);
	}
	/* Now make everyone's measure line up with their time.
	 */
	n = mSignatures.HeadNode();
	if (!n || n->StartTime() != 0) return B_ERROR;
	AmSignature*		sig = dynamic_cast<AmSignature*>(n->Event());
	if (!sig) return B_ERROR;
	int32				measure = 1;
	beats = sig->Beats();
	beatValue = sig->BeatValue();
	AmTime				sigLength = sig->Duration();
	AmRange				sigRange(sig->TimeRange());
//	AmSignature*		nextSig = 0;
//	AmNode*				nextNode = n->next;
//	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
printf("Before loop sig range %lld to %lld\n", sigRange.start, sigRange.end);
	while (sig) {
printf("\tloop sig range %lld to %lld, sig start %lld\n", sigRange.start, sigRange.end, sig->StartTime());
		if (sigRange.Contains(sig->StartTime())) {
printf("\tConvert sig (m %ld time %lld)", sig->Measure(), sig->StartTime());
			sig->Set(sigRange.start, measure, beats, beatValue);
printf("to sig (m %ld time %lld)\n", sig->Measure(), sig->StartTime());
			n = n->next;
//			nextNode = nextNode->next;
//			sig = nextSig;
//			if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
//			else nextSig = 0;
			if (n) {
				sig = dynamic_cast<AmSignature*>(n->Event());
				measure++;
				beats = sig->Beats();
				beatValue = sig->BeatValue();
				sigLength = sig->Duration();
				sigRange.Set(sigRange.end + 1, sigRange.end + sigLength);
			} else sig = 0;
		} else {
			sigRange.Set(sigRange.end + 1, sigRange.end + sigLength);
			measure++;
		}
	}
	/* To be safe.
	 */
//	straighten_out_signatures(mSignatures);

	return B_OK;
}

AmTrackRef AmSong::TempoRef() const
{
	return AmTrackRef(mTempoTrack);
}

const AmTrack* AmSong::TempoTrack() const
{
	return mTempoTrack;
}

AmTrack* AmSong::TempoTrack()
{
	return mTempoTrack;
}

const AmPhrase* AmSong::TempoPhrase() const
{
	if (!mTempoTrack) return NULL;
	AmNode*			head = mTempoTrack->Phrases().HeadNode();
	if (!head) return NULL;
	AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( head->Event() );
	if (!pe) return NULL;
	return pe->Phrase();
}

const char* AmSong::Title() const
{
	return Name();
}

void AmSong::SetTitle(const char* title, void* sender)
{
	SetDirty();
	
	if ( title == 0 ) SetName( SZ_UNTITLED_SONG );
	else SetName(title);

	BMessage	msg( AmSong::TITLE_CHANGE_OBS );
	msg.AddPointer( SZ_SONG_ID, Id() );
	if ( Name() ) msg.AddPointer( SZ_SONG_TITLE, Name() );
	if ( sender ) msg.AddPointer( SZ_SENDER, sender );

	SendNotices(msg.what, &msg);
}

// FIX:  Doesn't include any of my own MIDI events
AmTime AmSong::CountEndTime() const
{
	AmTime		songET = 0;
	for (uint32 k=0; k<mTrackRefs.size(); k++) {
		AmTime	trackET = mTrackRefs[k].mTrack->EndTime2();
		if ( trackET > songET ) songET = trackET;
	}
	return songET;
}

AmRange AmSong::RecordRange() const
{
	return mRecordRange;
}

void AmSong::SetRecordRange(AmRange range)
{
	mRecordRange = range;
}

const BUndoContext* AmSong::UndoContext() const
{
	if (mFlags&_AM_SUPPRESS_UNDO) return 0;
	return &mUndoContext;
}

BUndoContext* AmSong::UndoContext()
{
	if (mFlags&_AM_SUPPRESS_UNDO) return 0;
	return &mUndoContext;
}

bool AmSong::IsSuppressingUndo() const
{
	return mFlags&_AM_SUPPRESS_UNDO;
}

void AmSong::SetSuppressUndo(bool suppress)
{
	if (suppress) mFlags |= _AM_SUPPRESS_UNDO;
	else mFlags &= ~_AM_SUPPRESS_UNDO;
}

static status_t set_measure(AmSignature* sig, AmPhrase* sigPhrase)
{
	if (!sig || !sigPhrase) return B_ERROR;
	AmNode*			node = sigPhrase->HeadNode();
	if (!node) return B_OK;
	AmSignature*	nodeSig = dynamic_cast<AmSignature*>( node->Event() );
	if (!nodeSig) return B_OK;
	AmSignature		currentSig(*nodeSig);
	AmTime			sigLength = currentSig.Duration();
	AmSignature*	nextSig = NULL;
	AmNode*			nextNode = node->next;
	if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );

	if ( sig->StartTime() < currentSig.StartTime() ) return B_OK;
	if ( sig->StartTime() == currentSig.StartTime() ) {
		sig->SetMeasure( currentSig.Measure() );
		return B_OK;
	}

	while ( currentSig.StartTime() < sig->StartTime() ) {
		currentSig.Set( currentSig.StartTime() + sigLength,
						currentSig.Measure() + 1,
						currentSig.Beats(),
						currentSig.BeatValue() );

		if ( nextSig && ( currentSig.StartTime() == nextSig->StartTime() ) ) {
			currentSig.Set(*nextSig);
			sigLength = currentSig.Duration();
			nextNode = nextNode->next;
			if (nextNode) nextSig = dynamic_cast<AmSignature*>( nextNode->Event() );
			else nextSig = NULL;
		}
	}

	ArpASSERT( currentSig.StartTime() == sig->StartTime() );
	if ( sig->StartTime() == currentSig.StartTime() )
		sig->SetMeasure( currentSig.Measure() );
	return B_OK;
}

status_t AmSong::AddEvent(AmEvent* event)
{
	SetDirty();
	
	ArpASSERT(event);
	AmPhrase*	phrase = PhraseFor(event);
	if (!phrase) return B_ERROR;
	/* If someone's trying to set the first tempo change,
	 * then blast out the previous one.
	 */
	if (event->Type() == event->TEMPOCHANGE_TYPE && event->StartTime() == 0) {
		AmNode*		head = phrase->HeadNode();
		if (head) {
			AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( head->Event() );
			if (pe && pe->Phrase() ) {
				AmNode*	head2 = pe->Phrase()->HeadNode();
				if ( head2 && head2->StartTime() == event->StartTime() ) {
					AmEvent*	e = head2->Event();
					phrase->Remove(e);
					e->Delete();
				}
			}
		}
	}
	/* If someone's adding a signature change, then make sure the
	 * measure is set correctly.  This is necessary because standard MIDI
	 * files don't have measure information, so when files are loaded I
	 * get a bunch of signatures at measure 1.
	 */
	if (event->Type() == event->SIGNATURE_TYPE) {
		status_t	err = set_measure(dynamic_cast<AmSignature*>(event), phrase);
		if (err != B_OK) return err;
	}
	return phrase->Add(event);
}

status_t AmSong::Clear()
{
	StartLoad();
	FinishLoad();
	return B_OK;
}

void AmSong::StartLoad()
{
	SetSuppressUndo(true);
	for( uint32 k = 0; k < mTrackRefs.size(); k++ )
		TrackChangeNotice( mTrackRefs[k].TrackId(), AM_REMOVED, (int32)k );
	mTrackRefs.resize(0);
	
	if (mTempoTrack) {
		TrackChangeNotice( mTempoTrack->Id(), AM_REMOVED, 0 );
		mTempoTrack->RemoveRef();
	}
	mTempoTrack = new AmTrack(this, "Tempo");
	if (mTempoTrack) {
		mTempoTrack->AddRef();
		AmPhrase*		tempoPhrase = const_cast<AmPhrase*>( &(mTempoTrack->Phrases()) );
		if (tempoPhrase) {
			AmPhraseEvent*	pe = new AmRootPhraseEvent();
			if (pe) tempoPhrase->Add(pe);
		}
		/* Erase any secondary views.
		 */
		while (mTempoTrack->CountProperties(SEC_VIEW) > 0)
			mTempoTrack->SetProperty(0, SEC_VIEW, 0);
	}
	mSignatures.DeleteEvents();
}

void AmSong::FinishLoad()
{
	SetSuppressUndo(false);
	if (mTempoTrack) {
		AmPhrase*		tempoPhrase = const_cast<AmPhrase*>( &(mTempoTrack->Phrases()) );
		if (!tempoPhrase) SetBPM(_DEFAULT_TEMPO);
		else {
			AmNode*		n = tempoPhrase->HeadNode();
			if (!n) SetBPM(_DEFAULT_TEMPO);
			else {
				AmPhraseEvent*	pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
				if ( !pe || pe->IsEmpty() ) SetBPM(_DEFAULT_TEMPO);
			}
		}
	}
	if (mSignatures.IsEmpty()) {
		AmSignature		*ms;
		if ( (ms = new AmSignature(0)) != 0 ) {
			ms->Set(0, 1, 4, 4);
			AddEvent( ms );
		}
	}
	ClearDirty();
}

void AmSong::SetChangedTrack(track_id id)
{
	if (mChangedTrack == 0) mChangedTrack = id;
	else if (mChangedTrack != id) mChangedTrack = 0;
}

void AmSong::SetWindow(BMessenger m)
{
	mWindow = m;
}

	/*---------------------------------------------------------
	 * TRACK ACCESSING
	 *---------------------------------------------------------*/

status_t AmSong::TrackRefForId(track_id id, AmTrackRef& ref) const
{
	for (uint32 k=0; k<mTrackRefs.size(); k++) {
		if ( mTrackRefs[k].mTrack->Id() == id ) {
			ref.SetTo( mTrackRefs[k].mTrack );
			return B_OK;
		}
	}
	if (mTempoTrack && (mTempoTrack->Id() == id)) {
		ref.SetTo(mTempoTrack);
		return B_OK;
	}
	return B_ERROR;
}

status_t AmSong::TrackRefForIndex(uint32 index, AmTrackRef& ref) const
{
	if (index >= mTrackRefs.size() ) return B_ERROR;
	ref.SetTo( mTrackRefs[index].mTrack );
	return B_OK;
}

const AmTrack* AmSong::Track(AmTrackRef trackRef) const
{
	return Track(trackRef.TrackId() );
}

AmTrack* AmSong::Track(AmTrackRef trackRef)
{
	return Track(trackRef.TrackId() );
}

const AmTrack* AmSong::Track(uint32 index) const
{
	if (index >= mTrackRefs.size() ) return 0;
	return mTrackRefs[index].mTrack;
}

AmTrack* AmSong::Track(uint32 index)
{
	if ( index >= mTrackRefs.size() ) return 0;
	return mTrackRefs[index].mTrack;
}

const AmTrack* AmSong::Track(track_id id) const
{
	for (uint32 k=0; k<mTrackRefs.size(); k++)
		if( mTrackRefs[k].TrackId() == id )
			return mTrackRefs[k].mTrack;
	if (mTempoTrack && (mTempoTrack->Id() == id)) 
		return mTempoTrack;
	return 0;
}

AmTrack* AmSong::Track(track_id id)
{
	for (uint32 k=0; k<mTrackRefs.size(); k++)
		if( mTrackRefs[k].TrackId() == id )
			return mTrackRefs[k].mTrack;
	if (mTempoTrack && (mTempoTrack->Id() == id)) 
		return mTempoTrack;
	return 0;
}

int32 AmSong::TrackIndex(track_id id) const
{
	for (uint32 k=0; k<mTrackRefs.size(); k++)
		if (mTrackRefs[k].TrackId() == id)
			return int32(k);
	return -1;
}

uint32 AmSong::CountTracks() const
{
	return mTrackRefs.size();
}

status_t AmSong::AddTrack(int32 height, int32 index)
{
	AmTrack*		track = new AmTrack(this, SZ_UNTITLED_TRACK);
	if (!track) return B_NO_MEMORY;
	track->SetPhraseHeight(height);
	return AddTrack(track, index);
}

status_t AmSong::AddTrack(AmTrack* track, int32 index)
{
	return AddTrack(track, UndoContext(), index);
}

status_t AmSong::RemoveTrack(track_id id)
{
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == id) {
			return RemoveTrack( k, UndoContext() );
		}
	}
	return B_ERROR;
}

status_t AmSong::RemoveTrack(uint32 index)
{
	return RemoveTrack( index, UndoContext() );
}

status_t AmSong::MoveTrackBy(track_id tid, int32 delta)
{
	return MoveTrackBy(tid, delta, UndoContext() );
}

status_t AmSong::AddTrack(AmTrack* track, BUndoContext* undo, int32 index)
{
	ArpASSERT(track);
	SetDirty();
	bool		pushBack = false;
	if (index < 0 || index >= (int32)mTrackRefs.size() ) pushBack = true;
	if (undo) {
		undo->SuggestUndoName("Add Track");
		int32		undoIndex = (pushBack) ? (int32)mTrackRefs.size() : index;
		undo->AddOperation(new AmSongTrackUndo(this, track, true, undoIndex), BResEditor::B_ANY_UNDO_MERGE);
	}
	if (pushBack) {
		mTrackRefs.push_back( AmTrackRef(track) );
		TrackChangeNotice( track->Id(), AM_ADDED, (int32)mTrackRefs.size() - 1);
	} else {
		mTrackRefs.insert( mTrackRefs.begin() + index, AmTrackRef(track) );
		TrackChangeNotice( track->Id(), AM_ADDED, index );
	}
	return B_OK;
}

static void remove_connections(	AmSong* song,
								AmPipelineType type,
								AmFilterHolder* h)
{
	while (h) {
		h->BreakConnections(song, type);
		h = dynamic_cast<AmFilterHolder*>(h->NextInLine() );
	}
}

status_t AmSong::RemoveTrack(int32 index, BUndoContext* undo)
{
	if (index >= (int32)mTrackRefs.size() ) return B_ERROR;
	SetDirty();
	{
		BAutolock		l(mPulseAccess);
		delete mPulseFilters;
		mPulseFilters = 0;
		mPulseFiltersChanged = true;
	}

	/* Remove any connections the filters might have.
	 */
	AmFilterHolder*		h = dynamic_cast<AmFilterHolder*>(mTrackRefs[index].mTrack->Filter(INPUT_PIPELINE));
	if (h) remove_connections(this, INPUT_PIPELINE, h);
	h = dynamic_cast<AmFilterHolder*>(mTrackRefs[index].mTrack->Filter(OUTPUT_PIPELINE));
	if (h) remove_connections(this, OUTPUT_PIPELINE, h);
	
	if (undo) {
		undo->SuggestUndoName("Remove Track");
		undo->AddOperation(new AmSongTrackUndo(this, mTrackRefs[index].mTrack, false, index), BResEditor::B_ANY_UNDO_MERGE);
	}

	AmRange		range( 0, mTrackRefs[index].mTrack->EndTime2() );
	track_id	id = mTrackRefs[index].TrackId();
	mTrackRefs.erase(mTrackRefs.begin() + index);
	TrackChangeNotice(id, AM_REMOVED, index);
	MergeRangeChange(range, range);
	return B_OK;
}

status_t AmSong::MoveTrackBy(track_id tid, int32 delta, BUndoContext* undo)
{
	SetDirty();

	AmTrackRef	ref;
	int32		origPos = -1, newPos = -1;
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == tid) {
			ref = mTrackRefs[k];
			origPos = k;
			newPos = k + delta;
			mTrackRefs.erase(mTrackRefs.begin() + k);
			if (delta >= 0) TrackChangeNotice(ref.TrackId(), AM_CHANGED, (int32)k);
			break;
		}
	}
	if (ref.IsValid() && origPos >= 0 && newPos >= 0) {
		if (newPos >= int32(mTrackRefs.size()) ) mTrackRefs.push_back(ref);
		else mTrackRefs.insert(mTrackRefs.begin() + newPos, ref);
		if (undo) {
			undo->AddOperation(new AmSongTrackMoveUndo(this, ref.TrackId(), origPos, newPos), BResEditor::B_ANY_UNDO_MERGE);
		}
		if (delta < 0) TrackChangeNotice(ref.TrackId(), AM_CHANGED, newPos);
	}
	return B_OK;
}

	/*---------------------------------------------------------
	 * PLAYBACK
	 *---------------------------------------------------------*/

status_t AmSong::DoStartPlaying(AmTime startTime, AmTime stopTime) const
{
	return DoStartPlaying(AmTrackRef(), startTime, stopTime);
}

status_t AmSong::DoStartPlaying(const AmTrackRef& solo, AmTime startTime, AmTime stopTime) const
{
	DoStopTransport(AmFilterI::TRANSPORT_CONTEXT);
	return mTransport->Start(solo, startTime, stopTime);
}

status_t AmSong::DoStartRecording(AmTime startTime, AmTime stopTime)
{
	return DoStartRecording(AmTrackRef(), startTime, stopTime);
}

status_t AmSong::DoStartRecording(const AmTrackRef& solo, AmTime startTime, AmTime stopTime)
{
	DoStopTransport(AmFilterI::TRANSPORT_CONTEXT);
	
	// WRITE SONG BLOCK
	WriteLock();
	for (uint32 k=0; k<mTrackRefs.size(); k++) {
		if (!solo.IsValid() || mTrackRefs[k] == solo) {
			if (mTrackRefs[k].mTrack) mTrackRefs[k].mTrack->StartRecordingPhrase();
		}
	}
	WriteUnlock();
	// END WRITE SONG BLOCK
	
	status_t result = mInputQueue->StartRecording();
	
	if (result >= B_OK) {
		if (stopTime < 0) {
			// In the case of recording, the default stop time is to
			// go forever.
			stopTime = 0x1fffffff;	// a really long time.
		}
		
		result = mTransport->Start(solo, startTime, stopTime);
	}
	
	return result;
}

void AmSong::DoStopTransport(uint32 context) const
{
	const bool recording = mInputQueue->IsRecording();
	mInputQueue->StopRecording();
	
	mTransport->Stop(context);
	
	if (recording) {
		bool		added = false;
		// WRITE SONG BLOCK
		const_cast<AmSong*>(this)->WriteLock();
		for (uint32 k=0; k<mTrackRefs.size(); k++) {
			if (mTrackRefs[k].mTrack) {
				bool	add = mTrackRefs[k].mTrack->StopRecordingPhrase();
				if (add) added = true;
			}
		}
		if ( added && UndoContext() ) {
			const_cast<AmSong*>(this)->UndoContext()->SetUndoName("Record Events");
			const_cast<AmSong*>(this)->UndoContext()->CommitState();
		}
		const_cast<AmSong*>(this)->WriteUnlock();
		// END WRITE SONG BLOCK
	}
}

static void start_filter_holder(AmFilterHolderI* holder)
{
	while (holder) {
		if (holder->Filter()) holder->Filter()->Start(holder->Filter()->TRANSPORT_CONTEXT);
		holder = holder->NextInLine();
	}
}

void AmSong::StartFiltersHack() const
{
	pipeline_id		pid = NULL;
	for (uint32 pindex = 0; (pid = PipelineId(pindex)) != NULL; pindex++) {
		start_filter_holder(Filter(pid, INPUT_PIPELINE));
		start_filter_holder(Filter(pid, THROUGH_PIPELINE));
		start_filter_holder(Filter(pid, OUTPUT_PIPELINE));
	}
}

AmTime AmSong::RealtimeToPulse(bigtime_t time) const
{
	return -1;
}

bigtime_t AmSong::PulseToRealtime(AmTime pulse) const
{
	return -1;
}

AmTransport& AmSong::Transport() const
{
	return *mTransport;
}

bool AmSong::IsRecording() const
{
	return mInputQueue->IsRecording();
}

ArpRef<AmInputQueue> AmSong::InputQueue() const
{
	return mInputQueue;
}

AmEvent* AmSong::PlaybackList(AmTime startTime, AmTime stopTime, uint32 flags) const
{
	ArpASSERT(startTime >= 0 && startTime <= stopTime);
	
	AmEvent*		event = 0;
	
	if ((flags&PLAYBACK_NO_TEMPO) == 0) {
		MergeTemposInto(&event, startTime, stopTime, flags);
	}
	
	if ((flags&PLAYBACK_NO_SIGNATURE) == 0) {
		MergeSignaturesInto(&event, startTime, stopTime, flags);
	}
	
	if ((flags&PLAYBACK_NO_PERFORMANCE) == 0) {
		/* If solo is turned on for any of the tracks, then only
		 * play the tracks that are solo'ed.
		 */
		bool			solo = false;
		for (uint32 k = 0; k < mTrackRefs.size(); k++) {
			if (mTrackRefs[k].mTrack->ModeFlags() & AmTrack::SOLO_MODE) {
				solo = true;
				break;
			}
		}
	
		for (uint32 k=0; k<mTrackRefs.size(); k++) {
			uint32		flags = mTrackRefs[k].mTrack->ModeFlags();
			if (solo) {
				if (flags&AmTrack::SOLO_MODE)
					MergeTrackInto(mTrackRefs[k].mTrack, &event, startTime, stopTime, flags);
			} else if (!(flags&AmTrack::MUTE_MODE) )
				MergeTrackInto(mTrackRefs[k].mTrack, &event, startTime, stopTime, flags);
		}
	}
	
	if (event) event = event->HeadEvent();
	return event;
}

status_t AmSong::WindowMessage(BMessage* msg)
{
	if (!mWindow.IsValid()) return B_ERROR;
	return mWindow.SendMessage(msg);
}

void AmSong::FlushChanges()
{
	AmNotifier::FlushChanges();
	if (!mTrackChangeMsg.IsEmpty() )
		ReportMsgChange(&mTrackChangeMsg, BMessenger() );
	mTrackChangeMsg.MakeEmpty();
}

AmPhrase* AmSong::PhraseFor(AmEvent* event) const
{
	if (event->Type() == event->SIGNATURE_TYPE) {
		return (AmPhrase*)&mSignatures;
	} else if (event->Type() == event->TEMPOCHANGE_TYPE) {
		if (!mTempoTrack) return NULL;
		/* Ick!  The tempo track must always store a single phrase event
		 * to contain all its tempos -- this little bit of ugliness
		 * makes sure that phrase is there.
		 */
		AmPhrase*	p = const_cast<AmPhrase*>( &(mTempoTrack->Phrases()) );
		if (!p) return NULL;
		AmNode*		n = p->HeadNode();
		AmPhraseEvent*	pe = NULL;
		if (n) pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
		if (pe) return pe->Phrase();
		pe = new AmRootPhraseEvent();
		if (!pe) return NULL;
		p->Add(pe);
		return pe->Phrase();
	} else {
		return 0;
	}
}

void AmSong::AnnotateMessage(BMessage& msg) const
{
	msg.AddPointer(SZ_SONG_ID, Id() );
	msg.AddPointer(MATRIX_ID_STR, Id() );
	if (mChangedTrack != 0) {
		if (msg.what == AmNotifier::PIPELINE_CHANGE_OBS) {
			msg.AddInt32("pipeline_index", TrackIndex(mChangedTrack) );
		} else if (msg.what == AmNotifier::FILTER_CHANGE_OBS) {
			msg.AddInt32("pipeline_index", TrackIndex(mChangedTrack) );
		}
	}
}

void AmSong::MergeTemposInto(AmEvent** eventList,
							 AmTime startTime, AmTime stopTime, uint32 flags) const
{
	ArpASSERT(startTime >= 0 && startTime <= stopTime);
	if (!mTempoTrack) return;
	AmNode*		head = mTempoTrack->Phrases().HeadNode();
	if (!head) return;
	AmPhraseEvent* pe = dynamic_cast<AmPhraseEvent*>( head->Event() );
	if (!pe || !pe->Phrase() ) return;

	/* First, if context is requested, put into playback list
	 * closest tempo event before the start time.
	 */
	if (!(flags&PLAYBACK_NO_CONTEXT)) {
		AmNode* context = pe->Phrase()->FindNode(startTime, BACKWARDS_SEARCH);
		if (context && context->Event() && context->Event()->StartTime() < startTime) {
			AmEvent* event = context->Event()->Copy();
			if (!(flags&PLAYBACK_RAW_CONTEXT))
				event->SetStartTime(startTime);
			if (!(*eventList)) *eventList = event;
			else (*eventList)->MergeEvent(event);
		}
	}
	
	/* Put tempo events in time range into playback list.
	 */
	pe->Phrase()->MergeInto(eventList, NULL, NULL, startTime, stopTime);
}

void AmSong::MergeSignaturesInto(AmEvent** eventList,
								 AmTime startTime, AmTime stopTime, uint32 flags) const
{
	ArpASSERT(startTime >= 0 && startTime <= stopTime);
	
	/* First, if context is requested, put into playback list
	 * closest tempo event before the start time.
	 */
	if (!(flags&PLAYBACK_NO_CONTEXT)) {
		AmNode* context = mSignatures.FindNode(startTime, BACKWARDS_SEARCH);
		if (context && context->Event() && context->Event()->StartTime() < startTime) {
			AmEvent* event = context->Event()->Copy();
			if (!(flags&PLAYBACK_RAW_CONTEXT))
				event->SetStartTime(startTime);
			if (!(*eventList)) *eventList = event;
			else (*eventList)->MergeEvent(event);
		}
	}
	
	/* Put tempo events in time range into playback list.
	 */
	mSignatures.MergeInto(eventList, NULL, NULL, startTime, stopTime);
}

void AmSong::MergeTrackInto(AmTrack* track,
							AmEvent** event,
							AmTime startTime,
							AmTime stopTime,
							uint32 flags) const
{
	AmEvent*	trackEvents = track->RawPlaybackList(startTime, stopTime, flags);
	if (trackEvents == 0) return;

	if (*event == 0) *event = trackEvents;
	else (*event)->MergeList(trackEvents, true);
}

void AmSong::TrackChangeNotice(track_id id, AmStatus status, int32 position)
{
	mTrackChangeMsg.what = AmSong::TRACK_CHANGE_OBS;
	mTrackChangeMsg.AddPointer(SZ_TRACK_ID, id);
	mTrackChangeMsg.AddInt32(SZ_STATUS, status);
	mTrackChangeMsg.AddInt32("position", position);
}

const AmTrack* AmSong::TrackForPipeline(pipeline_id id) const
{
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == id) return mTrackRefs[k].mTrack;
	}
	return NULL;
}

AmTrack* AmSong::TrackForPipeline(pipeline_id id)
{
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (mTrackRefs[k].TrackId() == id) return mTrackRefs[k].mTrack;
	}
	return NULL;
}

AmPipelineSegment* AmSong::PipelineSegment(pipeline_id pid, AmPipelineType type)
{
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		if (pid == mTrackRefs[k].TrackId() ) return mTrackRefs[k].mTrack->PipelineSegment(type);
	}
	return NULL;
}

status_t AmSong::CachePulseFilters()
{
	/* NOTE:  NOT LOCKED!  THE CALLER MUST HAVE LOCKED mPulseAccess!
	 */
	delete mPulseFilters;
	mPulseFilters = new _AmPulseFilters();
	if (!mPulseFilters) return B_NO_MEMORY;
	for (uint32 k = 0; k < mTrackRefs.size(); k++) {
		mTrackRefs[k].mTrack->AddPulseFilters(this);
	}
	mPulseFiltersChanged = false;
	return B_OK;
}

bool AmSong::SendPulse()
{
//printf("SendPulse 1\n");
	BAutolock		l(mPulseAccess);
	if (!mPulseFilters || mPulseFilters->holders.size() < 1) return false;
//printf("SendPulse 2\n");
	/* FIX:  Does the song need to be locked for this??  Seems
	 * like it should.
	 */
	for (uint32 k = 0; k < mPulseFilters->holders.size(); k++) {
		AmFilterHolderI*		h = mPulseFilters->holders[k];
		if (h) {
			AmFilterI*			f = h->Filter();
			if (f) {
				AmEvent*		e = f->OscPulse(0, 0, 0);
				if (e) h->GenerateEvents(e);
			}
		}
	}
	return true;
}

int32 AmSong::PulseThreadEntry(void* arg)
{
	AmSong*		song = (AmSong*)arg;
	if (!song) return B_ERROR;

	while (song->SendPulse()) {
		snooze(20000);
	}

	return B_OK;
}

status_t AmSong::AddPulseFilter(AmFilterHolderI* holder)
{
	ArpVALIDATE(holder && mPulseFilters, return B_ERROR);
	return mPulseFilters->Add(holder);
}

// #pragma mark -

/**********************************************************************
 * _AM-PULSE-FILTERS
 **********************************************************************/
_AmPulseFilters::_AmPulseFilters()
{
}

_AmPulseFilters::~_AmPulseFilters()
{
	for (uint32 k = 0; k < holders.size(); k++) {
		if (holders[k]) holders[k]->DecRefs();
	}
	holders.resize(0);
}

status_t _AmPulseFilters::Add(AmFilterHolderI* holder)
{
	ArpASSERT(holder);
	holders.push_back(holder);
	holder->IncRefs();
	return B_OK;
}