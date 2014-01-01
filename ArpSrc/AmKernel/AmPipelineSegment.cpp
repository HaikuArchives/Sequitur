/* AmPipelineSegment.cpp
 */
#define _BUILDING_AmKernel 1

#include <stdio.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpMessage.h"
#include "AmPublic/AmPipelineMatrixI.h"
#include "AmKernel/AmKernelDefs.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmPipelineSegment.h"
#include "AmKernel/AmTrack.h"

static const char*  BYPASSED_STR			= "arp:bypassed";
static const char*	SUPPRESSNEXTINLINE_STR	= "arp:suppress_next_in_line";

// ------------------------------------------------------------------------

class AmTrackFilterUndo : public BUndoOperation
{
public:
	AmTrackFilterUndo(AmTrack* track, bool added,
					  AmPipelineSegment* pipeline, AmFilterHolder* pred,
					  AmFilterHolder* filter)
		: mTrack(track), mPipeline(pipeline),
		  mWhere(pred), mFilter(filter), mAdded(added)
	{
		mTrack->AddRef();
	}
	
	virtual ~AmTrackFilterUndo()
	{
		if (!mAdded) mFilter->Delete();
		mTrack->RemoveRef();
	}
	
	virtual const void* Owner() const
	{
		return mTrack;
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

private:
	void swap_data()
	{
		if (mAdded) mPipeline->RemoveFilter(mWhere, mFilter, NULL);
		else mPipeline->InsertFilter(mWhere, mFilter, NULL);
		mAdded = !mAdded;
		mTrack->MergePipelineChange(mPipeline);
	}
	
	AmTrack* const	mTrack;
	
	AmPipelineSegment*		mPipeline;
	AmFilterHolder*	mWhere;
	AmFilterHolder*	mFilter;
	bool mAdded;
};

// ------------------------------------------------------------------------
class AmFilterConnectionUndo : public BUndoOperation
{
public:
	AmFilterConnectionUndo(	AmPipelineMatrixI* matrix,
							const void* owner,
							AmPipelineSegment* segment,
							pipeline_id fromPid,
							AmPipelineType fromType,
							AmFilterHolder* fromFilter,
							AmFilterHolder* toFilter,
							bool connected)
		: mMatrix(matrix), mOwner(owner), mSegment(segment), mFromPid(fromPid),
		  mFromType(fromType), mFromFilter(fromFilter), mToFilter(toFilter),
		  mConnected(connected)
	{
		mFromFilter->IncRefs();
		mToFilter->IncRefs();
	}
	
	virtual ~AmFilterConnectionUndo()
	{
		mFromFilter->DecRefs();
		mToFilter->DecRefs();
	}
	
	virtual const void* Owner() const
	{
		return mOwner;
	}
	
	virtual bool MatchOwnerContext(const void* owner) const
	{
		return owner == mOwner;
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
		if (mConnected) mSegment->BreakConnection(mMatrix, mFromPid, mFromType, mFromFilter, mToFilter, NULL, NULL);
		else mSegment->MakeConnection(mMatrix, mFromPid, mFromType, mFromFilter, mToFilter, NULL, NULL);
		mConnected = !mConnected;
	}

