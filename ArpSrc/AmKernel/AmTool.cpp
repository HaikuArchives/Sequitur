#include <stdio.h>
#include <string.h>
#include <be/app/Application.h>
#include <be/app/Roster.h>
#include <be/storage/Entry.h>
#include <be/storage/Path.h>
#include "ArpKernel/ArpDebug.h"
#include "ArpViewsPublic/ArpPrefsI.h"
#include "AmPublic/AmFilterI.h"
#include "AmPublic/AmGraphicEffect.h"
#include "AmPublic/AmSelectionsI.h"
#include "AmPublic/AmToolSeedI.h"
#include "AmPublic/AmToolTarget.h"
#include "AmPublic/AmViewFactory.h"
#include "AmKernel/AmFilterHolder.h"
#include "AmKernel/AmFilterRoster.h"
#include "AmKernel/AmPhraseEvent.h"
#include "AmKernel/AmSong.h"
#include "AmKernel/AmTool.h"
#include "AmKernel/AmToolControls.h"
#include "AmKernel/AmTrack.h"

const char*			SZ_TOOL_CLASS_NAME		= "seqt:class_name";
static const char*	NAME_STR				= "name";
static const char*	CLASS_NAME_STR			= "class_name";
static const char*	AUTHOR_STR				= "author";
static const char*	EMAIL_STR				= "email";
static const char*	TOOL_TIP_STR			= "tool_tip";
static const char*	SHORT_DESCRIPTION_STR	= "Short Description";
static const char*	LONG_DESCRIPTION_STR	= "Long Description";
static const char*	ACTIONS_STR				= "actions";
static const char*	SEED_STR				= "seed";
static const char*	PIPELINE_STR			= "pipeline";
static const char*	CONNECTIONS_STR			= "connections";
static const char*	ICON_STR				= "icon";
static const char*	GRAPHIC_STR				= "graphic";
static const char*	KEY_STR					= "key";
static const char*	CONFIG_STR				= "config";
static const char*	CHILD_STR				= "child";
static const char*	CONTROL_LIST_STR		= "control_list";

static const uint32	ICON_MSG				= 'icon';

/*************************************************************************
 * AM-TOOL
 *************************************************************************/
AmTool::AmTool(	const char* label, const char* key, const char* toolTip,
				const char* author, const char* email, bool readOnly)
		: mRefCount(0), mLabel(label), mKey(key), mAuthor(author), mEmail(email),
		  mIsValid(true), mToolTip(toolTip), mCurrSeed(NULL), mActions(0),
		  mControlList(NULL), mReadOnly(readOnly), mBuiltDynamicParams(false)
{
	mIcon = NULL;
	const BBitmap*		bm = ImageManager().FindBitmap(AM_INITIAL_TOOL_NORMAL);
	if (bm) mIcon = new BBitmap(bm);
	if (!mIcon) mIcon = new BBitmap(IconBounds(), B_RGBA32);
	PushPipeline();
}

AmTool::AmTool(const BMessage& config, bool readOnly, const char* filePath)
		: mRefCount(0), mIsValid(true), mCurrSeed(NULL),
		  mActions(0),
		  mControlList(NULL),
		  mReadOnly(readOnly), mFilePath(filePath), mBuiltDynamicParams(false)
{
	mIcon = NULL;
	ReadFrom(config);
}

AmTool::~AmTool()
{
	CleanToolParams();
	delete mCurrSeed;
	delete mIcon;
	delete mControlList;
}

void AmTool::AddRef() const
{
	AmTool* me = const_cast<AmTool*>(this);
	atomic_add(&me->mRefCount, 1);
}

void AmTool::RemoveRef() const
{
	AmTool* me = const_cast<AmTool*>(this);
	if( atomic_add(&me->mRefCount, -1) == 1 ) {
//		printf("AmTool::RemoveRef() delete tool %s (%s)\n", Label().String(), Key().String() );
		delete me;
	}
}

bool AmTool::ReadLock() const
{
	return mLock.ReadLock();
}

bool AmTool::WriteLock(const char* name)
{
	return mLock.WriteLock();
}

bool AmTool::ReadUnlock() const
{
	return mLock.ReadUnlock();
}

bool AmTool::WriteUnlock()
{
	return mLock.WriteUnlock();
}

void AmTool::SetDirty(bool dirty)
{
}

tool_id AmTool::Id() const
{
	return (void*)this;
}

uint32 AmTool::CountPipelines() const
{
	return mPipelines.size();
}

pipeline_id AmTool::PipelineId(uint32 pipelineIndex) const
{
	if (pipelineIndex >= mPipelines.size() ) return 0;
	return (void*)&(mPipelines[pipelineIndex]);
}

status_t AmTool::PipelineHeight(uint32 pipelineIndex, float* outHeight) const
{
	*outHeight = 23;
	return B_OK;
}

const BUndoContext* AmTool::UndoContext() const
{
	return NULL;
}

BUndoContext* AmTool::UndoContext()
{
	return NULL;
}

AmFilterHolderI* AmTool::Filter(pipeline_id id,
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

status_t AmTool::InsertFilter(	AmFilterAddOn* addon,
								pipeline_id id,
								AmPipelineType type,
								int32 beforeIndex,
								const BMessage* config)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) {
			status_t	err = B_ERROR;
			if (addon->Type() == addon->SOURCE_FILTER)
				err = mPipelines[k].SetSource(addon, config);
			else if (addon->Type() == addon->DESTINATION_FILTER)
				err = mPipelines[k].SetDestination(addon, config);
			else err = mPipelines[k].InsertFilter(addon, beforeIndex, config);
			if (err == B_OK) PipelineChanged(id, NULLINPUTOUTPUT_PIPELINE);
			return err;
		}
	}
	return B_ERROR;
}

