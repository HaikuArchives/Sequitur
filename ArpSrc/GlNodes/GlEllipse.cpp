#include <cstdio>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlEllipse.h>

static const int32		_X_KEY		= 'xkey';
static const int32		_Y_KEY		= 'ykey';
static const int32		_Z_KEY		= 'zkey';

static const uint32		_X_INDEX	= 0;
static const uint32		_Y_INDEX	= 1;
static const uint32		_Z_INDEX	= 2;

static const float		DEF_REL_X	= 0.5f;
static const float		DEF_ABS_X	= 0;
static const float		DEF_REL_Y	= 0.5f;
static const float		DEF_ABS_Y	= 0;
static const float		DEF_REL_W	= 1;
static const float		DEF_ABS_W	= 0;
static const float		DEF_REL_H	= 1;
static const float		DEF_ABS_H	= 0;

/***************************************************************************
 * _GL-ELLIPSE-ALGO
 ***************************************************************************/
class _GlEllipseAlgo : public GlAlgo2d
{
public:
	_GlEllipseAlgo(	gl_node_id nid, uint32 targets, BPoint ptX, BPoint ptY,
					BPoint ptW, BPoint ptH, GlAlgo* xC, GlAlgo* yC, GlAlgo* zC);
	_GlEllipseAlgo(const _GlEllipseAlgo& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
								GlProcessStatus* status = 0);

private:
	typedef GlAlgo2d	inherited;
	float				mX, mY, mW, mH;		// A cache
	BPoint				mPtX, mPtY, mPtW, mPtH;

	void				Ellipse(uint8* mask, int32 w, int32 h, GlAlgo1d* za);
	void				Circle(	uint8* mask, int32 w, int32 h, int32 l,
								int32 t, int32 r, int32 b, GlAlgo1d* za);
	void				Shape(	uint8* mask, int32 w, int32 h, GlAlgo1d* xa,
								GlAlgo1d* ya, GlAlgo1d* za);

	status_t			CacheShapeFrame(int32 w, int32 h, int32* outL, int32* outT,
										int32* outR, int32* outB);
};

/***************************************************************************
  * GL-ELLIPSE-2D
 ***************************************************************************/
GlEllipse::GlEllipse(const GlEllipseAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	VerifyChain(new GlChain(_X_KEY, GL_1D_IO, SZ(SZ_X), this));
	VerifyChain(new GlChain(_Y_KEY, GL_1D_IO, SZ(SZ_Y), this));
	VerifyChain(new GlChain(_Z_KEY, GL_1D_IO, SZ(SZ_Z), this));
}

GlEllipse::GlEllipse(const GlEllipse& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlEllipse::Clone() const
{
	return new GlEllipse(*this);
}

GlAlgo* GlEllipse::Generate(const gl_generate_args& args) const
{
	BPoint				x, y, w, h;
	GlAlgo				*xC = 0, *yC = 0, *zC = 0;
	GetChainParams(args, x, y, w, h, &xC, &yC, &zC);
	return new _GlEllipseAlgo(Id(), PixelTargets(), x, y, w, h, xC, yC, zC);
}

status_t GlEllipse::GetChainParams(	const gl_generate_args& args,
									BPoint& ptX, BPoint& ptY,
									BPoint& ptW, BPoint& ptH,
									GlAlgo** outAX, GlAlgo** outAY, GlAlgo** outAZ) const
{
	ptX.Set(DEF_REL_X, DEF_ABS_X);
	ptY.Set(DEF_REL_Y, DEF_ABS_Y);
	ptW.Set(DEF_REL_W, DEF_ABS_W);
	ptH.Set(DEF_REL_H, DEF_ABS_H);
	ptX = Params().Point('x_pt');
	ptY = Params().Point('y_pt');
	ptW = Params().Point('w_pt');
	ptH = Params().Point('h_pt');
	ArpVALIDATE(outAX && outAY && outAZ, return B_ERROR);

	*outAX = GenerateChainAlgo(_X_KEY, args);
	*outAY = GenerateChainAlgo(_Y_KEY, args);
	*outAZ = GenerateChainAlgo(_Z_KEY, args);
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-ELLIPSE-2D-ADD-ON
 ***************************************************************************/
GlEllipseAddOn::GlEllipseAddOn()
		: inherited(SZI[SZI_arp], GL_ELLIPSE_KEY, SZ(SZ_Shapes), SZ(SZ_Ellipse), 1, 0)
{
	mX	= AddParamType(new GlPointParamType('x_pt', SZ(SZ_X), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1000), BPoint(10, 1000), BPoint(DEF_REL_X, DEF_ABS_X), 0.1f));
	mY	= AddParamType(new GlPointParamType('y_pt', SZ(SZ_Y), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1000), BPoint(10, 1000), BPoint(DEF_REL_Y, DEF_ABS_Y), 0.1f));
	mW	= AddParamType(new GlPointParamType('w_pt', SZ(SZ_W), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1000), BPoint(10, 1000), BPoint(DEF_REL_W, DEF_ABS_W), 0.1f));
	mH	= AddParamType(new GlPointParamType('h_pt', SZ(SZ_H), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1000), BPoint(10, 1000), BPoint(DEF_REL_H, DEF_ABS_H), 0.1f));
}

