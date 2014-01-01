/* AmGlobalsImpl.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <be/app/Application.h>
#include <be/app/Clipboard.h>
#include <be/interface/Alert.h>
#include <be/storage/Entry.h>
#include <be/storage/File.h>
#include <be/support/Autolock.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpKernel/ArpSafeDelivery.h"
#include "AmPublic/AmEvents.h"
#include "AmPublic/AmSongObserver.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmGlobalsImpl.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmPropertiesTool.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmToolBar.h"
#include "AmKernel/AmTransport.h"

#include "AmKernel/AmDevice.h"
#include "AmPublic/AmEvents.h"

static const char*	ARP_CURVES_STR		= "ARP Curves";
static const char*	CURVES_INITED		= "CurvesInited";

/* This utility answers the distance that time is from the start, end
 * range.  If time is within range, it answers 0.
 */
static AmTime distance_from(AmTime start, AmTime end, AmTime time)
{
	if (time >= start) {
		if (time <= end) return 0;
		else return time - end;
	}
	return start - time;
}

/**********************************************************************
 * AM-GLOBALS-I
 **********************************************************************/
static AmGlobalsI* gGlobals = NULL;

AmGlobalsI& AmGlobals()
{
	if (!gGlobals) debugger("Globals not yet set");
	return *gGlobals;
}

void SetGlobals(AmGlobalsI& globals)
{
	if (gGlobals) debugger("Globals already set");
	gGlobals = &globals;
}

/**********************************************************************
 * AM-GLOBALS-IMPL
 **********************************************************************/
AmGlobalsImpl::AmGlobalsImpl()
		: mClipboard(0)
{
	SetGlobals(*this);
}

AmGlobalsImpl::~AmGlobalsImpl()
{
	SelectSong(AmSongRef());
}

void AmGlobalsImpl::Initialize()
{
}

void AmGlobalsImpl::Shutdown()
{
	mToolBars.resize(0);
	mSongRefs.resize(0);
	/* Delete the view factories.
	 */
	for (uint32 k = 0; k < mFactories.size(); k++)
		delete mFactories[k];
	mFactories.resize(0);
	FreeClipboard();
}

void AmGlobalsImpl::AddObserver(uint32 code, BHandler* handler)
{
	ArpASSERT(handler);
	if (code == DEVICE_OBS) {
		BAutolock l(&mObserverLock);
		mDeviceObservers.push_back( BMessenger(handler) );
	} else if (code == TOOL_BAR_OBS) {
		BAutolock l(mToolBarLock);
		mToolBarObservers.push_back( BMessenger(handler) );
	}
}

void AmGlobalsImpl::RemoveObserver(uint32 code, BHandler* handler)
{
	ArpASSERT(handler);
	BAutolock l(&mObserverLock);
	if (code == DEVICE_OBS) {
		BAutolock l(&mObserverLock);
		while( RemoveObserver( mDeviceObservers, BMessenger(handler) ) ) ;
	} else if (code == TOOL_BAR_OBS) {
		BAutolock l(mToolBarLock);
		mToolBarObservers.push_back( BMessenger(handler) );
	}
}

status_t AmGlobalsImpl::NewSong(AmSongRef& songRef, int32 undoHistory)
{
	AmSong*		song = new AmSong("Untitled Song");
	if (!song) return B_NO_MEMORY;
	if ( song->UndoContext() ) song->UndoContext()->SetHistorySize(undoHistory);
	{
		BAutolock _l(mSongLock);
		mSongRefs.push_back( AmSongRef(song) );
		songRef = AmSongRef(song);
	}
	return B_OK;
}

