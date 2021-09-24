#include <cstdio>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlNodes/GlBevelSrf.h>

enum {
	ARP_BEVEL_LOW_QUALITY		= 'low_',
	ARP_BEVEL_TEST_QUALITY		= 'test',
	ARP_BEVEL_HIGH_QUALITY		= 'high'
};

static const int32		GL_BEVEL_KEY			= 'ApBv';
static const uint8		MIN_Z					= 0;

/***************************************************************************
 * GL-BEVEL-SURFACE
 * A surface for doing the actual beveling.
 ***************************************************************************/
class GlBevelSurface : public GlAlgo2d
{
public:
	GlBevelSurface(gl_node_id nid, int32 q, int32 w, int32 s);
	GlBevelSurface(const GlBevelSurface& o);
	virtual ~GlBevelSurface();

	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* src, uint8* dest, int32 w, int32 h,
								GlProcessStatus* status = 0);
	
private:
	typedef GlAlgo2d	inherited;
	int32				mQuality;
	int32				mW;
	int32				mSmoothing;

	uint8				mMaxZ;
	/* Cache the distances, considering the bottom left as the center point.
	 * The distance calculation is slow; this makes it a look-up.
	 */
	float*				mDist;
	int32				mDistW, mDistH;
	
	status_t			InitDist(int32 w, int32 h);

	/* ALGORITHMS
	 */
	status_t			LowQuality(	uint8* src, uint8* dest, int32 w, int32 h);

	status_t			HighQuality(uint8* src, uint8* dest, int32 w, int32 h);
	/* Step 1 actually is the full low-quality algorithm, although it also
	 * annotates the points with which points are being beveled (i.e. 'on'),
	 * which points are low, and which are high.
	 */
	status_t			HqStep1(uint8* src, uint8* dest, uint8* points, int32 w, int32 h);
	/* Step 2 adds new high points, specifically the ones that occur in
	 * sections where two sides of the original object are too close together
	 * to have generated high points in step 1.
	 */
	status_t			HqStep2(uint8* src, uint8* dest, uint8* points, int32 w, int32 h);
	/* Step 3 runs through all ON_PTs and assigns them a value based on
	 * their distance between the nearest LOW_PT and HIGH_PT.
	 */
	status_t			HqStep3(uint8* src, uint8* dest, uint8* points, int32 w, int32 h);

	status_t			TestQuality(uint8* src, uint8* dest, int32 w, int32 h);
	status_t			Accurate1(uint8* dest, uint8* flags, int32 w, int32 h);
	status_t			Accurate2(uint8* dest, uint8* flags, int32 w, int32 h);
	status_t			Accurate3(uint8* dest, uint8* flags, int32 w, int32 h);
};

/***************************************************************************
  * GL-BEVEL-SRF
 ***************************************************************************/
