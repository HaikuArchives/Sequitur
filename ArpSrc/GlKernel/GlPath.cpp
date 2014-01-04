#include <ArpKernel/ArpDebug.h>
#include "GlPublic/GlPath.h"

static const uint32			GROW_BY		= 10;

/***************************************************************************
 * GL-PATH
 ****************************************************************************/
GlPath::GlPath(uint32 inSize)
		: size(0), pts(0), mAllocated(0)
{
	if (inSize > 0 && (pts = new ArpPoint3d[inSize + GROW_BY]) != 0) {
		mAllocated = inSize + GROW_BY;
		size = inSize;
		for (uint32 k = 0; k < size; k++) {
			pts[k].x = 0.0;
			pts[k].y = 0.0;
			pts[k].z = 0.0;
		}
	}
}

GlPath::GlPath(const GlPath& o)
		: size(0), pts(0), mAllocated(0)
{
	if (o.size > 0 && (pts = new ArpPoint3d[o.size + GROW_BY]) != 0) {
		mAllocated = o.size + GROW_BY;
		size = o.size;
		for (uint32 k = 0; k < size; k++) pts[k] = o.pts[k];
	}
}

GlPath::~GlPath()
{
	FreeAll();
}

GlPath& GlPath::operator=(const GlPath &o)
{
	FreeAll();

	if ((pts = new ArpPoint3d[o.size + GROW_BY]) != 0) {
		mAllocated = o.size + GROW_BY;
		size = o.size;
		for (uint32 k = 0; k < size; k++) pts[k] = o.pts[k];
	}

	return *this;
}

GlPath* GlPath::Clone() const
{
	return new GlPath(*this);
}

status_t GlPath::ResizeTo(uint32 newSize)
{
	if (newSize < mAllocated) {
		for (uint32 k = size; k < newSize; k++) {
			pts[k].x = 0.0;
			pts[k].y = 0.0;
			pts[k].z = 0.0;
		}
		size = newSize;
		return B_OK;
	}
	
	ArpPoint3d*		newPoints = new ArpPoint3d[newSize + GROW_BY];
	if (!newPoints) return B_NO_MEMORY;
	for (uint32 k = 0; k < newSize; k++) {
		if (pts && k < size) {
			newPoints[k].x = pts[k].x;
			newPoints[k].y = pts[k].y;
			newPoints[k].z = pts[k].z;
		} else {
			newPoints[k].x = 0.0;
			newPoints[k].y = 0.0;
			newPoints[k].z = 0.0;
		}
	}
	delete[] pts;
	pts = newPoints;
	mAllocated = newSize + GROW_BY;
	size = newSize;
	return B_OK;
}

status_t GlPath::Add(float x, float y, float z)
{
	uint32		oldSize = size;
	if (size >= mAllocated) {
		status_t	err = ResizeTo(mAllocated + 20);
		if (err != B_OK) return err;
	}
	size = oldSize + 1;
	ArpVALIDATE(size > 0, return B_ERROR);
	pts[size - 1].Set(x, y, z);
	return B_OK;
}

void GlPath::FreeAll()
{
	delete[] pts;
	pts = 0;
	size = 0;
	mAllocated = 0;
}

void GlPath::Print() const
{
	printf("GlPath size %ld, allocated %ld\n", size, mAllocated);
	for (uint32 k = 0; k < size; k++) {
		printf("\t%ld: x %f y %f z %f\n", k, pts[k].x,
				pts[k].y, pts[k].z);
	}
}

status_t GlPath::GetAt(uint32 index, float* outX, float* outY, float* outVal) const
{
	ArpVALIDATE(index < size, return B_ERROR);
	*outX = pts[index].x;
	*outY = pts[index].y;
	if (outVal) *outVal = pts[index].z;
	return B_OK;
}

status_t GlPath::SetAt(uint32 index, float x, float y, float val)
{
	ArpVALIDATE(index < size, return B_ERROR);
	pts[index].x = x;
	pts[index].y = y;
	pts[index].z = val;
	return B_OK;
}
