/* AmMultiFilter.cpp
 */
#include <cstdio>
#include <cstdlib>
#include "ArpKernel/ArpDebug.h"
#include "AmPublic/AmEvents.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "AmKernel/AmMultiFilter.h"

static const char*	PIPELINE_STR		= "pipeline";
static const char*	CONNECTIONS_STR		= "connections";

/*****************************************************************************
 * AM-MULTI-FILTER
 *****************************************************************************/
AmMultiFilter::AmMultiFilter(	AmMultiFilterAddOn* addon,
								AmFilterHolderI* holder,
								const BMessage& definition,
								const BMessage* config)
	: AmFilterI(addon),
	  mAddOn(addon), mHolder(holder), mWtfHack(true)
{
	ReadFrom(definition);
	if (config) PutConfiguration(config);
}

AmMultiFilter::~AmMultiFilter()
{
}

void AmMultiFilter::AddRef() const {}
void AmMultiFilter::RemoveRef() const {}

bool AmMultiFilter::ReadLock() const {return true;}
bool AmMultiFilter::WriteLock(const char* name) {return true;}
bool AmMultiFilter::ReadUnlock() const {return true;}
bool AmMultiFilter::WriteUnlock() {return true;}

void AmMultiFilter::SetDirty(bool dirty) {}

const BUndoContext* AmMultiFilter::UndoContext() const { return NULL; }
BUndoContext* AmMultiFilter::UndoContext() { return NULL; }

static inline int32 next_holder_index(AmFilterHolderI* holder, std::vector<filter_id>& fids)
{
	if (!holder || !holder->Filter() ) return -1;
	for (uint32 k = 0; k < fids.size(); k++) {
		if (fids[k] == holder->Filter()->Id() ) return int32(k);
	}
	return -1;
}

AmEvent* AmMultiFilter::StartSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params)
{
	AmEvent* result = NULL;
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	head = mPipelines[k].Head();
		while (head) {
			if (head->Filter() ) {
				AmEvent* ret = head->Filter()->StartSection(firstTime, lastTime, params);
				if (ret) result = result->MergeList(ret)->HeadEvent();
			}
			head = head->NextInLine();
		}
	}
	return result;
}

AmEvent* AmMultiFilter::FinishSection(	AmTime firstTime, AmTime lastTime,
										const am_filter_params* params)
{
	AmEvent* result = NULL;
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	head = mPipelines[k].Head();
		while (head) {
			if (head->Filter() ) {
				AmEvent* ret = head->Filter()->FinishSection(firstTime, lastTime, params);
				if (ret) result = result->MergeList(ret)->HeadEvent();
			}
			head = head->NextInLine();
		}
	}
	return result;
}

void AmMultiFilter::Start(uint32 context)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	head = mPipelines[k].Head();
		while (head) {
			if (head->Filter() ) head->Filter()->Start(context);
			head = head->NextInLine();
		}
	}
}

void AmMultiFilter::Stop(uint32 context)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	head = mPipelines[k].Head();
		while (head) {
			if (head->Filter() ) head->Filter()->Stop(context);
			head = head->NextInLine();
		}
	}
}

AmEvent* AmMultiFilter::HandleEvent(AmEvent* event, const am_filter_params* params)
{
	return UnifiedHandleEvent(	event, NORMAL_EXEC_TYPE,
								const_cast<am_filter_params*>(params),
								NULL, NULL);
}

AmEvent* AmMultiFilter::HandleToolEvent(AmEvent* event,
										const am_filter_params* params,
										const am_tool_filter_params* toolParams)
{
	return UnifiedHandleEvent(	event, TOOL_EXEC_TYPE,
								const_cast<am_filter_params*>(params),
								const_cast<am_tool_filter_params*>(toolParams),
								NULL);
}

AmEvent* AmMultiFilter::HandleRealtimeEvent(AmEvent* event,
											const am_filter_params* params,
											const am_realtime_filter_params* realtimeParams)
{
	return UnifiedHandleEvent(	event, REALTIME_EXEC_TYPE,
								const_cast<am_filter_params*>(params),
								NULL,
								const_cast<am_realtime_filter_params*>(realtimeParams) );
}