GlBevelSrf::GlBevelSrf(const GlBevelSrfAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlBevelSrf::GlBevelSrf(const GlBevelSrf& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlBevelSrf::Clone() const
{
	return new GlBevelSrf(*this);
}

GlAlgo* GlBevelSrf::Generate(const gl_generate_args& args) const
{
	return new GlBevelSurface(	Id(), Params().Menu('qlty'),
								Params().Int32('wdth'),
								Params().Int32('smth'));
}

// #pragma mark -

/***************************************************************************
 * GL-BEVEL-SRF-ADD-ON
 ***************************************************************************/
GlBevelSrfAddOn::GlBevelSrfAddOn()
		: inherited(SZI[SZI_arp], GL_BEVEL_KEY, SZ(SZ_2D), SZ(SZ_Bevel), 1, 0)
{
	BMessage		m;
	m.AddString("item", "Low");			m.AddInt32("i", ARP_BEVEL_LOW_QUALITY);
	m.AddString("item", "High");		m.AddInt32("i", ARP_BEVEL_HIGH_QUALITY);
	m.AddString("item", "Test");		m.AddInt32("i", ARP_BEVEL_TEST_QUALITY);

	mQuality	= AddParamType(new GlMenuParamType('qlty',  SZ(SZ_Quality), m, ARP_BEVEL_HIGH_QUALITY));
	mWidth		= AddParamType(new GlInt32ParamType('wdth', SZ(SZ_Width), 0, 255, 5));
	mSmoothing	= AddParamType(new GlInt32ParamType('smth', SZ(SZ_Smoothing), 0, 255, 4));
}

GlNode* GlBevelSrfAddOn::NewInstance(const BMessage* config) const
{
	return new GlBevelSrf(this, config);
}

// #pragma mark -

/***************************************************************************
 * GL-BEVEL-SURFACE
 ***************************************************************************/
GlBevelSurface::GlBevelSurface(	gl_node_id nid, int32 q, int32 w, int32 s)
		: inherited(nid, 0), mQuality(q), mW(w), mSmoothing(s),
		  mMaxZ(255), mDist(0), mDistW(0), mDistH(0)
{
}

GlBevelSurface::GlBevelSurface(const GlBevelSurface& o)
		: inherited(o), mQuality(o.mQuality), mW(o.mW), mSmoothing(o.mSmoothing),
		  mMaxZ(o.mMaxZ), mDist(0), mDistW(0), mDistH(0)
{
}

 GlBevelSurface::~GlBevelSurface()
 {
	delete[] mDist;
 }

GlAlgo* GlBevelSurface::Clone() const
{
	return new GlBevelSurface(*this);
}

status_t GlBevelSurface::Process(	const GlPlanes* pixels, uint8* dest, int32 w, int32 h,
									GlProcessStatus* status)
{
bigtime_t				start = system_time(), stop;
	if (w >= mDistW || h >= mDistH) {
		if (InitDist(w, h) != B_OK) return B_OK;
	}
	GlMask		srcMask(dest, w, h);
	uint8*		src = srcMask.Data();
	if (!src) return B_OK;

	if (mQuality == ARP_BEVEL_HIGH_QUALITY) HighQuality(src, dest, w, h);
	else if (mQuality == ARP_BEVEL_TEST_QUALITY) TestQuality(src, dest, w, h);
	else LowQuality(src, dest, w, h);

#if 0
	int32		x, y, pix;
	/* SMOOTH
	 */
	if (mSmoothing > 0) {
		for (pix = 0; pix < w * h; pix++) src[pix] = dest[pix];
		
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				pix = ARP_PIXEL(x, y, w);
				int32		sum = 0;
				for (int32 row = y - mSmoothing; row <= y + mSmoothing; row++) {
					for (int32 col = x - mSmoothing; col <= x + mSmoothing; col++) {
						sum += src[ARP_PIXEL_SE(col, row, w, h)];
					}
				}
				dest[pix] = arp_clip_255(sum / ARP_SQR(mSmoothing * 2 + 1));
			}
		}
	}
#endif
	/* SCALE -- the blur operation has increased the bevel beyond the bounds of the object.
	 */
#if 0
	uint8		low = 255, high = 0;
	for (pix = 0; pix < w * h; pix++) {
		if (src[pix] > MIN_Z) {
			if (dest[pix] < low) low = dest[pix];
			if (dest[pix] > high) high = dest[pix];
		} else dest[pix] = 0;
	}
//	printf("LOW %d HIGH %d\n", low, high);
	if (low > 3) {
		int32		srcR = high - low;
		if (srcR == 0) return;
		uint8		from = 1, to = high;
		int32		destR = to - from;

		for (pix = 0; pix < w * h; pix++)
			dest[pix] = arp_clip_255(from + (( (dest[pix] - low) * destR) / srcR));
	}
#endif

stop = system_time();
printf("Bevel: %f sec\n", double(stop - start) / 1000000);
	return B_OK;
}

status_t GlBevelSurface::InitDist(int32 w, int32 h)
{
	delete[] mDist;
	mDistW = mDistH = 0;
	mDist = new float[(w + 1) * (h + 1)];
	if (!mDist) return B_NO_MEMORY;
	mDistW = w + 1;
	mDistH = h + 1;
	for (int32 y = 0; y < mDistH; y++) {
		for (int32 x = 0; x < mDistW; x++) {
			mDist[ARP_PIXEL(x, y, mDistW)] = ARP_DISTANCE(0, 0, x, y);
//			printf("%f\t", mDist[ARP_PIXEL(x, y, mDistW)]);
		}
//		printf("\n");
	}
	return B_OK;
}

// #pragma mark -

#define GET_DISTANCE(x0, y0, x1, y1)		(mDist[ARP_PIXEL(ARP_ABS((x1) - (x0)), ARP_ABS((y1) - (y0)), mDistW)])

/***************************************************************************
 * Low Quality Algorithm.  Very simple distance method -- the closer to
 * the edge, the more shaded the pixel.
 ***************************************************************************/