status_t AmTool::ReplaceFilter(	AmFilterAddOn* addon,
								pipeline_id id,
								AmPipelineType type,
								int32 atIndex,
								const BMessage* config)
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) {
			status_t	err = B_ERROR;
			if (addon->Type() == addon->SOURCE_FILTER)
				err = mPipelines[k].SetSource(addon, config);
			else if (addon->Type() == addon->DESTINATION_FILTER)
				err = mPipelines[k].SetDestination(addon, config);
			else err = mPipelines[k].ReplaceFilter(addon, atIndex, config);
			if (err == B_OK) PipelineChanged(id, NULLINPUTOUTPUT_PIPELINE);
			return err;
		}
	}
	return B_ERROR;
}

status_t AmTool::RemoveFilter(	pipeline_id id,
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

status_t AmTool::MakeConnection(pipeline_id fromPid,
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

status_t AmTool::BreakConnection(	pipeline_id fromPid,
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

void AmTool::PipelineChanged(pipeline_id id, AmPipelineType type)
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

void AmTool::FilterChanged(pipeline_id id, AmPipelineType type)
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

status_t AmTool::AddMatrixPipelineObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0 || PipelineIndex(id) >= 0)
		return mNotifier.AddObserver(handler, AmNotifier::PIPELINE_CHANGE_OBS);
	else return B_ERROR;
}

status_t AmTool::AddMatrixFilterObserver(pipeline_id id, BHandler* handler)
{
	if (id == 0 || PipelineIndex(id) >= 0)
		return mNotifier.AddObserver(handler, AmNotifier::FILTER_CHANGE_OBS);
	else return B_ERROR;
}

status_t AmTool::RemoveMatrixObserver(pipeline_id id, BHandler* handler)
{
	ArpASSERT(id == 0 || PipelineIndex(id) >= 0);
	return mNotifier.RemoveObserverAll(handler);
}

BString AmTool::Label() const
{
	return mLabel;
}

BString AmTool::LocalFileName() const
{
	return convert_to_filename( Key() );
}

BString AmTool::LocalFilePath() const
{
	return mFilePath;
}

status_t AmTool::WriteTo(BMessage& msg) const
{
	if (mLabel.Length() > 0) msg.AddString(NAME_STR, mLabel.String() );
	if (mKey.Length() > 0) msg.AddString(CLASS_NAME_STR, mKey.String() );
	if (mAuthor.Length() > 0) msg.AddString(AUTHOR_STR, mAuthor.String() );
	if (mEmail.Length() > 0) msg.AddString(EMAIL_STR, mEmail.String() );
	if (mToolTip.Length() > 0) msg.AddString(TOOL_TIP_STR, mToolTip.String() );
	if (mShortDescription.Length() > 0) msg.AddString(SHORT_DESCRIPTION_STR, mShortDescription.String() );
	if (mLongDescription.Length() > 0) msg.AddString(LONG_DESCRIPTION_STR, mLongDescription.String() );

	msg.AddInt32(ACTIONS_STR, mActions);

	BMessage		seedMsg;
	if (mSeedRoot.Flatten(&seedMsg) == B_OK)
		msg.AddMessage(SEED_STR, &seedMsg);
	BMessage		graphicMsg;
	if (mGraphicRoot.Flatten(&graphicMsg) == B_OK)
		msg.AddMessage(GRAPHIC_STR, &graphicMsg);

	if (mIcon) {
		BMessage	iconMsg(ICON_MSG);
		if (mIcon->Archive(&iconMsg) == B_OK)
			msg.AddMessage(ICON_STR, &iconMsg);
	}
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		BMessage	pipelineMsg('null');
		if (mPipelines[k].FlattenFilters(&pipelineMsg, AM_PLAIN | AM_SOURCE | AM_DESTINATION) == B_OK)
			msg.AddMessage(PIPELINE_STR, &pipelineMsg);
	}
	BMessage		connections;
	if (FlattenConnections(&connections) == B_OK)
		msg.AddMessage(CONNECTIONS_STR, &connections);
	
	if (mControlList) {
		BMessage	controlMsg('null');
		if (mControlList->WriteTo(controlMsg) == B_OK)
			msg.AddMessage(CONTROL_LIST_STR, &controlMsg);
	}
	
	return B_OK;
}

const char* AmTool::ToolTip() const
{
	mToolTip = Label();
	if (mToolTip.Length() < 1) mToolTip = "Tool";
	AmTool*		me = const_cast<AmTool*>(this);
	if (me) me->CacheInOutEntries();
	if (mInEntries.size() > 1 || mOutEntries.size() > 1)
		mToolTip << " (" << mInEntries.size() << " in / " << mOutEntries.size() << " out)";
	return mToolTip.String();
}

BString AmTool::ShortDescription() const
{
	return mShortDescription;
}

void AmTool::LongDescription(BString& str) const
{
	str << mLongDescription;
}

const BBitmap* AmTool::Icon() const
{
	return mIcon;
}

BRect AmTool::IconBounds() const
{
	if (mIcon) return mIcon->Bounds();
	return BRect(0, 0, 23, 23);
}

BString AmTool::Key() const
{
	return mKey;
}

BString AmTool::Author() const
{
	return mAuthor;
}

BString AmTool::Email() const
{
	return mEmail;
}

bool AmTool::IsValid() const
{
	return mIsValid;
}

