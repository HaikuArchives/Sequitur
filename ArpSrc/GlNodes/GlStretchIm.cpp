#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlStretchIm.h>

static const uint32		_HRZ_INDEX			= 0;
static const uint32		_VRT_INDEX			= 1;

struct _StretchedCell
{
	int32		destFrom, destTo;
	int32		srcFrom, srcTo;
};

static _StretchedCell*	_make_cells(float from, float to, int32 size,
									GlAlgo1dWrap* wrap, float* cache);

static uint32			_size(_StretchedCell* cells, uint32 count);
static int32			_count(_StretchedCell* cells, uint32 count);

static status_t			_stretch_x(	GlPlanes* src, GlPlanes* dest, _StretchedCell* xCells);
static status_t			_stretch_y(	GlPlanes* src, GlPlanes* dest, _StretchedCell* yCells);

/***************************************************************************
 * _GL-STRETCH-ALGO
 ***************************************************************************/
class _GlStretchAlgo : public GlAlgoIm
{
public:
	_GlStretchAlgo(	gl_node_id nid, const BPoint& minPt, const BPoint& maxPt,
					GlAlgo* hrz, GlAlgo* vrt);
	_GlStretchAlgo(const _GlStretchAlgo& o);

	virtual GlAlgo*		Clone() const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

	status_t			Stretch(GlImage* dest, BPoint minPt, BPoint maxPt,
								GlAlgo1dWrap* xCurve, GlAlgo1dWrap* yCurve) const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
	BPoint				mMinPt, mMaxPt;
};

/***************************************************************************
 * GL-STRETCH-IM
 ***************************************************************************/
GlStretchIm::GlStretchIm(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	VerifyChain(new GlChain(GL_HRZ_KEY, GL_1D_IO, SZ(SZ_Horizontal), this));
	VerifyChain(new GlChain(GL_VRT_KEY, GL_1D_IO, SZ(SZ_Vertical), this));
}

GlStretchIm::GlStretchIm(const GlStretchIm& o)
		: inherited(o)
{
}

GlNode* GlStretchIm::Clone() const
{
	return new GlStretchIm(*this);
}

GlAlgo* GlStretchIm::Generate(const gl_generate_args& args) const
{
	BPoint			hrzPt = Params().Point(GL_HRZ_KEY),
					vrtPt = Params().Point(GL_VRT_KEY);
	if (args.flags&GL_NODE_ICON_F) hrzPt = BPoint(0.5, 1.5);
	BPoint			minPt(hrzPt.x, vrtPt.x),
					maxPt(hrzPt.y, vrtPt.y);

	GlAlgo*			hrz = GenerateChainAlgo(GL_HRZ_KEY, args);
	GlAlgo*			vrt = GenerateChainAlgo(GL_VRT_KEY, args);

	return new _GlStretchAlgo(Id(), minPt, maxPt, hrz, vrt);
}

// #pragma mark -

/***************************************************************************
 * GL-STRETCH-IM-ADD-ON
 ***************************************************************************/
GlStretchImAddOn::GlStretchImAddOn()
		: inherited(SZI[SZI_arp], GL_STRETCH_KEY, SZ(SZ_Images), SZ(SZ_Stretch), 1, 0)
{
	AddParamType(new GlPointParamType(GL_HRZ_KEY, SZ(SZ_Horizontal), SZ(SZ_from), SZ(SZ_to), BPoint(0, 0), BPoint(128, 128), BPoint(1, 1), 0.1f));
	AddParamType(new GlPointParamType(GL_VRT_KEY, SZ(SZ_Vertical), SZ(SZ_from), SZ(SZ_to), BPoint(0, 0), BPoint(128, 128), BPoint(1, 1), 0.1f));
}

GlNode* GlStretchImAddOn::NewInstance(const BMessage* config) const
{
	return new GlStretchIm(this, config);
}

GlAlgo* GlStretchImAddOn::Generate(const gl_generate_args& args) const
{
	return new _GlStretchAlgo(0, BPoint(1, 1), BPoint(1, 1), 0, 0);
}

// #pragma mark -

/***************************************************************************
 * _GL-STRETCH-ALGO
 ***************************************************************************/
