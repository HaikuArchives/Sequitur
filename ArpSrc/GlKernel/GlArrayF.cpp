#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ArpCore/ArpDebug.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlArrayF.h>

static const uint32			GROW_BY		= 10;

/***************************************************************************
 * GL-ARRAY-F
 ****************************************************************************/
GlArrayF::GlArrayF(uint32 inSize)
		: size(0), n(0), mAllocated(0)
{
	if ((n = new float[inSize + GROW_BY]) != 0) {
		mAllocated = inSize + GROW_BY;
		size = inSize;
		for (uint32 k = 0; k < size; k++) n[k] = 0.;
	}
}

GlArrayF::GlArrayF(const GlArrayF& o)
		: size(0), n(0), mAllocated(0)
{
	if ((n = new float[o.size + GROW_BY]) != 0) {
		mAllocated = o.size + GROW_BY;
		size = o.size;
		if (o.size > 0) memcpy(n, o.n, o.size * sizeof(float));
//		for (uint32 k = 0; k < size; k++) n[k] = o.n[k];
	}
}

GlArrayF::~GlArrayF()
{
	FreeAll();
}

GlArrayF& GlArrayF::operator=(const GlArrayF &o)
{
	if (mAllocated >= o.mAllocated) {
		if (o.size > 0) memcpy(n, o.n, o.size * sizeof(float));
//		for (uint32 k = 0; k < o.size; k++) n[k] = o.n[k];
		size = o.size;
		return *this;
	}

	FreeAll();

	if ((n = new float[o.size + GROW_BY]) != 0) {
		mAllocated = o.size + GROW_BY;
		size = o.size;
		if (o.size > 0) memcpy(n, o.n, o.size * sizeof(float));
//		for (uint32 k = 0; k < size; k++) n[k] = o.n[k];
	}

	return *this;
}

GlArrayF* GlArrayF::Clone() const
{
	return new GlArrayF(*this);
}

float GlArrayF::At(float v) const
{
	ArpASSERT(v >= 0 && v <= 1);
	if (size == 1) return n[0];
	if (size < 1) return 0.0;
	if (v <= 0.0) return n[0];
	if (v >= 1.0) return n[size - 1];
	
	v = v * (size - 1);
	int32			low = ARP_ROUND(floor(v));
	float			frac = v - low;
	ArpASSERT(low >= 0 && low + 1 < int32(size));
	if (low < 0) low = 0;
	else if (low + 1 >= int32(size)) low = size - 2;
	return n[low] + ((n[low + 1] - n[low]) * frac);
}

static bool _sorted(GlArrayF& set)
{
	for (uint32 k = 1; k < set.size; k++) {
		if (set.n[k] < set.n[k - 1]) return false;
	}
	return true;
}

int _psort(const void* p1, const void* p2)
{
	if ( *(float*)p1 < *(float*)p2) return -1;
	if ( *(float*)p1 > *(float*)p2) return 1;
	return 0;
}

#if 1
void GlArrayF::Sort()
{
	if (size < 2) return;
	qsort(n, size, sizeof(float), _psort);
	ArpASSERT(_sorted(*this));
}
#endif

#if 0
void GlArrayF::Sort()
{
	if (size < 2) return;

	for (uint32 k1 = 0; k1 < size; k1++) {
		float	tmp = n[k1];
		uint32	ex = k1;
		for (uint32 k2 = k1 + 1; k2 < size; k2++) {
			if (n[k2] < tmp) {
				tmp = n[k2];
				ex = k2;
			}
		}
		if (ex != k1) {
			n[ex] = n[k1];	
			n[k1] = tmp;
		}
	}
	ArpASSERT(_sorted(*this));
}
#endif

status_t GlArrayF::Resize(uint32 newSize)
{
	if (newSize < mAllocated) {
		/* Don't initialize the data -- since it was before,
		 * hopefully this doesn't fuck with anyone.
		 */
//		for (uint32 k = size; k < newSize; k++) n[k] = 0.;
		size = newSize;
		return B_OK;
	}
	
	float*		newN = new float[newSize + GROW_BY];
	if (!newN) return B_NO_MEMORY;
	for (uint32 k = 0; k < newSize; k++) {
		if (n && k < size) newN[k] = n[k];
		else newN[k] = 0.;
	}
	delete[] n;
	n = newN;
	mAllocated = newSize + GROW_BY;
	size = newSize;
	return B_OK;
}

status_t GlArrayF::Add(float val)
{
	uint32		oldSize = size;
	if (size >= mAllocated) {
		status_t	err = Resize(mAllocated + 20);
		if (err != B_OK) return err;
	}
	size = oldSize + 1;
	ArpVALIDATE(size > 0, return B_ERROR);
	n[size - 1] = val;
	return B_OK;
}

void GlArrayF::FreeAll()
{
	delete[] n;
	n = 0;
	size = 0;
	mAllocated = 0;
}

void GlArrayF::Print() const
{
	printf("GlArrayF size %ld, allocated %ld\n", size, mAllocated);
	for (uint32 k = 0; k < size; k++) printf("\t%ld: %f\n", k, n[k]);
}