void AmTool::MouseDown(	AmSongRef songRef,
						AmToolTarget* target,
						BPoint where)
{
	ArpASSERT(target);
	CleanToolParams();
	if (!mCurrSeed || !target) return;
	mOriginPt = where;
	mOriginTime = target->TimeConverter().PixelToTick(where.x);
//	mOriginYValue = target->MoveYValueFromPixel(target->View()->ConvertToScreen(where).y);
	mOriginYValue = target->MoveYValueFromPixel(where.y);
	mSelectionRange.MakeInvalid();
	CacheInOutEntries();

	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	h = mPipelines[k].Head();
		while (h) {
			if (h->Filter() ) h->Filter()->Start(AmFilterI::TOOL_CONTEXT);
			h = h->NextInLine();
		}
	}

	AmSelectionsI*	selections = mCurrSeed->MouseDown(songRef, target, where);
	if (selections && mCurrSeed->NeedsProcessHack() ) selections = ProcessSelections(songRef, target, selections, where);
	if (selections) target->TrackWinProperties().SetSelections(selections);
	mCurrSeed->PostMouseDown(songRef, target, selections, where);
}

void AmTool::MouseMoved(AmSongRef songRef,
						AmToolTarget* target,
						BPoint where, uint32 code)
{
	ArpASSERT(mCurrSeed && target);
	if (!mCurrSeed || !target) return;
	AmSelectionsI*	selections = mCurrSeed->MouseMoved(songRef, target, where, code);
	if (selections) selections = ProcessSelections(songRef, target, selections, where);
	if (selections) target->TrackWinProperties().SetSelections(selections);
	mCurrSeed->PostMouseMoved(songRef, target, selections, where);
}

void AmTool::MouseUp(	AmSongRef songRef,
						AmToolTarget* target,
						BPoint where)
{
	ArpASSERT(mCurrSeed && target);
	if (!mCurrSeed || !target) return;
	AmSelectionsI*	selections = mCurrSeed->MouseUp(songRef, target, where);
	if (selections) selections = ProcessSelections(songRef, target, selections, where);
	if (selections) target->TrackWinProperties().SetSelections(selections);
	mCurrSeed->PostMouseUp(songRef, target, selections, where);

	// WRITE SONG BLOCK
	AmSong*					song = songRef.WriteLock();
	if (song && song->UndoContext() ) {
		/* Clearly I don't understand things very well, but apparently the
		 * mere act of asking for the last op is enough to merge the current
		 * fWorking in the undo state.  Once this is done, the name gets set
		 * correctly as specified in this method -- if it isn't, then the name
		 * is just a generic "Add Event"
		 */
		BUndoOperation*		op = song->UndoContext()->LastOperation(NULL, BResEditor::B_ANY_UNDO_MERGE);
		// Gads!  Just don't whine to me.
		if (op && op->AllowMerge() ) { ; }
		BString				undoName(mLabel);
		if (undoName.Length() < 1) undoName = "Unnamed Tool";
		else undoName << " Tool";
		song->UndoContext()->SetUndoName(undoName.String() );
		song->UndoContext()->CommitState();
	}
	songRef.WriteUnlock(song);
	// END WRITE SONG BLOCK

	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	h = mPipelines[k].Head();
		while (h) {
			if (h->Filter() ) h->Filter()->Stop(AmFilterI::TOOL_CONTEXT);
			h = h->NextInLine();
		}
	}
	mInEntries.resize(0);
	mOutEntries.resize(0);

	CleanToolParams();
}

void AmTool::DrawOn(BView* view, BRect clip)
{
	if (mCurrSeed) mCurrSeed->DrawOn(view, clip);
}

status_t AmTool::GetSeed(	uint32 index,
							BString& outFactoryKey,
							BString& outViewKey,
							BString& outSeedKey) const
{
	vector<BString>		path;
	uint32				count = 0;
	if (mSeedRoot.Key(index, &count, path, outSeedKey) != B_OK) return B_ERROR;
	if (path.size() == 1) {
		outFactoryKey = path[0];
	} else if (path.size() == 2) {
		outViewKey = path[0];
		outFactoryKey = path[1];
	} else {
		ArpASSERT(path.size() == 0);
	}
	return B_OK;
}

status_t AmTool::GetSeed(	const BString& factoryKey,
							const BString& viewKey,
							BString& outSeedKey,
							BMessage* outConfig) const
{
	vector<BString>		path;
	path.push_back(viewKey);
	path.push_back(factoryKey);
	status_t	err = mSeedRoot.GetKey(path, outSeedKey, outConfig);
	return err;
}

status_t AmTool::SetSeed(	const BString& factoryKey,
							const BString& viewKey,
							const BString& seedKey,
							const BMessage* config)
{
	return mSeedRoot.SetKey(factoryKey, viewKey, seedKey, config);
}

const AmToolSeedI* AmTool::CurrentSeed() const
{
	return mCurrSeed;
}

AmToolControlList* AmTool::NewControlList() const
{
	if (!mControlList) return NULL;
	AmToolControlList*	list = mControlList->Copy();
	if (!list) return NULL;
	list->SetTool(this);
	return list;
}

status_t AmTool::Graphic(	uint32 index,
							BString& outFactoryKey,
							BString& outViewKey,
							BString& outGraphicKey) const
{
	vector<BString>		path;
	uint32				count = 0;
	if (mGraphicRoot.Key(index, &count, path, outGraphicKey) != B_OK) return B_ERROR;
	if (path.size() == 1) {
		outFactoryKey = path[0];
	} else if (path.size() == 2) {
		outViewKey = path[0];
		outFactoryKey = path[1];
	} else {
		ArpASSERT(path.size() == 0);
	}
	return B_OK;
}

