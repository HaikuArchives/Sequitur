#include <be/support/Autolock.h>
#include <Glasslike/GlToolBar.h>
#include <ArpKernel/ArpDebug.h>

class _GlToolBarEntry
{
public:
	BString16			creator;
	int32				key;
	
	_GlToolBarEntry(const BString16& c, int32 k) : creator(c), key(k)	{ }
};

/***************************************************************************
 * GL-TOOL-BAR
 ***************************************************************************/
GlToolBar::GlToolBar(int32 type, const BString16* label)
		: mFlags(0), mType(type), mLabel(label)
{
}

GlToolBar::~GlToolBar()
{
	for (uint32 k = 0; k < mTools.size(); k++) delete mTools[k];
	mTools.resize(0);
}

bool GlToolBar::ReadLock() const
{
	return mReadLock.Lock();
}

bool GlToolBar::ReadUnlock() const
{
	mReadLock.Unlock();
	return true;
}

bool GlToolBar::WriteLock(const char* name)
{
	bool result = mWriteLock.Lock();
	return result;
}

bool GlToolBar::WriteUnlock()
{
	mWriteLock.Unlock();
	return true;
}

gl_id GlToolBar::Id() const
{
	return (void*)this;
}

int32 GlToolBar::Type() const
{
	return mType;
}

BString16 GlToolBar::Label() const
{
	return mLabel;
}

uint32 GlToolBar::Flags() const
{
	return mFlags;
}
	
status_t GlToolBar::GetTool(uint32 index, BString16& creator, int32* key) const
{
	if (index >= mTools.size()) return B_BAD_INDEX;
	ArpVALIDATE(mTools[index], return B_ERROR);
	creator = mTools[index]->creator;
	if (key) *key = mTools[index]->key;
	return B_OK;
}

status_t GlToolBar::AddTool(const BString16& creator, int32 key)
{
	/* FIX:  Make sure the tool doesn't already exist.
	 */
	_GlToolBarEntry*	e = new _GlToolBarEntry(creator, key);
	if (!e) return B_NO_MEMORY;
	mTools.push_back(e);
	return B_OK;
}

bool GlToolBar::Matches(int32 type, const BString16* str)
{
	if (mType != type) return false;
	ArpVALIDATE(str, return false);
	return mLabel == *str;
}

void GlToolBar::Print(uint32 tabs) const
{
	uint32			t;
	for (t = 0; t < tabs; t++) printf("\t");
	printf("GlToolBar %ld:%s (size %ld)\n", mType, mLabel.String(), mTools.size());
	for (uint32 k = 0; k < mTools.size(); k++) {
		for (t = 0; t < tabs + 1; t++) printf("\t");
		printf("%ld: %s:%ld\n", k, mTools[k]->creator.String(), mTools[k]->key);
	}
}

// #pragma mark -

/*************************************************************************
 * GL-TOOL-BAR-REF
 *************************************************************************/
GlToolBarRef::GlToolBarRef()
		: mBar(0)
{
}

GlToolBarRef::GlToolBarRef(const GlToolBar* bar)
		: mBar(const_cast<GlToolBar*>(bar))
{
	if (mBar) mBar->IncRefs();
}

GlToolBarRef::GlToolBarRef(const GlToolBarRef& ref)
		: mBar(0)
{
	mBar = ref.mBar;
	if (mBar) mBar->IncRefs();
}

GlToolBarRef::~GlToolBarRef()
{
	if (mBar) mBar->DecRefs();
}

GlToolBarRef& GlToolBarRef::operator=(const GlToolBarRef& ref)
{
	SetTo(ref.mBar);
	return *this;
}

bool GlToolBarRef::SetTo(const GlToolBar* bar)
{
	if (mBar) mBar->DecRefs();
	mBar = const_cast<GlToolBar*>(bar);
	if (mBar) mBar->IncRefs();
	
	return IsValid();
}

bool GlToolBarRef::SetTo(const GlToolBarRef& ref)
{
	SetTo(ref.mBar);
	return IsValid();
}

bool GlToolBarRef::IsValid() const
{
	return mBar != 0;
}

gl_id GlToolBarRef::ToolBarId() const
{
	if (!IsValid()) return 0;
	return mBar->Id();
}

int32 GlToolBarRef::ToolBarType() const
{
	if (!IsValid()) return 0;
	return mBar->Type();
}

const GlToolBar* GlToolBarRef::ReadLock() const
{
	if (!mBar) {
		return NULL;
	}
	if (!mBar->ReadLock() ) return NULL;
	return mBar;
}

void GlToolBarRef::ReadUnlock(const GlToolBar* tb) const
{
	if (tb) {
		if (tb != mBar) ArpASSERT(false);	// bad node returned to rootref
		else tb->ReadUnlock();
	}
}

GlToolBar* GlToolBarRef::WriteLock(const char* name)
{
	if (!mBar) {
		return 0;
	}
	if (!mBar->WriteLock(name) ) return NULL;
	return mBar;
}

void GlToolBarRef::WriteUnlock(GlToolBar* tb)
{
	if (tb) {
		if (tb != mBar) ArpASSERT(false);	// bad node returned to rootref
		else tb->WriteUnlock();
	}
}

// #pragma mark -

/***************************************************************************
 * GL-TOOL-BAR-ROSTER
 ****************************************************************************/
GlToolBarRoster::GlToolBarRoster()
{
}

uint32 GlToolBarRoster::Size() const
{
	BAutolock	l(&mLock);
	return uint32(mRefs.size());
}

GlToolBarRef GlToolBarRoster::GetToolBar(int32 type, uint32 index) const
{
	BAutolock	l(&mLock);
	uint32		c = 0;
	for (uint32 k = 0; k < mRefs.size(); k++) {
		if (type == mRefs[k].ToolBarType()) {
			if (c == index) return mRefs[k];
			c++;
		}
	}
	return GlToolBarRef();
}

status_t GlToolBarRoster::AddToolBar(GlToolBar* bar)
{
	ArpVALIDATE(bar, return B_ERROR);
	BAutolock	l(&mLock);
	/* FIX:  Make sure bar with same name doesn't exist.
	 */
	mRefs.push_back(GlToolBarRef(bar));
	return B_OK;
}

void GlToolBarRoster::Print() const
{
	BAutolock	l(&mLock);
	printf("GlToolBarRoster (size %ld)\n", mRefs.size());
	for (uint32 k = 0; k < mRefs.size(); k++) {
		const GlToolBar*	tb = mRefs[k].ReadLock();
		if (tb) tb->Print(1);
		mRefs[k].ReadUnlock(tb);
	}
}