	AmPipelineMatrixI*	mMatrix;
	const void*			mOwner;
	AmPipelineSegment*	mSegment;
	pipeline_id			mFromPid;
	AmPipelineType		mFromType;
	AmFilterHolder*		mFromFilter;
	AmFilterHolder*		mToFilter;
	bool				mConnected;
};

/* A little convenience for generating new filters from addons.
 */
static AmFilterHolder* new_filter(	AmPipelineMatrixI* matrix, AmTrack* track,
									pipeline_id pipelineId,
									AmFilterAddOn* addon, const BMessage* config )
{
	pipeline_matrix_id		matrixId = (matrix) ? matrix->Id() : NULL;
	track_id				trackId = (track) ? track->Id() : NULL;
	ArpRef<AmInputTarget>	inputTarget = NULL;
	if (track) inputTarget = track->InputTarget();
	AmFilterHolder*			h = new AmFilterHolder(matrixId, trackId, pipelineId, inputTarget);
	if (!h) return 0;
	AmFilterI*				filter = addon->NewInstance(h, config);
	if (!filter) {
		h->Delete();
		return 0;
	}
	if (config) {
		bool	b;
		if (config->FindBool(BYPASSED_STR, &b) != B_OK) b = false;
		h->SetBypassed(b);
		if (config->FindBool(SUPPRESSNEXTINLINE_STR, &b) != B_OK) b = false;
		h->SetSuppressNextInLine(b);
	}
	h->SetFilter(filter);
	return h;
}

/* Convenience to generate the configuration message when replacing
 * one filter with another.  If they are both of the same class, we
 * keep any configuration that the new one doesn't explicitly specify.
 */
const BMessage* compute_replace_config(AmFilterHolder* existing,
									   const BMessage* newConfig,
									   BMessage* tmp)
{
	const char* key;
	if (newConfig
			&& newConfig->FindString(SZ_FILTER_KEY, &key) == B_OK
			&& existing->Filter() && existing->Filter()->AddOn()
			&& strcmp(key, existing->Filter()->AddOn()->Key().String() ) == 0) {
		if (existing->Filter()->Archive(tmp, 0) == B_OK) {
			ArpUpdateMessage(*tmp, *newConfig);
			return tmp;
		}
	}
	return newConfig;
}

/***************************************************************************
 * AM-PIPELINE
 ****************************************************************************/
AmPipelineSegment::AmPipelineSegment()
		: mType(_NUM_PIPELINE), mMatrix(NULL), mTrack(NULL), mHead(NULL)
{
}

AmPipelineSegment::AmPipelineSegment(const AmPipelineSegment& o)
		: mType(o.mType), mMatrix(NULL), mTrack(NULL), mHead(NULL)
{
	Setup(o.mMatrix, o.mTrack, o.mType);
	BMessage		flat;
	int32			flags = AM_SOURCE | AM_DESTINATION | AM_PLAIN;
	if (o.FlattenFilters(&flat, flags) == B_OK)
		UnflattenFilters(&flat, flags);
}


AmPipelineSegment::AmPipelineSegment(	AmPipelineType type, AmPipelineMatrixI* matrix,
										AmTrack* track, const BMessage* filters, uint32 flags)
		: mType(type), mMatrix(NULL), mTrack(NULL), mHead(NULL)
{
	Setup(matrix, track, type);
	if (filters) UnflattenFilters(filters, flags);
}

AmPipelineSegment::~AmPipelineSegment()
{
//	AM_LOG("\nAmPipelineSegment::~AmPipelineSegment()\n");
	if (mHead) mHead->Delete();
	mHead = NULL;
}

void AmPipelineSegment::Setup(AmPipelineMatrixI* matrix, AmTrack* track, AmPipelineType type)
{
	mType = type;
	mMatrix = matrix;
	mTrack = track;
}
		
AmFilterHolderI* AmPipelineSegment::Filter(filter_id id) const
{
	AmFilterHolderI*	h = mHead;
	while (h) {
		if (h->Filter() && h->Filter()->Id() == id)
			return h;
		h = h->NextInLine();
	}
	return 0;
}

AmFilterHolderI* AmPipelineSegment::Head() const
{
	return mHead;
}

AmFilterHolderI* AmPipelineSegment::Tail() const
{
	if (!mHead) return NULL;
	AmFilterHolderI*	h = mHead;
	while (h) {
		if (!h->NextInLine() ) return h;
		h = h->NextInLine();
	}
	return NULL;
}

AmFilterHolderI* AmPipelineSegment::Source() const
{
	if (!mHead) return NULL;
	AmFilterHolderI*	h = mHead;
	while (h) {
		if (h->Type() == AmFilterAddOn::SOURCE_FILTER) return h;
		h = h->NextInLine();
	}
	return NULL;
}

AmFilterHolderI* AmPipelineSegment::Destination() const
{
	if (!mHead) return NULL;
	AmFilterHolderI*	h = mHead;
	while (h) {
		if (h->Type() == AmFilterAddOn::DESTINATION_FILTER) return h;
		h = h->NextInLine();
	}
	return NULL;
}

status_t AmPipelineSegment::InsertFilter(AmFilterAddOn* addon, int32 beforeIndex,
								 const BMessage* config)
{
	ArpASSERT( addon );
	if (addon->Type() != AmFilterAddOn::THROUGH_FILTER) return B_ERROR;

	/* Find the index where the new filter goes.  If beforeIndex is -1, add this as
	 * the last filter before the output.
	 */
	if( beforeIndex < 0 ) beforeIndex = 900000;
	AmFilterHolder*		prev;
	AmFilterHolder*		next;
	PrevAndNextFor( beforeIndex, &prev, &next );
	if( prev && prev->Type() == AmFilterAddOn::DESTINATION_FILTER ) return B_ERROR;
	if( next && next->Type() == AmFilterAddOn::SOURCE_FILTER ) return B_ERROR;

	pipeline_id			pid = (mTrack) ? mTrack->Id() : this;
	AmFilterHolder*	h = new_filter(mMatrix, mTrack, pid, addon, config);
	if( !h ) return B_NO_MEMORY;
	
	BUndoContext* 		undoContext = (mMatrix) ? mMatrix->UndoContext() : NULL;
	InsertFilter(prev, h, undoContext);
	return B_OK;
}

static void dec_filter_refs(vector<AmFilterHolder*>& holders)
{
	for (uint32 k = 0; k < holders.size(); k++)
		holders[k]->DecRefs();
}

status_t AmPipelineSegment::ReplaceFilter(	AmFilterAddOn* addon,
											int32 atIndex,
											const BMessage* config)
{
	ArpASSERT(mMatrix && addon);
	if (addon->Type() != AmFilterAddOn::THROUGH_FILTER) return B_ERROR;

	AmFilterHolder*		prev;
	AmFilterHolder*		which;
	PrevAndNextFor(atIndex, &prev, &which);
	if (!which || !which->Filter() ) {
		/* A bit of a hack I threw in because I'm sooo sick of the
		 * pipeline views going through so much work to determine what
		 * to do.
		 */
		if (atIndex == 0) return InsertFilter(addon, atIndex, config);
		return B_ERROR;
	}
	// If the old and new filters are the same, use the old filter's config
	BMessage		tmpMsg;
	const BMessage*	finalConfig = compute_replace_config(which, config, &tmpMsg);

	vector<AmFilterHolder*>		preds;
	AmFilterHolder*				pred;
	for (int32 k = 0; (pred = which->PredecessorAt(k)) != NULL; k++) {
		if (pred->PipelineId() != which->PipelineId() ) {
			pred->IncRefs();
			preds.push_back(pred);
		}
	}
	
	status_t		err = RemoveFilter(which->Filter()->Id(), 0 );
	if (err != B_OK) {
		dec_filter_refs(preds);
		return err;
	}
	
	pipeline_id			pid = (mTrack) ? mTrack->Id() : this;
	AmFilterHolder*	h = new_filter(mMatrix, mTrack, pid, addon, finalConfig);
	if (!h) {
		dec_filter_refs(preds);
		return B_NO_MEMORY;
	}
	BUndoContext* 		undoContext = (mMatrix) ? mMatrix->UndoContext() : NULL;
	InsertFilter(prev, h, undoContext, "Set ");

	for (uint32 k = 0; k < preds.size(); k++) {
		MakeConnection(	mMatrix, preds[k]->PipelineId(), mType, preds[k], h,
						mMatrix, undoContext);
	}

	dec_filter_refs(preds);
	return B_OK;
}

status_t AmPipelineSegment::RemoveFilter(filter_id id, const char* undoName, bool merge)
{
	AmFilterHolder*		prev = NULL;
	AmFilterHolder*		curr = mHead;
	while (curr) {
		if (curr->Filter()->Id() == id) break;
		prev = curr;	
		curr = dynamic_cast<AmFilterHolder*>(curr->NextInLine() );
	}
	if (!curr) return B_ERROR;

	BUndoContext* 		undoContext = (mMatrix) ? mMatrix->UndoContext() : NULL;
	RemoveFilter(prev, curr, undoContext, undoName);

	BMessage msg(B_QUIT_REQUESTED);
	curr->ConfigWindow().SendMessage(&msg);
	
	return B_OK;
}

status_t AmPipelineSegment::SetSource(AmFilterAddOn* addon, const BMessage* config)
{
	ArpASSERT(mMatrix && addon);
	if (addon->Type() != AmFilterAddOn::SOURCE_FILTER) return B_ERROR;
	const BMessage*	finalConfig = config;
	BMessage		tmpMsg;
	const char*		undoName = "Add ";
	
	if (mHead && (mHead->Type() == AmFilterAddOn::SOURCE_FILTER) ) {
		// If the old and new filters are the same, use the old filter's config
		finalConfig = compute_replace_config(mHead, config, &tmpMsg);
		status_t	err = RemoveFilter( mHead->Filter()->Id(), 0 );
		if (err != B_OK) return err;
		undoName = "Set ";
	}

	pipeline_id			pid = (mTrack) ? mTrack->Id() : this;
	AmFilterHolder*	h = new_filter(mMatrix, mTrack, pid, addon, finalConfig);
	if (!h) return B_NO_MEMORY;
	BUndoContext* 		undoContext = (mMatrix) ? mMatrix->UndoContext() : NULL;
	InsertFilter(NULL, h, undoContext, undoName);
	return B_OK;
}

status_t AmPipelineSegment::SetDestination(AmFilterAddOn* addon, const BMessage* config)
{
	ArpASSERT(mMatrix && addon);
	if (addon->Type() != AmFilterAddOn::DESTINATION_FILTER) return B_ERROR;
	const BMessage*	finalConfig = config;
	BMessage		tmpMsg;
	AmFilterHolder*	tail = dynamic_cast<AmFilterHolder*>( Destination() );
	const char*		undoName = "Add ";

	if (tail) {
		// If the old and new filters are the same, use the old filter's config
		finalConfig = compute_replace_config(tail, config, &tmpMsg);
		status_t	err = RemoveFilter( tail->Filter()->Id(), 0 );
		if (err != B_OK) return err;
		undoName = "Set ";
	}

	pipeline_id			pid = (mTrack) ? mTrack->Id() : this;
	AmFilterHolder*	h = new_filter(mMatrix, mTrack, pid, addon, finalConfig);
	if (!h) return B_NO_MEMORY;
	BUndoContext* 		undoContext = (mMatrix) ? mMatrix->UndoContext() : NULL;
	InsertFilter(dynamic_cast<AmFilterHolder*>( Tail() ), h, undoContext, undoName);
	return B_OK;
}

static bool operate_on(const AmFilterHolderI* holder, uint32 flags)
{
	if( (flags&AM_SOURCE) && holder->Type() == AmFilterAddOn::SOURCE_FILTER ) return true;
	if( (flags&AM_DESTINATION) && holder->Type() == AmFilterAddOn::DESTINATION_FILTER ) return true;
	if( (flags&AM_PLAIN) && holder->Type() == AmFilterAddOn::THROUGH_FILTER ) return true;
	return false;
}

static bool operate_on(const AmFilterAddOn* addon, uint32 flags)
{
	if( (flags&AM_SOURCE) && addon->Type() == AmFilterAddOn::SOURCE_FILTER ) return true;
	if( (flags&AM_DESTINATION) && addon->Type() == AmFilterAddOn::DESTINATION_FILTER ) return true;
	if( (flags&AM_PLAIN) && addon->Type() == AmFilterAddOn::THROUGH_FILTER ) return true;
	return false;
}

status_t AmPipelineSegment::FlattenFilters(BMessage* into, uint32 flags) const
{
	AmFilterHolderI*	filter = mHead;
	while (filter) {
		if (filter->Filter() && operate_on(filter, flags) ) {
			BMessage config;
			config.AddBool(BYPASSED_STR, filter->IsBypassed() );
			config.AddBool(SUPPRESSNEXTINLINE_STR, filter->IsSuppressingNextInLine() );
			status_t res = filter->Filter()->Archive(&config, 0);
			if (res == B_OK) res = into->AddMessage(SZ_FILTER_ARCHIVE, &config);
			if (res != B_OK) return res;
		}
		filter = filter->NextInLine();
	}
	return B_OK;
}

status_t AmPipelineSegment::UnflattenFilters(const BMessage* from, uint32 flags)
{
	if (!(flags&AM_APPEND)) Clear(flags);
	
	BMessage	config;
	for (int32 i=0; from->FindMessage(SZ_FILTER_ARCHIVE, i, &config)==B_OK; i++) {
		ArpRef<AmFilterAddOn> addon = am_find_filter_addon(&config);
		if (addon && operate_on(addon, flags) ) {
			if (addon->Type() == AmFilterAddOn::SOURCE_FILTER) SetSource(addon, &config);
			else if (addon->Type() == AmFilterAddOn::DESTINATION_FILTER) SetDestination(addon, &config);
			else InsertFilter(addon, -1, &config);
		}
	}
	return B_OK;
}

status_t AmPipelineSegment::Clear(uint32 flags)
{
	AmFilterHolder* prev = NULL;
	AmFilterHolder* pos = mHead;
	mHead = NULL;
	while (pos) {
		AmFilterHolder* next = dynamic_cast<AmFilterHolder*>( pos->NextInLine() );
		if (operate_on(pos, flags)) {
			// Removing this filter.
			pos->RemoveSelf();
			if (prev && next) prev->SetNextInLine(next);
		} else {
			// Keeping this filter.
			if (!mHead) mHead = pos;
		}
		pos = next;
	}
	return B_OK;
}

status_t AmPipelineSegment::MakeConnection(	AmPipelineMatrixI* matrix, pipeline_id fromPid,
											AmPipelineType fromType, filter_id fromFid,
											AmPipelineSegment* toPipeline, filter_id toFid,
											const void* undoOwner, BUndoContext* undo)
{
	ArpVALIDATE(toPipeline, return B_ERROR);
	AmFilterHolder*		fromFilter = dynamic_cast<AmFilterHolder*>(Filter(fromFid) );
	AmFilterHolder*		toFilter = dynamic_cast<AmFilterHolder*>(toPipeline->Filter(toFid) );
	ArpVALIDATE(fromFilter && fromFilter->Filter() && toFilter && toFilter->Filter(), return B_ERROR);
	return MakeConnection(matrix, fromPid, fromType, fromFilter, toFilter, undoOwner, undo);
}

status_t AmPipelineSegment::BreakConnection(AmPipelineMatrixI* matrix, pipeline_id fromPid,
											AmPipelineType fromType, filter_id fromFid,
											AmPipelineSegment* toPipeline, filter_id toFid,
											const void* undoOwner, BUndoContext* undo)
{
	ArpVALIDATE(toPipeline, return B_ERROR);
	AmFilterHolder*		fromFilter = dynamic_cast<AmFilterHolder*>(Filter(fromFid) );
	AmFilterHolder*		toFilter = dynamic_cast<AmFilterHolder*>(toPipeline->Filter(toFid) );
	ArpVALIDATE(fromFilter && fromFilter->Filter() && toFilter && toFilter->Filter(), return B_ERROR);
	return BreakConnection(matrix, fromPid, fromType, fromFilter, toFilter, undoOwner, undo);
}

void AmPipelineSegment::InsertFilter(	AmFilterHolder* pred, AmFilterHolder* which,
										BUndoContext* undo, const char* undoName)
{
	if (undo) {
		if (undoName) {
			BString name(undoName);
			name += which->Filter()->Label();
			undo->SuggestUndoName(name.String());
		}
		undo->AddOperation(new AmTrackFilterUndo(mTrack, true, this, pred, which));
	}
	
	if (pred) {
		AmFilterHolder* next = dynamic_cast<AmFilterHolder*>(pred->NextInLine());
		if (next) {
			pred->SetNextInLine(NULL);
			which->SetNextInLine(next);
		}
		pred->SetNextInLine(which);
	} else {
		if (mHead) which->SetNextInLine(mHead);
		mHead = which;
	}
}
#if 0
void AmPipelineSegment::RemoveFilter(AmFilterHolder* pred, AmFilterHolder* which,
							 BUndoContext* undo, const char* undoName)
{
	if (undo) {
		if (undoName) {
			BString	name(undoName);
			name += which->Filter()->Label();
			undo->SuggestUndoName(name.String());
		}		
		undo->AddOperation(new AmTrackFilterUndo(mTrack, false, this, pred, which));
	}
	
	AmFilterHolder* next = dynamic_cast<AmFilterHolder*>(which->NextInLine());
	if (next) which->SetNextInLine(NULL);
	if (pred) {
		pred->SetNextInLine(NULL);
		if (next) pred->SetNextInLine(next);
	} else mHead = next;
	which->SetRemoved(true);
}
#endif
status_t AmPipelineSegment::RemoveConnections()
{
	AmFilterHolder*		h = mHead;
	while (h) {
		h->RemoveConnections();
		h = dynamic_cast<AmFilterHolder*>(h->NextInLine() );
	}
	return B_OK;
}

AmPipelineSegment& AmPipelineSegment::operator=(const AmPipelineSegment& o)
{
	if (this != &o) {
		mType = o.mType;
		if (mHead) mHead->Delete();
		mHead = NULL;

		BMessage		flat;
		int32			flags = AM_SOURCE | AM_DESTINATION | AM_PLAIN;
		if (o.FlattenFilters(&flat, flags) == B_OK)
			UnflattenFilters(&flat, flags);
		mMatrix = o.mMatrix;
		mTrack = o.mTrack;
	}
	return *this;
}

void AmPipelineSegment::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("AmPipelineSegment:\n");
	AmFilterHolder*		h = mHead;
	while (h) {
		printf("\n");
		for (uint32 k = 0; k < tabs + 1; k++) printf("\t");
		printf("%s (%ld branches) %p", h->Filter()->Name().String(), h->CountConnections(), h);
		int32		rawCount = h->RawCountConnections();
		for (int32 k = 0; k < rawCount; k++) {
			AmFilterHolder*		h2 = h->RawConnectionAt(k);
			if (h2) {
				printf("\n");
				for (uint32 k = 0; k < tabs + 2; k++) printf("\t");
				printf("%s (%ld branches) %p", h2->Filter()->Name().String(), h2->CountConnections(), h2);
			}
		}
		h = dynamic_cast<AmFilterHolder*>(h->NextInLine() );
	}
}