AmGraphicEffect* AmTool::NewGraphic(const BString& factoryKey,
									const BString& viewKey) const
{
	BString				key;
	BMessage			config;
	vector<BString>		path;
	path.push_back(viewKey);
	path.push_back(factoryKey);
	mGraphicRoot.GetKey(path, key, &config);
	return AmGraphicEffect::NewEffect(key, Id() );
}

status_t AmTool::SetGraphic(const BString& factoryKey,
							const BString& viewKey,
							const BString& graphicKey,
							const BMessage* config)
{
	return mGraphicRoot.SetKey(factoryKey, viewKey, graphicKey, config);
}

AmToolKeyHandler* AmTool::NewToolKeyHandler() const
{
	if (!mCurrSeed) return 0;
	return mCurrSeed->NewToolKeyHandler();
}

void AmTool::KeyDown(AmSongRef songRef, AmToolTarget* target, char byte)
{
	if (!mCurrSeed || !target) return;
	mCurrSeed->KeyDown(songRef, target, byte);
}

status_t AmTool::Prepare(	const BString& factoryKey,
							const BString& viewKey)
{
	delete mCurrSeed;
	mCurrSeed = NULL;

	BString				key;
	BMessage			config;
	vector<BString>		path;
	path.push_back(viewKey);
	path.push_back(factoryKey);
	mSeedRoot.GetKey(path, key, &config);
	mCurrSeed = AmToolSeedI::NewSeed(key);
	if (!mCurrSeed) return B_ERROR;
	mCurrSeed->SetConfiguration(&config);
	return B_OK;
}

void AmTool::SetLabel(const char* label)
{
	mLabel = label;
}

void AmTool::SetKey(const char* key)
{
	mKey = key;
}

void AmTool::SetAuthor(const char* author)
{
	mAuthor = author;
}

void AmTool::SetEmail(const char* email)
{
	mEmail = email;
}

void AmTool::SetIsValid(bool isValid)
{
	mIsValid = isValid;
}

void AmTool::SetShortDescription(const char* s)
{
	mShortDescription = s;
}

void AmTool::SetLongDescription(const char* s)
{
	mLongDescription = s;
}

void AmTool::SetToolTip(const char* toolTip)
{
	mToolTip = toolTip;
}

BBitmap* AmTool::Icon()
{
	return mIcon;
}

status_t AmTool::PushPipeline()
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
		addon = AmFilterRoster::Default()->FindFilterAddOn("arp:NullInput");
		if (addon) mPipelines[index].SetSource(addon, NULL);
		addon = AmFilterRoster::Default()->FindFilterAddOn("arp:NullOutput");
		if (addon) mPipelines[index].SetDestination(addon, NULL);
	}
	return err;
}

status_t AmTool::PopPipeline()
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

bool AmTool::IsReadOnly() const
{
	return mReadOnly;
}

AmTool* AmTool::Copy() const
{
	BMessage		flattened;
	if (WriteTo(flattened) != B_OK) return NULL;
	const char*		filePath = NULL;
	if (!mReadOnly) filePath = mFilePath.String();
	return new AmTool(flattened, false, filePath);
}

status_t AmTool::ReadFrom(const BMessage& msg)
{
	const char*		str;
	if (msg.FindString(NAME_STR, &str) == B_OK) mLabel = str;
	if (msg.FindString(CLASS_NAME_STR, &str) == B_OK) mKey = str;
	if (msg.FindString(AUTHOR_STR, &str) == B_OK) mAuthor = str;
	if (msg.FindString(EMAIL_STR, &str) == B_OK) mEmail = str;
	if (msg.FindString(TOOL_TIP_STR, &str) == B_OK) mToolTip = str;
	if (msg.FindString(SHORT_DESCRIPTION_STR, &str) == B_OK) mShortDescription = str;
	if (msg.FindString(LONG_DESCRIPTION_STR, &str) == B_OK) mLongDescription = str;
	int32			i;
	if (msg.FindInt32(ACTIONS_STR, &i) == B_OK) mActions = (uint32)i;

	BMessage		seedMsg;
	if (msg.FindMessage(SEED_STR, &seedMsg) == B_OK)
		mSeedRoot.Unflatten(&seedMsg);
	BMessage		graphicMsg;
	if (msg.FindMessage(GRAPHIC_STR, &graphicMsg) == B_OK)
		mGraphicRoot.Unflatten(&graphicMsg);

	delete mIcon;
	mIcon = NULL;
	BMessage		iconMsg;
	if (msg.FindMessage(ICON_STR, &iconMsg) == B_OK)
		mIcon = dynamic_cast<BBitmap*>( BBitmap::Instantiate(&iconMsg) );
	iconMsg.MakeEmpty();

	BMessage		pipelineMsg;
	for (int32 k = 0; msg.FindMessage(PIPELINE_STR, k, &pipelineMsg) == B_OK; k++) {
		mPipelines.push_back( AmPipelineSegment(NULLINPUTOUTPUT_PIPELINE, this, NULL, &pipelineMsg, AM_PLAIN | AM_SOURCE | AM_DESTINATION) );
		pipelineMsg.MakeEmpty();
	}
	BMessage		connections;
	if (msg.FindMessage(CONNECTIONS_STR, &connections) == B_OK)
		UnflattenConnections(&connections);

	delete mControlList;
	mControlList = NULL;
	BMessage	controlMsg;
	if (msg.FindMessage(CONTROL_LIST_STR, &controlMsg) == B_OK)
		mControlList = new AmToolControlList(controlMsg);
	return B_OK;
}

void AmTool::Print() const
{
	printf("AmTool: %s (%s) - %p\n", mLabel.String(), mKey.String(), this);
	printf("SEED TREE:\n");
	mSeedRoot.Print();
}