status_t GlBevelSurface::LowQuality(uint8* src, uint8* dest, int32 w, int32 h)
{
	int32		x, y, pix;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (dest[pix] > MIN_Z) {
				float			dist = -1;
				for (int32 row = -mW; row <= mW; row++) {
					for (int32 col = -mW; col <= mW; col++) {
						int32		tx = x + col, ty = y + row;
						if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
							if (src[ARP_PIXEL(tx, ty, w)] <= MIN_Z) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d < mW && (dist < 0 || d < dist))
									dist = d;
							}
						}
					}
				}
				if (dist >= 0) dest[pix] = arp_clip_255((dist / mW) * 255);
				else dest[pix] = 255;
			}
		}
	}

	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * High Quality Algorithm.
 ***************************************************************************/
enum {
	LOW_PT			= 0x0001,
	HIGH_PT			= 0x0002,
	ON_PT			= 0x0004,
	IN_USE_PT		= 0x0008,

	TMP_HIGH_PT		= 0x0010
};

status_t GlBevelSurface::HighQuality(uint8* src, uint8* dest, int32 w, int32 h)
{
//	GlMask		srcMask(dest, w, h);
//	uint8*		src = srcMask.Data();
//	if (!src) return B_NO_MEMORY;

	uint8*		points = new uint8[w * h];
	if (!points) return B_NO_MEMORY;

//	bigtime_t				start = system_time(), stop;
	HqStep1(src, dest, points, w, h);
//	stop = system_time();
//	printf("Step 1: %f sec\n", double(stop - start) / 1000000);

//	start = system_time(), stop;
	HqStep2(src, dest, points, w, h);
//	stop = system_time();
//	printf("Step 2: %f sec\n", double(stop - start) / 1000000);

//	start = system_time(), stop;
	HqStep3(src, dest, points, w, h);
//	stop = system_time();
//	printf("Step 3: %f sec\n", double(stop - start) / 1000000);

#if 0
	for (int32 pix = 0; pix < w * h; pix++) {
		if (avg[pix].count > 1) dest[pix] = arp_clip_255((avg[pix].sum / avg[pix].count) * 255);
		else dest[pix] = arp_clip_255(avg[pix].sum* 255);
	}
#endif
	
#if 0
	for (int32 pix = 0; pix < w * h; pix++) {
		if (points[pix]&LOW_PT) dest[pix] = 127;
		else if (points[pix]&HIGH_PT) dest[pix] = 255;
		else if (points[pix]&ON_PT) dest[pix] = 80;
		else dest[pix] = 0;
	}
#endif
	
	delete[] points;

	return B_OK;
}

#define IS_LOW_EDGE(tx, ty, x, y, w, h, src, minZ)	\
	( (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) \
			&& (src[ARP_PIXEL(tx, ty, w)] <= minZ) )

