#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ArpKernel/ArpDebug.h>
#include <GlPublic/GlLineCache.h>

static const uint32			GROW_BY		= 10;

/***************************************************************************
 * GL-LINE-CACHE
 ****************************************************************************/
GlLineCache::GlLineCache(uint32 inSize)
		: size(0), x(0), y(0), mAllocated(0)
{
	x = new int32[inSize + GROW_BY];
	y = new int32[inSize + GROW_BY];
	if (!x || !y) {
		delete x;
		delete y;
		x = y = 0;
		return;
	}
	mAllocated = inSize + GROW_BY;
	size = inSize;
	memset(x, 0, inSize * 4);
	memset(y, 0, inSize * 4);
}

GlLineCache::GlLineCache(const GlLineCache& o)
		: size(0), x(0), y(0), mAllocated(0)
{
	Copy(o);
}

GlLineCache::~GlLineCache()
{
	FreeAll();
}

GlLineCache& GlLineCache::operator=(const GlLineCache &o)
{
	Copy(o);
	return *this;
}

GlLineCache* GlLineCache::Clone() const
{
	return new GlLineCache(*this);
}

status_t GlLineCache::Resize(uint32 newSize)
{
	if (newSize < mAllocated) {
		size = newSize;
		return B_OK;
	}

	if (newSize < 1) {
		size = 0;
		return B_OK;
	}
	
	int32*		newX = new int32[newSize + GROW_BY];
	if (!newX) return B_NO_MEMORY;
	int32*		newY = new int32[newSize + GROW_BY];
	if (!newY) {
		delete newX;
		return B_NO_MEMORY;
	}
	memset(newX, 0, newSize * 4);
	memset(newY, 0, newSize * 4);
	if (x && size > 0) memcpy(newX, x, (size < newSize) ? size : newSize);
	if (y && size > 0) memcpy(newY, y, (size < newSize) ? size : newSize);

	delete[] x;
	delete[] y;
	x = newX;
	y = newY;
	mAllocated = newSize + GROW_BY;
	size = newSize;
	return B_OK;
}

status_t GlLineCache::Add(int32 xv, int32 yv)
{
	uint32		oldSize = size;
	if (size >= mAllocated) {
		status_t	err = Resize(mAllocated + GROW_BY);
		if (err != B_OK) return err;
	}
	size = oldSize + 1;
	ArpVALIDATE(size > 0, return B_ERROR);
	x[size - 1] = xv;
	y[size - 1] = yv;
	return B_OK;
}

status_t GlLineCache::Copy(const GlLineCache& o)
{
	FreeAll();

	x = new int32[o.size + GROW_BY];
	y = new int32[o.size + GROW_BY];
	if (!x || !y) {
		delete x;
		delete y;
		x = y = 0;
		return B_NO_MEMORY;
	}

	mAllocated = o.size + GROW_BY;
	size = o.size;
	if (o.size < 1) return B_OK;

	memcpy(x, o.x, size * 4);
	memcpy(y, o.y, size * 4);
	return B_OK;
}

void GlLineCache::FreeAll()
{
	delete[] x;
	delete[] y;
	x = y = 0;
	size = 0;
	mAllocated = 0;
}

void GlLineCache::Print() const
{
	printf("GlLineCache size %ld, allocated %ld\n", size, mAllocated);
	for (uint32 k = 0; k < size; k++) printf("\t%ld: (%ld, %ld)\n", k, x[k], y[k]);
}
