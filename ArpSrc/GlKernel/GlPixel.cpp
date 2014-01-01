#include "GlPublic/GlDefs.h"
#include "GlPublic/GlPixel.h"
#include "GlPublic/GlPlanes.h"

/***************************************************************************
 * GL-PIXEL
 ****************************************************************************/
GlPixel::GlPixel(uint32 inSize)
		: size(0), c(0)
{
	if (inSize > 0) SetSize(inSize);
}

GlPixel::GlPixel(const GlPlanes& planes, int32 pix)
		: size(0), c(0)
{
	SetTo(planes, pix);
}

GlPixel::GlPixel(const GlPixel& o)
		: size(0), c(0)
{
	if (o.size > 0 && SetSize(o.size) == B_OK) {
		ArpASSERT(o.size == size);
		for (uint32 k = 0; k < size; k++) c[k] = o.c[k];
	}
}

GlPixel::~GlPixel()
{
	Free();
}

bool GlPixel::operator==(const GlPixel& o) const
{
	if (size != o.size) return false;
	for (uint32 k = 0; k < size; k++) {
		if (c[k] != o.c[k]) return false;
	}
	return true;
}

GlPixel* GlPixel::Clone() const
{
	return new GlPixel(*this);
}

status_t GlPixel::SetSize(uint32 cSize)
{
	Free();
	if (cSize < 1) return B_OK;
	c = new uint8[cSize];
	if (!cSize) return B_NO_MEMORY;
	size = cSize;
	return B_OK;
}

status_t GlPixel::SetTo(const GlPlanes& planes, int32 pix)
{
	ArpASSERT(planes.size > 0);
	if (size != planes.size) {
		status_t	err = SetSize(planes.size);
		if (err != B_OK) return err;
	}
	ArpASSERT(pix >= 0 && pix < planes.w * planes.h);
	for (uint32 k = 0; k < size; k++) c[k] = planes.plane[k][pix];
	return B_OK;
}

void GlPixel::Free()
{
	delete[] c;
	size = 0;
	c = 0;
}

void GlPixel::Print(uint32 tabs) const
{
	uint32		k;
	for (k = 0; k < tabs; k++) printf("\t");
	printf("Pix size %ld\n", size);
	for (k = 0; k < size; k++) printf("\t%ld: %d\n", k, c[k]);
}