status_t AmGlobalsImpl::RemoveSong(song_id id)
{
	AmSongRef empty;
	AmSongObserver obs(empty);
	
	{
		BAutolock _l(mSongLock);
		bool	found = false;
		uint32	index = 0;
		for( uint32 k = 0; k < mSongRefs.size(); k++ ) {
			if( mSongRefs[k].SongId() == id ) {
				index = k;
				found = true;
				break;
			}
		}
		if( !found ) return B_ERROR;
		obs.SetSongRef(mSongRefs[index]);
		mSongRefs.erase( mSongRefs.begin() + index );
		
		if (mSelectedSong.SongId() == id) SelectSong(AmSongRef());
	}
	
	// WRITE SONG BLOCK
	AmSong*		song = obs.WriteLock();
	if( song ) song->Clear();
	obs.WriteUnlock( song );
	// END WRITE SONG BLOCK
	
	/* Send out a notice that the song is now gone from the system.
	 */
	BMessage	msg( AmSong::STATUS_OBS );
	msg.AddPointer( SZ_SONG_ID, obs.SongRef().SongId() );
	msg.AddInt32( SZ_STATUS, AM_REMOVED );
	obs.SongRef().ReportMsgChange( &msg, BMessenger() );
	return B_OK;
}

AmSongRef AmGlobalsImpl::SongRef(song_id id) const
{
	BAutolock _l(mSongLock);
	for( uint32 k = 0; k < mSongRefs.size(); k++ ) {
		if( mSongRefs[k].SongId() == id )
			return AmSongRef( mSongRefs[k] );
	}
	return AmSongRef();
}

void AmGlobalsImpl::SelectSong(const AmSongRef& songRef)
{
	BAutolock _l(mSongLock);
	if (mSelectedSong.SongId() != songRef.SongId()) {
		AmTransport* transport = mSelectedSong.Transport();
		if (transport) transport->SetClockTarget(NULL);
		mSelectedSong = songRef;
		transport = mSelectedSong.Transport();
		if (transport) transport->SetClockTarget(this);
	}
}

AmPipelineMatrixRef AmGlobalsImpl::PipelineMatrixRef(pipeline_matrix_id id) const
{
	/* Check to see if this matrix is a song.
	 */
	AmSongRef	songRef = SongRef(id);
	if (songRef.IsValid() ) return AmPipelineMatrixRef(songRef);
	/* Check to see if this matrix has been temporarily registered.
	 */
	for (uint32 k = 0; k < mTemporaryMatrices.size(); k++) {
		if (mTemporaryMatrices[k].Id() == id)
			return mTemporaryMatrices[k];
	}
	/* Check to see if this matrix is a tool.
	 */
	AmToolRoster*	tools = AmToolRoster::Default();
	if (tools) {
		AmToolRef	toolRef;
		for (uint32 k = 0; (toolRef = tools->ToolAt(k)).IsValid(); k++) {
			if (toolRef.ToolId() == id)
				return AmPipelineMatrixRef(toolRef.mTool);
		}
	}
	return AmPipelineMatrixRef();
}

AmToolRef AmGlobalsImpl::ToolAt(uint32 index) const
{
	AmToolRoster*		roster = AmToolRoster::Default();
	if (!roster) return AmToolRef();
	return roster->ToolAt(index);
}

AmToolRef AmGlobalsImpl::Tool(uint32 button, uint32 modifierKeys) const
{
	AmToolRoster*		roster = AmToolRoster::Default();
	if (!roster) return AmToolRef();
	return roster->Tool(button, modifierKeys);
}

AmToolRef AmGlobalsImpl::FindTool(const BString& uniqueName) const
{
	AmToolRoster*		roster = AmToolRoster::Default();
	if (!roster) return AmToolRef();
	return roster->FindTool(uniqueName);
}

AmToolRef AmGlobalsImpl::FindTool(int32 key) const
{
	AmToolRoster*		roster = AmToolRoster::Default();
	if (!roster) return AmToolRef();
	return roster->FindTool(key);
}

void AmGlobalsImpl::SetTool(const BString& toolName, uint32 button, uint32 modifierKeys)
{
	AmToolRoster*		roster = AmToolRoster::Default();
	if (!roster) return;
	roster->SetTool(toolName, button, modifierKeys);
}

#if 0
// Didn't look like this was being used...  If it's put back in, needs
// to include the modifier key.
	virtual status_t			GetToolMapping(const BString& toolName, uint32* button) const = 0;
	virtual status_t			GetToolMapping(const BString& toolName, uint32* button) const;