GlNode* GlEllipseAddOn::NewInstance(const BMessage* config) const
{
	return new GlEllipse(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-ELLIPSE-ALGO
 ***************************************************************************/
_GlEllipseAlgo::_GlEllipseAlgo(	gl_node_id nid, uint32 targets, BPoint ptX,
								BPoint ptY, BPoint ptW, BPoint ptH,
								GlAlgo* xC, GlAlgo* yC, GlAlgo* zC)
		: inherited(nid, targets), mPtX(ptX), mPtY(ptY), mPtW(ptW), mPtH(ptH)
{
	SetChain(xC, _X_INDEX);
	SetChain(yC, _Y_INDEX);
	SetChain(zC, _Z_INDEX);
}

_GlEllipseAlgo::_GlEllipseAlgo(const _GlEllipseAlgo& o)
		: inherited(o), mPtX(o.mPtX), mPtY(o.mPtY), mPtW(o.mPtW), mPtH(o.mPtH)
{
}

GlAlgo* _GlEllipseAlgo::Clone() const
{
	return new _GlEllipseAlgo(*this);
}

status_t _GlEllipseAlgo::Process(	const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
									GlProcessStatus* status)
{
	if (!mask || w < 1 || h < 1) return B_OK;
	int32				l, t, r, b;
	if (CacheShapeFrame(w - 1, h - 1, &l, &t, &r, &b) != B_OK) return B_OK;
//printf("(%f, %f) w %f h %f\n", mX, mY, mW, mH);

	GlAlgo1d*		xa = Algo1dAt(_X_INDEX);
	GlAlgo1d*		ya = Algo1dAt(_Y_INDEX);
	GlAlgo1d*		za = Algo1dAt(_Z_INDEX);
	/* If I have no x or y objects, then my default object is an ellipse.
	 * I figure circles will probably be the most common thing, so it's nice
	 * to optimize drawing them.
	 */
	if (!xa && !ya) {
		if (r - l == b - t) Circle(mask, w, h, l, t, r, b, za);
		else Ellipse(mask, w, h, za);
	} else Shape(mask, w, h, xa, ya, za);
	return B_OK;
}

/* The function f(x,y) = b^2x^2 + a^2y^2 - a^2b^2 determines an ellipse in the
 * following way:
 *		 all points x, y for which f(x, y) > 0 lie outside the ellipse
 *		 all points x, y for which f(x, y) = 0 lie on the ellipse
 *		 all points x, y for which f(x, y) < 0 lie inside the ellipse
 * FIX:  This can obviously be optimized, only need to perform calculation
 * for pixels within the ellipse bounds, and only need to calculate one
 * quarter of the ellipse and then mirror the rest.
 */
void _GlEllipseAlgo::Ellipse(uint8* mask, int32 w, int32 h, GlAlgo1d* za)
{
	int32			cenX = ARP_ROUND(mX), cenY = ARP_ROUND(mY),
					radX = ARP_ROUND(mW), radY = ARP_ROUND(mH);
	float			a = radX / 2.0f, b = radY / 2.0f;
	float			a2 = a * a, b2 = b * b, a2b2 = (a * a) * (b * b);
	int32			pix = 0;
//printf("Ellipse\n");
	GlCache1d*		cache = 0;

	if (za) cache = za->NewCache(ARP_MAX(radX, radY) * 2, 360);

	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			float				f = (b2 * ARP_SQR(x - cenX)) + (a2 * ARP_SQR(y - cenY)) - a2b2;
			if (f > 0) mask[pix] = 0;
			else {
				float			curve = f / -a2b2;
				float			v = (curve + (1 - pow(1 - curve, 0.35f))) / 2;
				if (cache) {
					if (cache->h > 1) {
						float	deg = arp_degree(float(x - cenX), float(cenY - y)) / 359.0f;
						if (deg < 0) deg = 0; else if (deg > 1) deg = 1;
						v = cache->At(1 - v, deg);
					} else v = cache->At(1 - v);
				}
				uint8			uv = arp_clip_255(v * 255);
				mask[pix] = ARP_MIN(mask[pix], uv);
			}
			pix++;
		}
	}
	delete cache;
}

