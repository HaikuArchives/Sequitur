/* AmToolBar.cpp
 */
#include <cstdio>
#include <cstring>
#include "ArpKernel/ArpDebug.h"
#include "AmKernel/AmFileRosters.h"
#include "AmKernel/AmToolBar.h"

static const char*		NAME_STR				= "name";
static const char*		SHOWING_STR				= "showing";
static const char*		TOOL_KEY_STR			= "tool_key";

/*************************************************************************
 * AM-TOOL-BAR
 *************************************************************************/
AmToolBar::AmToolBar(const char* name, bool showing)
		: mName(name), mShowing(showing)
{
}

AmToolBar::AmToolBar(const BMessage& config)
		: mShowing(false)
{
	const char*		s;
	bool			b;
	if (config.FindString(NAME_STR, &s) == B_OK) mName = s;
	if (config.FindBool(SHOWING_STR, &b) == B_OK) mShowing = b;
	for (uint32 k = 0; config.FindString(TOOL_KEY_STR, k, &s) == B_OK; k++) {
		mToolKeys.push_back(s);
	}
}

AmToolBar::AmToolBar(const AmToolBar& o)
		: mName(o.mName), mShowing(o.mShowing)
{
	for (uint32 k = 0; k < o.mToolKeys.size(); k++)
		mToolKeys.push_back(o.mToolKeys[k] );
}

AmToolBar::~AmToolBar()
{
}

void AmToolBar::AddRef() const
{
	AmToolBar* me = const_cast<AmToolBar*>(this);
	atomic_add(&me->mRefCount, 1);
}

void AmToolBar::RemoveRef() const
{
	AmToolBar* me = const_cast<AmToolBar*>(this);
	if (atomic_add(&me->mRefCount, -1) == 1) {
		printf("AmToolBar::RemoveRef() delete tool bar %s\n", Name() );
		delete me;
	}
}

bool AmToolBar::ReadLock() const
{
	return mLock.ReadLock();
}

bool AmToolBar::WriteLock(const char* name)
{
	return mLock.WriteLock();
}

bool AmToolBar::ReadUnlock() const
{
	return mLock.ReadUnlock();
}

bool AmToolBar::WriteUnlock()
{
	return mLock.WriteUnlock();
}

status_t AmToolBar::AddObserver(BHandler* handler)
{
	return mNotifier.AddObserver(handler, TOOL_BAR_OBS);
}

status_t AmToolBar::RemoveObserver(BHandler* handler)
{
	return mNotifier.RemoveObserverAll(handler);
}

tool_bar_id AmToolBar::Id() const
{
	return (void*)this;
}

const char* AmToolBar::Name() const
{
	return mName.String();
}

bool AmToolBar::IsEmpty() const
{
	return mToolKeys.size() < 1;
}

bool AmToolBar::IsShowing() const
{
	return mShowing;
}

void AmToolBar::SetShowing(bool showing)
{
	mShowing = showing;
}

uint32 AmToolBar::CountTools() const
{
	return mToolKeys.size();
}

AmToolRef AmToolBar::ToolAt(uint32 index) const
{
	if (index >= mToolKeys.size() ) return AmToolRef();
	AmToolRoster*	roster = AmToolRoster::Default();
	if (!roster) return AmToolRef();
	return roster->FindTool(mToolKeys[index].String() );
}

status_t AmToolBar::InsertTool(const BString& toolKey, int32 index)
{
	bool		needsCleanup = ContainsTool(toolKey);
	int32		insertedIndex = int32(index);
	if (index < 0 || index >= (int32)mToolKeys.size() ) {
		mToolKeys.push_back(toolKey);
		insertedIndex = int32(mToolKeys.size()) - 1;
	} else {
		mToolKeys.insert(mToolKeys.begin() + index, toolKey);
	}
	if (needsCleanup && insertedIndex >= 0) {
		for (uint32 k = 0; k < mToolKeys.size(); k++) {
			if (int32(k) != insertedIndex && mToolKeys[k] == toolKey) {
				mToolKeys.erase(mToolKeys.begin() + k);
				break;
			}
		}
	}
	ArpASSERT(ContainsTool(toolKey) == true);
	BMessage	msg(TOOL_BAR_OBS);
	msg.AddInt32("action", AM_ADDED);
	msg.AddInt32("tool_index", insertedIndex);
	mNotifier.ReportMsgChange( &msg, BMessenger() );
	return B_OK;
}