status_t AmGlobalsImpl::GetToolMapping(const BString& toolName, uint32* button) const
{
	AmToolRoster*		roster = AmToolRoster::Default();
	if (!roster) return B_ERROR;
	return roster->GetToolMapping(toolName, button);
}
#endif

AmToolBarRef AmGlobalsImpl::ToolBarAt(uint32 index, bool init) const
{
	BAutolock l(mToolBarLock);
	if (init && mToolBars.size() < 1) {
		AmGlobalsImpl* me = const_cast<AmGlobalsImpl*>(this);
		me->InitializeToolBars();
	}

	if (index >= mToolBars.size() ) return AmToolBarRef();
	return mToolBars[index];
}

AmToolBarRef AmGlobalsImpl::FindToolBar(const BString& toolBarName, bool init) const
{
	BAutolock l(mToolBarLock);
	if (init && mToolBars.size() < 1) {
		AmGlobalsImpl* me = const_cast<AmGlobalsImpl*>(this);
		me->InitializeToolBars();
	}

	for (uint32 k = 0; k < mToolBars.size(); k++) {
		if (toolBarName == mToolBars[k].ToolBarName() )
			return mToolBars[k];
	}
	return AmToolBarRef();
}

status_t AmGlobalsImpl::AddToolBar(AmToolBar* bar)
{
	BAutolock l(mToolBarLock);
	BString		n(bar->Name());
	for (uint32 k = 0; k < mToolBars.size(); k++) {
		if (n == mToolBars[k].ToolBarName() ) return B_NAME_IN_USE;
	}
	mToolBars.push_back( AmToolBarRef(bar) );

	BMessage	msg(TOOL_BAR_OBS);
	for (uint32 k = 0; k < mToolBarObservers.size(); k++) {
		if( mToolBarObservers[k].IsValid() ) {
			SafeSendMessage(mToolBarObservers[k], &msg);
		}
	}
	return B_OK;
}

status_t AmGlobalsImpl::ToggleShowing(const BString& toolBarName)
{
	AmToolBarRef	toolBarRef = FindToolBar(toolBarName);
	if (!toolBarRef.IsValid() ) return B_ERROR;
	// WRITE TOOL BAR BLOCK
	AmToolBar*		toolBar = toolBarRef.WriteLock();
	if (toolBar) {
		if (toolBar->IsShowing() ) toolBar->SetShowing(false);
		else toolBar->SetShowing(true);
	}
	toolBarRef.WriteUnlock(toolBar);
	// END WRITE TOOL BAR BLOCK
	BAutolock	l(mToolBarLock);

	BMessage	msg(TOOL_BAR_OBS);
	for (uint32 k = 0; k < mToolBarObservers.size(); k++) {
		if( mToolBarObservers[k].IsValid() ) {
			SafeSendMessage(mToolBarObservers[k], &msg);
		}
	}
	return B_OK;
}

status_t AmGlobalsImpl::DeleteToolBar(const BString& toolBarName)
{
	BAutolock l(mToolBarLock);
	for (uint32 k = 0; k < mToolBars.size(); k++) {
		if (toolBarName == mToolBars[k].ToolBarName() ) {
			mToolBars.erase(mToolBars.begin() + k);
			BMessage	msg(TOOL_BAR_OBS);
			for (uint32 k = 0; k < mToolBarObservers.size(); k++) {
				if( mToolBarObservers[k].IsValid() ) {
					SafeSendMessage(mToolBarObservers[k], &msg);
				}
			}
			return B_OK;
		}	
	}
	return B_ERROR;
}

AmViewFactory* AmGlobalsImpl::FactoryAt(uint32 index) const
{
	BAutolock l(mObserverLock);
	if( index >= mFactories.size() ) return 0;
	return mFactories[index];
}

AmViewFactory* AmGlobalsImpl::FactoryNamed(const BString& signature) const
{
	BAutolock l(mObserverLock);
	for( uint32 k = 0; k < mFactories.size(); k++ ) {
		if( mFactories[k]->Signature() == signature ) return mFactories[k];
	}
	return 0;
}

