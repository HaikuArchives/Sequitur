#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlPromote.h>

// PromoteSrf operations
static const uint32		GL_PROMOTE_AVG_OP			= 'avg_';
static const uint32		GL_PROMOTE_MIN_OP			= 'min_';
static const uint32		GL_PROMOTE_MAX_OP			= 'max_';
static const uint32		GL_PROMOTE_ADD_OP			= 'add_';
static const uint32		GL_PROMOTE_SUB_OP			= 'sub_';
static const uint32		GL_PROMOTE_CONTRAST_OP		= 'cntr';
static const uint32		GL_PROMOTE_INTERSECTION_OP	= '^U__';

static const int32		_X_KEY						= 'x___';
static const int32		_X_INDEX					= 0;
static const int32		_Y_KEY						= 'y___';
static const int32		_Y_INDEX					= 1;
static const int32		_TIE_KEY					= 'tie_';
static const int32		_TIE_INDEX					= 2;

/***************************************************************************
 * _GL-PROMOTE-SURFACE
 ***************************************************************************/
class _GlPromoteSurface : public GlAlgo2d
{
public:
	_GlPromoteSurface(	gl_node_id nid, uint32 targets, uint32 op, GlAlgo* xC,
						GlAlgo* yC, GlAlgo* tie);
	_GlPromoteSurface(const _GlPromoteSurface& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
								GlProcessStatus* status = 0);

private:
	typedef GlAlgo2d	inherited;
	uint32				mOp;

	status_t			PromoteXAxis(uint8* mask, int32 w, int32 h, GlAlgo1d* map);
	status_t			PromoteYAxis(uint8* mask, int32 w, int32 h, GlAlgo1d* map);
	status_t			PromoteXYAxes(	uint8* mask, int32 w, int32 h,
										GlAlgo1d* xm, GlAlgo1d* ym);
};

/***************************************************************************
  * GL-PROMOTE
 ***************************************************************************/
GlPromote::GlPromote(const GlPromoteAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_X_KEY, GL_1D_IO, SZ(SZ_X), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SINE_KEY, 0);
		if (node) chain->AddNode(node);
	}
	chain = VerifyChain(new GlChain(_Y_KEY, GL_1D_IO, SZ(SZ_Y), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SQUARE_KEY, 0);
		if (node) chain->AddNode(node);
	}
	VerifyChain(new GlChain(_TIE_KEY, GL_1D_IO, SZ(SZ_Tie), this));
}

