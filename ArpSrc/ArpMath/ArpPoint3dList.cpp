#include <be/support/Errors.h>
#include <ArpCore/ArpDebug.h>
#include <ArpMath/ArpPoint3dList.h>

static const uint32			GROW_BY		= 10;

/***************************************************************************
 * ARP-POINT-3D-LIST
 ****************************************************************************/
ArpPoint3dList::ArpPoint3dList(uint32 inSize)
		: size(0), pts(0), mAllocated(0)
{
	if ((pts = new ArpPoint3d[inSize + GROW_BY]) != 0) {
		mAllocated = inSize + GROW_BY;
		size = inSize;
		for (uint32 k = 0; k < size; k++) {
			pts[k].x = 0.0;
			pts[k].y = 0.0;
			pts[k].z = 0.0;
		}
	}
}

ArpPoint3dList::ArpPoint3dList(const ArpPoint3dList& o)
		: size(0), pts(0), mAllocated(0)
{
	if ((pts = new ArpPoint3d[o.size + GROW_BY]) != 0) {
		mAllocated = o.size + GROW_BY;
		size = o.size;
		for (uint32 k = 0; k < size; k++) pts[k] = o.pts[k];
	}
}

ArpPoint3dList::~ArpPoint3dList()
{
	FreeAll();
}

ArpPoint3dList& ArpPoint3dList::operator=(const ArpPoint3dList &o)
{
	FreeAll();

	if ((pts = new ArpPoint3d[o.size + GROW_BY]) != 0) {
		mAllocated = o.size + GROW_BY;
		size = o.size;
		for (uint32 k = 0; k < size; k++) pts[k] = o.pts[k];
	}

	return *this;
}

ArpPoint3dList* ArpPoint3dList::Clone() const
{
	return new ArpPoint3dList(*this);
}

status_t ArpPoint3dList::ResizeTo(uint32 newSize)
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
	delete pts;
	pts = newPoints;
	mAllocated = newSize + GROW_BY;
	size = newSize;
	return B_OK;
}

status_t ArpPoint3dList::Add(float x, float y, float z)
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

static bool _below_range(uint32* indexes, uint32 size, uint32 range)
{
	for (uint32 k = 0; k < size; k++) {
		if (indexes[k] >= range) return false;
	}
	return true;
}

static bool _sorted(uint32* indexes, uint32 size)
{
	for (uint32 k = 1; k < size; k++) {
		if (indexes[k] < indexes[k - 1]) return false;
	}
	return true;
}

static void _sort(uint32* indexes, uint32 size)
{
	if (_sorted(indexes, size)) return;

	for (uint32 k1 = 0; k1 < size; k1++) {
		uint32	tmp = indexes[k1];
		uint32	ex = k1;
		for (uint32 k2 = k1 + 1; k2 < size; k2++) {
			if (indexes[k2] < tmp) {
				tmp = indexes[k2];
				ex = k2;
			}
		}
		if (ex != k1) {
			indexes[ex] = indexes[k1];	
			indexes[k1] = tmp;
		}
	}
	ArpASSERT(_sorted(indexes, size));
}

status_t ArpPoint3dList::Erase(uint32* indexes, uint32 indexesSize)
{
	ArpVALIDATE(indexesSize > 0, return B_ERROR);
	ArpASSERT(_below_range(indexes, indexesSize, size));
	_sort(indexes, indexesSize);

	uint32		fromIndex, toIndex = 0, indexesIndex = 0;
	for (fromIndex = 0; fromIndex < size; fromIndex++) {
		if (indexesIndex < indexesSize && indexes[indexesIndex] == fromIndex) {
			indexesIndex++;
		} else {
			pts[toIndex] = pts[fromIndex];
			toIndex++;
		}
	}
	ArpASSERT(toIndex == size - indexesSize);
	size = toIndex;
	return B_OK;
}

void ArpPoint3dList::FreeAll()
{
	delete pts;
	pts = 0;
	size = 0;
	mAllocated = 0;
}

void ArpPoint3dList::Print() const
{
	printf("ArpPoint3dList size %ld, allocated %ld\n", size, mAllocated);
	for (uint32 k = 0; k < size; k++) {
		printf("\t%ld: x %f y %f z %f\n", k, pts[k].x,
				pts[k].y, pts[k].z);
	}
}