status_t GlBevelSurface::HqStep1(	uint8* src, uint8* dest,
									uint8* points, int32 w, int32 h)
{
	ArpASSERT(src && dest && points);

	int32		x, y, pix;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (dest[pix] > MIN_Z) {
				float			dist = -1;
				int32			diamond = 1;
				/* Use a diamond-square search pattern to find the nearest MIN_Z soonest.
				 * First search in a pattern emanating out from the point of interest.
				 * Stop when the bevel width or (hopefully) a dist value is reached.
				 * Since the search is based on distance, the actual values are somewhere
				 * between a diamond and a square, so once the diamond is found, fill
				 * in the edges to make it a square in case someone closer is lurking
				 * in the corners.
				 */
				/* DIAMOND
				 */
				while (diamond <= mW + 1) {
					for (int32 row = -diamond; row <= diamond; row++) {
						int32	col = diamond - ARP_ABS(row);
						/* Do the left side of the diamond (or the top/bottom peak)
						 */
						int32	tx = x - col, ty = y + row;
						if (IS_LOW_EDGE(tx, ty, x, y, w, h, src, MIN_Z)) {
							float	d = GET_DISTANCE(x, y, tx, ty);
							ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//							float	d = ARP_DISTANCE(x, y, tx, ty);
							if (d <= mW && (dist < 0 || d < dist)) dist = d;
						}
						/* Do the right side of the diamond (if there's no top/bottom peak)
						 */
						if (col != 0) {
							tx = x + col, ty = y + row;
							if (IS_LOW_EDGE(tx, ty, x, y, w, h, src, MIN_Z)) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d <= mW && (dist < 0 || d < dist)) dist = d;
							}
						}
					}
					diamond++;
					if (dist >= 0) break;
				}
				/* SQUARE
				 */
				diamond--;
				int32			square = 1;
				while (square <= diamond) {
					for (int32 row = -diamond; row <= diamond; row++) {
						int32	col = diamond - ARP_ABS(row) + square;
						if (col <= mW + 1) {
							ArpASSERT(col != 0);
							int32	tx = x - col, ty = y + row;
							/* Left side of square.
							 */
							if (IS_LOW_EDGE(tx, ty, x, y, w, h, src, MIN_Z)) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d <= mW && (dist < 0 || d < dist)) dist = d;
							}
							/* Right side of square.
							 */
							tx = x + col, ty = y + row;
							if (IS_LOW_EDGE(tx, ty, x, y, w, h, src, MIN_Z)) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d <= mW && (dist < 0 || d < dist)) dist = d;
							}
						}
					}
					square++;
				}

				if (dist >= 0) {
					if (dist > mW - 1) {
						points[pix] = HIGH_PT | ON_PT;
						dest[pix] = mMaxZ;
					} else if (dist <= 1) {
						points[pix] = LOW_PT | ON_PT;
					} else {
						points[pix] = ON_PT;
					}
				} else {
					// Set all pixels at or above the high points to the maximum.
					dest[pix] = mMaxZ;
					points[pix] = 0;
				}
			} else {
				// Do nothing with all pixels below or at the minimum Z.
				points[pix] = 0;
			}
		}
	}
	return B_OK;
}

#define				ADJACENT(x1, y1, x2, y2)		(ARP_ABS((x2) - (x1)) <= 1 && ARP_ABS((y2) - (y1)) <= 1)
static int32		DISCONNECTED_BIN = 0;
static int32		FIRST_BIN = 1;

class _ClosestPt
{
public:
	int32*		xes;
	int32*		ys;
	float*		dists;
	uint32		count;
	uint32		size;
	
	float		lowDist;
	
	_ClosestPt(uint32 inSize)
	{
		size = inSize;
		xes = new int32[size];
		ys = new int32[size];
		dists = new float[size];
		mBin = new int32[size];
	}

	~_ClosestPt()
	{
		delete[] xes;
		delete[] ys;
		delete[] dists;
		delete[] mBin;
	}
	
	status_t	InitCheck() const
	{
		if (!xes) return B_NO_MEMORY;
		if (!ys) return B_NO_MEMORY;
		if (!dists) return B_NO_MEMORY;
		if (!mBin) return B_NO_MEMORY;
		return B_OK;
	}
	
	inline void	Begin()
	{
		count = 0;
		lowDist = -1;
	}
	
	inline void	Add(int32 x, int32 y, float d)
	{
		ArpVALIDATE(count < size, return);
		xes[count] = x;
		ys[count] = y;
		dists[count] = d;
		count++;
		if (lowDist < 0 || d < lowDist) lowDist = d;
	}