AmEvent* AmMultiFilter::HandleBatchEvents(AmEvent* event, const am_filter_params* params)
{
	return UnifiedHandleEvent(	event, NORMAL_EXEC_TYPE,
								const_cast<am_filter_params*>(params),
								NULL, NULL);
}

AmEvent* AmMultiFilter::HandleBatchToolEvents(	AmEvent* event,
												const am_filter_params* params,
												const am_tool_filter_params* toolParams)
{
	return UnifiedHandleEvent(	event, TOOL_EXEC_TYPE,
								const_cast<am_filter_params*>(params),
								const_cast<am_tool_filter_params*>(toolParams),
								NULL);
}

AmEvent* AmMultiFilter::HandleBatchRealtimeEvents(	AmEvent* event,
													const am_filter_params* params,
													const am_realtime_filter_params* realtimeParams)
{
	return UnifiedHandleEvent(	event, REALTIME_EXEC_TYPE,
								const_cast<am_filter_params*>(params),
								NULL,
								const_cast<am_realtime_filter_params*>(realtimeParams) );
}

status_t AmMultiFilter::GetConfiguration(BMessage* values) const
{
	status_t				err = AmFilterI::GetConfiguration(values);
	if (err != B_OK) return err;

	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	holder = mPipelines[k].Head();
		uint32				findex = 0;
		while (holder) {
			if (holder->Filter() ) {
				BMessage	entry;
				if (entry.AddInt32("p_index", k) == B_OK
						&& entry.AddInt32("f_index", findex) == B_OK
						&& entry.AddString("unique_name", holder->Filter()->AddOn()->Key()) == B_OK
						&& holder->Filter()->GetConfiguration(&entry) == B_OK)
					values->AddMessage("entry", &entry);
			}
			holder = holder->NextInLine();
			findex++;
		}
	}

	return B_OK;
}

status_t AmMultiFilter::PutConfiguration(const BMessage* values)
{
	status_t			err = AmFilterI::PutConfiguration(values);
	if (err != B_OK) return err;

	BMessage			entry;
	for (int32 k = 0; values->FindMessage("entry", k, &entry) == B_OK; k++) {
		int32			pindex, findex;
		const char*		s;
		if (entry.FindInt32("p_index", &pindex) == B_OK
				&& pindex >= 0 && pindex < int32(mPipelines.size())
				&& entry.FindInt32("f_index", &findex) == B_OK
				&& entry.FindString("unique_name", &s) == B_OK) {
			AmFilterHolderI*	h = mPipelines[pindex].Head();
			for (int32 k = 0; k <= findex; k++) {
				if (k == findex && h && h->Filter() && h->Filter()->AddOn() ) {
					BString		str(s);
					if (str == h->Filter()->AddOn()->Key() )
						h->Filter()->PutConfiguration(&entry);
					break;
				}
				if (!h) break;
				h = h->NextInLine();
			}
		}
		entry.MakeEmpty();
	}

	return B_OK;
}

status_t AmMultiFilter::Configure(ArpVectorI<BView*>& panels)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	holder = mPipelines[k].Head();
		while (holder) {
			if (holder->Filter() ) {
				status_t	err = holder->Filter()->Configure(panels);
				if (err != B_OK) return err;
			}
			holder = holder->NextInLine();
		}
	}

	return B_OK;
}

static AmGlobalsImpl* gobals_impl()
{
	AmGlobalsI*		globalsI = &(AmGlobals());
	return dynamic_cast<AmGlobalsImpl*>(globalsI);
}

void AmMultiFilter::ConfigWindowOpened()
{
	AmGlobalsImpl*	globals = gobals_impl();
	if (globals) globals->RegisterTemporaryMatrix(this);
}

void AmMultiFilter::ConfigWindowClosed()
{
	AmGlobalsImpl*	globals = gobals_impl();
	if (globals) globals->UnregisterTemporaryMatrix(Id() );
}

pipeline_matrix_id AmMultiFilter::Id() const
{
	return (void*)this;
}

uint32 AmMultiFilter::CountPipelines() const
{
	return mPipelines.size();
}

