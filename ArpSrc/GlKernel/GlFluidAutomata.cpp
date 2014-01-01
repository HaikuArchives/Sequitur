#include <be/support/Errors.h>
#include "ArpMath/ArpDefs.h"
#include "GlPublic/GlFluidAutomata.h"
#include "GlPublic/GlPlanes.h"

#define _VERIFY_RANGE(s, newTo) \
	(newTo < s->z + s->size) ? B_OK : s->VerifyRange(newTo)

#define _UNCACHE_CELL(cell, currentAge) \
	if ((cell)->age != currentAge) { \
		(cell)->age = currentAge; \
		(cell)->mass = (cell)->newMass; \
	}

#define _INC_FRAME(x, y) \
	if (mL < 0 || x < mL) mL = x; \
	if (mT < 0 || y < mT) mT = y; \
	if (mR < 0 || x > mR) mR = x; \
	if (mB < 0 || y > mB) mB = y;

/***************************************************************************
 * GL-CA-CELL
 ****************************************************************************/
GlCaCell::GlCaCell()
		: mass(0), newMass(0), age(0)
{
}

GlCaCell& GlCaCell::operator=(const GlCaCell& o)
{
	mass = o.mass;
	newMass = o.newMass;
	age = o.age;
	return *this;
}

// #pragma mark -

/***************************************************************************
 * GL-CA-SLICE
 ****************************************************************************/
GlCaSlice::GlCaSlice()
		: cells(0), size(0), flow(0)
{
}

GlCaSlice::~GlCaSlice()
{
	delete[] cells;
}

status_t GlCaSlice::VerifyRange(uint8 newTo)
{
	ArpASSERT(z + size <= 256);
	ArpVALIDATE(newTo >= z, return B_ERROR);
	if (newTo - z < size) return B_OK;
	if (!cells) {
		ArpASSERT(uint16(newTo - z) + 1 <= 256);
		cells = new GlCaCell[uint16(newTo - z) + 1];
		if (!cells) return B_NO_MEMORY;
		size = uint16(newTo - z) + 1;
		for (uint32 k = 0; k < size; k++) {
			cells[k].newMass = cells[k].mass = 0.0;
			cells[k].age = 0;
		}
		flow = 0;
		return B_OK;
	}
	uint16			newSize = uint16(newTo - z) + 1;
	ArpASSERT(newSize <= 256);
	GlCaCell*		newCells = new GlCaCell[newSize];
	if (!newCells) return B_NO_MEMORY;
	for (uint16 k = 0; k < newSize; k++) {
		if (k < size) newCells[k] = cells[k];
		else {
			newCells[k].newMass = newCells[k].mass = 0.0;
			newCells[k].age = 0;
		}
	}
	delete[] cells;
	cells = newCells;
	size = newSize;
	flow = 0;
	return B_OK;
}

status_t GlCaSlice::FillRange(uint8 low, uint8 high, float mass)
{
	if (!cells || size < 1) return B_ERROR;
	if (low >= z + size || high < z) return B_ERROR;
	int32		from = ARP_MAX(0, int32(low) - z),
				to = ARP_MIN(size - 1, int32(high) - z);
	while (from <= to) {
		cells[from].mass = cells[from].newMass = mass;
		from++;
	}
	return B_OK;
}