AmStudio& AmGlobalsImpl::Studio()
{
	return mStudio;
}

const AmStudio& AmGlobalsImpl::Studio() const
{
	return mStudio;
}

status_t AmGlobalsImpl::GetDeviceInfo(	const am_studio_endpoint& endpoint,
										BString& deviceMfg,
										BString& deviceProduct,
										BString& deviceLabel) const
{
	return mStudio.GetDeviceInfo(endpoint, deviceLabel, deviceMfg, deviceProduct);
}

ArpCRef<AmDeviceI> AmGlobalsImpl::DeviceAt(uint32 index) const
{
	AmDeviceRoster*		roster = AmDeviceRoster::Default();
	if (!roster) return NULL;
	return roster->DeviceAt(index);
}

ArpCRef<AmDeviceI> AmGlobalsImpl::DeviceAt(am_studio_endpoint& endpoint) const
{
//	BAutolock l(mObserverLock);
	return mStudio.DeviceFor(endpoint);
}

ArpCRef<AmDeviceI> AmGlobalsImpl::DeviceNamed(const char* mfg, const char* name) const
{
	AmDeviceRoster*		roster = AmDeviceRoster::Default();
	if (!roster) return NULL;
	return roster->DeviceNamed(mfg, name);
}

status_t AmGlobalsImpl::GetStudioInfoAt(uint32 index, am_studio_endpoint& outEndpoint,
										BString* outLabel, BString* outDevMfg,
										BString* outDevName) const
{
	return mStudio.GetInfoAt(index, outEndpoint, outLabel, outDevMfg, outDevName);
}

status_t AmGlobalsImpl::GetMotionInfo(	uint32 index,
										BString& outLabel,
										BString& outKey) const
{
	AmMotionRoster*		roster = AmMotionRoster::Default();
	if (!roster) return B_ERROR;
	return roster->GetMotionInfo(index, outLabel, outKey, NULL);
}

AmMotionI* AmGlobalsImpl::NewMotion(const BString& key) const
{
	AmMotionRoster*		roster = AmMotionRoster::Default();
	if (!roster) return NULL;
	return roster->NewMotion(key);
}

AmTime AmGlobalsImpl::EndTimeBuffer() const
{
	/* Default to eight measures of 4/4 time
	 */
	return (PPQN * 4) * 8;
}

AmPhraseEvent* AmGlobalsImpl::PhraseEventNear(const AmTrack* track, AmTime time) const
{
	ArpASSERT(track);
	/* Default to a tolerance of four measures in 4/4 time.
	 */
	AmTime				tolerance = (PPQN * 4) * 4;
	return PhraseEventNear(track, time, tolerance);
}

AmPhraseEvent* AmGlobalsImpl::PhraseEventNear(	const AmTrack* track,
												AmTime time,
												AmTime tolerance) const
{
	ArpASSERT(track);
	const AmPhrase&		phrase = track->Phrases();
	AmNode*				n = phrase.HeadNode();
	if (!n) return NULL;
	AmPhraseEvent*		pe = NULL;
	AmTime				currDistance = ( (tolerance + PPQN ) * 2), newDistance;
	while( n && (n->StartTime() <= ( time + tolerance ) ) ) {
		newDistance = distance_from( n->StartTime(), n->EndTime(), time );
		if( newDistance <= tolerance ) {
			if( !pe || newDistance <= currDistance ) {
				pe = dynamic_cast<AmPhraseEvent*>( n->Event() );
				currDistance = newDistance;
			}
		}
		n = n->next;
	}
	return pe;
}

status_t AmGlobalsImpl::GetMeasure(const AmPhrase& signatures, AmTime time, AmSignature* signature) const
{
	if( !signature ) return B_ERROR;
	AmNode*			node = signatures.FindNode( time, BACKWARDS_SEARCH );
	if( !node ) return B_ERROR;
	AmSignature*	nodeSig = dynamic_cast<AmSignature*>( node->Event() );
	if( !nodeSig ) return B_ERROR;
	
	AmTime			start = nodeSig->StartTime(), end = nodeSig->EndTime();
	AmTime			duration = end - start;
	int32			measure = nodeSig->Measure();
	while( end < time ) {
		start = end + 1;
		end = start + duration;
		measure++;
	}
	signature->Set( start, measure, nodeSig->Beats(), nodeSig->BeatValue() );
	return B_OK;
}

