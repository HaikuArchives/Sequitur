/* AmFilterHolder.cpp
*/

#define _BUILDING_AmKernel 1

#include "AmKernel/AmFilterHolder.h"


#ifndef AMPUBLIC_AMGLOBALSI_H
#include "AmPublic/AmGlobalsI.h"
#endif

#ifndef AMPUBLIC_AMPIPELINEMATRIXI_H
#include "AmPublic/AmPipelineMatrixI.h"
#endif

#ifndef AMPUBLIC_AMPREFSI_H
#include "AmPublic/AmPrefsI.h"
#endif

#ifndef AMPUBLIC_AMSONGREF_H
#include "AmPublic/AmSongRef.h"
#endif

#ifndef AMKERNEL_AMFILTERROSTER_H
#include "AmKernel/AmFilterRoster.h"
#endif

#ifndef AMKERNEL_AMSONG_H
#include "AmKernel/AmSong.h"
#endif

#ifndef AMKERNEL_AMTRACK_H
#include "AmKernel/AmTrack.h"
#endif

#ifndef AMKERNEL_AMTRACKLOOKAHEAD_H
#include "AmKernel/AmTrackLookahead.h"
#endif

#ifndef AMKERNEL_AMTRANSPORT_H
#include "AmKernel/AmTransport.h"
#endif

#ifndef ARPVIEWSPUBLIC_ARPPREFSI_H
#include "ArpViewsPublic/ArpPrefsI.h"
#endif

#ifndef ARPKERNEL_ARPBITMAPTOOLS_H
#include <ArpKernel/ArpBitmapTools.h>
#endif

#ifndef ARPKERNEL_ARPDEBUG_H
#include <ArpKernel/ArpDebug.h>
#endif

#include "AmKernel/AmFileRosters.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <map>

ArpMOD();

#define NOISY 0

class AmFilterUndo : public BUndoOperation
{
public:
	AmFilterUndo(AmPipelineMatrixI* matrix, track_id track, AmFilterHolder* owner)
		: mMatrix(matrix), mTrack(track), mFilter(owner)
	{
		mFilter->IncRefs();
		mFilter->GetConfigurationDirect(matrix, &mLastConfig);
	}
	
	virtual ~AmFilterUndo()
	{
		mFilter->DecRefs();
	}
	
	virtual const void* Owner() const
	{
		return mFilter;
	}
	
	virtual bool HasData() const
	{
		return !mLastConfig.IsEmpty();
	}
	