/* FIX:  This can obviously be optimized, only need to perform a quarter
 * of the circle and mirror it.
 */
void _GlEllipseAlgo::Circle(uint8* mask, int32 w, int32 h, int32 l, int32 t,
							int32 r, int32 b, GlAlgo1d* za)
{
//printf("Circle\n");
	int32		rad = (r - l) / 2, pix = 0;
	int32		cenX = l + rad, cenY = t + rad;
	r = cenX + rad;
	b = cenY + rad;
//printf("Circle w %ld h %ld cen (%ld, %ld) rad (%ld) l %ld t %ld r %ld b %ld\n",
//		w, h, cenX, cenY, rad, l, t, r, b);	
	GlCache1d*		cache = 0;
	if (za) cache = za->NewCache(rad * 2, 360);

	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			if (mask[pix] > 0) {
				if (x >= l && y >= t && x <= r && y <= b) {
					/* Algo
					 */
					float	v;
					if (rad <= 0) v = 0;
					else v = ARP_DISTANCE(cenX, cenY, x, y) / rad;
					if (rad == 0 || (v >= 0 && v <= 1)) {
						uint8		val;
						if (cache) {
							if (cache->h > 1) {
								float	deg = arp_degree(float(x - cenX), float(cenY - y)) / 359.0f;
								if (deg < 0) deg = 0; else if (deg > 1) deg = 1;
								val = arp_clip_255(cache->At(v, deg) * 255);
							} else val = arp_clip_255(cache->At(v) * 255);
						} else val = arp_clip_255((1 - v) * 255);
//						if (za) val = arp_clip_255(za->At(v) * 255);
//						else val = arp_clip_255((1 - v) * 255);
						mask[pix] = ARP_MIN(mask[pix], val);
					} else mask[pix] = 0;
					/* end
					 */
				} else mask[pix] = 0;
			}
			pix++;
		}
	}
}

static inline uint8 get_rect_shaded_val(int32 x, int32 y, int32 cenX, int32 cenY,
										int32 l, int32 t, int32 r, int32 b,
										const GlAlgo1d* zAlgo)
{
	float		size = float((r - l <= b - t) ? (r - cenX) : (b - cenY));
	int32		distXEdge, distYEdge;
	if (x > cenX) distXEdge = r - x;
	else distXEdge = x - l;
	if (y > cenY) distYEdge = b - y;
	else distYEdge = y - t;
	float		v;
	if (distXEdge <= distYEdge) v = distXEdge / size;
	else v = distYEdge / size;
	if (v > 1) v = 1;
ArpASSERT(false);
// FIX: Replace thr zAlgo with a cache, not quite sure how it should work
//	if (zAlgo) v = zAlgo->At(1 - v);
	return arp_clip_255(v * 255);
}