pipeline_id AmMultiFilter::PipelineId(uint32 pipelineIndex) const
{
	if (pipelineIndex >= mPipelines.size() ) return 0;
	return (void*)&(mPipelines[pipelineIndex]);
}

status_t AmMultiFilter::PipelineHeight(uint32 pipelineIndex, float* outHeight) const
{
	*outHeight = 23;
	return B_OK;
}

AmFilterHolderI* AmMultiFilter::Filter(	pipeline_id id,
										AmPipelineType type,
										filter_id filterId) const
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) {
			if (filterId == 0) return mPipelines[k].Head();
			else return mPipelines[k].Filter(filterId);
		}
	}
	return NULL;
}

status_t AmMultiFilter::InsertFilter(	AmFilterAddOn* addon,
										pipeline_id id,
										AmPipelineType type,
										int32 beforeIndex,
										const BMessage* config)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) {
			status_t	err = B_ERROR;
			if (addon->Type() == addon->SOURCE_FILTER) err = B_ERROR;
			else if (addon->Type() == addon->DESTINATION_FILTER)
				err = mPipelines[k].SetDestination(addon, config);
			else err = mPipelines[k].InsertFilter(addon, beforeIndex, config);
			if (err == B_OK) PipelineChanged(id, NULLINPUTOUTPUT_PIPELINE);
			return err;
		}
	}
	return B_ERROR;
}

status_t AmMultiFilter::ReplaceFilter(	AmFilterAddOn* addon,
										pipeline_id id,
										AmPipelineType type,
										int32 atIndex,
										const BMessage* config)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) {
			status_t	err = B_ERROR;
			if (addon->Type() == addon->SOURCE_FILTER) err = B_ERROR;
			else if (addon->Type() == addon->DESTINATION_FILTER)
				err = mPipelines[k].SetDestination(addon, config);
			else err = mPipelines[k].ReplaceFilter(addon, atIndex, config);
			if (err == B_OK) PipelineChanged(id, NULLINPUTOUTPUT_PIPELINE);
			return err;
		}
	}
	return B_ERROR;
}

status_t AmMultiFilter::RemoveFilter(	pipeline_id id,
										AmPipelineType type,
										filter_id filterId)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) {
			status_t	err = mPipelines[k].RemoveFilter(filterId);
			if (err == B_OK) PipelineChanged(id, NULLINPUTOUTPUT_PIPELINE);
			return err;
		}
	}
	return B_ERROR;
}

status_t AmMultiFilter::MakeConnection(	pipeline_id fromPid,
										AmPipelineType fromType,
										filter_id fromFid,
										pipeline_id toPid,
										AmPipelineType toType,
										filter_id toFid)
{
	AmPipelineSegment*	fromSegment = NULL;
	AmPipelineSegment*	toSegment = NULL;
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (fromPid == (void*)&(mPipelines[k])) fromSegment = &(mPipelines[k]);
		if (toPid == (void*)&(mPipelines[k])) toSegment = &(mPipelines[k]);
		if (fromSegment && toSegment) break;
	}
	if (!fromSegment || !toSegment) return B_ERROR;
	return fromSegment->MakeConnection(	this, fromPid, fromType, fromFid,
										toSegment, toFid);
}

status_t AmMultiFilter::BreakConnection(pipeline_id fromPid,
										AmPipelineType fromType,
										filter_id fromFid,
										pipeline_id toPid,
										AmPipelineType toType,
										filter_id toFid)
{
	AmPipelineSegment*	fromSegment = NULL;
	AmPipelineSegment*	toSegment = NULL;
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (fromPid == (void*)&(mPipelines[k])) fromSegment = &(mPipelines[k]);
		if (toPid == (void*)&(mPipelines[k])) toSegment = &(mPipelines[k]);
		if (fromSegment && toSegment) break;
	}
	if (!fromSegment || !toSegment) return B_ERROR;
	return fromSegment->BreakConnection(this, fromPid, fromType, fromFid,
										toSegment, toFid);
}

