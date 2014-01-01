#include <be/app/Message.h>
#include <be/support/Autolock.h>
#include <ArpKernel/ArpDebug.h>
#include <GlKernel/GlDefs.h>
#include <Glasslike/GlKeyTracker.h>

static const char*		TRACKER_STR						= "tracker";
static const char*		M_STR							= "m";

static const char*		GL_ROOT_CREATOR_STR				= "crt";
static const char*		GL_ROOT_CATEGORY_STR			= "cat";
static const char*		GL_ROOT_LABEL_STR				= "lbl";
static const char*		GL_ROOT_KEY_STR					= "key";

/*************************************************************************
 * GL-KEY-TRACKER
 *************************************************************************/
GlKeyTracker::GlKeyTracker()
		: mDirty(false)
{
}

bool GlKeyTracker::IsDirty() const
{
	BAutolock			l(mAccess);
	return mDirty;	
}

status_t GlKeyTracker::GetCurrent(BString16& creator, int32* key) const
{
	BAutolock			l(mAccess);
	if (mCurrent.creator.Length() < 1) return B_ERROR;
	creator = mCurrent.creator;
	if (key) *key = mCurrent.key;
	return B_OK;
}

status_t GlKeyTracker::SetCurrent(const BString16& creator, int32 key)
{
	BAutolock			l(mAccess);
	if (creator == mCurrent.creator) {
		mCurrent.key = key;
		mDirty = true;
		return B_OK;
	}
	if (mCurrent.creator.Length() > 0) {
		ArpASSERT(IndexFor(creator) < 0);
		mEntries.push_back(_GlKeyTrackerEntry(mCurrent));
	}
	mCurrent.creator = creator;
	mCurrent.key = key;
	mDirty = true;
	return B_OK;
}

status_t GlKeyTracker::IncKey(int32* out)
{
	ArpVALIDATE(out, return B_ERROR);
	BAutolock			l(mAccess);
	if (mCurrent.creator.Length() < 1) return B_ERROR;

	mCurrent.key++;
	if (mCurrent.key < 0 || mCurrent.key >= 0x7FFFFFFF)
		mCurrent.key = 0;
	*out = mCurrent.key;
	return B_OK;
}

status_t GlKeyTracker::ReadFrom(const BMessage& config)
{
	BAutolock				l(mAccess);
	mCurrent.creator = "";
	mCurrent.key = 0;
	mEntries.resize(0);
	
	BMessage				trackerMsg;
	status_t				err = config.FindMessage(TRACKER_STR, &trackerMsg);
	if (err != B_OK) return err;

	mCurrent.ReadFrom(trackerMsg);
	BMessage				entryMsg;
	for (int32 k = 0; trackerMsg.FindMessage(M_STR, k, &entryMsg) == B_OK; k++) {
		_GlKeyTrackerEntry	e;
		if (e.ReadFrom(entryMsg) == B_OK) mEntries.push_back(e);
		entryMsg.MakeEmpty();
	}
	
	mDirty = false;
	return B_OK;
}

status_t GlKeyTracker::WriteTo(BMessage& config) const
{
	BAutolock			l(mAccess);

	BMessage			trackerMsg;
	status_t			err = mCurrent.WriteTo(trackerMsg);
	if (err != B_OK) return err;

	BMessage			entryMsg;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].WriteTo(entryMsg) == B_OK)
			trackerMsg.AddMessage(M_STR, &entryMsg);
		entryMsg.MakeEmpty();
	}
	err = config.AddMessage(TRACKER_STR, &trackerMsg);
	if (err == B_OK) mDirty = false;
	return err;
}

int32 GlKeyTracker::IndexFor(const BString16& creator) const
{
	if (creator.Length() < 1) return -1;
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k].creator == creator) return k;
	}
	return -1;
}

// #pragma mark -

/***************************************************************************
 * _GL-KEY-TRACKER-ENTRY
 ***************************************************************************/
_GlKeyTrackerEntry& _GlKeyTrackerEntry::operator=(const _GlKeyTrackerEntry& o)
{
	creator = o.creator;
	key = o.key;
	return *this;
}

status_t _GlKeyTrackerEntry::ReadFrom(const BMessage& config)
{
	config.FindInt32(GL_ROOT_KEY_STR, &key);
	status_t		err = config.FindString16(GL_ROOT_CREATOR_STR, &creator);
	if (err != B_OK) return err;
	if (creator.Length() < 1) return B_ERROR;
	return B_OK;
}

status_t _GlKeyTrackerEntry::WriteTo(BMessage& config) const
{
	if (creator.Length() < 1) return B_ERROR;
	status_t		err;
	if ((err = config.AddString16(GL_ROOT_CREATOR_STR, creator)) != B_OK) return err;
	if ((err = config.AddInt32(GL_ROOT_KEY_STR, key)) != B_OK) return err;
	return B_OK;
}
