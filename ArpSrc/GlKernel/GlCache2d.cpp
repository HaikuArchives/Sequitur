#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlCache2d.h>
#include <GlPublic/GlPlanes.h>

/***************************************************************************
 * GL-CACHE-2D
 ****************************************************************************/
GlCache2d::GlCache2d()
		: mStatus(B_ERROR), mData(0), mW(0), mH(0)
{
}

GlCache2d::GlCache2d(int32 w, int32 h)
		: mStatus(B_ERROR), mData(0), mW(0), mH(0)
{
	SetDimensions(w, h);
}

int32 GlCache2d::Width() const
{
	return mW;
}

int32 GlCache2d::Height() const
{
	return mH;
}

#if 0
	GlCache2d(uint8* mask, int32 w, int32 h);
	GlCache2d(const GlPlanes& pixels, GlAlgo2d* s);

GlCache2d::GlCache2d(uint8* mask, int32 w, int32 h)
		: mStatus(B_ERROR), mData(0), mW(0), mH(0)
{
	Make(mask, w, h);
}

GlCache2d::GlCache2d(const GlPlanes& pixels, GlAlgo2d* a)
		: mStatus(B_ERROR), mData(0), mW(0), mH(0)
{
	Make(pixels, a);
}
#endif

GlCache2d::~GlCache2d()
{
	delete[] mData;
}

uint8* GlCache2d::Make(	const GlPlanes& pixels, GlAlgo2d* a,
						GlProcessStatus* status)
{
	mStatus = B_ERROR;
	if (!a) return 0;
	int32		size = pixels.w * pixels.h;
	if (size < 1) return 0;
	GlPlanes	cache(pixels.w, pixels.h);
	if (cache.SetSize(1) != B_OK) return 0;
	if (mW >= pixels.w && mH >= pixels.h) {
		cache.plane[0] = mData;
		// Start at max
		memset(cache.plane[0], 255, size);
		a->ProcessAll(&pixels, cache, status);
		mStatus = B_OK;
		return mData;
	}
	delete[] mData;
	mData = new uint8[size];
	if (!mData) {
		mW = mH = 0;
		return 0;
	}
	mW = pixels.w;
	mH = pixels.h;
	cache.plane[0] = mData;
	// Start at max
	memset(cache.plane[0], 255, size);
	a->ProcessAll(&pixels, cache, status);
	mStatus = B_OK;
	return mData;
}

#if 0
	/* Copy the mask into me.  If my data isn't large enough for it, grow me.
	 */
	status_t		Make(uint8* mask, int32 w, int32 h);

status_t GlCache2d::Make(uint8* mask, int32 w, int32 h)
{
	mStatus = B_ERROR;
	int32		maskSize = w * h;
	if (maskSize < 1) return B_OK;
	int32		mySize = mW * mH;
	if (mySize < maskSize) {
		delete[] mData;
		mData = new uint8[maskSize];
		if (!mData) {
			mW = mH = 0;
			return B_NO_MEMORY;
		}
		mW = w;
		mH = h;
		mySize = mW * mH;
	}
	ArpASSERT(mySize >= maskSize);
	if (mask) memcpy(mData,	mask,	maskSize);
	mStatus = B_OK;
	return B_OK;
}
#endif

uint8* GlCache2d::Data(int32* outWidth, int32* outHeight)
{
	if (mStatus != B_OK) return 0;
	if (outWidth) *outWidth = mW;
	if (outHeight) *outHeight = mH;
	return mData;
}

uint8* GlCache2d::SetDimensions(int32 w, int32 h)
{
	if (mW == w && mH == h) return mData;

	mStatus = B_ERROR;
	delete[] mData;
	mW = mH = 0;

	int32		size = w * h;
	if (size < 1) return 0;

	mData = new uint8[w * h];
	if (!mData) return 0;
	mW = w;
	mH = h;
	mStatus = B_OK;
	return mData;
}