_GlStretchAlgo::_GlStretchAlgo(	gl_node_id nid, const BPoint& minPt,
								const BPoint& maxPt, GlAlgo* hrz,
								GlAlgo* vrt)
		: inherited(nid), mMinPt(minPt), mMaxPt(maxPt)
{
	if (hrz) SetChain(hrz, _HRZ_INDEX);
	if (vrt) SetChain(vrt, _VRT_INDEX);
}

_GlStretchAlgo::_GlStretchAlgo(const _GlStretchAlgo& o)
		: inherited(o), mMinPt(o.mMinPt), mMaxPt(o.mMaxPt)
{
}

GlAlgo* _GlStretchAlgo::Clone() const
{
	return new _GlStretchAlgo(*this);
}

status_t _GlStretchAlgo::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_POINT_TYPE) return B_ERROR;
	
	if (key.key == GL_HRZ_KEY) {
		mMinPt.x = ((const GlPointWrap&)wrap).v.x;
		mMaxPt.x = ((const GlPointWrap&)wrap).v.y;
	} else if (key.key == GL_VRT_KEY) {
		mMinPt.y = ((const GlPointWrap&)wrap).v.x;
		mMaxPt.y = ((const GlPointWrap&)wrap).v.y;
	} else return B_ERROR;
	return B_OK;
}

status_t _GlStretchAlgo::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_POINT_TYPE) return B_ERROR;

	if (key.key == GL_HRZ_KEY) {
		((GlPointWrap&)wrap).v.x = mMinPt.x;
		((GlPointWrap&)wrap).v.y = mMaxPt.x;
	} else if (key.key == GL_VRT_KEY) {
		((GlPointWrap&)wrap).v.x = mMinPt.y;
		((GlPointWrap&)wrap).v.y = mMaxPt.y;
	} else return B_ERROR;
	return B_OK;
}

status_t _GlStretchAlgo::Perform(	GlNodeDataList& list,
									const gl_process_args* args)
{
	GlAlgo1dWrap	hrz(AlgoAt(_HRZ_INDEX)),
					vrt(AlgoAt(_VRT_INDEX));

	GlImage*		im;
	GlAlgo1dWrap*	phrz = (hrz.InitCheck() == B_OK) ? &hrz : 0;
	GlAlgo1dWrap*	pvrt = (vrt.InitCheck() == B_OK) ? &vrt : 0;
	
	for (uint32 k = 0; (im = list.ImageAt(k)) != 0; k++)
		Stretch(im, mMinPt, mMaxPt, phrz, pvrt);
	return B_OK;
}

