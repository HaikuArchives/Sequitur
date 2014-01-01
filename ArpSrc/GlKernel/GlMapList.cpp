#include <GlPublic/GlMapList.h>

/*******************************************************
 * GL-MAP-LIST
 *******************************************************/
GlMapList::GlMapList(uint32 inSize)
		: size(0), maps(0), mAllocated(0)
{
	if (inSize > 0 && (maps = new GlAlgo1d*[inSize]) != 0) {
		mAllocated = inSize;
		size = inSize;
		for (uint32 k = 0; k < size; k++) maps[k] = 0;
	}
}

GlMapList::GlMapList(const GlMapList& o)
		: size(0), maps(0), mAllocated(0)
{
	if (o.size > 0 && (maps = new GlAlgo1d*[o.size]) != 0) {
		mAllocated = o.size;
		size = o.size;
		for (uint32 k = 0; k < size; k++) {
			if (o.maps[k]) maps[k] = (GlAlgo1d*)(o.maps[k]->Clone());
			else maps[k] = 0;
		}
	}
}

GlMapList::~GlMapList()
{
	FreeAll();
}

GlMapList& GlMapList::operator=(const GlMapList &o)
{
	FreeAll();

	if (o.size > 0 && (maps = new GlAlgo1d*[o.size]) != 0) {
		mAllocated = o.size;
		size = o.size;
		for (uint32 k = 0; k < size; k++) {
			if (o.maps[k]) maps[k] = (GlAlgo1d*)(o.maps[k]->Clone());
			else maps[k] = 0;
		}
	}

	return *this;
}

GlMapList* GlMapList::Clone() const
{
	return new GlMapList(*this);
}

status_t GlMapList::ResizeTo(uint32 newSize, int32 method)
{
	if (newSize < 1) {
		FreeAll();
		return B_OK;
	}

	GlAlgo1d**			newMaps = new GlAlgo1d*[newSize];
	if (!newMaps) return B_NO_MEMORY;

	uint32			k;
	for (k = 0; k < newSize; k++) newMaps[k] = 0;

	if (method == RELATIVE_METHOD) {
		for (k = 0; k < size; k++) {
			int32	newIndex = int32((newSize -1) * (k / float(size - 1)));
			ArpASSERT(newIndex >= 0);
			if (newIndex >= 0 && newIndex < int32(newSize)) {
				delete newMaps[k];
				newMaps[k] = maps[k];
				maps[k] = 0;
			}
		}
	} else { // ABSOLUTE_METHOD
		for (k = 0; k < newSize; k++) {
			if (k < size) {
				newMaps[k] = maps[k];
				maps[k] = 0;
			}
		}
	}

	FreeAll();
	
	maps = newMaps;
	size = newSize;
	mAllocated = newSize;
	return B_OK;
}

status_t GlMapList::Set(int32 index, GlAlgo1d* map)
{
	ArpVALIDATE(map, return B_ERROR);
	uint32			newIndex = size;
	if (index >= 0) newIndex = uint32(index);
	
	if (newIndex >= size) {
		status_t	err = ResizeTo(newIndex + 1, ABSOLUTE_METHOD);
		if (err != B_OK) return err;
	}
	ArpVALIDATE(newIndex < size, return B_ERROR);
	delete maps[newIndex];
	maps[newIndex] = map;
	return B_OK;
}

void GlMapList::FreeAll()
{
	for (uint32 k = 0; k < mAllocated; k++) delete maps[k];
	delete[] maps;
	maps = 0;
	size = 0;
	mAllocated = 0;
}

void GlMapList::Print() const
{
	printf("GlMapList size %ld, allocated %ld\n", size, mAllocated);
	for (uint32 k = 0; k < size; k++) {
		if (maps[k]) {
			printf("\t%ld: ", k);
			maps[k]->Print();
		}
	}
}