void AmMultiFilter::PipelineChanged(pipeline_id id, AmPipelineType type)
{
	ArpASSERT(id == 0 || PipelineIndex(id) >= 0);
	BMessage	msg(AmNotifier::PIPELINE_CHANGE_OBS);
	msg.AddPointer(MATRIX_ID_STR, Id() );
	int32		index = PipelineIndex(id);
	if (index >= 0) msg.AddInt32("pipeline_index", index);
	if (id) msg.AddPointer("pipeline_id", id);
	if (type != _NUM_PIPELINE) msg.AddInt32("pipeline_type", type);
	mNotifier.ReportMsgChange( &msg, BMessenger() );
}

void AmMultiFilter::FilterChanged(pipeline_id id, AmPipelineType type)
{
	ArpASSERT(id == 0 || PipelineIndex(id) >= 0);
	BMessage	msg(AmNotifier::FILTER_CHANGE_OBS);
	msg.AddPointer(MATRIX_ID_STR, Id() );
	int32		index = PipelineIndex(id);
	if (index >= 0) msg.AddInt32("pipeline_index", index);
	if (id) msg.AddPointer("pipeline_id", id);
	if (type != _NUM_PIPELINE) msg.AddInt32("pipeline_type", type);
	mNotifier.ReportMsgChange( &msg, BMessenger() );
}

status_t AmMultiFilter::AddMatrixPipelineObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0 || PipelineIndex(id) >= 0)
		return mNotifier.AddObserver(handler, AmNotifier::PIPELINE_CHANGE_OBS);
	else return B_ERROR;
}

status_t AmMultiFilter::AddMatrixFilterObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0 || PipelineIndex(id) >= 0)
		return mNotifier.AddObserver(handler, AmNotifier::FILTER_CHANGE_OBS);
	else return B_ERROR;
}

status_t AmMultiFilter::RemoveMatrixObserver(pipeline_id id, BHandler* handler)
{
	ArpASSERT(id == 0 || PipelineIndex(id) >= 0);
	return mNotifier.RemoveObserverAll(handler);
}

status_t AmMultiFilter::WriteTo(BMessage& msg) const
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		BMessage	pipelineMsg('null');
		if (mPipelines[k].FlattenFilters(&pipelineMsg, AM_PLAIN | AM_SOURCE | AM_DESTINATION) == B_OK)
			msg.AddMessage(PIPELINE_STR, &pipelineMsg);
	}
	BMessage		connections;
	if (FlattenConnections(&connections) == B_OK)
		msg.AddMessage(CONNECTIONS_STR, &connections);
	
	return B_OK;
}

status_t AmMultiFilter::PushPipeline()
{
	BMessage		connections;
	status_t		err = FlattenConnections(&connections);
	if (err != B_OK) return err;
	mPipelines.push_back( AmPipelineSegment(NULLINPUTOUTPUT_PIPELINE, this, NULL, NULL, 0) );
	for (uint32 k = 0; k < mPipelines.size(); k++) mPipelines[k].RemoveConnections();
	err = UnflattenConnections(&connections);

	int32			index = mPipelines.size() - 1;
	if (index >= 0 && mPipelines[index].Head() == NULL) {
		ArpRef<AmFilterAddOn>	addon;
		addon = AmFilterRoster::Default()->FindFilterAddOn(NULL_OUTPUT_KEY);
		if (addon) mPipelines[index].SetDestination(addon, NULL);
	}
	return err;
}

status_t AmMultiFilter::PopPipeline()
{
	if (mPipelines.size() < 1) return B_ERROR;
	AmFilterHolder*		h = dynamic_cast<AmFilterHolder*>(mPipelines[mPipelines.size() -1].Head() );
	while (h) {
		h->BreakConnections(this, NULLINPUTOUTPUT_PIPELINE);
		h = dynamic_cast<AmFilterHolder*>(h->NextInLine() );
	}
	mPipelines.pop_back();
	return B_OK;
}

uint32 AmMultiFilter::CountOutputs() const
{
	uint32		count = 0;
	BString		outKey(NULL_OUTPUT_KEY);
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	head = mPipelines[k].Head();
		while (head) {
			if (outKey == head->Filter()->AddOn()->Key() ) {
				count++;
				break;
			}
			head = head->NextInLine();
		}
	}
	return count;
}