status_t AmToolBar::ReplaceTool(const BString& toolKey, int32 index)
{
	ArpASSERT(toolKey.Length() > 0);
	if (index < 0 || index >= (int32)mToolKeys.size() ) return B_BAD_INDEX;
	int32		oldIndex = ToolIndex(toolKey);
	mToolKeys[(uint32)index] = toolKey;
	if (oldIndex >= 0 && oldIndex != index) {
		if (oldIndex > index) oldIndex = ToolIndex(toolKey, index + 1);
		if (oldIndex >= 0) mToolKeys.erase(mToolKeys.begin() + oldIndex);
	}
	ArpASSERT(ContainsTool(toolKey) == true);
	BMessage	msg(TOOL_BAR_OBS);
	msg.AddInt32("action", AM_CHANGED);
	msg.AddInt32("tool_index", index);
	mNotifier.ReportMsgChange( &msg, BMessenger() );
	return B_OK;
}

status_t AmToolBar::RemoveTool(const BString& toolKey)
{
	for (uint32 k = 0; k < mToolKeys.size(); k++) {
		if (mToolKeys[k] == toolKey) {
			mToolKeys.erase(mToolKeys.begin() + k);
			BMessage	msg(TOOL_BAR_OBS);
			msg.AddInt32("action", AM_REMOVED);
			msg.AddInt32("tool_index", k);
			mNotifier.ReportMsgChange( &msg, BMessenger() );
			ArpASSERT(ContainsTool(toolKey) == false);
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}

status_t AmToolBar::ToolChange(const BString& toolKey)
{
	for (uint32 k = 0; k < mToolKeys.size(); k++) {
		if (mToolKeys[k] == toolKey) {
			BMessage	msg(TOOL_BAR_OBS);
			msg.AddInt32("action", AM_CHANGED);
			msg.AddInt32("tool_index", k);
			mNotifier.ReportMsgChange( &msg, BMessenger() );
			return B_OK;
		}
	}
	return B_BAD_INDEX;
}

static bool scrub_one(std::vector<BString>& keys, AmToolRoster* roster)
{
	for (uint32 k = 0; k < keys.size(); k++) {
		if (roster->KeyExists(keys[k].String()) == false) {
			keys.erase(keys.begin() + k);
			return true;
		}
	}
	return false;
}

status_t AmToolBar::Sync()
{
	AmToolRoster*	roster = AmToolRoster::Default();
	if (!roster) return B_ERROR;
	while (scrub_one(mToolKeys, roster) ) ;
	return B_OK;
}

status_t AmToolBar::WriteTo(BMessage& config) const
{
	config.AddString(NAME_STR, mName.String() );
	config.AddBool(SHOWING_STR, mShowing);
	for (uint32 k = 0; k < mToolKeys.size(); k++) {
		if (mToolKeys[k].Length() > 0)
			config.AddString(TOOL_KEY_STR, mToolKeys[k]);
	}
	return B_OK;
}

bool AmToolBar::ContainsTool(const BString& toolKey) const
{
	for (uint32 k = 0; k < mToolKeys.size(); k++) {
		if (mToolKeys[k] == toolKey) return true;
	}
	return false;
}

int32 AmToolBar::ToolIndex(const BString& toolKey, uint32 startingAt) const
{
	if (startingAt >= mToolKeys.size() ) return -1;
	for (uint32 k = startingAt; k < mToolKeys.size(); k++) {
		if (mToolKeys[k] == toolKey) return int32(k);
	}
	return -1;
}