	virtual bool MatchOwnerContext(const void* owner) const
	{
		return owner == mFilter || owner == mTrack;
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

	#if 0
	void UpdateConfig(const BMessage* values)
	{
		const int32 N = values->CountNames(B_ANY_TYPE);
		const char* name;
		type_code type;
		int32 count;
		for (int32 i=0; i<values->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++) {
			if (mLastConfig.GetInfo(type, name)) continue;
		}
		
	}
	#endif
	
private:
	void swap_data()
	{
		BMessage old;
		mFilter->GetConfigurationDirect(mMatrix, &old);
		mFilter->PutConfigurationDirect(mMatrix, &mLastConfig);
		mLastConfig = old;
	}
	
	AmPipelineMatrixI* const	mMatrix;
	const track_id				mTrack;
	AmFilterHolder*				mFilter;
	BMessage					mLastConfig;
};

/* ----------------------------------------------------------------
   AmFilterHolder Implementation
   ---------------------------------------------------------------- */

AmFilterHolder::AmFilterHolder(	pipeline_matrix_id matrix,
								track_id track,
								pipeline_id pipeline,
								AmInputTarget* inputTarget)
	: mMatrix(matrix), mTrack(track), mPipeline(pipeline),
	  mInputTarget(inputTarget), mFilter(NULL), mNextInLine(NULL),
	  mRemoved(false), mBypassed(false), mSuppressNextInLine(false)
{
}

AmFilterHolder::~AmFilterHolder()
{
//	AM_LOG("AmFilterHolder::~AmFilterHolder()\n");
	SetFilter(0);
	EmptyConnections();
}

void AmFilterHolder::Delete()
{
//	AM_LOG("AmFilterHolder::Delete()\n");
	EmptyConnections();

	AmFilterHolder*		cur = mNextInLine;
	while (cur) {
		AmFilterHolder*	next = dynamic_cast<AmFilterHolder*>(cur->NextInLine() );
		cur->EmptyConnections();
		cur->AmSafeDelete::Delete();
		cur = next;
	}
	AmSafeDelete::Delete();
#if 0
	AmFilterHolder*		cur = mNextInLine;
	while (cur) {
		AmFilterHolder*	next = dynamic_cast<AmFilterHolder*>(cur->ConnectionAt() );
		cur->EmptyConnections();
		cur->AmSafeDelete::Delete();
		cur = next;
	}

	cur = this;
	while (cur) {
		AmFilterHolder* next = dynamic_cast<AmFilterHolder*>(cur->FirstConnection());
		cur->EmptyConnections();
		cur->AmSafeDelete::Delete();
		cur = next;
	}
#endif
}

status_t AmFilterHolder::GetConfiguration(BMessage* values) const
{
	status_t err = B_ERROR;
	
	if (mFilter) {
		AmPipelineMatrixRef matrixRef(AmGlobals().PipelineMatrixRef(mMatrix));
		// READ MATRIX BLOCK
		#ifdef AM_TRACE_LOCKS
		printf("AmFilterHolder::GetConfiguration() read lock\n");
		#endif
		const AmPipelineMatrixI* matrix = matrixRef.ReadLock();
		if (matrix) {
			err = GetConfigurationDirect(matrix, values);
			matrixRef.ReadUnlock(matrix);
		}
		// END READ MATRIX BLOCK
	}
	return B_OK;
}

status_t AmFilterHolder::PutConfiguration(const BMessage* values)
{
	status_t err = B_ERROR;
	if (mFilter) {
		AmPipelineMatrixRef matrixRef(AmGlobals().PipelineMatrixRef(mMatrix));
		// WRITE MATRIX BLOCK
		AmPipelineMatrixI* matrix = matrixRef.WriteLock(NULL);
		if (matrix) {
			matrix->SetDirty();
			
			BUndoContext* u = matrix->UndoContext();
			if (u) {
				BString name(mFilter->Label());
				name += " Settings";
				u->SuggestUndoName(name.String());
			
				if (u->LastOperation(this, B_UNIQUE_UNDO_MERGE) == NULL)
					u->AddOperation(new AmFilterUndo(matrix, mTrack, this));
			}
					
			err = PutConfigurationDirect(matrix, values);
			matrix->FilterChanged(mTrack, _NUM_PIPELINE);
			matrixRef.WriteUnlock(matrix);
		}
		// END WRITE MATRIX BLOCK
	}
	return err;
}

status_t AmFilterHolder::GetConfigurationDirect(const AmPipelineMatrixI* matrix, BMessage* values) const
{
	status_t err = B_ERROR;
	
	if (mFilter) {
		err = mFilter->GetConfiguration(values);
	}
	return B_OK;
}

status_t AmFilterHolder::PutConfigurationDirect(AmPipelineMatrixI* matrix, const BMessage* values)
{
	status_t err = B_ERROR;
	
	if (mFilter) {
		err = mFilter->PutConfiguration(values);
		if (err == B_OK) ReportChange(values);
	}
	return err;
}

void AmFilterHolder::CommitUndoState()
{
	if (mFilter) {
		AmPipelineMatrixRef matrixRef(AmGlobals().PipelineMatrixRef(mMatrix));
		// WRITE MATRIX BLOCK
		AmPipelineMatrixI* matrix = matrixRef.WriteLock("Filter Settings");
		if (matrix) {
			BUndoContext* undoContext = matrix->UndoContext();
			if (undoContext) undoContext->CommitState(this);
			matrixRef.WriteUnlock(matrix);
		}
		// END WRITE MATRIX BLOCK
	}
}

status_t AmFilterHolder::Configure(ArpVectorI<BView*>& views)
{
	if (mFilter) return mFilter->Configure(views);
	return B_OK;
}

AmFilterI* AmFilterHolder::Filter() const
{
	return mFilter;
}

AmFilterAddOn::type AmFilterHolder::Type() const
{
	return mType;
}

const pipeline_matrix_id AmFilterHolder::MatrixId() const
{
	return mMatrix;
}

const pipeline_id AmFilterHolder::PipelineId() const
{
	return mPipeline;
}

const track_id AmFilterHolder::TrackId() const
{
	return mTrack;
}

ArpCRef<AmDeviceI> AmFilterHolder::TrackDevice() const
{
	ArpCRef<AmDeviceI>		device = NULL;
	if (!mMatrix || !mTrack) return device;
	AmSongRef	songRef(AmGlobals().SongRef(mMatrix));
	#ifdef AM_TRACE_LOCKS
	printf("AmFilterHolder::TrackDevice() read lock\n");
	#endif
	const AmSong* song = songRef.ReadLock();
	if (song) {
		const AmTrack*		track = song->Track(mTrack);
		if (track) device = track->Device();
		songRef.ReadUnlock(song);
	}
	return device;
}

bool AmFilterHolder::IsBypassed() const
{
	return mBypassed;
}

void AmFilterHolder::SetBypassed(bool bypass)
{
	mBypassed = bypass;
}

bool AmFilterHolder::IsSuppressingNextInLine() const
{
	return mSuppressNextInLine;
}

void AmFilterHolder::SetSuppressNextInLine(bool suppress)
{
	mSuppressNextInLine = suppress;
}

AmTime AmFilterHolder::RealtimeToPulse(bigtime_t time) const
{
	if (!mMatrix) return -1;
	
	// Try not to lock the song.
	AmSongRef songRef(AmGlobals().SongRef(mMatrix));
	AmTransport* t = songRef.Transport();
	AmTime pulse = t ? t->RealtimeToPulse(time, true) : -1;
	
	if (pulse < 0) {
		#ifdef AM_TRACE_LOCKS
		printf("AmFilterHolder::RealtimeToPulse() read lock\n");
		#endif
		const AmSong* song = songRef.ReadLock();
		if( song ) {
			pulse = song->RealtimeToPulse(time);
			songRef.ReadUnlock(song);
		}
	}
	
	//printf("Converted realtime %Ld to pulse %Ld\n", time, pulse);
	return pulse;
}

bigtime_t AmFilterHolder::PulseToRealtime(AmTime pulse) const
{
	if (!mMatrix) return -1;
	
	// Try not to lock the song.
	AmSongRef songRef(AmGlobals().SongRef(mMatrix));
	AmTransport* t = songRef.Transport();
	bigtime_t time = t ? t->PulseToRealtime(pulse, true) : -1;
	
	if (time < 0) {
		#ifdef AM_TRACE_LOCKS
		printf("AmFilterHolder::PulseToRealtime() read lock\n");
		#endif
		const AmSong* song = songRef.ReadLock();
		if( song ) {
			time = song->PulseToRealtime(pulse);
			songRef.ReadUnlock(song);
		}
	}
	return time;
}

status_t AmFilterHolder::GenerateEvents(AmEvent* events)
{
	if (!events) {
		return B_OK;
	}
	
	if (mInputTarget == NULL || mRemoved || mBypassed) {
		events->DeleteChain();
		return B_OK;
	}
	
	AmEvent* pos = events;
	while (pos) {
		pos->SetNextFilter(this);
		pos = pos->NextEvent();
	}
	
	mInputTarget->HandleEvents(events);
	
	return B_OK;
}

uint32 AmFilterHolder::CountConnections() const
{
	uint32		pad = 1;
	if (mSuppressNextInLine) pad = 0;
	return pad + uint32(mConnections.CountItems() );
}

AmFilterHolderI* AmFilterHolder::ConnectionAt(uint32 index) const
{
	if (mSuppressNextInLine) return (AmFilterHolderI*)mConnections.ItemAt(index);
	if (index == 0) return mNextInLine;
	return (AmFilterHolderI*)mConnections.ItemAt(index - 1);
}

AmFilterHolderI* AmFilterHolder::FirstConnection() const
{
	if (!mSuppressNextInLine && mNextInLine) return mNextInLine;
	return (AmFilterHolderI*)mConnections.ItemAt(0);
}

AmFilterHolderI* AmFilterHolder::NextInLine() const
{
	return mNextInLine;
}

void AmFilterHolder::FilterChangeNotice()
{
	/* If I've got a track (i.e. pipeline), then I can just send out
	 * a notification based on that.  If I don't, then unfortunately I
	 * have to rifle through every track in the matrix to find the one
	 * this filter resides in.
	 * FIX:  Currently this is a write lock.  This is because 
	 * AmTrack::MergeFilterChange() (which might be called as a result
	 * of FilterChanged()) makes a change to a variable in the track.
	 * That slight little danger is enough to prevent the FilterChanged()
	 * method from being const, so clearly I need a better way to deal
	 * with this -- maybe move that variable into the notifier and
	 * protect it with the notifier's lock.
	 */
	AmPipelineMatrixRef matrixRef(AmGlobals().PipelineMatrixRef(mMatrix));
	// WRITE MATRIX BLOCK
	#ifdef AM_TRACE_LOCKS
	printf("AmFilterHolder::FilterChangeNotice() write lock\n");
	#endif
	AmPipelineMatrixI* matrix = matrixRef.WriteLock();
	if (matrix) {
		if (mTrack) matrix->FilterChanged(mTrack, _NUM_PIPELINE);
		else {
			pipeline_id		id;
			pipeline_id		foundId = 0;
			AmPipelineType	foundType = _NUM_PIPELINE;
			for (uint32 k = 0; (id = matrix->PipelineId(k)) != 0; k++) {
				for (int32 t = INPUT_PIPELINE; t < _NUM_PIPELINE; t++) {
					AmFilterHolderI*	filt = matrix->Filter(id, (AmPipelineType)t, mFilter->Id() );
					if (filt) {
						foundId = id;
						foundType = (AmPipelineType)t;
					}
					if (foundId && foundType) break;
				}
				if (foundId && foundType) break;
			}
			if (foundId && foundType) matrix->FilterChanged(foundId, foundType);
		}
	}
	matrixRef.WriteUnlock(matrix);
	// END WRITE MATRIX BLOCK
}

void AmFilterHolder::SetFilter(AmFilterI* filter)
{
	AmDeviceRoster*				roster = AmDeviceRoster::Default();

	if (mFilter) {
		if (roster && mFilter->Flags()&mFilter->WATCH_DEVICE_FLAG)
			roster->RemoveFilterObserver(this);
		delete mFilter;
	}

	mFilter = filter;
	if (mFilter) {
		mType = mFilter->AddOn()->Type();
		if (roster && mFilter->Flags()&mFilter->WATCH_DEVICE_FLAG)
			roster->AddFilterObserver(this);
	} else mType = AmFilterAddOn::THROUGH_FILTER;
}

void AmFilterHolder::SetRemoved(bool state)
{
	mRemoved = state;
}

ArpRef<AmInputTarget> AmFilterHolder::InputTarget() const
{
	return mInputTarget;
}

uint32 AmFilterHolder::FilterFlags() const
{
	return mFilter ? mFilter->Flags() : 0;
}

void AmFilterHolder::SetConfigWindow(BMessenger who)
{
	if (mConfigWindow != who) {
		mConfigWindow = who;
		CommitUndoState();
	}
}

BMessenger AmFilterHolder::ConfigWindow() const
{
	return mConfigWindow;
}

status_t AmFilterHolder::SetNextInLine(AmFilterHolder* item)
{
	AmFilterHolder* old = mNextInLine;
	mNextInLine = item;
	if (old) old->mPredecessors.RemoveItem(this);
	if (item) item->mPredecessors.AddItem(this);
	return B_OK;
}

status_t AmFilterHolder::AddConnection(AmFilterHolder* item)
{
	ArpVALIDATE(item != NULL, return B_ERROR);
	if (mConnections.AddItem(item) ) {
		item->mPredecessors.AddItem(this);
//		FilterChangeNotice();
		return B_OK;
	}
	return B_ERROR;
}

status_t AmFilterHolder::AddConnection(AmFilterHolder* item, int32 atIndex)
{
	ArpVALIDATE(item != NULL, return B_ERROR);
	if (mConnections.AddItem(item, atIndex) ) {
		item->mPredecessors.AddItem(this);
//		FilterChangeNotice();
		return B_OK;
	}
	return B_ERROR;
}

AmFilterHolder* AmFilterHolder::RemoveConnection(filter_id filterId)
{
	AmFilterHolder*		item = NULL;
	int32				count = mConnections.CountItems();
	for (int32 k = 0; k < count; k++) {
		item = (AmFilterHolder*)mConnections.ItemAt(k);
		if (item && item->Filter() && item->Filter()->Id() == filterId) {
			mConnections.RemoveItem(k);
			item->mPredecessors.RemoveItem(this);
//			FilterChangeNotice();
			break;
		}
		item = NULL;
	}

	/* The ONLY time I suppress the next in line is when
	 * a filter with only 1 allowable connection is connected
	 * to someone other than it's next in line.  If I'm disconnecting,
	 * then either I have a filter with more than 1 connection or
	 * I want that filter to go back to looking at its next in
	 * line, so no check is necessary, just stop suppressing.
	 */
	SetSuppressNextInLine(false);

	return item;
}

status_t AmFilterHolder::RemoveConnections(int32 index, int32 count)
{
	for (int i = index; i < index + count; i++) {
		AmFilterHolder* item = (AmFilterHolder*)mConnections.ItemAt(i);
		ArpVALIDATE(item != NULL, continue);
		item->mPredecessors.RemoveItem(this);
	}
	if (mConnections.RemoveItems(index, count) ) {
//		FilterChangeNotice();
		return B_OK;
	}
	return B_ERROR;
}

status_t AmFilterHolder::RemoveConnections()
{
	return RemoveConnections(0, mConnections.CountItems() );
}

status_t AmFilterHolder::RemoveSelf()
{
	AmFilterHolder*		h;
	while ((h = (AmFilterHolder*)(mPredecessors.ItemAt(0))) != NULL) {
		h->RemoveConnection(this);
	}
	if (mNextInLine) mNextInLine->mPredecessors.RemoveItem(this);
	mNextInLine = NULL;
	int32				count = mConnections.CountItems();
	for (int32 k = 0; k < count; k++) {
		if ((h = (AmFilterHolder*)(mConnections.ItemAt(0))) != NULL)
			h->mPredecessors.RemoveItem(this);
	}
	EmptyConnections();
	return B_OK;
}

status_t AmFilterHolder::ReplaceConnection(	int32 index,
											AmFilterHolder* item)
{
	ArpVALIDATE(item != NULL, return B_ERROR);
	AmFilterHolder* old = (AmFilterHolder*)mConnections.ItemAt(index);
	if (mConnections.ReplaceItem(index, item) ) {
		if (old) old->mPredecessors.RemoveItem(this);
		item->mPredecessors.AddItem(this);
//		FilterChangeNotice();
		return B_OK;
	}
	return B_ERROR;
}

void AmFilterHolder::EmptyConnections()
{
//	FilterChangeNotice();
	mConnections.RemoveItems(0, mConnections.CountItems());
}

AmFilterHolder* AmFilterHolder::PredecessorAt(int32 index) const
{
	return (AmFilterHolder*)mPredecessors.ItemAt(index);
}

int32 AmFilterHolder::RawCountConnections() const
{
	return mConnections.CountItems();
}

AmFilterHolder* AmFilterHolder::RawConnectionAt(uint32 index) const
{
	return (AmFilterHolder*)mConnections.ItemAt(index);
}

status_t AmFilterHolder::BreakConnections(AmPipelineMatrixI* matrix, AmPipelineType type)
{
	AmFilterHolder*		conn;
	for (int32 k = 0; (conn = PredecessorAt(k)) != NULL; k++) {
		if (conn->PipelineId() != PipelineId() )
			matrix->BreakConnection(conn->PipelineId(), type, conn->Filter()->Id(),
									PipelineId(), type, Filter()->Id() );
	}
	uint32				count = CountConnections();
	for (uint32 k = 0; k < count; k++) {
		conn = dynamic_cast<AmFilterHolder*>(ConnectionAt(k) );
		if (conn && conn != NextInLine() ) {
			matrix->BreakConnection(PipelineId(), type, Filter()->Id() ,
									conn->PipelineId(), type, conn->Filter()->Id() );
		}	
	}
	return B_OK;
}

/* ----------------------------------------------------------------
   ArpMakeFilterBitmap Function
   ---------------------------------------------------------------- */

BBitmap* ArpMakeFilterBitmap(BBitmap* original, BPoint requestedSize, int32 bgIndex)
{
	BBitmap* result = NULL;
	/* Track down which background to use, based on the bgIndex supplied.
	 */
	int32	bgCount = Prefs().Int32(AM_FILTER_BG_COUNT_I32);
	if (bgIndex == 0 || bgCount <= 0)
		result = const_cast<BBitmap*>(ImageManager().FindBitmap(FILTER_BG_0) );
	else {
		int32	newIndex = bgIndex;
		if (newIndex >= bgCount) newIndex = bgCount - 1;
		if (newIndex < 0) newIndex = 0;
		BString			str(FILTER_BG_PREFIX);
		str << newIndex;
		result = const_cast<BBitmap*>(ImageManager().FindBitmap(str.String() ) );
		if (!result && newIndex != 0) result = const_cast<BBitmap*>(ImageManager().FindBitmap(FILTER_BG_0) );
	}
	result = (result) ? new BBitmap(result) : NULL;
	if (!result) return original;
	if (!original) return result;
	overlay_bitmap(result, original);
//	delete original;
	return result;
}

BBitmap* ArpMakeFilterBitmap(const AmFilterAddOn* addOn, BPoint requestedSize, int32 bgIndex)
{
	return ArpMakeFilterBitmap(addOn->Image(requestedSize), requestedSize, bgIndex);
}

BBitmap* ArpMakeFilterBitmap(const AmFilterI* filter, BPoint requestedSize, int32 bgIndex)
{
	return ArpMakeFilterBitmap(filter->Image(requestedSize), requestedSize, bgIndex);
}

/* ----------------------------------------------------------------
   ArpExecFilters Function
   ---------------------------------------------------------------- */

struct am_filter_batch
{
	AmEvent* events;
	AmEvent* pos;
	bool flush;
	