GlPromote::GlPromote(const GlPromote& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlPromote::Clone() const
{
	return new GlPromote(*this);
}

GlAlgo* GlPromote::Generate(const gl_generate_args& args) const
{
	ArpVALIDATE(mAddOn, return 0);
	GlAlgo*			xC = GenerateChainAlgo(_X_KEY, args);
	GlAlgo*			yC = GenerateChainAlgo(_Y_KEY, args);
	GlAlgo*			tb = GenerateChainAlgo(_TIE_KEY, args);
	return new _GlPromoteSurface(	Id(), PixelTargets(),
									Params().Menu(GL_MODE_PARAM_KEY),
									xC, yC, tb);
}

// #pragma mark -

/***************************************************************************
 * GL-PROMOTE-ADD-ON
 ***************************************************************************/
GlPromoteAddOn::GlPromoteAddOn()
		: inherited(SZI[SZI_arp], GL_PROMOTE_KEY, SZ(SZ_2D), SZ(SZ_Promote), 1, 0)
{
	BMessage		m;
	m.AddString("item", "Average");			m.AddInt32("i", GL_PROMOTE_AVG_OP);
	m.AddString("item", "Min");				m.AddInt32("i", GL_PROMOTE_MIN_OP);
	m.AddString("item", "Max");				m.AddInt32("i", GL_PROMOTE_MAX_OP);
	m.AddString("item", "Add");				m.AddInt32("i", GL_PROMOTE_ADD_OP);
	m.AddString("item", "Sub");				m.AddInt32("i", GL_PROMOTE_SUB_OP);
	m.AddString("item", "Contrast");		m.AddInt32("i", GL_PROMOTE_CONTRAST_OP);
	m.AddString("item", "Intersection");	m.AddInt32("i", GL_PROMOTE_INTERSECTION_OP);

	mOp		= AddParamType(new GlMenuParamType(GL_MODE_PARAM_KEY, SZ(SZ_Mode), m, GL_PROMOTE_AVG_OP));
}

GlNode* GlPromoteAddOn::NewInstance(const BMessage* config) const
{
	return new GlPromote(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-PROMOTE-SURFACE
 ***************************************************************************/
_GlPromoteSurface::_GlPromoteSurface(	gl_node_id nid, uint32 targets,
										uint32 op, GlAlgo* xC,
										GlAlgo* yC, GlAlgo* tie)
		: inherited(nid, targets), mOp(op)
{
	if (xC) SetChain(xC, _X_INDEX);
	if (yC) SetChain(yC, _Y_INDEX);
	if (tie) SetChain(tie, _TIE_INDEX);
}

_GlPromoteSurface::_GlPromoteSurface(const _GlPromoteSurface& o)
		: inherited(o), mOp(o.mOp)
{
}

GlAlgo* _GlPromoteSurface::Clone() const
{
	return new _GlPromoteSurface(*this);
}

status_t _GlPromoteSurface::Process(const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
									GlProcessStatus* status)
{
	ArpVALIDATE(mask && w > 0 && h > 0, return B_OK);
	GlAlgo1d*		xm = Algo1dAt(_X_INDEX);
	GlAlgo1d*		ym = Algo1dAt(_Y_INDEX);

	if (!xm && !ym) {
		for (int32 pix = 0; pix < w * h; pix++) mask[pix] = 0;
		return B_OK;
	} else if (xm && ym) {
		if (xm->Promote2d(mask, w, h, ym) != B_OK)
			PromoteXYAxes(mask, w, h, xm, ym);
		return B_OK;
	} else if (xm) PromoteXAxis(mask, w, h, xm);
	else if (ym) PromoteYAxis(mask, w, h, ym);
	return B_OK;
}

status_t _GlPromoteSurface::PromoteXAxis(uint8* mask, int32 w, int32 h, GlAlgo1d* map)
{
	ArpASSERT(mask && map && w > 0 && h > 0);
	map->SetStep(0.0);
	GlCache1d*		cache = map->NewCache(w, 0);
	if (!cache) return B_NO_MEMORY;

	int32			x, y, pix;
	uint8			v;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			v = arp_clip_255(cache->n[x] * 255);
			pix = ARP_PIXEL(x, y, w);
			mask[pix] = ARP_MIN(mask[pix], v);
		}
		cache->SetStep(map, y / float(h - 1));
	}

	delete cache;
	return B_OK;
}

status_t _GlPromoteSurface::PromoteYAxis(uint8* mask, int32 w, int32 h, GlAlgo1d* map)
{
	ArpASSERT(mask && map && w > 0 && h > 0);
	map->SetStep(0.0);
	GlCache1d*		cache = map->NewCache(h, 0);
	if (!cache) return B_NO_MEMORY;

	int32			x, y, pix;
	uint8			v;

	for (x = 0; x < w; x++) {
		for (y = 0; y < h; y++) {
			v = arp_clip_255(cache->n[y] * 255);
			pix = ARP_PIXEL(x, y, w);
			mask[pix] = ARP_MIN(mask[pix], v);
		}
		cache->SetStep(map, x / float(w + 1));
	}

	delete cache;
	return B_OK;
}

#define _GET_PROMOTION(op, xv, yv, tieX, tieY, x, w, y, h) \
	(op == GL_PROMOTE_AVG_OP)					? ((int32(xv) + yv) / 2) \
		: (op == GL_PROMOTE_MIN_OP)				? ((xv <= yv) ? xv : yv) \
		: (op == GL_PROMOTE_MAX_OP)				? ((xv >= yv) ? xv : yv) \
		: (op == GL_PROMOTE_ADD_OP)				? (int32(xv) + yv) \
		: (op == GL_PROMOTE_SUB_OP)				? (int32(xv) - yv) \
		: (op == GL_PROMOTE_CONTRAST_OP)		? ((int32(xv) * yv) / 255) \
		: (op == GL_PROMOTE_INTERSECTION_OP)	? ((xv == yv) ? xv : arp_clip_255(((tieX ? tieX->n[x] : 0.5) + (tieY ? tieY->n[y] : 0.5)) * 127.5)) \
		: ((int32(x) + y) / 2)
		// Default to average

status_t _GlPromoteSurface::PromoteXYAxes(	uint8* mask, int32 w, int32 h,
											GlAlgo1d* xm, GlAlgo1d* ym)
{
	ArpVALIDATE(mask && xm && ym, return B_ERROR);

	/* Initialize all the caches
	 */
	xm->SetStep(0.0);
	GlCache1d*				cacheX = xm->NewCache(w, 0);
	GlCache1d*				cacheY = ym->NewCache(h, w);
	if (!cacheX || !cacheY) {
		delete cacheX;			delete cacheY;
		return B_NO_MEMORY;
	}
	GlAlgo1d*				tiem = Algo1dAt(_TIE_INDEX);
	GlCache1d*				tieX = 0;
	GlCache1d*				tieY = 0;
	if (tiem) {
		tieX = tiem->NewCache(w);
		tieY = tiem->NewCache(h);
		if (!tieX || !tieY) {
			delete cacheX;		delete cacheY;		delete tieX;	delete tieY;
			return B_NO_MEMORY;
		}
	}
	/* Run the algo
	 */
	for (int32 y = 0; y < h; y++) {
		uint8		yv = 0;
		if (cacheY->h == 1) yv = arp_clip_255(cacheY->n[y] * 255);						// No morphing on Y
		
		for (int32 x = 0; x < w; x++) {
			if (cacheY->h > 1) yv = arp_clip_255(cacheY->n[ARP_PIXEL(y, x, h)] * 255);	// Morphing Y
			uint8	xv = arp_clip_255(cacheX->n[x] * 255);

			uint8	val = arp_clip_255(_GET_PROMOTION(mOp, xv, yv, tieX, tieY, x, w, y, h));
			int32	pix = ARP_PIXEL(x, y, w);
			mask[pix] = ARP_MIN(mask[pix], val);
		}
		cacheX->SetStep(xm, y / float(h - 1));
	}

	delete cacheX;
	delete cacheY;
	delete tieX;
	delete tieY;
	return B_OK;
}