	inline bool	IsHigh(int32 x, int32 y, uint8* points, int32 w, int32 h)
	{
		if (count < 2) return false;
		uint32		k, compareK, l = 0, t = 0, r = 0, b = 0;
		// If all points are above, all are left, etc. then we're not high.
		for (k = 0; k < count; k++) {
			if (xes[k] < x) l++;
			if (ys[k] < y) t++;
			if (xes[k] > x) r++;
			if (ys[k] > y) b++;
		}

		for (k = 1; k < count; k++) mBin[k] = -1;
		/* Find the initial bin -- the first point that connects.
		 */
		uint32		binCount = 1;
		bool		moreBins = false;
		/* Bin 0 is for points that have been tested but aren't connected.
		 */
		for (k = 0; k < count; k++) {
			if (IsConnected(points, w, xes[k], ys[k], x, y)) {
				mBin[k] = FIRST_BIN;
				moreBins = true;
				binCount++;
				break;
			} else {
				mBin[k] = DISCONNECTED_BIN;
			}
		}
		/* Fill the bins -- find each point adjacent to the current bin.
		 */
		while (moreBins) {
			bool	morePoints = true;
			while (morePoints) {
				morePoints = false;
				for (k = 0; k < count; k++) {
					if (mBin[k] < DISCONNECTED_BIN) {
						/* Make sure the point is connected, otherwise do not bin it.
						 */
						if (!IsConnected(points, w, xes[k], ys[k], x, y)) {
							mBin[k] = DISCONNECTED_BIN;
						} else {
							for (compareK = 0; compareK < count; compareK++) {
								if (k != compareK && mBin[compareK] >= FIRST_BIN
										&& ADJACENT(xes[k], ys[k], xes[compareK], ys[compareK])) {
									mBin[k] = mBin[compareK];
									morePoints = true;
									break;
								}
							}
						}
					}
				}
			}
			moreBins = false;
			/* When I complete a bin, check to see if I still have unbinned data.  If
			 * so, create a new bin and keep searching.
			 */
			for (k = 0; k < count; k++) {
				if (mBin[k] < DISCONNECTED_BIN) {
					mBin[k] = binCount;
					binCount++;
					moreBins = true;
					compareK = k;
					break;
				}
			}
		}
#if 0
if ((x == 77 && y == 101) || (x == 78 && y == 101)) {
//if ((x == 78 && y == 101)) {
	printf("(%ld, %ld) IsHigh() %ld bins\n", x, y, binCount);
	for (k = 0; k < count; k++) printf("\t(%ld, %ld) d %f bin %ld\n", xes[k], ys[k], dists[k], mBin[k]);
	printf("\tl %ld t %ld r %ld b %ld\n", l, t, r, b);
}
#endif
		/* If everything is in the same bin, all the points are connected
		 * and that means I can't be a high.  The check is against 2 because
		 * the first bin (bin 0) is always points that have been tested
		 * but are not connected.
		 */
		if (binCount <= 2) return false;
		/* Find the nearest point in each bin.  If the nearest points are all
		 * in range of each other, then I'm at the center and should be high.
		 */
		float			range = 1.7f;
		/* Otherwise, if all the low dists in all the bins are within 0.5 of each
		 * other, I'm a high.
		 */
		float			bin0Dist = BinDist(FIRST_BIN);
		for (k = FIRST_BIN + 1; k < binCount; k++) {
			if (ARP_ABS(bin0Dist - BinDist(k)) > range) return false;
		}
		return true;
	}

private:
	int32*		mBin;

	inline float	BinDist(int32 index) const
	{
		float			dist = -1;
		for (uint32 k = 0; k < count; k++) {
			if (mBin[k] == index) {
				if (dist < 0 || dists[k] < dist)
					dist = dists[k];
			}
		}
		return dist;
	}

	/* Answer true if the line between pt1 and pt2 is ON_PT at every step.
	 */
	inline bool IsConnected(uint8* points, int32 w, int32 x1, int32 y1, int32 x2, int32 y2) const
	{
		int32	d, x, y, ax, ay, sx, sy, dx, dy, pix;
		dx = x2-x1;  ax = ARP_ABS(dx)<<1;  sx = ARP_SGN(dx);
		dy = y2-y1;  ay = ARP_ABS(dy)<<1;  sy = ARP_SGN(dy);

		x = x1;
		y = y1;
		if (ax>ay) {		/* x dominant */
			d = ay-(ax>>1);
			for (;;) {
				pix = ARP_PIXEL(x, y, w);
				if (!(points[pix]&ON_PT)) return false;
				if (x == x2) return true;
				if (d >= 0) {
					y += sy;
					d -= ax;
				}
				x += sx;
				d += ay;
			}
		} else {			/* y dominant */
			d = ax-(ay>>1);
			for (;;) {
				pix = ARP_PIXEL(x, y, w);
				if (!(points[pix]&ON_PT)) return false;
				if (y == y2) return true;
				if (d >= 0) {
					x += sx;
					d -= ay;
				}
				y += sy;
				d += ax;
			}
		}
		return true;
	}
};