void AmPipelineSegment::PrevAndNextFor( int32 beforeIndex, AmFilterHolder** prev, AmFilterHolder** next ) const
{
	*prev = 0;
	*next = mHead;

	int32	k = 0;
	while( *next
			&& ( (*next)->Type() != AmFilterAddOn::DESTINATION_FILTER )
			&& (k < beforeIndex) ) {
		*prev = *next;
		*next = dynamic_cast<AmFilterHolder*>( (*next)->NextInLine() );
		k++;
	}
}

void AmPipelineSegment::RemoveFilter(	AmFilterHolder* pred, AmFilterHolder* which,
										BUndoContext* undo, const char* undoName)
{

	if (mMatrix) mMatrix->SetDirty();

	/* Break connections to predeccessors in other pipelines.
	 */
	while (break_pred_connection(which, undo) ) ;
	/* Break connections to connections in other pipelines.
	 */
	while (break_next_connection(which, undo) ) ;

	if (mMatrix && mTrack && undo) {
		BString		n(undoName);
		if (n.Length() < 1) n = "Remove Fitler";
		undo->SuggestUndoName(n.String() );
		undo->AddOperation(new AmTrackFilterUndo(mTrack, false, this, pred, which) );
	}

	AmFilterHolder* next = dynamic_cast<AmFilterHolder*>(which->NextInLine());
	if (next) which->SetNextInLine(NULL);
	if (pred) {
		pred->SetNextInLine(NULL);
		if (next) pred->SetNextInLine(next);
	} else mHead = next;
	which->SetRemoved(true);
}

