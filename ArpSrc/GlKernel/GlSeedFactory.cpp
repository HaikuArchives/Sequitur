#include <math.h>
#include "ArpMath/ArpPoint3d.h"
#include "GlPublic/GlAlgo2d.h"
#include "GlPublic/GlPlanes.h"
#include "GlPublic/GlSeedFactory.h"

/***************************************************************************
 * GL-ABSTRACT-SEED-FACTORY
 ***************************************************************************/
GlAbstractSeedFactory::GlAbstractSeedFactory(int32 maxSeeds, uint8 low, uint8 high)
		: mMaxSeeds(maxSeeds), mLow(low), mHigh(high)
{
}

status_t GlAbstractSeedFactory::MakeSeeds(const GlPlanes& pixels, GlAlgo2d* s)
{
	ArpVALIDATE(s, return B_ERROR);
	uint8*				mask = s->NewMask(pixels);
	if (!mask) return B_NO_MEMORY;
	status_t			err = MakeDistributedSeeds(mask, pixels.w, pixels.h);
	delete mask;
	return err;
}

static uint32 weight(uint32 n)
{
	uint32		ans = 0;
	while (n > 0) {
		ans += n;
		n -= 1;
	}
	return ans;
}

static void pick_seeds(	uint8 val, uint32 counts, uint32 picks,
						const uint8* mask, int32 w, int32 h,
						GlAbstractSeedFactory& fac, uint32* index)
{
	uint32		c = 0;
	uint32		steps = 1;
	uint32		step = 0;
	if (picks < counts) steps = uint32(floor(float(counts) / picks));
	if (steps <= 1) steps = 1;
	else step = steps / 2;

	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			if (mask[ARP_PIXEL(x, y, w)] == val) {
				step++;
				if (step >= steps) {
					fac.SetSeed(*index, float(x), float(y), val);
					*index = *index + 1;
					c++;
					if (c >= picks) return;
					step = 0;
				}
			}
		}
	}
}

status_t GlAbstractSeedFactory::MakeDistributedSeeds(const uint8* mask, int32 w, int32 h)
{
	uint32		counts[256], picks[256], totalCounts = 0, slotCount = 0;
	int32		k, pix;
	for (k = 0; k < 256; k++) counts[k] = picks[k] = 0;
	for (pix = 0; pix < w * h; pix++) {
		if (mask[pix] >= mLow && mask[pix] <= mHigh)
			counts[mask[pix]] += 1;
	}
	for (k = 0; k < 256; k++) {
		if (counts[k] > 0) {
			totalCounts += counts[k];
			slotCount++;
		}
	}
	if (slotCount < 1) return B_ERROR;
	if (int32(totalCounts) <= mMaxSeeds) return MakeAdjacentSeeds(mask, w, h);
	
	/* I need to pick a certain number from each value, so that the higher
	 * the value the more seeds I pick, but I still pick values from the
	 * entire range, and the total number picked equals mMaxSeeds.
	 *
	 * I do this by applying a general distribution to the number
	 * of slots in use, weighted at the high end, then working
	 * backwards down through the list and supplying any overflow
	 * to those below me.
	 */

	int32		prevK = 255;
	uint32		spillover = 0;
	uint32		slotNum = slotCount;
	uint32		totalPicks = 0;
	if (mMaxSeeds < int32(slotNum)) slotNum = uint32(mMaxSeeds);
	uint32		slotCountWeighted = weight(slotNum);
	for (k = 255; k >= 0; k--) {
		if (counts[k] > 0) {
			/* Find ideally how much I'd take from this count.
			 */
			uint32	ideal = uint32(ceil((slotNum / float(slotCountWeighted)) * mMaxSeeds));
			uint32	spilloverTake = uint32((1 - ((prevK - k) / 255.)) * spillover);
			uint32	take = ideal + spilloverTake;
			if (take <= counts[k]) {
				picks[k] = take;
				spillover -= spilloverTake;
			} else {
				/* If picks is less than the count, then we took from
				 * the spillover, just not everything we wanted
				 */
				int32	diff = int32(ideal) - counts[k];
				picks[k] = counts[k];
				ArpASSERT(int32(spilloverTake) + diff >= 0);
				spillover += uint32(int32(spilloverTake) + diff);
			}
			slotNum--;
			if (slotNum == 0) break;
			totalPicks += picks[k];
			if (int32(totalPicks) >= mMaxSeeds) break;
			prevK = k;
		}
		/* Decrease it by enough that it will only last for 1/4
		 * of the total steps (1/4 of 0 - 255 being 63).
		 */
		if (spillover > 0) spillover = uint32(spillover - (spillover / 63.));
	}
#if 0
printf("Distributed Seeds totalCounts %ld totalPicks %ld slotCount %ld maxSeeds %ld\n",
				totalCounts, totalPicks, slotCount, mMaxSeeds);
printf("\tCount\tPicks\n");
for (k = 0; k < 256; k++) {
	printf("%ld:\t%ld\t%ld\n", k, counts[k], picks[k]);
}
#endif
	/* Make seeds from the picks.
	 */
	status_t			err = NewSeeds(totalPicks);
	if (err != B_OK) return B_NO_MEMORY;

	uint32				index = 0;
	for (k = 255; k >= 0; k--) {
		if (picks[k] > 0) {
			pick_seeds(uint8(k), counts[k], picks[k], mask, w, h, *this, &index);
			if (index >= totalPicks) break;
		}
	}
	ArpASSERT(index == totalPicks);
	
	return B_ERROR;
}

status_t GlAbstractSeedFactory::MakeAdjacentSeeds(const uint8* mask, int32 w, int32 h)
{
	int32				pix, count = 0;
	for (pix = 0; pix < w * h; pix++) {
		if (mask[pix] >= mLow && mask[pix] <= mHigh) {
			count++;
			if (count >= mMaxSeeds) break;
		}
	}
	ArpASSERT(count <= mMaxSeeds);
	if (count < 1) return B_ERROR;

	status_t			err = NewSeeds(count);
	if (err != B_OK) return B_NO_MEMORY;

	int32				index = 0;
	for (int32 y = 0; y < h && index < count; y++) {
		for (int32 x = 0; x < w && index < count; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (mask[pix] >= mLow && mask[pix] <= mHigh) {
				SetSeed(index, float(x), float(y), mask[pix]);
				index++;
			}
		}
	}
	return B_OK;
}

/***************************************************************************
 * GL-SEED-FACTORY
 ****************************************************************************/
GlSeedFactory::GlSeedFactory(int32 maxSeeds, uint8 low, uint8 high)
			: inherited(maxSeeds, low, high), size(0), seeds(0)
{
}

GlSeedFactory::~GlSeedFactory()
{
	delete[] seeds;
}
	
status_t GlSeedFactory::NewSeeds(uint32 count)
{
	delete[] seeds;
	size = 0;
	seeds = new ArpPoint3d[count];
	if (!seeds) return B_NO_MEMORY;
	size = count;
	return B_OK;
}

void GlSeedFactory::SetSeed(uint32 index, float x, float y, uint8 val)
{
	ArpASSERT(index < size);
	seeds[index].x = x;
	seeds[index].y = y;
	seeds[index].z = val;
}
