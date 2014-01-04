//#include <AppKit.h>
#include <StorageKit.h>
#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpBitmap.h>
#include <ArpInterface/ArpPainter.h>
#include <ArpInterface/ArpPrefs.h>
#include <Glasslike/GlSkinCache.h>


// GL-LABEL-CACHE-ENTRY
class GlLabelCacheEntry
{
public:
	BString16		label;
	ArpBitmap*		bm;

	GlLabelCacheEntry(const BString16& s, ArpBitmap* b) : label(s), bm(b)		{ }
	~GlLabelCacheEntry()														{ delete bm; }
};

/***************************************************************************
 * GL-LABEL-CACHE
 ***************************************************************************/
class GlLabelCache
{
public:
	GlLabelCache();
	virtual ~GlLabelCache();
	
	const ArpBitmap*			Acquire(const BString16& text);
	void						Release(const ArpBitmap* bm);

	ArpBitmap*					MakeFallback(const BString16& text);

private:
	vector<GlLabelCacheEntry*>	mEntries;
};

class GlChainCache : public GlLabelCache
{
public:
	GlChainCache()		{ }

//	status_t			InitCheck() const;
};

#if 0
/**********************************************************************
 * GL-SKIN-I
 **********************************************************************/
static GlSkinI* gSkin = NULL;

GlSkinI& GlSkin()
{
	if (!gSkin) ArpASSERT(false);
	return *gSkin;
}

void SetSkin(GlSkinI& skin)
{
	if (gSkin) ArpASSERT(false);
	gSkin = &skin;
}
#endif

/***************************************************************************
 * GL-SKIN-CACHE
 ***************************************************************************/
GlSkinCache::GlSkinCache()
		: mChainCache(0)
{
//	SetSkin(*this);
}

GlSkinCache::~GlSkinCache()
{
	Free();
}

const ArpBitmap* GlSkinCache::AcquireChain(const BString16& text)
{
	if (!mChainCache) return 0;
	return mChainCache->Acquire(text);
}

void GlSkinCache::ReleaseChain(const ArpBitmap* bm)
{
	if (!bm || !mChainCache) return;
	mChainCache->Release(bm);
}

status_t GlSkinCache::Initialize()
{
	if (!mChainCache) mChainCache = new GlChainCache();
	if (!mChainCache) return B_NO_MEMORY;

//	mProjectCache->MakeFallback(ArpVoxel(0, 0, 0));
//	if (mProjectCache->InitCheck() != B_OK) return B_ERROR;

	return B_OK;
}

void GlSkinCache::Free()
{
	delete mChainCache;
	mChainCache = 0;
}

// #pragma mark -

/***************************************************************************
 * GL-LABEL-CACHE
 ***************************************************************************/
GlLabelCache::GlLabelCache()
{
}

GlLabelCache::~GlLabelCache()
{
	for (uint32 k = 0; k < mEntries.size(); k++) delete mEntries[k];
	mEntries.resize(0);
}

const ArpBitmap* GlLabelCache::Acquire(const BString16& text)
{
//	ArpASSERT(grid || mFallback);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k]->label == text) return mEntries[k]->bm;
	}
	ArpBitmap*			bm = MakeFallback(text);
	if (!bm) return 0;
	GlLabelCacheEntry*	entry = new GlLabelCacheEntry(text, bm);
	if (!entry) {
		delete bm;
		return 0;
	}
	mEntries.push_back(entry);
	return bm;
}

void GlLabelCache::Release(const ArpBitmap* bm)
{
}

ArpBitmap* GlLabelCache::MakeFallback(const BString16& text)
{
	ArpFont			font;
	font.SetSize(12);
	ArpBitmap*		bm = new ArpBitmap(1, 1);
	if (!bm) return 0;
	float			descent;
	font.GetHeight(0, &descent, 0);
	rgb_color		fg, bg;
	fg.red = fg.green = fg.blue = 1; fg.alpha = 255;
	bg.red = bg.green = bg.blue = bg.alpha = 0;
	{
		ArpPainter	painter(bm);
		painter.SetColour(ARP_BACKGROUND_COLOUR, bg);
		painter.SetColour(ARP_FOREGROUND_COLOUR, fg);
		painter.SetFont(&font);
		BRect		b = painter.StringBounds(text);
		painter.Resize(b.Width(), b.Height());
		painter.FillRect(BRect(0, 0, b.Width(), b.Height()), bg);
		painter.DrawString(text, 0, 0, b.Height() - descent);
	}
	bm->TrimFromTextHack();
	return bm;
}

#if 0
GlImage* GlTextToImage::ImageFromText(	const BString16& text,
										const ArpFont& font,
										ArpVoxel fg, ArpVoxel bg,
										bool fit) const
{
	ArpBitmap*		bm = BitmapFromText(text, font, fit);
	if (!bm) return 0;
	GlImage*		im = new GlImage();
	if (im->FromTextHack(*bm, bg, fg) != B_OK) {
		delete im;
		im = 0;
	}
	delete bm;
	if (im) {
		im->SetProperty(ARP_BACKGROUND_COLOUR, bg);
		im->SetProperty(ARP_FOREGROUND_COLOUR, fg);
	}
	return im;
}
#endif