AmToolControlList* AmTool::ControlList() const
{
	if (!mControlList) mControlList = new AmToolControlList();
	return mControlList;
}

AmSelectionsI* AmTool::ProcessSelections(	AmSongRef songRef, AmToolTarget* target,
											AmSelectionsI* selections, BPoint where)
{
	ArpASSERT(selections);
	if (!selections) return NULL;
	AmSelectionsI*		newSelections = AmSelectionsI::NewSelections();
	if (!newSelections) return selections;
	// WRITE TRACK BLOCK
	AmSong*			song = songRef.WriteLock();
	if (song) {
		AmTime		oldSongTime = song->CountEndTime();
		uint32		trackCount = target->TrackWinProperties().CountOrderedTracks();

		if (!mBuiltDynamicParams) {
			AmEvent*		tempos = (song) ? song->PlaybackList(0, oldSongTime, PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_SIGNATURE) : NULL;
			if (tempos) tempos = tempos->HeadEvent();
			mParams.cur_tempo = dynamic_cast<AmTempoChange*>(tempos);
			AmEvent*		signatures = (song) ? song->PlaybackList(0, oldSongTime, PLAYBACK_NO_PERFORMANCE|PLAYBACK_NO_TEMPO) : NULL;
			if (signatures) signatures = signatures->HeadEvent();
			mParams.cur_signature = dynamic_cast<AmSignature*>(signatures);
			mParams.AddMotionChanges(song);
			mBuiltDynamicParams = true;
		}
		/* Cache the track in my input / output filters.
		 */
		for (uint32 ti = 0; ti < trackCount; ti++) {
			AmTrack*	track = song->Track(target->TrackWinProperties().OrderedTrackAt(ti).TrackId() );
			if (ti < mInEntries.size() ) mInEntries[ti].mTrack = track;
			if (ti < mOutEntries.size() ) mOutEntries[ti].mTrack = track;
			if (ti >= mInEntries.size() && ti >= mOutEntries.size() ) break;
		}
		/* Process each track's selections.
		 */
		trackCount = selections->CountTracks();
		for (uint32 ti = 0; ti < trackCount; ti++) {
			AmTrack*	track = song->Track(selections->TrackAt(ti) );
			if (track) ProcessSelections(track, ti, target, selections, where, newSelections);
		}
		newSelections->Sync(song);
		AmTime		newSongTime = song->CountEndTime();
		if (oldSongTime != newSongTime) song->EndTimeChangeNotice(newSongTime);
		for (uint32 k = 0; k < mInEntries.size(); k++) mInEntries[k].mTrack = NULL;
		for (uint32 k = 0; k < mOutEntries.size(); k++) mOutEntries[k].mTrack = NULL;
	}
	songRef.WriteUnlock(song);
	// END WRITE TRACK BLOCK
	return newSelections;
}

static AmPhraseEvent* get_top_phrase(_AmInOutEntry* entry, AmToolTarget* target, AmTime time)
{
	if (entry->mTopPhrase) return entry->mTopPhrase;
	AmPhraseEvent*		pe = target->PhraseEventNear(entry->mTrack, time);
	if (pe) {
		entry->mTopPhrase = pe;
		return pe;
	}
	pe = new AmRootPhraseEvent();
	if (pe) entry->mTopPhrase = pe;
	return pe;
}

