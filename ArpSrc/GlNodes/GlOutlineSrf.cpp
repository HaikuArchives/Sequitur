#include <cstdio>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlCache2d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlOutlineSrf.h>

static const int32		GL_OUTLINE_KEY		= 'ApOl';
static const int32		_DOT_KEY			= 'dot_';
static const int32		_DOT_INDEX			= 0;

static const int32		_ERASE_BG_KEY		= 'ebg_';
static const bool		_ERASE_BG_INIT		= true;

/***************************************************************************
 * _GL-OUTLINE-SURFACE
 ***************************************************************************/
class _GlOutlineAlgo : public GlAlgo2d
{
public:
	_GlOutlineAlgo(	gl_node_id nid, uint32 targets, float rr, int32 ra,
					bool eraseBg, GlAlgo* dot);
	_GlOutlineAlgo(const _GlOutlineAlgo& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
								GlProcessStatus* status);

private:
	typedef GlAlgo2d	inherited;
	float				mRadiusRel;
	int32				mRadiusAbs;
	bool				mEraseBg;

	status_t			ProcessFromEdges(	uint8* mask, uint8* on, int32 w, int32 h,
											uint8* dot, int32 dotW, int32 dotH);
	status_t			PutDot(	uint8* mask, int32 w, int32 h, int32 maskX, int32 maskY,
								uint8* dot, int32 dotW, int32 dotH);
};

/***************************************************************************
  * GL-OUTLINE-SRF
 ***************************************************************************/
GlOutlineSrf::GlOutlineSrf(const GlOutlineSrfAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_DOT_KEY, GL_2D_IO, SZ(SZ_Dot), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_ELLIPSE_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlOutlineSrf::GlOutlineSrf(const GlOutlineSrf& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlOutlineSrf::Clone() const
{
	return new GlOutlineSrf(*this);
}

GlAlgo* GlOutlineSrf::Generate(const gl_generate_args& args) const
{
	GlAlgo*		dot = GenerateChainAlgo(_DOT_KEY, args);
	return new _GlOutlineAlgo(	Id(), PixelTargets(),
								Params().Float('radr'),
								Params().Int32('rada'),
								Params().Bool(_ERASE_BG_KEY, _ERASE_BG_INIT),
								dot);
}

// #pragma mark -

/***************************************************************************
 * GL-OUTLINE-SRF-ADD-ON
 ***************************************************************************/
GlOutlineSrfAddOn::GlOutlineSrfAddOn()
		: inherited(SZI[SZI_arp], GL_OUTLINE_KEY, SZ(SZ_2D), SZ(SZ_Outline), 1, 0)
{
	mRadiusRel		= AddParamType(new GlFloatParamType('radr', SZ(SZ_Rel_radius), 0, 1, 0, 0.1f));
	mRadiusAbs		= AddParamType(new GlInt32ParamType('rada', SZ(SZ_Abs_radius), 0, 1024, 5));
	mEraseBg		= AddParamType(new GlBoolParamType(	_ERASE_BG_KEY, SZ(SZ_Erase_background), _ERASE_BG_INIT));
}

GlNode* GlOutlineSrfAddOn::NewInstance(const BMessage* config) const
{
	return new GlOutlineSrf(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-OUTLINE-SURFACE
 ***************************************************************************/
_GlOutlineAlgo::_GlOutlineAlgo(	gl_node_id nid, uint32 targets, float rr,
								int32 ra, bool eraseBg, GlAlgo* dot)
		: inherited(nid, targets), mRadiusRel(rr), mRadiusAbs(ra),
		  mEraseBg(eraseBg)
{
	if (dot) SetChain(dot, _DOT_INDEX);
}

_GlOutlineAlgo::_GlOutlineAlgo(const _GlOutlineAlgo& o)
		: inherited(o), mRadiusRel(o.mRadiusRel), mRadiusAbs(o.mRadiusAbs),
		  mEraseBg(o.mEraseBg)
{
}

GlAlgo* _GlOutlineAlgo::Clone() const
{
	return new _GlOutlineAlgo(*this);
}

status_t _GlOutlineAlgo::Process(	const GlPlanes* pixels, uint8* dest, int32 w, int32 h,
									GlProcessStatus* status)
{
	int32				dotSize = int32((((w + h) / 2.0) * mRadiusRel) + mRadiusAbs);
	if (!dest || dotSize < 1) return B_OK;
	GlAlgo2dWrap		wrap(AlgoAt(_DOT_INDEX));
	if (wrap.InitCheck() != B_OK) return B_OK;
	GlCache2d			cache(dotSize, dotSize);
	uint8*				dot = wrap.Cache(cache);
	if (!dot) return B_OK;
	GlCache2d			onCache(w, h);
	uint8*				on = onCache.Data();
	if (!on) return B_OK;
	/* The mask is binary, 0 or 1.  1 indicates pixels that can be outlined,
	 * but haven't yet.  When it's all done, 1 values will be turned off.
	 */
	int32				pix;
	for (pix = 0; pix < w * h; pix++) {
		if (dest[pix] > 0) on[pix] = 1;
		else on[pix] = 0;
		if (mEraseBg) dest[pix] = 0;
	}
	return ProcessFromEdges(dest, on, w, h, dot, dotSize, dotSize);	
}

/* Outline by sending out a ray from each pixel on the edge -- for example,
 * every pixel on the top travels down until it hits a value.  This method
 * is for masks that are basically a filled center.
 */
status_t _GlOutlineAlgo::ProcessFromEdges(	uint8* dest, uint8* on, int32 w, int32 h,
											uint8* dot, int32 dotW, int32 dotH)
{
	int32			x, y, pix;
	/* From the left.
	 */
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			if (on[pix] > 0) {
				PutDot(dest, w, h, x, y, dot, dotW, dotH);
				on[pix] = 0;
				break;
			}
		}
	}
	/* From the top.
	 */
	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			pix = ARP_PIXEL(x, y, w);
			if (on[pix] > 0) {
				PutDot(dest, w, h, x, y, dot, dotW, dotH);
				on[pix] = 0;
				break;
			}
		}
	}
	/* From the right.
	 */
	for (y = 0; y < h; y++) {
		for (x = w - 1; x >= 0; x--) {
			pix = ARP_PIXEL(x, y, w);
			if (on[pix] > 0) {
				PutDot(dest, w, h, x, y, dot, dotW, dotH);
				on[pix] = 0;
				break;
			}
		}
	}
	/* From the bottom.
	 */
	for (x = 0; x < w; x++) {
		for (y = h - 1; y >= 0; y--) {
			pix = ARP_PIXEL(x, y, w);
			if (on[pix] > 0) {
				PutDot(dest, w, h, x, y, dot, dotW, dotH);
				on[pix] = 0;
				break;
			}
		}
	}
	return B_OK;
}

status_t _GlOutlineAlgo::PutDot(uint8* dest, int32 w, int32 h, int32 maskX, int32 maskY,
								uint8* dot, int32 dotW, int32 dotH)
{
	int32				hotX = dotW / 2, hotY = dotH / 2;

	for (int32 srcY = 0; srcY < dotH; srcY++) {
		for (int32 srcX = 0; srcX < dotW; srcX++) {
			int32		destX = maskX + (srcX - hotX),
						destY = maskY + (srcY - hotY);
			if (GL_IN_BOUNDS(destX, destY, w, h)) {
				int32	srcPix = ARP_PIXEL(srcX, srcY, dotW),
						destPix = ARP_PIXEL(destX, destY, w);
				dest[destPix] = arp_clip_255(dest[destPix] + dot[srcPix]);
			}
		}
	}
	return B_OK;
}