bool AmPipelineSegment::break_pred_connection(	AmFilterHolder* toFilter,
												BUndoContext* undo)
{
	AmFilterHolder*		h;
	for (int32 k = 0; (h = toFilter->PredecessorAt(k)) != NULL; k++) {
		if (h->PipelineId() != toFilter->PipelineId() ) {
			BreakConnection(mMatrix, h->PipelineId(), mType, h, toFilter, mMatrix, undo);
			return true;
		}
	}
	return false;
}

bool AmPipelineSegment::break_next_connection(	AmFilterHolder* fromFilter,
												BUndoContext* undo)
{
	uint32				count = fromFilter->RawCountConnections();
	for (uint32 k = 0; k < count; k++) {
		AmFilterHolder*		h = fromFilter->RawConnectionAt(k);
		if (h && h->PipelineId() != fromFilter->PipelineId() ) {
			BreakConnection(mMatrix, fromFilter->PipelineId(), mType, fromFilter, h, mMatrix, undo);
			return true;
		}
	}
	return false;
}

status_t AmPipelineSegment::MakeConnection(	AmPipelineMatrixI* matrix, pipeline_id fromPid,
											AmPipelineType fromType, AmFilterHolder* fromFilter,
											AmFilterHolder* toFilter,
											const void* undoOwner, BUndoContext* undo)
{
	ArpASSERT(fromFilter && fromFilter->Filter() && toFilter && toFilter->Filter() );
	if (fromFilter == toFilter) return B_ERROR;

	if (matrix) matrix->SetDirty();
	/* This is a special rule -- normally, all holders automatically have
	 * their NextInLine() holder as a connection.  However, in the case of
	 * filters with only 1 allowed connection, they can redirect that
	 * connection.
	 */
	if (fromFilter->Filter()->AddOn()->MaxConnections() == 1) {
		AmFilterHolder*		h;
		while ( (h = fromFilter->RawConnectionAt(0)) != NULL) {
			BreakConnection(matrix, fromPid, fromType, fromFilter, h, undoOwner, undo);
		}
		fromFilter->SetSuppressNextInLine(true);
	}

	if (undoOwner && undo) {
		undo->SuggestUndoName("Make Connection");
		undo->AddOperation(new AmFilterConnectionUndo(matrix, undoOwner, this, fromPid, fromType, fromFilter, toFilter, true) );
	}

	fromFilter->AddConnection(toFilter);
	if (matrix) matrix->PipelineChanged(fromPid, fromType);
	return B_OK;
}

status_t AmPipelineSegment::BreakConnection(AmPipelineMatrixI* matrix, pipeline_id fromPid,
											AmPipelineType fromType, AmFilterHolder* fromFilter,
											AmFilterHolder* toFilter,
											const void* undoOwner, BUndoContext* undo)
{
	ArpASSERT(fromFilter && fromFilter->Filter() && toFilter && toFilter->Filter() );

	if (matrix) matrix->SetDirty();
	if (undoOwner && undo) {
		undo->SuggestUndoName("Break Connection");
		undo->AddOperation(new AmFilterConnectionUndo(matrix, undoOwner, this, fromPid, fromType, fromFilter, toFilter, false) );
	}
	fromFilter->RemoveConnection(toFilter->Filter()->Id() );
	if (matrix) matrix->PipelineChanged(fromPid, fromType);
	return B_OK;
}