status_t GlCaSlice::GetVisibleRange(uint8 fromZ, uint32* outStart, uint32* outEnd) const
{
	ArpASSERT(outStart && outEnd);
	if (size < 1 || fromZ >= z + size) return B_ERROR;

	*outEnd = size - 1;
	if (fromZ <= z) *outStart = 0;
	else *outStart = fromZ - z;
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-FLUID-AUTOMATA
 ***************************************************************************/
GlFluidAutomata::GlFluidAutomata(	float rate, float evaporation,
									float viscosity, float initAmount)
		: slices(0), mW(0), mH(0), mRate(rate), mEvaporation(evaporation),
		  mViscosity(viscosity), mInitAmount(initAmount), mReport(0),
		  mFlags(GRAVITY_BOTTOM_F)
{
	if (mViscosity <= 0.0f) mViscosity = 0.000001f;
}

GlFluidAutomata::~GlFluidAutomata()
{
	Free();
}

status_t GlFluidAutomata::Init(GlPlanes& p)
{
	Free();
	if (p.w < 1 || p.h < 1) return B_ERROR;
	slices = new GlCaSlice[p.w * p.h];
	if (!slices) return B_NO_MEMORY;
	mW = p.w;
	mH = p.h;
	mL = mT = mR = mB = -1;
	mCurrentAge = 0;
	
	for (int32 pix = 0; pix < mW * mH; pix++) slices[pix].z = p.z[pix];

	return B_OK;
}

status_t GlFluidAutomata::Init(GlPlanes& p, float radius)
{
	Free();
	if (p.w < 1 || p.h < 1) return B_ERROR;
	slices = new GlCaSlice[p.w * p.h];
	if (!slices) return B_NO_MEMORY;
	mW = p.w;
	mH = p.h;
	mL = mT = mR = mB = -1;
	mCurrentAge = 0;

	for (int32 pix = 0; pix < mW * mH; pix++) slices[pix].z = p.z[pix];

	int32		x = int32(mW * 0.5), y = int32(mH * 0.5);
	Seed(x, y, ((mW + mH) / 2) * radius);
	return B_OK;
}

status_t GlFluidAutomata::Init(GlPlanes& p, uint8* seed)
{
	Free();
	if (p.w < 1 || p.h < 1 || !seed) return B_ERROR;
	slices = new GlCaSlice[p.w * p.h];
	if (!slices) return B_NO_MEMORY;
	mW = p.w;
	mH = p.h;
	mL = mT = mR = mB = -1;
	mCurrentAge = 0;

	int32			x, y, pix;
	for (y = 0; y < mH; y++) {
		for (x = 0; x < mW; x++) {
			pix = ARP_PIXEL(x, y, mW);
			slices[pix].z = p.z[pix];
			if (seed[pix] > 0 && p.z[pix] < 255) {
				float		v = glTable256[seed[pix]];
				status_t	err = slices[pix].VerifyRange(p.z[pix]);
				if (err != B_OK) return err;
				slices[pix].FillRange(p.z[pix], p.z[pix], v);
				_INC_FRAME(x, y);
			}
		}
	}
	return B_OK;
}

void GlFluidAutomata::Free()
{
	delete[] slices;
	slices = 0;
	mW = mH = 0;
	mReport = 0;
}

status_t GlFluidAutomata::Run(uint32 age, const GlCaReport** report)
{
	if (report) {
		mReportHead.MakeEmpty();
		mReport = &mReportHead;
	} else mReport = 0;

#if 0
for (int32 y = 100 - 10; y <= 100 + 10; y++) {
	for (int32 x = 100 - 10; x <= 100 + 10; x++) {
		printf("%d\t", slices[ARP_PIXEL(x, y, mW)].z);
	}
	printf("\n");
}
#endif

#if 0
float		startMass = 0, endMass = 0;
int32		pix;
for (pix = 0; pix < mW * mH; pix++) {
	for (uint32 k2 = 0; k2 < slices[pix].size; k2++) startMass += slices[pix].cells[k2].mass;
}
#endif

	if (age == 1) age = mCurrentAge + 1;
//	for (mCurrentAge = 0; mCurrentAge < age; mCurrentAge++) {
	while (mCurrentAge < age) {
#if 0
uint32		count = 0;
for (int32 pix = 0; pix < mW * mH; pix++) {
	for (uint32 k2 = 0; k2 < slices[pix].size; k2++) {
		if (slices[pix].cells[k2].mass < 0.01) count++;
	}
}
printf("AGE %ld (under 0.01 count %ld)\n", k, count);
#endif

		if (mL >= 0) {
			if (mFlags&GRAVITY_BOTTOM_F) AgeBottom();
			else AgeBack();
		}
		mCurrentAge++;
	}

	if (report) *report = &mReportHead;

#if 0
for (pix = 0; pix < mW * mH; pix++) {
	for (uint32 k2 = 0; k2 < slices[pix].size; k2++) endMass += slices[pix].cells[k2].mass;
}
printf("start mass %f end mass %f\n", startMass, endMass);
#endif
	return B_OK;
}

#if 1
status_t GlFluidAutomata::Add(uint8* z, float* v, int32 w, int32 h, int32 ox, int32 oy)
{
	ArpVALIDATE(z && v, return B_ERROR);
	for (int32 srcY = 0; srcY < h; srcY++) {
		for (int32 srcX = 0; srcX < w; srcX++) {
			int32					destX = ox + srcX, destY = oy + srcY;
			if (GL_IN_BOUNDS(destX, destY, mW, mH)) {
				int32				srcPix = ARP_PIXEL(srcX, srcY, w);
				if (v[srcPix] > 0.0) {
					int32			destPix = ARP_PIXEL(destX, destY, mW);
					ArpASSERT(destPix >= 0 && destPix < mW * mH);
					GlCaSlice&		s = slices[destPix];
					if (z[srcPix] >= s.z) {
						status_t	err = s.VerifyRange(s.z);
						if (err != B_OK) return err;
						int32		cell = 0;
						_UNCACHE_CELL(&(s.cells[cell]), mCurrentAge);
//						s.cells[cell].mass += v[srcPix];
						s.cells[cell].newMass += v[srcPix];
//						s.cells[cell].mass = v[srcPix];
//						s.cells[cell].newMass = v[srcPix];
						_INC_FRAME(destX, destY);
					}
				}
			}
		}
	}
	return B_OK;
}
#endif

#if 0
status_t GlFluidAutomata::Add(uint8* z, float* v, int32 w, int32 h, int32 ox, int32 oy)
{
	ArpVALIDATE(z && v, return B_ERROR);
	for (int32 srcY = 0; srcY < h; srcY++) {
		for (int32 srcX = 0; srcX < w; srcX++) {
			int32					destX = ox + srcX, destY = oy + srcY;
			if (GL_IN_BOUNDS(destX, destY, mW, mH)) {
				int32				srcPix = ARP_PIXEL(srcX, srcY, w);
				if (v[srcPix] > 0.0) {
					int32			destPix = ARP_PIXEL(destX, destY, mW);
					ArpASSERT(destPix >= 0 && destPix < mW * mH);
					GlCaSlice&		s = slices[destPix];
					if (z[srcPix] >= s.z) {
						status_t	err = s.VerifyRange(z[srcPix]);
						if (err != B_OK) return err;
						int32		cell = z[srcPix] - s.z;
						_UNCACHE_CELL(&(s.cells[cell]), mCurrentAge);
//						s.cells[cell].mass += v[srcPix];
//						s.cells[cell].newMass += v[srcPix];
						s.cells[cell].mass = v[srcPix];
						s.cells[cell].newMass = v[srcPix];
						_INC_FRAME(destX, destY);
					}
				}
			}
		}
	}
	return B_OK;
}
#endif

#if 0
status_t GlFluidAutomata::Seed(int32 seedX, int32 seedY, float rad)
{
	ArpVALIDATE(seedX >= 0 && seedX < mW && seedY >= 0 && seedY < mH, return B_ERROR);
	int32		l = ARP_MAX(0, int32(seedX - rad)), t = ARP_MAX(0, int32(seedY - rad)),
				r = ARP_MIN(mW - 1, int32(seedX + rad)), b = ARP_MIN(mH - 1, int32(seedY + rad));
	ArpASSERT(l >= 0 && l < mW && t >= 0 && t < mH && r >= 0 && r < mW && b >= 0 && b < mH);

	int32		pix = ARP_PIXEL(seedX, seedY, mW);
	uint8		z = slices[pix].z;
	uint8		high = ARP_CLIP_255(int16(z + 2));
	status_t	err = slices[pix].VerifyRange(high);
	if (err != B_OK) return err;
	slices[pix].FillRange(z, high, mInitAmount);
	_INC_FRAME(seedX, seedY);
	return B_OK;
}
#endif
#if 1
status_t GlFluidAutomata::Seed(int32 seedX, int32 seedY, float rad)
{
	ArpVALIDATE(seedX >= 0 && seedX < mW && seedY >= 0 && seedY < mH, return B_ERROR);
	int32		l = ARP_MAX(0, int32(seedX - rad)), t = ARP_MAX(0, int32(seedY - rad)),
				r = ARP_MIN(mW - 1, int32(seedX + rad)), b = ARP_MIN(mH - 1, int32(seedY + rad));
	ArpASSERT(l >= 0 && l < mW && t >= 0 && t < mH && r >= 0 && r < mW && b >= 0 && b < mH);
	
	for (int32 y = t; y <= b; y++) {
		for (int32 x = l; x <= r; x++) {
			float			dist = ARP_DISTANCE(seedX, seedY, x, y);
			if (dist < rad) {
				int32		pix = ARP_PIXEL(x, y, mW);
				uint8		z = slices[pix].z;
				uint8		high = ARP_CLIP_255(int16(z + 2));
				status_t	err = slices[pix].VerifyRange(high);
				if (err != B_OK) return err;
				slices[pix].FillRange(z, high, mInitAmount);
				_INC_FRAME(x, y);
			}
		}
	}
	return B_OK;
}
#endif

#define _CLOSE_ENOUGH(v1, v2)	(ARP_ABS((v1) - (v2)) < 0.001)

static bool _diverged(	GlCaSlice* slice, int32 w, int32 h, int32 l,
						int32 t, int32 r, int32 b, int32* outS, int32* outD)
{
	int32			cenX = 100, cenY = 100;
	uint16			k;
	int32			startT = ARP_MAX(0, t - 10), startL = ARP_MAX(0, l - 10);
	int32			startB = (cenY + (cenY - startT)), startR = (cenX + (cenX - startL));
	for (int32 y = startT; y <= cenY; y++) {
		for (int32 x = startL; x <= cenX; x++) {
			int32	s = ARP_PIXEL(x, y, w);
			*outS = s;
			/* LT quad to RT quad
			 */
			int32	d = ARP_PIXEL(startR - (x - startL), y, w);
//			int32	d = ARP_PIXEL(cenX + (cenX - x), cenY, w);
			*outD = d;
			if (slice[s].size != slice[d].size) return true;
			for (k = 0; k < slice[s].size; k++) {
				if (!_CLOSE_ENOUGH(slice[s].cells[k].mass, slice[d].cells[k].mass)) return true;
				if (!_CLOSE_ENOUGH(slice[s].cells[k].newMass, slice[d].cells[k].newMass)) return true;
			}
			/* LT quad to RB quad
			 */
			d = ARP_PIXEL(startR - (x - startL), startB - (y - startT), w);
			*outD = d;
			if (slice[s].size != slice[d].size) return true;
			for (k = 0; k < slice[s].size; k++) {
				if (!_CLOSE_ENOUGH(slice[s].cells[k].mass, slice[d].cells[k].mass)) return true;
				if (!_CLOSE_ENOUGH(slice[s].cells[k].newMass, slice[d].cells[k].newMass)) return true;
			}
			/* LT quad to LB quad
			 */
			d = ARP_PIXEL(x, startB - (y - startT), w);
			*outD = d;
			if (slice[s].size != slice[d].size) return true;
			for (k = 0; k < slice[s].size; k++) {
				if (!_CLOSE_ENOUGH(slice[s].cells[k].mass, slice[d].cells[k].mass)) return true;
				if (!_CLOSE_ENOUGH(slice[s].cells[k].newMass, slice[d].cells[k].newMass)) return true;
			}
		}
	}
	return false;
}
#undef _CLOSE_ENOUGH

//#define _TRAP_DIVERGED		(1)

#define _PRINT_STUFF(tx, ty) \
	p = ARP_PIXEL(tx, ty, mW); \
	printf("\t%s: (%d, %d), z %d ", str, tx, ty, slices[p].z); \
	for (uint16 kk = 0; kk < slices[p].size; kk++) printf("(%d m %f nm %f) ", \
			kk, slices[p].cells[kk].mass, slices[p].cells[kk].newMass); \
	printf("\n"); fflush(stdout);

/* Make sure I've got a report I can write to, if need be.
 */
#define _VERIFY_REPORT() \
	if (mReport) { \
		if (mReport->size >= mReport->MAX_SIZE) { \
			if (mReport->next) { \
				mReport = mReport->next; \
				mReport->MakeEmpty(); \
			} else { \
				mReport = (mReport->next = new GlCaReport()); \
				if (!mReport) return B_NO_MEMORY; \
			} \
		} \
		mReport->pix[mReport->size] = pix; \
	}

#if 1
#define _INIT_SLICES() \
	if (!initS) { \
		initS = true; \
		countS = 0; \
		incFrame[0] = incFrame[1] = incFrame[2] = incFrame[3] = false; \
		if (y > 0) { \
			neighS[countS] = &(slices[pix - mW]); \
			if (mReport) reportChange[countS] = &(mReport->north[mReport->size]); \
			neighX[countS] = x; \
			neighY[countS] = y - 1; \
			countS++; \
		} \
		if (y < mH - 1) { \
			neighS[countS] = &(slices[pix + mW]); \
			if (mReport) reportChange[countS] = &(mReport->south[mReport->size]); \
			neighX[countS] = x; \
			neighY[countS] = y + 1; \
			countS++; \
		} \
		if (x < mW - 1) { \
			neighS[countS] = &(slices[pix + 1]); \
			if (mReport) reportChange[countS] = &(mReport->east[mReport->size]); \
			neighX[countS] = x + 1; \
			neighY[countS] = y; \
			countS++; \
		} \
		if (x > 0) { \
			neighS[countS] = &(slices[pix - 1]); \
			if (mReport) reportChange[countS] = &(mReport->west[mReport->size]); \
			neighX[countS] = x - 1; \
			neighY[countS] = y; \
			countS++; \
		} \
	}
	
#define _INIT_CELLS(curZ, curMass) \
	countC = 0; \
	for (k = 0; k < countS; k++) { \
		if (neighS[k]->z <= curZ) { \
			if (_VERIFY_RANGE(neighS[k], curZ) != B_OK) return B_NO_MEMORY; \
			neighC[countC] = &(neighS[k]->cells[curZ - neighS[k]->z]); \
			_UNCACHE_CELL(neighC[countC], mCurrentAge); \
			if (neighC[countC]->mass < 1.0 && neighC[countC]->mass < curMass) { \
				indexC[countC] = k; \
				countC++; \
			} \
		} \
	}

status_t GlFluidAutomata::AgeBack()
{
#if 1
printf("AGE %ld\n", mCurrentAge);
int32		p;
const char*	str = "start";
_PRINT_STUFF(184, 271);
_PRINT_STUFF(183, 272);
_PRINT_STUFF(185, 272);
_PRINT_STUFF(184, 273);
_PRINT_STUFF(184, 272);
#endif
	ArpASSERT(mL >= 0 && mL < mW && mT >= 0 && mT < mH && mR >= 0 && mR < mW && mB >= 0 && mB < mH);
	ArpASSERT(mL <= mR && mT <= mB);

	bool					initS;
	uint32					countS, k;
	/* The neighS, neighX, neighY, reportChange and incFrame
	 * are indexed by countS or indexC
	 */
	GlCaSlice*				neighS[4];
	int32					neighX[4], neighY[4];
	float*					reportChange[4];
	bool					incFrame[4], toNeigh;
	/* The neighC and indexC are indexed by countC
	 */
	GlCaCell*				neighC[4];
	uint32					indexC[4], countC;			// indexes into the other arrays
	float					curGive, give, take;
#if 0
float		startMass = 0, mass;
for (p = 0; p < mW * mH; p++) {
	for (uint32 k2 = 0; k2 < slices[p].size; k2++) startMass += slices[p].cells[k2].newMass;
}
#endif

	for (int32 y = mT; y <= mB; y++) {
		int32			pix = ARP_PIXEL(mL, y, mW);
		for (int32 x = mL; x <= mR; x++) {
			GlCaSlice&		s = slices[pix];
			if (s.size > 0) {
//printf("(%ld, %ld)\n", x, y);
				ArpASSERT(pix == ARP_PIXEL(x, y, mW));
				_VERIFY_REPORT();
				initS = false;
				toNeigh = false;
//if (x == 100 && y == 100) debugger("dsds");
				/* The first cell, which always corresponds to the z value of the slice,
				 * is the only one that will spread.  Find all valid neighbors (that is,
				 * neighbors that exist and have a z less than or equal to mine, and that
				 * aren't against my current direction), divide up my mass between them,
				 * and let it flow.
				 */
				for (uint32 j = 0; j < s.size; j++) {
					_UNCACHE_CELL(&(s.cells[j]), mCurrentAge);
					if (s.cells[j].mass >= mViscosity) {
						curGive = ARP_MIN(s.cells[j].mass, mRate);
						if (j > 0 && s.cells[j-1].mass < 1.0f) {
							take = 1 - s.cells[j-1].mass;
							give = ARP_MIN(take, curGive);
							s.cells[j-1].newMass += give;
							s.cells[j].newMass -= give;
							curGive -= give;
						}
						if (curGive > 0.0) {
							_INIT_SLICES();
							if (countS > 0) {
								ArpASSERT(uint32(s.z) + j <= 255);
								_INIT_CELLS(uint8(s.z + j), s.cells[j].mass);

								if (countC > 0) {
									toNeigh = true;
									take = (curGive / countC) * 0.5f;
//									s.cells[j].newMass -= curGive;
									for (k = 0; k < countC; k++) {
										s.cells[j].newMass -= take;
										neighC[k]->newMass += take;
										if (mReport) (*(reportChange[indexC[k]])) += take;
										incFrame[indexC[k]] = true;
									}
								}
							}
						}
					}
				}
				if (toNeigh) {
					if (mReport) mReport->size++;
					for (k = 0; k < countS; k++) {
						if (incFrame[k]) _INC_FRAME(neighX[k], neighY[k]);
					}
				}
			}
			pix++;
		}
	}
#if 1
str = "end";
_PRINT_STUFF(184, 271);
_PRINT_STUFF(183, 272);
_PRINT_STUFF(185, 272);
_PRINT_STUFF(184, 273);
_PRINT_STUFF(184, 272);
#endif
#ifdef _TRAP_DIVERGED
int32		errS, errD;
if (_diverged(slices, mW, mH, mL, mT, mR, mB, &errS, &errD)) {
	uint16	iter;
	printf("END DIVERGED\n(%ld, %ld) ", ARP_X_FROM_PIXEL(errS, mW), ARP_Y_FROM_PIXEL(errS, mW));
	for (iter = 0; iter < slices[errS].size; iter++) printf("(m %f nm %f) ", slices[errS].cells[iter].mass, slices[errS].cells[iter].newMass);
	printf("\n(%ld, %ld) ", ARP_X_FROM_PIXEL(errD, mW), ARP_Y_FROM_PIXEL(errD, mW));
	for (iter = 0; iter < slices[errD].size; iter++) printf("(m %f nm %f) ", slices[errD].cells[iter].mass, slices[errD].cells[iter].newMass);
	printf("\n");
	ArpASSERT(false);
}
#endif	
	return B_OK;
}








status_t GlFluidAutomata::AgeBottom()
{
	ArpASSERT(mL >= 0 && mL < mW && mT >= 0 && mT < mH && mR >= 0 && mR < mW && mB >= 0 && mB < mH);
	ArpASSERT(mL <= mR && mT <= mB);
ArpASSERT(false);
return B_ERROR;
#if 0
#if 0
	bool					initS;
	uint32					countS, k;
	/* The neighS, neighX, neighY, reportChange and incFrame
	 * are indexed by countS or indexC
	 */
	GlCaSlice*				neighS[4];
	int32					neighX[4], neighY[4];
	float*					reportChange[4];
	bool					incFrame[4], toNeigh;
	/* The neighC and indexC are indexed by countC
	 */
	GlCaCell*				neighC[4];
	uint32					indexC[4], countC;			// indexes into the other arrays
	float					curGive, give, take;
#endif
	GlCaSlice*				southS;
	GlCaCell*				southC;
	float					give, take;
	
	for (int32 y = mT; y <= mB; y++) {
		int32			pix = ARP_PIXEL(mL, y, mW);
		for (int32 x = mL; x <= mR; x++) {
			GlCaSlice&		s = slices[pix];
			if (s.size > 0) {
				ArpASSERT(pix == ARP_PIXEL(x, y, mW));
				_VERIFY_REPORT();
//				initS = false;
//				toNeigh = false;
				/* Only the bottom cell can do anything but fall further down the slice.
				 */
				give = 0.0;
				_UNCACHE_CELL(&(s.cells[0]), mCurrentAge);
				if (s.cells[0].mass > 0.0) {
					give = ARP_MAX(s.cells[0].mass, mRate);
					if (y < mH - 1) {
						southS = &(slices[pix + mW]);
						if (southS->z <= s.z) {
							_VERIFY_RANGE(southS, s.z);
							southC = &(southS->cells[s.z - southS->z]);
							_UNCACHE_CELL(southC, mCurrentAge);
							if (southC->mass < 1.0) {
								take = 
							}
						}
					}
				}
								
				/* The first cell, which always corresponds to the z value of the slice,
				 * is the only one that will spread.  Find all valid neighbors (that is,
				 * neighbors that exist and have a z less than or equal to mine, and that
				 * aren't against my current direction), divide up my mass between them,
				 * and let it flow.
				 */
				for (uint32 j = 1; j < s.size; j++) {
					_UNCACHE_CELL(&(s.cells[j]), mCurrentAge);
					if (s.cells[j].mass >= mViscosity) {
						curGive = ARP_MIN(s.cells[j].mass, mRate);
						if (j > 0 && s.cells[j-1].mass < 1.0) {
							take = 1 - s.cells[j-1].mass;
							give = ARP_MIN(take, curGive);
							s.cells[j-1].newMass += give;
							s.cells[j].newMass -= give;
							curGive -= give;
						}
						if (curGive > 0.0) {
							_INIT_SLICES();
							if (countS > 0) {
								ArpASSERT(uint32(s.z) + j <= 255);
								_INIT_CELLS(uint8(s.z + j), s.cells[j].mass);

								if (countC > 0) {
									toNeigh = true;
									take = (curGive / countC) * 0.5;
//									s.cells[j].newMass -= curGive;
									for (k = 0; k < countC; k++) {
										s.cells[j].newMass -= take;
										neighC[k]->newMass += take;
										if (mReport) (*(reportChange[indexC[k]])) += take;
										incFrame[indexC[k]] = true;
									}
								}
							}
						}
					}
				}
				if (toNeigh) {
					if (mReport) mReport->size++;
					for (k = 0; k < countS; k++) {
						if (incFrame[k]) _INC_FRAME(neighX[k], neighY[k]);
					}
				}
			}
			pix++;
		}
	}
	return B_OK;
#endif
}




#undef _INIT_SLICES
#undef _INIT_CELLS

#endif

#undef _TRAP_DIVERGED

#undef _VERIFY_REPORT
#undef _VERIFY_RANGE
#undef _UNCACHE_CELL
#undef _INC_FRAME

// #pragma mark -

/***************************************************************************
 * GL-CA-REPORT
 ****************************************************************************/
GlCaReport::GlCaReport()
		: next(0), size(0)
{
	MakeEmpty();
}

GlCaReport::~GlCaReport()
{
	delete next;
	next = 0;
}

void GlCaReport::MakeEmpty()
{
	size = 0;
	memset(north, 0, MAX_SIZE * 4);
	memset(south, 0, MAX_SIZE * 4);
	memset(east, 0, MAX_SIZE * 4);
	memset(west, 0, MAX_SIZE * 4);
}
