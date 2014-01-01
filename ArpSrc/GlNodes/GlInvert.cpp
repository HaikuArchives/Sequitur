#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlInvert.h>

static const int32		_MAP_KEY			= 'map_';
static const int32		_MAP_INDEX			= 0;

/***************************************************************************
 * GL-INVERT-2D
 ***************************************************************************/
class GlInvert2d : public GlAlgo2d
{
public:
	GlInvert2d(gl_node_id nid, uint32 targets, GlAlgo* map);
	GlInvert2d(const GlInvert2d& o);
	
	virtual GlAlgo*			Clone() const;
	virtual status_t		Process(const GlPlanes* src, GlPlanes& dest,
									GlProcessStatus* status = 0);

private:
	typedef GlAlgo2d		inherited;

	status_t				ApplyTo2d(	GlAlgo1dWrap& w, GlPlanes& dest,
										GlProcessStatus* status) const;
};

/***************************************************************************
  * GL-INVERT
 ***************************************************************************/
GlInvert::GlInvert(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SAWTOOTH_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlInvert::GlInvert(const GlInvert& o)
		: inherited(o)
{
}

GlNode* GlInvert::Clone() const
{
	return new GlInvert(*this);
}

GlAlgo* GlInvert::Generate(const gl_generate_args& args) const
{
	GlAlgo*			map = GenerateChainAlgo(_MAP_KEY, args);
	if (!map) return 0;
	return new GlInvert2d(Id(), PixelTargets(), map);
}

// #pragma mark -

/***************************************************************************
 * GL-INVERT-ADD-ON
 ***************************************************************************/
GlInvertAddOn::GlInvertAddOn()
		: inherited(SZI[SZI_arp], GL_INVERT_KEY, SZ(SZ_Color), SZ(SZ_Invert), 1, 0)
{
}

GlNode* GlInvertAddOn::NewInstance(const BMessage* config) const
{
	return new GlInvert(this, config);
}

// #pragma mark -

/***************************************************************************
 * GL-INVERT-2D
 ***************************************************************************/
GlInvert2d::GlInvert2d(gl_node_id nid, uint32 targets, GlAlgo* map)
		: inherited(nid, targets)
{
	if (map) SetChain(map, _MAP_INDEX);
}

GlInvert2d::GlInvert2d(const GlInvert2d& o)
		: inherited(o)
{
}

GlAlgo* GlInvert2d::Clone() const
{
	return new GlInvert2d(*this);
}

status_t GlInvert2d::Process(	const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status)
{
	if (dest.w < 1 || dest.h < 1 || dest.size < 1) return B_OK;
	GlAlgo1dWrap			wrap(AlgoAt(_MAP_INDEX));
	if (wrap.InitCheck() != B_OK) return B_ERROR;
	return ApplyTo2d(wrap, dest, status);
}


static bool _flush(GlPlanes& dest, float* cache)
{
	int32			pix, size = dest.w * dest.h;
	for (pix = 0; pix < size; pix++) {
		for (uint32 k = 0; k < dest.size; k++) {
			dest.plane[k][pix] = arp_clip_255(cache[dest.plane[k][pix]] * 255);
		}
	}
	for (pix = 0; pix < 256; pix++) cache[pix] = glTable256[pix];
//	for (pix = 0; pix < 256; pix++) cache[pix] = 1.0;
	return false;
}

/* Apply my chain to the 2d.  Some algos need to implement this themselves
 * because they need to know the entire range of values -- for example,
 * a scale operation, moved to 2d, needs to know the actual values in the
 * 2d plane.  Some algos are random, like pink noise, and so will need to
 * implement this.  But for most situations, it's OK to build a cache of
 * the initial possible values (0 - 255), then run that through the chain
 * and do a replace at the end.  Since these different modes of working are
 * possibly mixed in a single chain, I need to cache where I can, then flush
 * that when necessary.
 */
status_t GlInvert2d::ApplyTo2d(	GlAlgo1dWrap& w, GlPlanes& dest,
								GlProcessStatus* status) const
{
	ArpASSERT(dest.w > 0 && dest.h > 0 && dest.size > 0);
	if (w.size < 1) return B_ERROR;
	float*					cache = new float[256];
	if (!cache) return B_NO_MEMORY;
	bool					needsFlush = false;
	int32					pix;
	for (pix = 0; pix < 256; pix++) cache[pix] = glTable256[pix];
//	for (pix = 0; pix < 256; pix++) cache[pix] = 1.0;

	uint32					flags = GlAlgo1d::ALGO_HEAD_F;
	for (uint32 k = 0; k < w.size; k++) {
		ArpASSERT(w.cache[k]);
		GlAlgo1d*			a1d = w.cache[k];
		if (a1d->Properties(a1d->SINGLE_ALGO_F)&a1d->APPLY_TO_2D_F) {
			if (needsFlush) needsFlush = _flush(dest, cache);
			a1d->ApplyTo2d(dest, status);
		} else {
			a1d->Algo(cache, 0, 256, flags);
			needsFlush = true;
		}
		flags = 0;
	}
	if (needsFlush) _flush(dest, cache);
	delete[] cache;
	return B_OK;
}
