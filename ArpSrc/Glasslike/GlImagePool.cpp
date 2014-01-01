#include <be/support/Autolock.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpInterface/ArpBitmap.h>
#include "Glasslike/GlImagePool.h"

class GlImagePoolEntry
{
public:
	GlImagePoolEntry();
	~GlImagePoolEntry();
	
	int32			count;
	BString16		filename;
	GlImage*		image;
	
	status_t		Cache();
	void			Uncache();
};

/*************************************************************************
 * GL-IMAGE-POOL
 *************************************************************************/
GlImagePool::GlImagePool()
{
}

GlImagePool::~GlImagePool()
{
	for (uint32 k = 0; k < mEntries.size(); k++) delete mEntries[k];
	mEntries.resize(0);
}

gl_image_id GlImagePool::Acquire(const BString16& filename)
{
	BAutolock			l(mAccess);
	GlImagePoolEntry*	e = GetEntry(filename, 0);
	if (e) {
		if (!e->image) return 0;
		e->count++;
		return e->image->Id();
	}
	e = new GlImagePoolEntry();
	if (!e) return 0;
	e->filename = filename;
	e->Cache();
	if (e->image) {
		e->count++;
		mEntries.push_back(e);
		return e->image->Id();
	}
	delete e;
	return 0;
}

gl_image_id GlImagePool::Acquire(GlImage* image)
{
	ArpVALIDATE(image, return 0);
	BAutolock			l(mAccess);
	GlImagePoolEntry*	e = GetEntry(image->Id(), 0);
	if (e) {
		ArpASSERT(e->image);
		e->count++;
		return e->image->Id();
	}
	e = new GlImagePoolEntry();
	if (!e) return 0;
	e->image = image;
	e->count++;
	mEntries.push_back(e);
	return e->image->Id();
}

gl_image_id GlImagePool::Acquire(gl_image_id id)
{
	ArpVALIDATE(id, return 0);
	BAutolock			l(mAccess);
	GlImagePoolEntry*	e = GetEntry(id, 0);
	if (!e) return 0;
	ArpASSERT(e->image);
	e->count++;
	return e->image->Id();
}

status_t GlImagePool::Release(gl_image_id id)
{
	if (!id) return B_OK;
	BAutolock			l(mAccess);
	uint32				index;
	GlImagePoolEntry*	e = GetEntry(id, &index);
	if (!e) return B_ERROR;
	e->count--;
	if (e->count > 0) return B_OK;
	mEntries.erase(mEntries.begin() + index);
	delete e;
	return B_OK;
}

GlImage* GlImagePool::Clone(gl_image_id id)
{
	if (!id) return 0;
	BAutolock			l(mAccess);
	GlImagePoolEntry*	e = GetEntry(id, 0);
	if (!e) return 0;
	if (e->image == 0) e->Cache();
	if (e->image) return e->image->Clone();
	return 0;
}

ArpBitmap* GlImagePool::CloneBitmap(gl_image_id id)
{
	if (!id) return 0;
	BAutolock			l(mAccess);
	GlImagePoolEntry*	e = GetEntry(id, 0);
	if (!e) return 0;
	if (e->image) return e->image->AsBitmap();
	e->Cache();
	if (e->image) return e->image->AsBitmap();
	return 0;
}

const GlImage* GlImagePool::Source(gl_image_id id)
{
	if (!id) return 0;
	BAutolock			l(mAccess);
	GlImagePoolEntry*	e = GetEntry(id, 0);
	if (e && e->image) return e->image;
	return 0;	
}

GlImagePoolEntry* GlImagePool::GetEntry(const BString16& filename, uint32* index)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		ArpASSERT(mEntries[k]);
		if (mEntries[k]->filename == filename) {
			if (index) *index = k;
			return mEntries[k];
		}
	}
	return 0;
}

GlImagePoolEntry* GlImagePool::GetEntry(gl_image_id id, uint32* index)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		ArpASSERT(mEntries[k]);
		if (mEntries[k]->image && mEntries[k]->image == id) {
			if (index) *index = k;
			return mEntries[k];
		}
	}
	return 0;
}

// #pragma mark -

/*************************************************************************
 * GL-IMAGE-POOL-ENTRY
 *************************************************************************/
GlImagePoolEntry::GlImagePoolEntry()
		: count(0), image(0)
{
}	

GlImagePoolEntry::~GlImagePoolEntry()
{
#if 0
if (count > 0 && image) {
	ArpBitmap*	bm = image->AsBitmap();
	if (bm) {
		bm->Save("/boot/home/glasslikefuck.png", ARP_PNG_FORMAT);
		delete bm;
	}
}
#endif
	ArpASSERT(count == 0);
	Uncache();
}

status_t GlImagePoolEntry::Cache()
{
	if (image) return B_OK;
	ArpBitmap*		bm = new ArpBitmap(filename);
	if (!bm) return B_ERROR;
	image = new GlImage(*bm);
	delete bm;
	if (image && image->InitCheck() == B_OK) return B_OK;
	delete image;
	image = 0;
	return B_NO_MEMORY;
}

void GlImagePoolEntry::Uncache()
{
	delete image;
	image = 0;
}