void AmMultiFilter::TurnOffWtfHack()
{
	mWtfHack = false;
}

AmEvent* AmMultiFilter::UnifiedHandleEvent(	AmEvent* event, filter_exec_type type,
											am_filter_params* params,
											am_tool_filter_params* toolParams,
											am_realtime_filter_params* realtimeParams)
{
	ArpVALIDATE(event != NULL && mHolder != NULL, return event);

	AmFilterHolderI*		holder = NULL;
	std::vector<filter_id>		fids;
	BString					outKey(NULL_OUTPUT_KEY);
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	head = mPipelines[k].Head();
		if (!holder) holder = head;
		while (head) {
			if (outKey == head->Filter()->AddOn()->Key() ) {
				fids.push_back(head->Filter()->Id() );
				break;
			}
			head = head->NextInLine();
		}
	}
	if (!holder) return event;
	AmEvent*				next = event;
	while (next) {
		next->SetNextFilter(holder);
		next = next->NextEvent();
	}
	AmEvent*				t = NULL;
	AmEvent*				s = NULL;
	if (params) t = const_cast<AmTempoChange*>(params->cur_tempo);
	if (params) s = const_cast<AmSignature*>(params->cur_signature);
	if (t) t = t->HeadEvent();
	if (s) s = s->HeadEvent();
	next = event = ArpExecFilters(event, type, false, params, toolParams, realtimeParams, t, s);
	/* OK, I have NO FUCKING CLUE what this code is doing or where
	 * it's used.  But I know it screws up the mInputFilter on the
	 * AmProducerFilter.
	 * -- This might be related in some way to targeting events
	 * for specific out ports?  Maybe?  Probably not.  Actually,
	 * looks like it's deleting events that haven't been targeted
	 * or something like that.  Not sure.
	 */
	while (mWtfHack && next) {
		int32		index = next_holder_index(next->NextFilter(), fids);
		AmFilterHolderI*	nextFilter = NULL;
		if (index >= 0) nextFilter = mHolder->ConnectionAt(index);
		if (nextFilter) {
			next->SetNextFilter(nextFilter);
			next = next->NextEvent();
		} else {
			if (next != event) {
				next->Delete();
				next = next->RemoveEvent();
			} else {
				next = next->NextEvent();
				event->Delete();
				event = next;
			}
		}
	}
	return event;
}

int32 AmMultiFilter::PipelineIndex(pipeline_id id) const
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) return int32(k);
	}
	return -1;
}

status_t AmMultiFilter::ReadFrom(const BMessage& msg)
{
	BMessage		pipelineMsg;
	for (int32 k = 0; msg.FindMessage(PIPELINE_STR, k, &pipelineMsg) == B_OK; k++) {
		mPipelines.push_back( AmPipelineSegment(NULLINPUTOUTPUT_PIPELINE, this, NULL, &pipelineMsg, AM_PLAIN | AM_SOURCE | AM_DESTINATION) );
		pipelineMsg.MakeEmpty();
	}
	BMessage		connections;
	if (msg.FindMessage(CONNECTIONS_STR, &connections) == B_OK)
		UnflattenConnections(&connections);

	return B_OK;
}