status_t GlBevelSurface::HqStep2(	uint8* src, uint8* dest,
									uint8* points, int32 w, int32 h)
{
	ArpASSERT(src && dest && points);

	int32		x, y, pix;
	/* The closest stores all the closest low or high points.  There can't
	 * be more than mW * 8.
	 */
	_ClosestPt	closest(mW * 20);
	if (closest.InitCheck() != B_OK) return B_NO_MEMORY;
	float		limit = float(mW - 1);
	int32		range = mW;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (points[pix]&ON_PT && !(points[pix]&HIGH_PT) && !(points[pix]&LOW_PT)) {
				closest.Begin();
				for (int32 row = -range; row <= range; row++) {
					for (int32 col = -range; col <= range; col++) {
						int32		tx = x + col, ty = y + row;
						if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
							int32	pix2 = ARP_PIXEL(tx, ty, w);
							if (points[pix2]&LOW_PT) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d <= limit) closest.Add(tx, ty, d);
							}
						}
					}
				}
				if (closest.lowDist > 1) {
					if (closest.IsHigh(x, y, points, w, h)) {
						points[pix] |= TMP_HIGH_PT;
						dest[pix] = mMaxZ;
					}
				}
			}
		}
	}

	for (pix = 0; pix < w * h; pix++) {
		if (points[pix]&TMP_HIGH_PT) points[pix] |= HIGH_PT;
	}
	return B_OK;
}

status_t GlBevelSurface::HqStep3(	uint8* src, uint8* dest,
									uint8* points, int32 w, int32 h)
{
	ArpASSERT(src && dest && points);

	int32		x, y, pix;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (points[pix]&ON_PT && !(points[pix]&HIGH_PT)) {
				float			lowDist = -1, highDist = -1;
				int32			diamond = 1;
				if (points[pix]&LOW_PT) lowDist = 0;
				/* Use a diamond-square search pattern to find the nearest MIN_Z soonest.
				 * First search in a pattern emanating out from the point of interest.
				 * Stop when the bevel width or (hopefully) a dist value is reached.
				 * Since the search is based on distance, the actual values are somewhere
				 * between a diamond and a square, so once the diamond is found, fill
				 * in the edges to make it a square in case someone closer is lurking
				 * in the corners.
				 */
				/* DIAMOND
				 */
				while (diamond <= mW + 1) {
					for (int32 row = -diamond; row <= diamond; row++) {
						int32	col = diamond - ARP_ABS(row);
						int32	tx = x - col, ty = y + row;
						/* Do the left side of the diamond (or the top/bottom peak)
						 */
						if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
							int32	tpix = ARP_PIXEL(tx, ty, w);
							if (points[tpix]&LOW_PT) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d <= mW && (lowDist < 0 || d < lowDist)) lowDist = d;
							} else if (points[tpix]&HIGH_PT) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//								float	d = ARP_DISTANCE(x, y, tx, ty);
								if (d <= mW && (highDist < 0 || d < highDist)) highDist = d;
							}
						}
						/* Do the right side of the diamond (if there's no top/bottom peak)
						 */
						if (col != 0) {
							tx = x + col, ty = y + row;
							if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
								int32	tpix = ARP_PIXEL(tx, ty, w);
								if (points[tpix]&LOW_PT) {
									float	d = GET_DISTANCE(x, y, tx, ty);
									ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//									float	d = ARP_DISTANCE(x, y, tx, ty);
									if (d <= mW && (lowDist < 0 || d < lowDist)) lowDist = d;
								} else if (points[tpix]&HIGH_PT) {
									float	d = GET_DISTANCE(x, y, tx, ty);
									ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//									float	d = ARP_DISTANCE(x, y, tx, ty);
									if (d <= mW && (highDist < 0 || d < highDist)) highDist = d;
								}
							}
						}
					}
					diamond++;
					if (lowDist >= 0 && highDist >= 0) break;
				}
				/* SQUARE
				 */
				diamond--;
				int32			square = 1;
				while (square <= diamond) {
					for (int32 row = -diamond; row <= diamond; row++) {
						int32	col = diamond - ARP_ABS(row) + square;
						if (col <= mW + 1) {
							ArpASSERT(col != 0);
							int32	tx = x - col, ty = y + row;
							/* Left side of square.
							 */
							if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
								int32	tpix = ARP_PIXEL(tx, ty, w);
								if (points[tpix]&LOW_PT) {
									float	d = GET_DISTANCE(x, y, tx, ty);
									ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//									float	d = ARP_DISTANCE(x, y, tx, ty);
									if (d <= mW && (lowDist < 0 || d < lowDist)) lowDist = d;
								} else if (points[tpix]&HIGH_PT) {
									float	d = GET_DISTANCE(x, y, tx, ty);
									ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//									float	d = ARP_DISTANCE(x, y, tx, ty);
									if (d <= mW && (highDist < 0 || d < highDist)) highDist = d;
								}
							}
							/* Right side of square.
							 */
							tx = x + col, ty = y + row;
							if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
								int32	tpix = ARP_PIXEL(tx, ty, w);
								if (points[tpix]&LOW_PT) {
									float	d = GET_DISTANCE(x, y, tx, ty);
									ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//									float	d = ARP_DISTANCE(x, y, tx, ty);
									if (d <= mW && (lowDist < 0 || d < lowDist)) lowDist = d;
								} else if (points[tpix]&HIGH_PT) {
									float	d = GET_DISTANCE(x, y, tx, ty);
									ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
//									float	d = ARP_DISTANCE(x, y, tx, ty);
									if (d <= mW && (highDist < 0 || d < highDist)) highDist = d;
								}
							}
						}
					}
					square++;
				}
				/* HACK!  Every ON_PT should be within distance of a low and
				 * a high, but right now certain pts can't find the high.  So
				 * munge this for now.
				 */
				if (lowDist >= 0 && highDist < 0) highDist = float(mW);
				
				if (lowDist >= 0 && highDist >= 0) {
					float	r = lowDist + highDist;
					dest[pix] = arp_clip_255((lowDist + 1) * (mMaxZ / (r + 1)));
				}
			}
		}
	}
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * Low Quality Algorithm.  Very simple distance method -- the closer to
 * the edge, the more shaded the pixel.
 ***************************************************************************/