AmSelectionsI* AmTool::ProcessSelections(	AmTrack* track, uint32 trackIndex,
											AmToolTarget* target,
											AmSelectionsI* oldSelections, BPoint where,
											AmSelectionsI* newSelections)
{
	ArpASSERT(mCurrSeed && target && target->View() && oldSelections && newSelections);
	if (!oldSelections) return NULL;
	if (!mSelectionRange.IsValid() ) mSelectionRange = oldSelections->TimeRange();
	/* Find the filter for the track that I input from.  If there is exactly one
	 * input and one output in my matrix, than my input and output track are always
	 * just the track supplied to this method.  Otherwise, I need to look those
	 * tracks up in the grid I've built.
	 */
	AmFilterHolderI*	holder = NULL;
	_AmInOutEntry		outEntry;
	bool				hasOutEntry = false;
	if (mInEntries.size() == 1 && mOutEntries.size() == 1) {
		holder = mInEntries[0].mHolder;
		outEntry = mOutEntries[0];
		outEntry.mTrack = track;
		hasOutEntry = true;
	} else {
		_AmInOutEntry*	entry = InEntry(track->Id() );
		if (entry) holder = entry->mHolder;
	}
	
	if (holder) {
		AmEvent*		tempos = (AmEvent*)mParams.cur_tempo;
		if (tempos) tempos = tempos->HeadEvent();
		AmEvent*		signatures = (AmEvent*)mParams.cur_signature;
		if (signatures) signatures = signatures->HeadEvent();

		am_tool_filter_params 	toolParams;
		FillToolParams(toolParams, track->Id(), target, where);
		/* Perform the filter, add the results to the new selections.
		 */
		AmPhraseEvent*	topPhrase;
		track_id		trackId = oldSelections->TrackAt(trackIndex);
		AmEvent*		cur = NULL;
		for (uint32 phraseIndex = 0; (cur = oldSelections->AsPhraseChain(track, phraseIndex, &topPhrase, holder)) != NULL; phraseIndex++) {
			cur = ArpExecFilters(cur, TOOL_EXEC_TYPE, false, &mParams, &toolParams, NULL, tempos, signatures);
			/* ArpExecFilters leaves these variables cleared out, so put them back.
			 */
			mParams.cur_tempo = dynamic_cast<AmTempoChange*>(tempos);
			mParams.cur_signature = dynamic_cast<AmSignature*>(signatures);

			/* The ArpExecFilters may have deleted events, in which case it may or may not still be
			 * in the cur chain, depending on what the filter that deleted it did, so I need to
			 * step through the selections and weed out any newly-deleted events.
			 */
			AmEvent*		delEvent;
			AmPhraseEvent*	delTopPhrase;
			track_id		deltTrackId;
			for (uint32 k = 0; oldSelections->EventAt(trackIndex, k, &deltTrackId, &delTopPhrase, &delEvent) == B_OK; k++) {
				if (delEvent->IsDeleted() ) mCurrSeed->ProcessEvent(track, delTopPhrase, delEvent, 0);			
			}

			while (cur) {
				/* This is set to true if the event existed before running ArpExecFilters(),
				 * i.e. it's in oldSelections.
				 */
				int32			extraData = 0;
				bool			preexisting = oldSelections->IncludesEvent(trackId, topPhrase, cur, &extraData);

//printf("\tLoop cur\n");
				AmEvent*			next = cur->RemoveEvent();
				_AmInOutEntry*		entry = NULL;
				if (cur->NextFilter() && cur->NextFilter()->Filter() ) {
					if (hasOutEntry) {
						outEntry.mTopPhrase = topPhrase;
						entry = &outEntry;
					} else entry = OutEntry(cur->NextFilter()->Filter()->Id() );
				}
				/* If the event doesn't have an output filter and it's not the
				 * event that was passed in (i.e. it was created in this ArpExecFilters),
				 * then it quietly disappears.
				 */
				if (!entry || !entry->mTrack) {
//printf("\tno entry or track\n");
					if (!preexisting) {
						ArpASSERT(cur->Parent() == NULL);
						cur->Delete();
					}
				} else {
					/* If the event is being moved between two tracks, that's a special
					 * case -- remove it from the old and add it to the new.
					 */
					if (preexisting && trackId != entry->mTrack->Id() ) {
//printf("\tHandle cur == event but different track\n");
						AmEvent*			copy = cur->Copy();
						if (copy) {
							cur->Delete();
							mCurrSeed->ProcessEvent(track, topPhrase, cur, 0);
							copy->SetPrevEvent(NULL);
							copy->SetNextEvent(NULL);
							AmPhraseEvent*	pe = get_top_phrase(entry, target, copy->StartTime() );
							if (pe) {
								newSelections->AddEvent(entry->mTrack->Id(), pe, copy, extraData);
								mCurrSeed->ProcessEvent(entry->mTrack, pe, copy, 0);
							}
						}
					} else {
//printf("\tHandle normal\n");
						/* If this event is in the same track as the event it was generated
						 * from, then automatically put it in the same phrase.
						 */
						if (!entry->mTopPhrase && trackId == entry->mTrack->Id() ) entry->mTopPhrase = topPhrase;
						AmPhraseEvent*	pe = get_top_phrase(entry, target, cur->StartTime() );
						if (pe) {
							if (cur->IsDeleted() == false)
								newSelections->AddEvent(entry->mTrack->Id(), pe, cur, extraData);
							mCurrSeed->ProcessEvent(entry->mTrack, pe, cur, 0);
						}
					}
				}
				cur = next;
			}
			for (uint32 k = 0; k < mOutEntries.size(); k++) mOutEntries[k].mTopPhrase = NULL;
		}
	} else {
		/* If there's no pipeline, then just process the selections.  Note that it IS
		 * necessary to process the events -- for example, the Create seed wil create
		 * events that need to be added.
		 */
		track_id		trackId;
		AmPhraseEvent*	topPhrase;
		AmEvent*		event;
		int32			extraData;
		for (uint32 k = 0; oldSelections->EventAt(trackIndex, k, &trackId, &topPhrase, &event, &extraData) == B_OK; k++) {
			ArpASSERT(topPhrase && event);
			newSelections->AddEvent(track->Id(), topPhrase, event, extraData);
			mCurrSeed->ProcessEvent(track, topPhrase, event, AmToolSeedI::NO_PROCESSING_FLAG);
		}
	}
	return newSelections;
}

void AmTool::CacheInOutEntries()
{
	mInEntries.resize(0);
	mOutEntries.resize(0);
	BString			inKey("arp:NullInput");
	BString			outKey("arp:NullOutput");
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		AmFilterHolderI*	h = mPipelines[k].Head();
		while (h) {
			if (h->Filter() && h->Filter()->AddOn() ) {
				if (inKey == h->Filter()->AddOn()->Key() )
					mInEntries.push_back(_AmInOutEntry(h));
				else if (outKey == h->Filter()->AddOn()->Key() )
					mOutEntries.push_back(_AmInOutEntry(h));
			}
			h = h->NextInLine();
		}
	}
}

_AmInOutEntry* AmTool::InEntry(track_id tid)
{
	if (mInEntries.size() == 1) return &(mInEntries[0]);

	for (uint32 k = 0; k < mInEntries.size(); k++) {
		if (mInEntries[k].mTrack && mInEntries[k].mTrack == tid) return &(mInEntries[k]);
	}
	return NULL;
}

_AmInOutEntry* AmTool::OutEntry(filter_id fid)
{
	if (mOutEntries.size() == 1) return &(mOutEntries[0]);
	
	for (uint32 k = 0; k < mOutEntries.size(); k++) {
		if (mOutEntries[k].mHolder && mOutEntries[k].mHolder->Filter()
				&& mOutEntries[k].mHolder->Filter()->Id() == fid)
			return &(mOutEntries[k]);
	}
	return NULL;
}