status_t _GlStretchAlgo::Stretch(	GlImage* dest, BPoint minPt, BPoint maxPt,
									GlAlgo1dWrap* xCurve, GlAlgo1dWrap* yCurve) const
{
	ArpVALIDATE(dest, return B_ERROR);
	int32					srcW = dest->Width(), srcH = dest->Height();
	ArpVALIDATE(srcW > 0 && srcH > 0, return B_ERROR);
	float*					cache = new float[(srcW > srcH) ? srcW : srcH];
	if (!cache) return B_NO_MEMORY;
	_StretchedCell*			xCells = _make_cells(minPt.x, maxPt.x, srcW, xCurve, cache);
	if (!xCells) {
		delete[] cache;
		return B_NO_MEMORY;
	}
	_StretchedCell*			yCells = _make_cells(minPt.y, maxPt.y, srcH, yCurve, cache);
	if (!yCells) {
		delete[] xCells;
		delete[] cache;
		return B_NO_MEMORY;
	}
	delete[] cache;
#if 0
printf("%ld X cells\n", srcW);
for (int32 k = 0; k < srcW; k++) {
	printf("%ld: dest (%ld - %ld) src (%ld - %ld) (v %f)\n", k,
			xCells[k].destFrom, xCells[k].destTo, xCells[k].srcFrom, xCells[k].srcTo, 0);
//			xCells[k].v);
}
#endif
#if 0
printf("%ld Y cells\n", srcH);
for (int32 k = 0; k < srcH; k++) {
	printf("%ld: dest (%ld - %ld) src (%ld - %ld) (v %f)\n", k,
			yCells[k].destFrom, yCells[k].destTo, yCells[k].srcFrom, yCells[k].srcTo, 0);
//			yCells[k].v);
}
#endif

	int32					destW = _size(xCells, srcW), destH = _size(yCells, srcH);
	if (destW > 0 && destH > 0) {
		/* Stretch the x.
		 */
		GlImage*			src = dest->SourceClone(destW, srcH);
		if (src) {
			GlPlanes*		srcPixels = src->LockPixels(GL_PIXEL_ALL);
			GlPlanes*		destPixels = dest->LockPixels(GL_PIXEL_ALL);
			if (srcPixels && destPixels) _stretch_x(srcPixels, destPixels, xCells);
			src->UnlockPixels(srcPixels);
			dest->UnlockPixels(destPixels);
			delete src;
		}
		/* Stretch the y.
		 */
		src = dest->SourceClone(destW, destH);
		if (src) {
			GlPlanes*		srcPixels = src->LockPixels(GL_PIXEL_ALL);
			GlPlanes*		destPixels = dest->LockPixels(GL_PIXEL_ALL);
			if (srcPixels && destPixels) _stretch_y(srcPixels, destPixels, yCells);
			src->UnlockPixels(srcPixels);
			dest->UnlockPixels(destPixels);
			delete src;
		}
	}
	delete[] xCells;
	delete[] yCells;
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * Misc - initialization
 ***************************************************************************/
static _StretchedCell* _make_cells(	float from, float to, int32 size,
									GlAlgo1dWrap* wrap, float* cache)
{
	ArpVALIDATE(size > 0, return 0);
	_StretchedCell*		cells = new _StretchedCell[size];
	if (!cells) return 0;
	for (int32 c = 0; c < size; c++)
		cells[c].destFrom = cells[c].destTo = cells[c].srcFrom = cells[c].srcTo = -1;
	float				destCell = 0;
	int32				cell = 0;
	if (wrap) wrap->Process(cache, 0, size);
	for (float srcCell = 0; srcCell < size; srcCell++) {
		if (cell >= size) return cells;
		float			step = float(GL_1D_ENV_STEP(srcCell, size)),
						v = 1;
		if (wrap) v = cache[int32(srcCell)];
		else v = step;
		v = from + ((to - from) * v);
		/* v is the stretch ratio -- how many pixels from the source get placed
		 * into a single pixel in the dest.  So at 1, there's a 1 to 1 ratio.  Values
		 * less than one mean multiple source pixels are placed into the same pixel
		 * in the dest -- i.e. minimizing.
		 */
		if (v <= 0) ; //srcCell++;
		else if (v < 1) {
			cells[cell].destFrom = cells[cell].destTo = int32(destCell);
			cells[cell].srcFrom = cells[cell].srcTo = int32(srcCell);
			float		wall = floor(destCell) + 1;
			destCell += v;
			while (destCell < wall) {
				cells[cell].srcTo++;
				if (cells[cell].srcTo >= size) {
					cells[cell].srcTo = size -1;
					return cells;
				}
				destCell += v;
				srcCell++;
				if (srcCell >= size) return cells;
			}
//cells[cell].v = v;
			cell++;
		} else if (v > 1) {
			cells[cell].destFrom = int32(floor(destCell));
			cells[cell].destTo = int32(ceil(cells[cell].destFrom + v) - 1);
			cells[cell].srcFrom = cells[cell].srcTo = int32(srcCell);
			destCell += v;
//cells[cell].v = v;
			cell++;
		} else {
			cells[cell].destFrom = cells[cell].destTo = int32(destCell);
			cells[cell].srcFrom = cells[cell].srcTo = int32(srcCell);
			destCell++;
//cells[cell].v = v;
			cell++;
		}
	}
	return cells;
}

// #pragma mark -

/***************************************************************************
 * Misc - sizing
 ***************************************************************************/
static uint32 _size(_StretchedCell* cells, uint32 count)
{
	if (count < 1) return 0;
	if (count == 1) return uint32(cells[0].destTo + 1);
	for (int32 k = int32(count - 1); k >= 0; k--) {
		if (cells[k].destTo >= 0) return uint32(cells[k].destTo + 1);
	}
	return 0;
}

static int32 _count(_StretchedCell* cells, uint32 count)
{
	uint32		newCount = 0;
	for (uint32 k = 0; k < count; k++) {
		if (cells[k].srcFrom >= 0) newCount++;
		else return newCount;
	}
	return newCount;
}

// #pragma mark -

/***************************************************************************
 * Misc - stretching
 *
 * _flush_x/y:  If there's a nextSum then it's interpolating to a value,
 * otherwise it's just a straight replacement.  You never have to worry about
 * interpolating on the Average code because Average is only valid for the
 * first pixel I'm writing to, and the first pixel will always be the
 * sum value (whether we're dealing with Average, Replace or Replace interpolate)
 *
 * _stetch_x/y:  Do one of two things:  Either combine multiple source
 * pixels into a single target, or stretch a single source across multiple
 * targets.  If it's stretching the source, it uses simple linear interpolation
 * to smooth things out HOWEVER it doesn't know the pixel it's interpolating
 * to until it's completed the next xCell, so it has to cache.
 ***************************************************************************/
static inline void _flush_x(int32* sum, int32* nextSum, uint32 sumSize,
							_StretchedCell* cells, int32 xCell, int32 y,
							GlPlanes* src, GlPlanes* dest)
{
	uint32			k;
	int32			range = (cells[xCell].destTo - cells[xCell].destFrom) + 1;
	ArpASSERT(cells[xCell].destFrom >= 0 && cells[xCell].destFrom <= cells[xCell].destTo && cells[xCell].destTo < dest->w);
	for (int32 x = cells[xCell].destFrom; x <= cells[xCell].destTo; x++) {
		int32	pix = ARP_PIXEL(x, y, dest->w);
		/* If I overlap with someone that's already been placed, average the
		 * new colour in with the old.
		 */
		if ( (xCell > 0 && x == cells[xCell].destFrom && cells[xCell].destFrom <= cells[xCell - 1].destTo) ) {
			// Average
			for (k = 0; k < src->size; k++) dest->plane[k][pix] = arp_clip_255( (dest->plane[k][pix] + sum[k]) / 2);
		} else {
			// Replace
			if (!nextSum) {
				for (k = 0; k < src->size; k++) dest->plane[k][pix] = arp_clip_255(sum[k]);
			} else {
				float	scale = (x - cells[xCell].destFrom) / float(range);
				for (k = 0; k < src->size; k++) dest->plane[k][pix] = arp_clip_255(sum[k] + ((nextSum[k] - sum[k]) * scale));
			}
		}
	}
}

static status_t _stretch_x(GlPlanes* src, GlPlanes* dest, _StretchedCell* xCells)
{
	ArpVALIDATE(src && dest && xCells, return B_ERROR);
	ArpVALIDATE(src->h == dest->h, return B_ERROR);
	ArpVALIDATE(src->size == dest->size, return B_ERROR);
	ArpVALIDATE(src->size > 0, return B_ERROR);
	int32				xCellSize = _count(xCells, src->w);
	ArpVALIDATE(xCellSize > 0, return B_ERROR);
	int32				x, xCell, pix, h = src->h;
	uint32				k;
	int32*				sums = new int32[src->size];
	int32*				prevSums = new int32[src->size];
	int32				prevCell;
	if (!sums || !prevSums) {
		delete[] sums;
		delete[] prevSums;
		return B_NO_MEMORY;
	}
	for (int32 y = 0; y < h; y++) {
		prevCell = -1;
		for (xCell = 0; xCell < xCellSize; xCell++) {
			/* Get sums for all the source colour pixels.
			 */
			for (k = 0; k < src->size; k++) sums[k] = 0;
			uint32		count = 0;
			ArpASSERT(xCells[xCell].srcFrom >= 0 && xCells[xCell].srcFrom <= xCells[xCell].srcTo && xCells[xCell].srcTo < src->w);
			for (x = xCells[xCell].srcFrom; x <= xCells[xCell].srcTo; x++) {
				pix = ARP_PIXEL(x, y, src->w);
				for (k = 0; k < src->size; k++) sums[k] += src->plane[k][pix];
				count++;
			}
			/* Place the new colour in all the destination pixels
			 */
			if (count > 0) {
				if (count > 1) {
					for (k = 0; k < src->size; k++) sums[k] /= count;
				}
				/* If I've got prevSums, then flush them here.
				 */
				if (prevCell >= 0) {
					_flush_x(prevSums, sums, src->size, xCells, prevCell, y, src, dest);
					prevCell = -1;
				}
				/* If the current sums spread across multiple pixels then cache them
				 * for flushing later, otherwise flush them here.
				 */
				if (xCells[xCell].destFrom == xCells[xCell].destTo)
					_flush_x(sums, 0, src->size, xCells, xCell, y, src, dest);
				else {
					int32*		tmp = sums;
					sums = prevSums;
					prevSums = tmp;
					prevCell = xCell;
				}
			}
		}
		if (prevCell >= 0)
			_flush_x(prevSums, 0, src->size, xCells, prevCell, y, src, dest);
	}
	delete[] sums;
	delete[] prevSums;
	return B_OK;
}

static inline void _flush_y(int32* sum, int32* nextSum, uint32 sumSize,
							_StretchedCell* cells, int32 x, int32 yCell,
							GlPlanes* src, GlPlanes* dest)
{
	uint32			k;
	int32			range = (cells[yCell].destTo - cells[yCell].destFrom) + 1;
	ArpASSERT(cells[yCell].destFrom >= 0 && cells[yCell].destFrom <= cells[yCell].destTo && cells[yCell].destTo < dest->h);
	for (int32 y = cells[yCell].destFrom; y <= cells[yCell].destTo; y++) {
		int32	pix = ARP_PIXEL(x, y, dest->w);
		/* If I overlap with someone that's already been placed, average the
		 * new colour in with the old.
		 */
		if ( (yCell > 0 && y == cells[yCell].destFrom && cells[yCell].destFrom <= cells[yCell - 1].destTo) ) {
			// Average
			for (k = 0; k < src->size; k++) dest->plane[k][pix] = arp_clip_255( (dest->plane[k][pix] + sum[k]) / 2);
		} else {
			// Replace
			if (!nextSum) {
				for (k = 0; k < src->size; k++) dest->plane[k][pix] = arp_clip_255(sum[k]);
			} else {
				float	scale = (y - cells[yCell].destFrom) / float(range);
				for (k = 0; k < src->size; k++) dest->plane[k][pix] = arp_clip_255(sum[k] + ((nextSum[k] - sum[k]) * scale));
			}
		}
	}
}

static status_t _stretch_y(GlPlanes* src, GlPlanes* dest, _StretchedCell* yCells)
{
	ArpVALIDATE(src && dest && yCells, return B_ERROR);
	ArpVALIDATE(src->w == dest->w, return B_ERROR);
	ArpVALIDATE(src->size == dest->size, return B_ERROR);
	ArpVALIDATE(src->size > 0, return B_ERROR);
	int32				yCellSize = _count(yCells, src->h);
	ArpVALIDATE(yCellSize > 0, return B_ERROR);
	int32				y, yCell, pix, w = src->w;
	uint32				k;
	int32*				sums = new int32[src->size];
	int32*				prevSums = new int32[src->size];
	int32				prevCell;
	if (!sums || !prevSums) {
		delete[] sums;
		delete[] prevSums;
		return B_NO_MEMORY;
	}
	for (int32 x = 0; x < w; x++) {
		prevCell = -1;
		for (yCell = 0; yCell < yCellSize; yCell++) {
			/* Get sums for all the source colour pixels.
			 */
			for (k = 0; k < src->size; k++) sums[k] = 0;
			uint32		count = 0;
			ArpASSERT(yCells[yCell].srcFrom >= 0 && yCells[yCell].srcFrom <= yCells[yCell].srcTo && yCells[yCell].srcTo < src->h);
			for (y = yCells[yCell].srcFrom; y <= yCells[yCell].srcTo; y++) {
				pix = ARP_PIXEL(x, y, src->w);
				for (k = 0; k < src->size; k++) sums[k] += src->plane[k][pix];
				count++;
			}
			/* Place the new colour in all the destination pixels
			 */
			if (count > 0) {
				if (count > 1) {
					for (k = 0; k < src->size; k++) sums[k] /= count;
				}
				/* If I've got prevSums, then flush them here.
				 */
				if (prevCell >= 0) {
					_flush_y(prevSums, sums, src->size, yCells, x, prevCell, src, dest);
					prevCell = -1;
				}
				/* If the current sums spread across multiple pixels then cache them
				 * for flushing later, otherwise flush them here.
				 */
				if (yCells[yCell].destFrom == yCells[yCell].destTo)
					_flush_y(sums, 0, src->size, yCells, x, yCell, src, dest);
				else {
					int32*		tmp = sums;
					sums = prevSums;
					prevSums = tmp;
					prevCell = yCell;
				}
			}
		}
		if (prevCell >= 0)
			_flush_y(prevSums, 0, src->size, yCells, x, prevCell, src, dest);
	}
	delete[] sums;
	delete[] prevSums;
	return B_OK;
}