void AmGlobalsImpl::SetUndoHistory(int32 size)
{
	BAutolock _l(mSongLock);
	for( uint32 k = 0; k < mSongRefs.size(); k++ ) {
		AmSongObserver	obs( mSongRefs[k] );
		// WRITE SONG BLOCK
		AmSong*		song = obs.WriteLock();
		if ( song && song->UndoContext() ) song->UndoContext()->SetHistorySize(size);
		obs.WriteUnlock(song);
		// END WRITE SONG BLOCK
	}
}

status_t AmGlobalsImpl::WriteTo(BMessage* state)
{
	ArpASSERT(state);
	if (!state) return B_ERROR;
	for (uint32 k = 0; k < mToolBars.size(); k++) {
		AmToolBar*	toolbar = mToolBars[k].WriteLock();
		if (toolbar) {
			toolbar->Sync();
			BMessage	msg;
			if (toolbar->WriteTo(msg) == B_OK) state->AddMessage("toolbar", &msg);			
		}
		mToolBars[k].WriteUnlock(toolbar);
	}
	state->AddBool(CURVES_INITED, true);

	return B_OK;
}

status_t AmGlobalsImpl::ReadFrom(const BMessage* state)
{
	ArpASSERT(state);
	if (!state) return B_ERROR;
	mToolBars.resize(0);
	BMessage		msg;
	for (int32 k = 0; state->FindMessage("toolbar", k, &msg) == B_OK; k++) {
		AmToolBar*	toolbar = new AmToolBar(msg);
		if (toolbar && !toolbar->IsEmpty() ) mToolBars.push_back(toolbar);
		else delete toolbar;
		msg.MakeEmpty();
	}
	/* The FIRST time after the 2.0.2 update is run, Sequitur should
	 * create the Curves tool bar.  Never again.
	 */
	bool		b = false;
	if (state->FindBool(CURVES_INITED, &b) != B_OK) b = false;
	if (!b) {
		bool	add = true;
		BString	arpCurves(ARP_CURVES_STR);
		for (uint32 k = 0; k < mToolBars.size(); k++) {
			if (arpCurves == mToolBars[k].ToolBarName()) {
				add = false;
				break;
			}
		}
		if (add) InitializeCurvesToolBar();
	}
	
	return B_OK;
}

void AmGlobalsImpl::AddClockTarget(AmFilterAddOn* target)
{
	BAutolock _l(mSongLock);
	for( uint32 k = 0; k < mClockTargets.size(); k++ ) {
		if (mClockTargets[k] == target) return;
	}
	mClockTargets.push_back(target);
}

status_t AmGlobalsImpl::RemoveClockTarget(AmFilterAddOn* target)
{
	BAutolock _l(mSongLock);
	bool	found = false;
	uint32	index = 0;
	for( uint32 k = 0; k < mClockTargets.size(); k++ ) {
		if (mClockTargets[k] == target) {
			index = k;
			found = true;
			break;
		}
	}
	if( !found ) return B_ERROR;
	mClockTargets.erase( mClockTargets.begin() + index );
	return B_OK;
}

void AmGlobalsImpl::Clock(AmTime time)
{
	// XXX *** REALLY SHOULDN'T CALL THESE WITH THE LOCK HELD!! ***
	// But I am too tired to think of another way, right now.
//	printf("Clock at: %f QN!\n", ((double)time)/PPQN);
	BAutolock _l(mSongLock);
	for( uint32 k = 0; k < mClockTargets.size(); k++ ) {
		mClockTargets[k]->Clock(time);
	}
}