void AmTool::FillToolParams(am_tool_filter_params& params,
							track_id tid, AmToolTarget* target, BPoint where)
{
	params.orig_time		= mOriginTime;
	params.cur_time			= target->TimeConverter().PixelToTick(where.x);
	params.start_time		= mSelectionRange.start;
	params.end_time			= mSelectionRange.end;
	params.orig_y_pixel		= mOriginPt.y;
	params.cur_y_pixel		= where.y;
	params.orig_y_value		= mOriginYValue;
//	params.cur_y_value 		= target->MoveYValueFromPixel(target->View()->ConvertToScreen(where).y);
//printf("Where.y %f\n", where.y);
	params.cur_y_value		= target->MoveYValueFromPixel(where.y);
	params.track_context	= tid;
	int32		m, d;
	AmTime		v;
	target->TrackWinProperties().GetSplitGridTime(&m, &v, &d);
	params.grid_multiplier	= m;
	params.grid_value		= v;
	params.grid_divider		= d;
}

void AmTool::CleanToolParams()
{
	if (mParams.cur_tempo) const_cast<AmTempoChange*>(mParams.cur_tempo)->DeleteChain();
	if (mParams.cur_signature) const_cast<AmSignature*>(mParams.cur_signature)->DeleteChain();
	mParams.DeleteMotionChanges();
	mParams.cur_tempo = NULL;
	mParams.cur_signature = NULL;
	mBuiltDynamicParams = false;
}

int32 AmTool::PipelineIndex(pipeline_id id) const
{
	for (uint32 k = 0; k < mPipelines.size(); k++) {
		if (id == (void*)&(mPipelines[k])) return int32(k);
	}
	return -1;
}

status_t AmTool::FlattenConnections(BMessage* into) const
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

status_t AmTool::AddConnectionInfo(AmFilterHolderI* connection, BMessage& msg) const
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

status_t AmTool::UnflattenConnections(const BMessage* into)
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