void _GlEllipseAlgo::Shape(uint8* mask, int32 w, int32 h, GlAlgo1d* xa, GlAlgo1d* ya, GlAlgo1d* za)
{
	int32				cenX = ARP_ROUND(mX), cenY = ARP_ROUND(mY),
						halfW = ARP_ROUND(mW * 0.5), halfH = ARP_ROUND(mH * 0.5);
	int32				l = cenX - halfW, t = cenY - halfH, r = cenX + halfW, b = cenY + halfH;
	int32				x, y, yOffset, xOffset, pix;
	ArpASSERT(r - l == halfW + halfW);
	int32				xCacheSize = (r - l) + 1, yCacheSize = (b - t) + 1;

	/* Initialize all the caches
	 */
	if (xa) xa->SetStep(0.0);
	GlCache1d*			cacheX = (xa) ? xa->NewCache(xCacheSize, 0) : 0;
	GlCache1d*			cacheY = (ya) ? ya->NewCache(yCacheSize, xCacheSize) : 0;
	if ( (xa && !cacheX) || (ya && !cacheY) ) {
		delete cacheX;	delete cacheY;
		return;
	}

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (mask[pix] > 0) {
				/* Find if this pixel is on -- to be on the x and y curves have to intersect
				 */
				uint8		val = 0;
				if (x >= l && x <= r && (r - l > 0) && y >= t && y <= b && (b - t > 0)) {
					/* Get the two offset values -- the range on the X and Y that I need
					 * to be between at these points on the axes to be valid.
					 */
					if (cacheX) xOffset = int32(cacheX->n[x - 1] * halfH);
					else xOffset = 0;

					if (cacheY && cacheY->h == 1) yOffset = int32(cacheY->n[y - t] * halfW);
					else if (cacheY && cacheY->h > 1) yOffset = int32(cacheY->n[ARP_PIXEL(y - t, x - l, yCacheSize)] * halfW);
					else yOffset = 0;

					/* See if I'm between the range of my current algo(s).
					 */
					if (xa && ya) {
						if (y >= cenY - xOffset && y <= cenY + xOffset && x >= cenX - yOffset && x <= cenX + yOffset)
							val = 255; 
					} else if (xa) {
						if (y >= cenY - xOffset && y <= cenY + xOffset)
							val = 255;
					} else if (x >= cenX - yOffset && x <= cenX + yOffset)
						val = 255;
				}
				if (val > 0) val = get_rect_shaded_val(x, y, cenX, cenY, l, t, r, b, za);
				mask[pix] = ARP_MIN(mask[pix], val);
			}
		}
		if (cacheX) cacheX->SetStep(xa, y / float(h - 1));
	}
	delete cacheX;
	delete cacheY;
}

status_t _GlEllipseAlgo::CacheShapeFrame(	int32 w, int32 h, int32* outL, int32* outT,
											int32* outR, int32* outB)
{
	ArpASSERT(outL && outT && outR && outB);
//printf("Cache shape from x (%f, %f) y (%f, %f) w (%f, %f) h (%f, %f)\n",
//		mPtX.x, mPtX.y, mPtY.x, mPtY.y, mPtW.x, mPtW.y, mPtH.x, mPtH.y);
	mX = (w * mPtX.x) + mPtX.y;
	mY = (h * mPtY.x) + mPtY.y;
	mW = (w * mPtW.x) + mPtW.y;
	mH = (h * mPtH.x) + mPtH.y;
//printf("\tx %f y %f w %f h %f\n", mX, mY, mW, mH);
	float		halfW = mW * 0.5f, halfH = mH * 0.5f;
	*outL = ARP_ROUND(mX - halfW);
	*outT = ARP_ROUND(mY - halfH);
	*outR = ARP_ROUND(mX + halfW);
	*outB = ARP_ROUND(mY + halfH);

//printf("\tl %ld t %ld r %ld b %ld\n", *outL, *outT, *outR, *outB);

	return B_OK;
}