status_t AmMultiFilter::FlattenConnections(BMessage* into) const
{
	ArpASSERT(into);
	for (uint32 pi = 0; pi < mPipelines.size(); pi++) {
		AmFilterHolderI*	h = mPipelines[pi].Head();
		uint32				fi = 0;
		while (h) {
			uint32			count = h->CountConnections();
			for (uint32 k = 0; k < count; k++) {
				AmFilterHolderI*	connection = h->ConnectionAt(k);
				if (connection && h->PipelineId() != connection->PipelineId() ) {
					BMessage	msg;
					if (AddConnectionInfo(connection, msg) == B_OK) {
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

status_t AmMultiFilter::AddConnectionInfo(AmFilterHolderI* connection, BMessage& msg) const
{
	for (uint32 pi = 0; pi < mPipelines.size(); pi++) {
		if (&mPipelines[pi] == connection->PipelineId() ) {
			AmFilterHolderI*	h = mPipelines[pi].Head();
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

status_t AmMultiFilter::UnflattenConnections(const BMessage* into)
{
	ArpASSERT(into);
	BMessage		entry;
	for (int32 k = 0; into->FindMessage("entry", k, &entry) == B_OK; k++) {
		int32		source_pi, source_fi, dest_pi, dest_fi;
		if (entry.FindInt32("source_pi", &source_pi) == B_OK
				&& entry.FindInt32("source_fi", &source_fi) == B_OK
				&& entry.FindInt32("dest_pi", &dest_pi) == B_OK
				&& entry.FindInt32("dest_fi", &dest_fi) == B_OK) {
			UnflattenConnection(source_pi, source_fi, dest_pi, dest_fi);
		}
		entry.MakeEmpty();
	}
	return B_OK;
}

status_t AmMultiFilter::UnflattenConnection(int32 source_pi, int32 source_fi,
											int32 dest_pi, int32 dest_fi)
{
	if (source_pi >= (int32)mPipelines.size() || dest_pi >= (int32)mPipelines.size() )
		return B_ERROR;
	AmFilterHolderI*	source = mPipelines[source_pi].Head();
	AmFilterHolderI*	dest = mPipelines[dest_pi].Head();
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

// #pragma mark -

/*****************************************************************************
 * AM-MULTI-FILTER-ADD-ON
 *****************************************************************************/
static const char*		UNIQUE_NAME_STR			= "Unique Name";
static const char*		LABEL_STR				= "Label";
static const char*		EMAIL_STR				= "email";
static const char*		AUTHOR_STR				= "Author";
static const char*		SHORT_DESCRIPTION_STR	= "Short Description";
static const char*		LONG_DESCRIPTION_STR	= "Long Description";
static const char*		MINOR_VERSION_STR		= "Minor Version";
static const char*		MAJOR_VERSION_STR		= "Major Version";
static const char*		DEFINITION_STR			= "Definition";
static const char*		ICON_20X20_STR			= "Icon 20x20";

AmMultiFilterAddOn::AmMultiFilterAddOn()
		: inherited(NULL), mIsValid(true),
		  mMaxConnections(0), mMajorVersion(1), mMinorVersion(0),
		  mIcon(NULL), mReadOnly(false)
{
	SetTint(B_TRANSPARENT_COLOR);
}

AmMultiFilterAddOn::AmMultiFilterAddOn(const char* author, const char* email)
		: inherited(NULL),
		  mAuthor(author), mEmail(email), mIsValid(true), mMaxConnections(0),
		  mMajorVersion(1), mMinorVersion(0), mIcon(NULL), mReadOnly(false)
{
	SetTint(B_TRANSPARENT_COLOR);
}

AmMultiFilterAddOn::AmMultiFilterAddOn(const AmMultiFilterAddOn& o)
		: inherited(NULL), mIsValid(true),
		  mMaxConnections(o.mMaxConnections), mMajorVersion(1), mMinorVersion(0),
		  mIcon(NULL), mReadOnly(false), mFilePath(o.mFilePath)
{
	SetTint(B_TRANSPARENT_COLOR);
	BMessage	msg;
	if (o.WriteTo(msg) == B_OK) ReadFrom(msg);
}

AmFilterAddOn::VersionType AmMultiFilterAddOn::Version() const { return VERSION_CURRENT; }
AmFilterAddOn::type AmMultiFilterAddOn::Type() const {return THROUGH_FILTER; }
AmFilterAddOn::subtype AmMultiFilterAddOn::Subtype() const {return MULTI_SUBTYPE; }

AmMultiFilterAddOn::AmMultiFilterAddOn(const BMessage& config, bool readOnly, const char* filePath)
		: inherited(NULL), mIsValid(true),
		  mMaxConnections(0), mMajorVersion(1), mMinorVersion(0),
		  mIcon(NULL), mReadOnly(readOnly), mFilePath(filePath)
{
	SetTint(B_TRANSPARENT_COLOR);
	ReadFrom(config);
}

AmMultiFilterAddOn::~AmMultiFilterAddOn()
{
	delete mIcon;
}

AmMultiFilterAddOn& AmMultiFilterAddOn::operator=(const AmMultiFilterAddOn& o)
{
	mDefinition = o.mDefinition;
	mUniqueName = o.mUniqueName;
	mLabel = o.mLabel;
	mAuthor = o.mAuthor;
	mEmail = o.mEmail;
	mIsValid = o.mIsValid;
	mMaxConnections = o.mMaxConnections;
	mShortDescription = o.mShortDescription;
	mLongDescription = o.mLongDescription;
	mMinorVersion = o.mMinorVersion;
	mMajorVersion = o.mMajorVersion;
	if (mIcon) delete mIcon;
	mIcon = NULL;
	if (o.mIcon) mIcon = new BBitmap(o.mIcon);
	mReadOnly = o.mReadOnly;
	mFilePath = o.mFilePath;
	return *this;
}

BString AmMultiFilterAddOn::UniqueName() const
{
	return mUniqueName;
}

bool AmMultiFilterAddOn::IsReadOnly() const
{
	return mReadOnly;
}

AmMultiFilterAddOn* AmMultiFilterAddOn::Copy() const
{
	return new AmMultiFilterAddOn(*this);
}

BString AmMultiFilterAddOn::Name() const
{
	return mLabel;
}

BString AmMultiFilterAddOn::Key() const
{
	return mUniqueName;
}

int32 AmMultiFilterAddOn::MaxConnections() const
{
	return mMaxConnections;
}

BString AmMultiFilterAddOn::ShortDescription() const
{
	return mShortDescription;
}

void AmMultiFilterAddOn::LongDescription(BString& name, BString& str) const
{
	name = Name();
	if (name.Length() < 1) name = "<unnamed filter>";	
	str << "<EM>Type: Multi Through.</EM><BR>";
	if (mLongDescription.Length() > 0) str << mLongDescription;
}

void AmMultiFilterAddOn::GetVersion(int32* major, int32* minor) const
{
	*major = mMajorVersion;
	*minor = mMinorVersion;
}

BBitmap* AmMultiFilterAddOn::Image(BPoint requestedSize) const
{
	if (!mIcon) return NULL;
	return new BBitmap(mIcon);
}

AmFilterI* AmMultiFilterAddOn::NewInstance(	AmFilterHolderI* holder,
											const BMessage* config)
{
	return new AmMultiFilter(this, holder, mDefinition, config);
}

file_entry_id AmMultiFilterAddOn::Id() const
{
	return (void*)this;
}

BString AmMultiFilterAddOn::Label() const
{
	return Name();
}

BString AmMultiFilterAddOn::Author() const
{
	return mAuthor;
}

BString AmMultiFilterAddOn::Email() const
{
	return mEmail;
}

bool AmMultiFilterAddOn::IsValid() const
{
	return mIsValid;
}

BString AmMultiFilterAddOn::LocalFileName() const
{
	BString		name(Key() );
	return convert_to_filename(name);
}

BString AmMultiFilterAddOn::LocalFilePath() const
{
	return mFilePath;
}

status_t AmMultiFilterAddOn::WriteTo(BMessage& config) const
{
	status_t	err = B_OK;
	if (mUniqueName.Length() > 0) err = config.AddString(UNIQUE_NAME_STR, mUniqueName);
	if (err == B_OK && mLabel.Length() > 0) err = config.AddString(LABEL_STR, mLabel);
	if (err == B_OK && mAuthor.Length() > 0) err = config.AddString(AUTHOR_STR, mAuthor);
	if (err == B_OK && mEmail.Length() > 0) err = config.AddString(EMAIL_STR, mEmail);
	if (err == B_OK && mShortDescription.Length() > 0) err = config.AddString(SHORT_DESCRIPTION_STR, mShortDescription);
	if (err == B_OK && mLongDescription.Length() > 0) err = config.AddString(LONG_DESCRIPTION_STR, mLongDescription);
	if (err == B_OK) err = config.AddInt32(MINOR_VERSION_STR, mMinorVersion);
	if (err == B_OK) err = config.AddInt32(MAJOR_VERSION_STR, mMajorVersion);
	/* Definition.
	 */
	if (err == B_OK) err = config.AddMessage(DEFINITION_STR, &mDefinition);
	/* Icon.
	 */
	if (err == B_OK && mIcon) {
		BMessage	iconMsg;
		if (mIcon->Archive(&iconMsg) == B_OK)
			err = config.AddMessage(ICON_20X20_STR, &iconMsg);
	}
	return err;
}

void AmMultiFilterAddOn::SetDefinition(const BMessage& definition)
{
	mDefinition = definition;
	mMaxConnections = 0;
	/* Yuck!  Parse through the definition and count the output
	 * filters.  I can't just instantiate a new instance of the
	 * multi filter because it might try to instantiate filters
	 * when the roster hasn't been created.
	 */
	BMessage			pipeMsg;
	for (int32 k = 0; mDefinition.FindMessage(PIPELINE_STR, k, &pipeMsg) == B_OK; k++) {
		BMessage		filtersMsg;
		for (int32 i = 0; pipeMsg.FindMessage(SZ_FILTER_ARCHIVE, i, &filtersMsg) == B_OK; i++) {
			BString		key;
			if (filtersMsg.FindString(SZ_FILTER_KEY, &key) == B_OK
					&& key == NULL_OUTPUT_KEY)
				mMaxConnections++;
			filtersMsg.MakeEmpty();
		}
		pipeMsg.MakeEmpty();
	}
}

void AmMultiFilterAddOn::SetLabel(const char* s)
{
	mLabel = s;
}

void AmMultiFilterAddOn::SetKey(const char* s)
{
	mUniqueName = s;
}

void AmMultiFilterAddOn::SetAuthor(const char* author)
{
	mAuthor = author;
}

void AmMultiFilterAddOn::SetEmail(const char* email)
{
	mEmail = email;
}

void AmMultiFilterAddOn::SetIsValid(bool isValid)
{
	mIsValid = isValid;
}

void AmMultiFilterAddOn::SetShortDescription(const char* s)
{
	mShortDescription = s;
}

void AmMultiFilterAddOn::SetLongDescription(const char* s)
{
	mLongDescription = s;
}

void AmMultiFilterAddOn::SetVersion(int32 major, int32 minor)
{
	mMajorVersion = major;
	mMinorVersion = minor;
}

const BBitmap* AmMultiFilterAddOn::Icon(BPoint size) const
{
	return mIcon;
}

void AmMultiFilterAddOn::SetIcon(const BBitmap* icon)
{
	delete mIcon;
	mIcon = NULL;
	if (!icon) return;
	mIcon = new BBitmap(icon);
}

const char* AmMultiFilterAddOn::LongDescriptionContents() const
{
	return mLongDescription.String();
}

status_t AmMultiFilterAddOn::ReadFrom(const BMessage& config)
{
	const char*		s;
	if (config.FindString(UNIQUE_NAME_STR, &s) == B_OK) mUniqueName = s;
	if (config.FindString(LABEL_STR, &s) == B_OK) mLabel = s;
	if (config.FindString(AUTHOR_STR, &s) == B_OK) mAuthor = s;
	if (config.FindString(EMAIL_STR, &s) == B_OK) mEmail = s;
	if (config.FindString(SHORT_DESCRIPTION_STR, &s) == B_OK) mShortDescription = s;
	if (config.FindString(LONG_DESCRIPTION_STR, &s) == B_OK) mLongDescription = s;
	int32			i;
	if (config.FindInt32(MINOR_VERSION_STR, &i) == B_OK) i = mMinorVersion;
	if (config.FindInt32(MAJOR_VERSION_STR, &i) == B_OK) i = mMajorVersion;
	/* Definition.
	 */
	BMessage		def;
	if (config.FindMessage(DEFINITION_STR, &def) == B_OK) SetDefinition(def);
	/* Icon.
	 */
	delete mIcon;
	mIcon = NULL;
	BMessage		iconMsg;
	if (config.FindMessage(ICON_20X20_STR, &iconMsg) == B_OK) {
		mIcon = dynamic_cast<BBitmap*>( BBitmap::Instantiate(&iconMsg) );
	}
	return B_OK;
}