status_t AmTool::UnflattenConnection(	int32 source_pi, int32 source_fi,
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

#if 0
	void					ProcessEvent(	AmTrack* track,
											AmPhraseEvent* topPhrase,
											AmEvent* event);
	void					AddPhrase(AmTrack* track, AmPhraseEvent* pe);
	void					AddEvent(	AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event);
	void					DeleteEvent(AmTrack* track,
										AmPhraseEvent* topPhrase,
										AmEvent* event);
#endif

#if 0
void AmTool::ProcessEvent(AmTrack* track, AmPhraseEvent* topPhrase, AmEvent* event)
{
	ArpASSERT(track && topPhrase && event);
	if (event->IsDeleted() ) {
		DeleteEvent(track, topPhrase, event);
		return;
	}
	if (topPhrase->Parent() == NULL) {
		AddPhrase(track, topPhrase);
	}
	if (event->Parent() == NULL) {
		AddEvent(track, topPhrase, event);
	}
}

void AmTool::AddPhrase(AmTrack* track, AmPhraseEvent* pe)
{
	ArpASSERT(track && pe);
	track->AddEvent(NULL, pe);
}

void AmTool::AddEvent(AmTrack* track, AmPhraseEvent* topPhrase, AmEvent* event)
{
	ArpASSERT(track && topPhrase && event);
	track->AddEvent(topPhrase->Phrase(), event);
}

bool static is_first_tempo(AmPhrase* container, AmEvent* event)
{
	ArpASSERT(event && container);
	if (!container) return false;
	if (event->Type() != event->TEMPOCHANGE_TYPE) return false;
	AmNode*		head = container->HeadNode();
	if (!head) return false;
	return head->Event() == event;
}

void AmTool::DeleteEvent(AmTrack* track, AmPhraseEvent* topPhrase, AmEvent* event)
{
	ArpASSERT(track && topPhrase && event);
	if ( is_first_tempo(topPhrase->Phrase(), event) ) return;
	if (track->RemoveEvent(topPhrase->Phrase(), event) != B_OK) return;
	/* Remove the container if it's now empty.
	 */
	if ( topPhrase->IsEmpty() ) {
		track->RemoveEvent(NULL, topPhrase);
	}
}
#endif

/*************************************************************************
 * _AM-TOOL-VIEW-NODE
 *************************************************************************/
_AmToolViewNode::_AmToolViewNode()
{
}

_AmToolViewNode::_AmToolViewNode(const BString& name)
		: mName(name)
{
}

_AmToolViewNode::_AmToolViewNode(	const BString& name,
									const BString& key)
		: mName(name), mKey(key)
{
}

_AmToolViewNode::_AmToolViewNode(const _AmToolViewNode& o)
		: mName(o.mName), mKey(o.mKey), mConfig(o.mConfig)
{
	for (uint32 k = 0; k < o.mChildren.size(); k++) {
		mChildren.push_back(o.mChildren[k]);
	}
}

_AmToolViewNode& _AmToolViewNode::operator=(const _AmToolViewNode &e)
{
	mName = e.mName;
	mKey = e.mKey;
	mConfig = e.mConfig;
	mChildren.resize(0);
	for (uint32 k = 0; k < e.mChildren.size(); k++) {
		mChildren.push_back(e.mChildren[k]);
	}
	return *this;
}

status_t _AmToolViewNode::SetKey(	const BString& factoryKey,
									const BString& viewKey,
									const BString& key,
									const BMessage* config)
{
	/* If there's no factory, then set my key.
	 */
	if (factoryKey.Length() < 1) {
		mKey = key;
		mConfig.MakeEmpty();
		if (config) mConfig = *config;
		return B_OK;
	}
	/* Find the factory node.  If there's no view key, set up
	 * the key for the factory node.
	 */
	_AmToolViewNode*	facNode = FindChild(factoryKey);
	if (!facNode) {
		mChildren.push_back(factoryKey);
		facNode = FindChild(factoryKey);
	}
	if (!facNode) return B_NO_MEMORY;
	if (viewKey.Length() < 1) {
		facNode->mKey = key;
		facNode->mConfig.MakeEmpty();
		if (config) facNode->mConfig = *config;
		return B_OK;
	}
	/* Final case, there's a view key.  Track it down or
	 * make it and set up its key.
	 */
	_AmToolViewNode*	viewNode = facNode->FindChild(viewKey);
	if (!viewNode) {
		facNode->mChildren.push_back(viewKey);
		viewNode = facNode->FindChild(viewKey);
	}
	if (!viewNode) return B_NO_MEMORY;
	viewNode->mKey = key;
	viewNode->mConfig.MakeEmpty();
	if (config) viewNode->mConfig = *config;
	return B_OK;
}

status_t _AmToolViewNode::GetKey(	vector<BString> path,
									BString& outKey,
									BMessage* config) const
{
	if (path.size() == 0) {
		if (mKey.Length() > 0) outKey = mKey;
		if (config) *config = mConfig;
		return B_OK;
	}
	const _AmToolViewNode*	child = FindChild(path[path.size() - 1]);
	if (!child) {
		if (mKey.Length() > 0) {
			outKey = mKey;
			if (config) *config = mConfig;
			return B_OK;
		}
		return B_ERROR;
	}
	path.pop_back();
	child->GetKey(path, outKey, config);
	if (outKey.Length() > 0) return B_OK;
	if (mKey.Length() > 0) {
		outKey = mKey;
		if (config) *config = mConfig;
		return B_OK;
	}
	return B_ERROR;
}

status_t _AmToolViewNode::Key(	uint32 index, uint32* count,
								vector<BString>& outPath,
								BString& outKey) const
{
	if (index == *count) {
		outKey = mKey;
		if (mName.Length() > 0) outPath.push_back(mName);
		return B_OK;
	}
	
	for (uint32 k = 0; k < mChildren.size(); k++) {
		(*count)++;
		if (mChildren[k].Key(index, count, outPath, outKey) == B_OK) {
			if (mName.Length() > 0) outPath.push_back(mName);
			return B_OK;
		}
		ArpASSERT(*count <= index);
		if (*count > index) return B_ERROR;
	}
	return B_ERROR;
}

status_t _AmToolViewNode::Flatten(BMessage* into) const
{
	ArpASSERT(into);
	status_t	err;
	if (mName.Length() > 0) into->AddString(NAME_STR, mName);
	if (mKey.Length() > 0) into->AddString(KEY_STR, mKey);
	if (!mConfig.IsEmpty() ) into->AddMessage(CONFIG_STR, &mConfig);
	
	for (uint32 k = 0; k < mChildren.size(); k++) {
		BMessage	childMsg;
		if (mChildren[k].Flatten(&childMsg) == B_OK) {
			err = into->AddMessage(CHILD_STR, &childMsg);
			if (err != B_OK) return err;
		}
	}
	return B_OK;
}

status_t _AmToolViewNode::Unflatten(const BMessage* from)
{
	ArpASSERT(from);
	mName = (const char*)NULL;
	mKey = (const char*)NULL;
	mConfig.MakeEmpty();
	mChildren.resize(0);

	const char*		s;
	BMessage		msg;
	if (from->FindString(NAME_STR, &s) == B_OK) mName = s;
	if (from->FindString(KEY_STR, &s) == B_OK) mKey = s;
	if (from->FindMessage(CONFIG_STR, &msg) == B_OK) mConfig = msg;
	
	BMessage		childMsg;
	for (int32 k = 0; from->FindMessage(CHILD_STR, k, &childMsg) == B_OK; k++) {
		_AmToolViewNode		node;
		if (node.Unflatten(&childMsg) == B_OK) mChildren.push_back(node);
		childMsg.MakeEmpty();
	}
	return B_OK;
}

void _AmToolViewNode::Print(int32 tabs) const
{
	for (int32 k = 0; k < tabs; k++) printf("\t");
	printf("NAME: %s KEY: %s config:\n", mName.String(), mKey.String());
	mConfig.PrintToStream();
	for (uint32 k = 0; k < mChildren.size(); k++) {
		mChildren[k].Print(tabs + 1);
	}
}

const _AmToolViewNode* _AmToolViewNode::FindChild(const BString& name) const
{
	for (uint32 k = 0; k < mChildren.size(); k++) {
		if (mChildren[k].mName == name) return &(mChildren[k]);
	}
	return NULL;
}

_AmToolViewNode* _AmToolViewNode::FindChild(const BString& name)
{
	for (uint32 k = 0; k < mChildren.size(); k++) {
		if (mChildren[k].mName == name) return &(mChildren[k]);
	}
	return NULL;
}

/*************************************************************************
 * _AM-INOUT-ENTRY
 *************************************************************************/
_AmInOutEntry::_AmInOutEntry()
		: mHolder(NULL), mTrack(NULL), mTopPhrase(NULL)
{
}

_AmInOutEntry::_AmInOutEntry(const _AmInOutEntry& o)
		: mHolder(o.mHolder), mTrack(NULL), mTopPhrase(NULL)
{
}

_AmInOutEntry::_AmInOutEntry(AmFilterHolderI* holder)
		: mHolder(holder), mTrack(NULL), mTopPhrase(NULL)
{
}

_AmInOutEntry& _AmInOutEntry::operator=(const _AmInOutEntry& o)
{
	mHolder = o.mHolder;
	mTrack = NULL;
	mTopPhrase = NULL;
	return *this;
}