static const uint8		_PEAK_F = 0x1;
static const uint8		_DIRTY_F = 0x2;

status_t GlBevelSurface::TestQuality(uint8* src, uint8* dest, int32 w, int32 h)
{
	status_t		err = LowQuality(src, dest, w, h);
	if (err != B_OK) return err;

	uint8*			flags = new uint8[w * h];
	if (!flags) return B_NO_MEMORY;

	memset(flags, 0, sizeof(uint8) * (w * h));
	
	err = Accurate1(dest, flags, w, h);
	if (err == B_OK) err = Accurate2(dest, flags, w, h);
	if (err == B_OK) err = Accurate3(dest, flags, w, h);

#if 0
	if (err == B_OK) {
		int32				pix, c = 0;
		for (pix = 0; pix < w * h; pix++) {
			if (flags[pix]&_DIRTY_F) {
				c++;
//				dest[pix] = 255;
				dest[pix] = 0;
			}
		}
		printf("There were %ld peaks\n", c);
	}
#endif
#if 0
//	float				s = 255.0 / high;
	float				s = 215.0 / high;
	for (pix = 0; pix < w * h; pix++) {
		if (cache[pix].count > 0) dest[pix] = 255;
		else dest[pix] = arp_clip_255(dest[pix] * s);
	}
#endif	

	delete[] flags;
	return err;
}

/* Step 1 -- mark the peaks for every point who's highest value
 * within range isn't the max value.
 */
status_t GlBevelSurface::Accurate1(uint8* dest, uint8* flags, int32 w, int32 h)
{
	int32								s = (mW * 2) + 1;
	int32*								highs = new int32[s * s];
	if (!highs) return B_NO_MEMORY;

	int32								x, y, pix = 0;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if (dest[pix] > MIN_Z) {
				memset(highs, 0, sizeof(int32) * (s * s));
				uint8					dv = 0;
				int32					cpix = 0;
				bool					more = true;
				for (int32 row = -mW; more && row <= mW; row++) {
					for (int32 col = -mW; more && col <= mW; col++) {
						ArpASSERT(cpix < s * s);
						int32			tx = x + col, ty = y + row;
						if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
							int32	tpix = ARP_PIXEL(tx, ty, w);
							if (dest[tpix] > MIN_Z && dest[tpix] >= dv) {
								float	d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
								if (d < mW) {
									/* Halt as soon as I find the max value.  If
									 * I have one, then I don't count.
									 */
									if (dest[tpix] == 255) more = false;
									else {
										if (dest[tpix] > dv) {
											memset(highs, 0, sizeof(int32) * (s * s));
											dv = dest[tpix];
										}
										ArpASSERT(dest[tpix] >= dv);
										highs[cpix] = tpix;
									}
								}
							}
						}
						cpix++;
					}
				}
				/* Umm... this means the top left pixel could never be a high
				 * one.  In order for that to happen, the init for highs would
				 * have to be -1, but then I'd lose the memset, right?  I'd have
				 * to iterate.
				 */
				if (more) {
					for (cpix = 0; cpix < s * s; cpix++) {
						if (highs[cpix] > 0) {
							ArpASSERT(highs[cpix] < w * h);
							flags[highs[cpix]] |= _PEAK_F;
							/* Also mark me, as the setter of this flag -- this
							 * indicates I need to be recomputed.
							 */
							flags[pix] |= _DIRTY_F;
						}
					}
				}
			}
			pix++;
		}
	}
	delete[] highs;
	return B_OK;
}

