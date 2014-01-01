#include "GlPublic/GlAlgo2d.h"
#include "GlPublic/GlMask.h"
#include "GlPublic/GlPlanes.h"

/***************************************************************************
 * GL-MASK
 ****************************************************************************/
GlMask::GlMask()
		: mData(0), mW(0), mH(0), mSuppress(true)
{
}

GlMask::GlMask(uint8* mask, int32 w, int32 h)
		: mData(0), mW(0), mH(0), mSuppress(true)
{
	Make(mask, w, h);
}

GlMask::GlMask(const GlPlanes& pixels, GlAlgo2d* a)
		: mData(0), mW(0), mH(0), mSuppress(true)
{
	Make(pixels, a);
}

GlMask::~GlMask()
{
	delete[] mData;
}

uint8* GlMask::Make(const GlPlanes& pixels, GlAlgo2d* a,
					GlProcessStatus* status)
{
	mSuppress = true;
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
		mSuppress = false;
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
	mSuppress = false;
	return mData;
}

status_t GlMask::Make(uint8* mask, int32 w, int32 h)
{
	mSuppress = true;
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
	mSuppress = false;
	return B_OK;
}

uint8* GlMask::Data(int32* outWidth, int32* outHeight)
{
	if (mSuppress) return 0;
	if (outWidth) *outWidth = mW;
	if (outHeight) *outHeight = mH;
	return mData;
}