bool AmGlobalsImpl::RemoveObserver(observer_vec& observers, BMessenger messenger)
{
	BAutolock l(mObserverLock);
	observer_vec::iterator		i;
	for (i = observers.begin(); i != observers.end(); i++) {
		if( ! ((*i).IsValid() ) ) {
			observers.erase( i );
			return true;
		}
		if( ! ((*i) == messenger ) ) {
			observers.erase( i );
			return true;
		}
	}
	return false;
}

void AmGlobalsImpl::InitializeToolBars()
{
	if (mToolBars.size() > 0) return;
	AmToolRoster*	roster = AmToolRoster::Default();
	if (roster) {
		/* Basic
		 */
		AmToolBar*		bar = new AmToolBar("ARP Basic", true);
		if (bar) {
			AmToolRef	toolRef = roster->FindTool("arp:Pencil");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Select");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Wand");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Eraser");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Properties");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			mToolBars.push_back( AmToolBarRef(bar) );
		}
		/* Advanced
		 */
		bar = new AmToolBar("ARP Advanced", false);
		if (bar) {
			AmToolRef	toolRef;
			toolRef = roster->FindTool("arp:Echosystem");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Echosystem4");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:FollowTheLeader");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:GridWand");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:HotBeatInjection");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:PencilFlam");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Quantize");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Tail");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:TheMountain");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:TrackSplitter");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Tripletize");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Velocirapture");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			toolRef = roster->FindTool("arp:Chaos");
			if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
			mToolBars.push_back( AmToolBarRef(bar) );
		}
	}
	InitializeCurvesToolBar();
}

void AmGlobalsImpl::InitializeCurvesToolBar()
{
	AmToolRoster*	roster = AmToolRoster::Default();
	if (!roster) return;
	AmToolBar*		bar = new AmToolBar(ARP_CURVES_STR, false);
	if (!bar) return;

	AmToolRef	toolRef;
	toolRef = roster->FindTool("arp:Line");
	if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
	toolRef = roster->FindTool("arp:Bloom");
	if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());

	toolRef = roster->FindTool("arp:BrokenDownLine");
	if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());
	toolRef = roster->FindTool("pit:Sneaky Pitchmen");
	if (toolRef.IsValid() ) bar->InsertTool(toolRef.ToolKey());

	mToolBars.push_back( AmToolBarRef(bar) );
}

BClipboard*	AmGlobalsImpl::Clipboard() const
{
	if (mClipboard) return mClipboard;
	BString		str = "arp:sequitur_";
	str << system_time();
	mClipboard = new BClipboard( str.String() );
	return mClipboard;
}

void AmGlobalsImpl::AddFactory(AmViewFactory* factory)
{
	if (factory) mFactories.push_back(factory);
}

void AmGlobalsImpl::RegisterTemporaryMatrix(AmPipelineMatrixI* matrix)
{
	ArpASSERT(matrix);
	for (uint32 k = 0; k < mTemporaryMatrices.size(); k++) {
		ArpASSERT( mTemporaryMatrices[k].Id() != matrix->Id() );
	}
	mTemporaryMatrices.push_back(matrix);
}

void AmGlobalsImpl::UnregisterTemporaryMatrix(pipeline_matrix_id id)
{
	ArpASSERT(id);
	for (uint32 k = 0; k < mTemporaryMatrices.size(); k++) {
		if (mTemporaryMatrices[k].Id() == id) {
			mTemporaryMatrices.erase(mTemporaryMatrices.begin() + k);
			break;
		}
	}
	for (uint32 k = 0; k < mTemporaryMatrices.size(); k++) {
		ArpASSERT(mTemporaryMatrices[k].Id() != id);
	}
}

void AmGlobalsImpl::FreeClipboard()
{
	if( !mClipboard ) return;

	if( mClipboard->Lock() ) {
		BMessage*	data = mClipboard->Data();
		AmPhrase*	phrase;
		if( data && data->FindPointer( "sequitur_temp", (void**)&phrase ) == B_OK )
			delete phrase;
		data->MakeEmpty();
		mClipboard->Clear();
		mClipboard->Commit();
		mClipboard->Unlock();
	}

	delete mClipboard;
}
