#include <stdio.h>
#include <ArpKernel/ArpDebug.h>
#include <ArpCore/ArpChar.h>
#include <support/Errors.h>
#include <GlPublic/GlFilterKernel.h>

/*******************************************************
 * GL-FILTER-KERNEL
 *******************************************************/
GlFilterKernel::GlFilterKernel()
		: mStatus(B_ERROR), mTaps(0), mL(0), mT(0), mR(0), mB(0)
{
}

GlFilterKernel::GlFilterKernel(	uint32 left, uint32 top,
								uint32 right, uint32 bottom)
		: mStatus(B_ERROR), mTaps(0), mL(0), mT(0), mR(0), mB(0)
{
	Init(left, top, right, bottom);
}

GlFilterKernel::GlFilterKernel(const GlFilterKernel& o)
		: mStatus(B_ERROR), mTaps(0), mL(0), mT(0), mR(0), mB(0)
{
	Init(o.mL, o.mT, o.mR, o.mB);
	if (mTaps && o.mTaps) {
		uint32		size = (mL + mR + 1) * (mT + mB + 1);
		for (uint32 k = 0; k < size; k++) mTaps[k] = o.mTaps[k];
	}
}

GlFilterKernel::~GlFilterKernel()
{
	delete[] mTaps;
}

GlFilterKernel* GlFilterKernel::Clone() const
{
	return new GlFilterKernel(*this);
}

status_t GlFilterKernel::InitCheck() const
{
	return mStatus;
}

int32 GlFilterKernel::Left() const
{
	return mL;
}

int32 GlFilterKernel::Top() const
{
	return mT;
}

int32 GlFilterKernel::Right() const
{
	return mR;
}

int32 GlFilterKernel::Bottom() const
{
	return mB;
}

int32 GlFilterKernel::Width() const
{
	return mL + mR + 1;
}

int32 GlFilterKernel::Height() const
{
	return mT + mB + 1;
}

int32 GlFilterKernel::Size() const
{
	return Width() * Height();
}

float* GlFilterKernel::LockTaps(int32* width, int32* height)
{
	if (mStatus != B_OK) return 0;
	if (!mTaps) return 0;
	if (width) *width = Width();
	if (height) *height = Height();
	return mTaps;
}

void GlFilterKernel::UnlockTaps(float* taps)
{
	if (taps) ArpASSERT(taps == mTaps);
}

#if 0
int32 GlFilterKernel::XRange() const
{
	if (mKernel.size() < 1) return 0;
	if (mKernel[0].size() <= 1) return 0;
	return (mKernel[0].size() - 1) / 2;
}

int32 GlFilterKernel::YRange() const
{
	if (mKernel.size() <= 1) return 0;
	return (mKernel.size() - 1) / 2;
}

status_t GlFilterKernel::Set(int32 x, int32 y, float value)
{
	int32		xRange = XRange(), yRange = YRange();
	assert(abs(x) <= xRange && abs(y) <= yRange);
	if ( !(abs(x) <= xRange && abs(y) <= yRange) ) return B_ERROR;
	mKernel[y + yRange][x + xRange] = value;
	return B_OK;
}

status_t GlFilterKernel::Get(int32 x, int32 y, float* value)
{
	int32		xRange = XRange(), yRange = YRange();
	assert(abs(x) <= xRange && abs(y) <= yRange);
	if ( !(abs(x) <= xRange && abs(y) <= yRange) ) return B_ERROR;
	*value = mKernel[y + yRange][x + xRange];
	return B_OK;
}
#endif

status_t GlFilterKernel::Init(uint32 width, uint32 height)
{
	return Init(width, height, width, height);
}

status_t GlFilterKernel::Init(	uint32 left, uint32 top,
								uint32 right, uint32 bottom)
{
	if (left == mL && top == mT && right == mR && bottom == mB) return B_OK;
	mStatus = B_ERROR;
	delete[] mTaps;
	mTaps = 0;
	mL = mT = mR = mB = 0;
	uint32		w = left + right + 1, h = top + bottom + 1;
	uint32		size = w * h;
	
	mTaps = new float[size];
	if (!mTaps) {
		mStatus = B_NO_MEMORY;
		return mStatus;
	}
	
	mL = left;
	mT = top;
	mR = right;
	mB = bottom;
	for (uint32 k = 0; k < size; k++) mTaps[k] = 0;
	mTaps[mT * w + mL] = 1;
	mStatus = B_OK;
	return mStatus;
}

void GlFilterKernel::Print() const
{
	uint32		w = mL + mR + 1, h = mT + mB + 1;
	for (uint32 y = 0; y < h; y++) {
		for (uint32 x = 0; x < w; x++) {
			printf("%2.2f ", mTaps[y * w + x]);
		}
		printf("\n");
	}
	printf("\n");
}