	am_filter_batch()	: events(NULL), pos(NULL), flush(false) { }
	~am_filter_batch()	{ }
};

typedef std::map<AmFilterHolderI*, am_filter_batch> batch_map;

struct am_exec_state
{
	batch_map flushes;
	batch_map batches;
	
	AmEvent* process;
	AmEvent* output;
	AmEvent* procPlace;
	AmEvent* outPlace;
	
	AmEvent* nextTempo;
	AmEvent* nextSignature;
	
	// merge the generated events into one of two lists:
	//	•	the process list if it has some remaining filters to
	//		execute;
	//	•	the output list if there are no more filters or its
	//		next is an output filter.
	inline void merge_generated(AmEvent* gen)
	{
		AmEvent* genNext;
		
		while (gen) {
			#if NOISY
			ArpD(cdb << ADH << "Process generated event=" << gen << std::endl);
			#endif
			AmFilterHolderI* filter = gen->NextFilter();
#if 0
			if (!filter && successor) {
				gen->SetNextFilter(successor);
				filter = successor;
			}
#endif
			genNext = gen->RemoveEvent();
			if (filter == NULL || filter->Type() == AmFilterAddOn::DESTINATION_FILTER) {
				#if NOISY
				ArpD(cdb << ADH << "Merge output: "; gen->Print());
				#endif
				outPlace = outPlace->MergeEvent(gen);
				if (!output) output = outPlace;
			} else if ((static_cast<AmFilterHolder*>(filter)->FilterFlags()&AmFilterI::BATCH_FLAG) != 0
							&& !filter->IsBypassed()) {
				am_filter_batch& batch = batches[filter];
				batch.pos = batch.pos->MergeEvent(gen);
				if (!batch.events) batch.events = batch.pos;
			} else {
				#if NOISY
				ArpD(cdb << ADH << "Merge process: "; gen->Print());
				#endif
				procPlace = procPlace->MergeEvent(gen);
				if (!process) process = procPlace;
			}
			gen = genNext;
		}
	}
};

AmEvent* ArpExecFilters(AmEvent* list, filter_exec_type type, bool deleteNULL,
						am_filter_params* params,
						am_tool_filter_params* toolParams,
						am_realtime_filter_params* realtimeParams,
						AmEvent* tempoChain, AmEvent* signatureChain,
						const AmTrackLookahead* lookahead)
{
	am_exec_state state;
	
	state.process=0;
	state.output=0;
	state.procPlace=0;
	state.outPlace=0;
	
	state.nextTempo = tempoChain;
	state.nextSignature = signatureChain;
	// run until all events in the input list have been processed until
	// their last non-output filter.
	while (list) {
		// extract front event from the list and run it through its
		// next filter.
		AmEvent* gen = list;
		list = list->RemoveEvent();
		#if NOISY
		ArpD(cdb << ADH << "Filtering " << gen
						<< "; next is " << list << std::endl);
		#endif
		AmFilterHolderI* filter = gen->NextFilter();
		AmFilterHolderI* successor;
		
		if (filter) {
			ArpASSERT(filter->Filter());
			successor = filter->FirstConnection();
			if (successor) successor->IncRefs();
			const uint32 flags = static_cast<AmFilterHolder*>(filter)->FilterFlags();
			if (filter->IsBypassed()) {
				gen->SetNextFilter(successor);
				#if NOISY
				ArpD(cdb << ADH << "Bypassed a filter" << std::endl);
				#endif
			} else if(filter->Type() != AmFilterAddOn::DESTINATION_FILTER) {
				//printf("Exec filter %s: flags=0x%08lx\n", filter->Filter()->Name().String(), flags);
				if ((flags&AmFilterI::BATCH_FLAG) != 0) {
					am_filter_batch& batch = state.batches[filter];
					batch.pos = batch.pos->MergeEvent(gen);
					if (!batch.events) batch.events = batch.pos;
					gen = NULL;
				} else {
					if (params) {
						// First set up the current tempo and signature context.
						while (state.nextTempo && gen->StartTime() >= state.nextTempo->StartTime()) {
							AmTempoChange* tc = dynamic_cast<AmTempoChange*>(state.nextTempo);
							state.nextTempo = state.nextTempo->NextEvent();
							if (tc)
								params->cur_tempo = tc;
						}
						while (state.nextSignature && gen->StartTime() >= state.nextSignature->StartTime()) {
							AmSignature* sc = dynamic_cast<AmSignature*>(state.nextSignature);
							state.nextSignature = state.nextSignature->NextEvent();
							if (sc)
								params->cur_signature = sc;
						}
					}
					
					if ((flags&AmFilterI::FLUSH_FLAG) != 0) {
						am_filter_batch& batch = state.flushes[filter];
						if (!batch.flush) {
							//printf("Register flush: %p\n", filter);
							batch.flush = true;
							filter->Filter()->Ready(params);
						}
					}
					
					// Now ram the event through its filter.
					gen->SetNextFilter(successor);
					switch(type) {
						case NORMAL_EXEC_TYPE:
							gen = filter->Filter()->HandleEvent(gen, params);
							break;
						case TOOL_EXEC_TYPE:
							gen = filter->Filter()->HandleToolEvent(gen, params, toolParams);
							break;
						case REALTIME_EXEC_TYPE:
							gen = filter->Filter()->HandleRealtimeEvent(gen, params, realtimeParams);
							break;
					}
					/* Assign the current filter's trackId to the event.  This is
					 * so the AmInputQueue knows which track to insert the event into.
					 */
					const track_id		tid = filter->TrackId();
					AmEvent*			tidE = gen;
					while (tidE) { tidE->trackId = tid; tidE = tidE->NextEvent(); }
						
					#if NOISY
					ArpD(cdb << ADH << "First resulting event is " << gen << std::endl);
					#endif
				}
			} else {
				#if NOISY
				ArpD(cdb << ADH << "Whoops, this is the last filter." << std::endl);
				#endif
				state.outPlace = state.outPlace->MergeEvent(gen);
				if (!state.output) state.output = state.outPlace;
				gen = NULL;
				successor = NULL;
			}
		} else if (!filter && deleteNULL) {
			#if NOISY
			ArpD(cdb << ADH << "Hit a NULL filter -- deleting event." << std::endl);
			#endif
			gen->Delete();
			gen = NULL;
			successor = NULL;
		} else {
			#if NOISY
			ArpD(cdb << ADH << "Whoops, this is the last filter." << std::endl);
			#endif
			state.outPlace = state.outPlace->MergeEvent(gen);
			if (!state.output) state.output = state.outPlace;
			gen = NULL;
			successor = NULL;
		}
		
		state.merge_generated(gen);
		
		if (successor) successor->DecRefs();
		
		// If this was the last event on the input list, restart processing
		// on the newly generated list.
		if (!list) {
			if (state.process) list = state.process->HeadEvent();
			
			// If there are no more events to process, start flushing the
			// filters that had been executed.
			while (!list && !state.flushes.empty()) {
				const batch_map::iterator i(state.flushes.begin());
				if (i->second.flush) {
					//printf("Execute flush: %p\n", i->first);
					list = i->first->Filter()->Flush(params);
				}
				state.flushes.erase(i);
			}
			
			// If there are still no more events to process, look for the next batch
			// to execute.  NOTE that we should be checking for dependencies between
			// filters here, to correctly order multiple batches.
			while (!list && !state.batches.empty()) {
				AmFilterHolderI* filter = NULL;
				batch_map::iterator i(state.batches.begin());
				if (i->second.events) {
					filter = i->first;
					list = i->second.events->HeadEvent();
				}
				state.batches.erase(i);
				
				// If a batch was found, execute it.
				if (filter && list) {
					AmFilterHolderI* successor = filter->FirstConnection();
					if (successor) successor->IncRefs();
					// First set up the current tempo and signature context.
					if (params) {
						params->cur_tempo = dynamic_cast<AmTempoChange*>(tempoChain);
						params->cur_signature = dynamic_cast<AmSignature*>(signatureChain);
					}
					
					if ((static_cast<AmFilterHolder*>(filter)->FilterFlags()&AmFilterI::FLUSH_FLAG) != 0) {
						am_filter_batch& batch = state.flushes[filter];
						if (!batch.flush) {
							batch.flush = true;
							filter->Filter()->Ready(params);
						}
					}
					
					// Prepare all events for next filter.
					gen = list;
					while (gen) {
						gen->SetNextFilter(successor);
						gen = gen->NextEvent();
					}

					const AmEvent*	e = NULL;
					if (lookahead) e = lookahead->Lookahead(filter);
					
					// Now ram the events through the filter.
					switch(type) {
						case NORMAL_EXEC_TYPE:
							list = filter->Filter()->HandleBatchEvents(list, params, e);
							break;
						case TOOL_EXEC_TYPE:
							list = filter->Filter()->HandleBatchToolEvents(list, params, toolParams, e);
							break;
						case REALTIME_EXEC_TYPE:
							list = filter->Filter()->HandleBatchRealtimeEvents(list, params, realtimeParams, e);
							break;
					}
					/* Assign the current filter's trackId to the event.  This is
					 * so the AmInputQueue knows which track to insert the event into.
					 */
					const track_id		tid = filter->TrackId();
					AmEvent*			tidE = gen;
					while (tidE) { tidE->trackId = tid; tidE = tidE->NextEvent(); }

					#if NOISY
					ArpD(cdb << ADH << "First resulting event is " << list << std::endl);
					#endif
					
					state.merge_generated(list);
					if (successor) successor->DecRefs();
					list = state.process ? state.process->HeadEvent() : NULL;
				}
			}
			
			// Reset state.
			state.process = 0;
			state.procPlace = 0;
			if (params) {
				params->cur_tempo = NULL;
				params->cur_signature = NULL;
				state.nextTempo = tempoChain;
				state.nextSignature = signatureChain;
			}
		}
	}
	
	// This is it!
	return state.output ? state.output->HeadEvent() : state.output;
}