/* Step 2 - find more peaks.  This time, all the values that are between
 * the highest and lowest and are >= everyone around them.
 */
status_t GlBevelSurface::Accurate2(uint8* dest, uint8* flags, int32 w, int32 h)
{
	int32			x, y, pix;
	for (y = 1; y < h - 1; y++) {
		pix = (y * w) + 1;
		ArpASSERT(y == ARP_Y_FROM_PIXEL(pix, w));
		for (x = 1; x < w - 1; x++) {
			ArpASSERT(x == ARP_X_FROM_PIXEL(pix, w));
			uint8		v = dest[pix];
#if 0
//if (x == 410 && y == 22) {
if (x == 401 && y == 57) {
	printf("(%ld, %ld) pix %ld count %d (w %ld h %ld)\n", x, y, pix, cache[pix].count, w, h);
//int32		p = pix - w - 1;
int32		p = pix - w;
	printf("pix - w - 1 is (%ld, %ld)\n", ARP_X_FROM_PIXEL(p, w), ARP_Y_FROM_PIXEL(p, w));
	printf("\t%d\t%d\t%d\n\t%d\t%d\t%d\n\t%d\t%d\t%d\n",
			dest[pix - w - 1],	dest[pix - w],	dest[pix - w + 1],
			dest[pix - 1],		dest[pix],		dest[pix + 1],
			dest[pix + w - 1],	dest[pix + w],	dest[pix + w + 1]);

}
#endif

			if (v > MIN_Z && v < 255 && !(flags[pix]&_PEAK_F)) {
				if (v >= dest[pix - w - 1] && v >= dest[pix - w] && v >= dest[pix - w + 1]
						&& v >= dest[pix - 1] && v >= dest[pix + 1]
						&& v >= dest[pix + w - 1] && v >= dest[pix + w] && v >= dest[pix + w + 1])
					flags[pix] |= _PEAK_F;
			}
			pix++;
		}
	}
	return B_OK;
}

/* Recompute all the dirty pixels by finding the nearest peak and trough,
 * the distance to each, and their new scaled value.
 */
status_t GlBevelSurface::Accurate3(uint8* dest, uint8* flags, int32 w, int32 h)
{
	int32								x, y, pix = 0;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if (dest[pix] > MIN_Z && dest[pix] < 255 && (!(flags[pix]&_PEAK_F))) {
//					&& flags[pix]&_DIRTY_F && (!(flags[pix]&_PEAK_F))) {
				float					peakD = -1, troughD = -1;
				bool					more = true;
				bool					peak = false;
				for (int32 row = -mW; more && row <= mW; row++) {
					for (int32 col = -mW; more && col <= mW; col++) {
						int32			tx = x + col, ty = y + row;
						if (!(tx == x && ty == y) && tx >= 0 && tx < w && ty >= 0 && ty < h) {
							int32		tpix = ARP_PIXEL(tx, ty, w);
							if (flags[tpix]&_PEAK_F || dest[tpix] <= MIN_Z) {
								if (flags[tpix]&_PEAK_F) peak = true;
								float		d = GET_DISTANCE(x, y, tx, ty);
								ArpASSERT(ARP_ABS(d - ARP_DISTANCE(x, y, tx, ty)) < 0.0001);
								if (d < mW) {
									if (peakD < 0 || d < peakD) {
										if (dest[tpix] == 255 || flags[tpix]&_PEAK_F) peakD = d;
									}
									if (troughD < 0 || d < troughD) {
										if (dest[tpix] <= MIN_Z) troughD = d;
									}
									if (peakD <= 1 && troughD <= 1) more = false;
								}
							}
						}
					}
				}
				if (peak && peakD >= 0 && troughD >= 0) {
					uint8		v = arp_clip_255((troughD / (peakD + troughD)) * 255.0);
					if (v > dest[pix]) dest[pix] = v;
				}
			}
			if (flags[pix]&_PEAK_F) dest[pix] = 255;
			pix++;
		}
	}
	return B_OK;
}