#include <stdio.h>
#include <typeinfo>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamList.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlReplicateMap.h>

static const int32		GL_REPLICATE_MAP_KEY	= 'AmRp';
static const int32		_MAP_KEY				= 'map_';
static const uint32		_MAP_INDEX				= 0;
static const int32		_DEPTH_KEY				= 'dpth';

/***************************************************************************
 * _GL-REPLICATE-MAP
 ***************************************************************************/
class _GlReplicateMap : public GlAlgo1d
{
public:
	_GlReplicateMap(gl_node_id nid, GlAlgo* map, float depth);
	_GlReplicateMap(const _GlReplicateMap& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d	inherited;
	float				mDepth;

	mutable GlArrayF	mLineCache;
	mutable GlArrayF	mAtCache;
	mutable GlArrayF	mLineSrcCache;
	mutable GlArrayF	mAtSrcCache;

	status_t			InitCaches(	float* line, float* at, int32 size,
									int32 cacheSize) const;
	void				CacheScale(	float* src, int32 srcSize,
									float* dest, int32 destSize) const;
};

/***************************************************************************
 * GL-REPLICATE-MAP
 ***************************************************************************/
GlReplicateMap::GlReplicateMap(const GlReplicateMapAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_SINE_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlReplicateMap::GlReplicateMap(const GlReplicateMap& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlReplicateMap::Clone() const
{
	return new GlReplicateMap(*this);
}

GlAlgo* GlReplicateMap::Generate(const gl_generate_args& args) const
{
	GlAlgo*			map = GenerateChainAlgo(_MAP_KEY, args);
	if (!map) return 0;
	return new _GlReplicateMap(Id(), map, Params().Float(_DEPTH_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-REPLICATE-MAP-ADD-ON
 ***************************************************************************/
GlReplicateMapAddOn::GlReplicateMapAddOn()
		: inherited(SZI[SZI_arp], GL_REPLICATE_MAP_KEY, SZ(SZ_1D), SZ(SZ_Replicate), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_REPLICATE_MAP_KEY));
	mDepth =	AddParamType(new GlFloatParamType(_DEPTH_KEY, SZ(SZ_Depth), 0, 1, 0.5f, 0.01f));
}

GlNode* GlReplicateMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlReplicateMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-REPLICATE-MAP
 ***************************************************************************/
_GlReplicateMap::_GlReplicateMap(gl_node_id nid, GlAlgo* map, float depth)
		: inherited(GL_REPLICATE_MAP_KEY, nid), mDepth(depth)
{
	if (map) SetChain(map, _MAP_INDEX);
}

_GlReplicateMap::_GlReplicateMap(const _GlReplicateMap& o)
		: inherited(o), mDepth(o.mDepth)
{
}

GlAlgo* _GlReplicateMap::Clone() const
{
	return new _GlReplicateMap(*this);
}

status_t _GlReplicateMap::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	const GlAlgo1d*		map = Algo1dAt(_MAP_INDEX);
	if (!map) return B_ERROR;
	/* 0.5 is 2 replications -- anything below that is obviously meaningless.
	 */
	if (mDepth < 0.50) return map->Algo(line, at, size, flags);
	/* Default to repping every element in the line -- i.e, depth is 1.
	 */
	int32			repCount = size;
	int32			repSize = 1;
	if (mDepth < 1.0) {
		repCount = int32(floor(1 / (1 - mDepth)));
		repSize = int32(ceil(size / float(repCount)));
		if (repSize <= 0) return map->Algo(line, at, size, flags);
		if (repSize > size) repSize = size;
	}
	
	status_t		err = InitCaches(line, at, size, repSize);
	if (err != B_OK) return err;

	int32			from = 0, to = repSize - 1;
	int32			curRep = 0;
	while (from < size) {
		if (to >= size) to = size - 1;
		ArpVALIDATE(to >= from, break);
		/* Run my map on a cache of the line and copy it over.
		 */
		memcpy(mLineCache.n, mLineSrcCache.n, repSize * 4);
		if (at) memcpy(mAtCache.n, mAtSrcCache.n, repSize * 4);
		((GlAlgo1d*)map)->SetStep(curRep / float(repCount));
		map->Algo(mLineCache.n, (at) ? mAtCache.n : 0, repSize, flags);
		/* Copy over the results.
		 */
		for (int32 k = from; k <= to; k++) line[k] *= mLineCache.n[k - from];

		from = to + 1;
		to = from + repSize - 1;
		curRep++;
		ArpASSERT(curRep <= repCount);
		if (curRep > repCount) curRep = repCount;
	}
	return B_OK;
}

status_t _GlReplicateMap::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == _DEPTH_KEY) mDepth = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlReplicateMap::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == _DEPTH_KEY) ((GlFloatWrap&)wrap).v = mDepth;
	else return B_ERROR;
	return B_OK;
}

status_t _GlReplicateMap::InitCaches(	float* line, float* at, int32 size,
										int32 cacheSize) const
{
	/* Make sure everyone's big enough
	 */
	status_t		err = B_OK;
	if (int32(mLineCache.size) < cacheSize) err = mLineCache.Resize(cacheSize);
	if (err != B_OK) return err;
	if (at && int32(mAtCache.size) < cacheSize) err = mAtCache.Resize(cacheSize);
	if (err != B_OK) return err;
	if (int32(mLineSrcCache.size) < cacheSize) err = mLineSrcCache.Resize(cacheSize);
	if (err != B_OK) return err;
	if (at && int32(mAtSrcCache.size) < cacheSize) err = mAtSrcCache.Resize(cacheSize);
	if (err != B_OK) return err;

	/* Initialize the sources with scaled down values
	 */
	if (cacheSize >= size) {
		memcpy(mLineSrcCache.n, line, size * 4);
		if (at) memcpy(mAtSrcCache.n, at, size * 4);
		return B_OK;
	}

	CacheScale(line, size, mLineSrcCache.n, cacheSize);
	if (at) CacheScale(at, size, mAtSrcCache.n, cacheSize);
	return B_OK;
}

void _GlReplicateMap::CacheScale(	float* src, int32 srcSize,
									float* dest, int32 destSize) const
{
	ArpASSERT(srcSize > destSize);
	float			sum = 0;
	float			sumC = 0;
	float			step = destSize / float(srcSize),
					stepC = 0;
	int32			to = 0;
	for (int32 from = 0; from < srcSize; from++) {
		if (stepC + step < 1.0) {
			stepC += step;
			sum += src[from];
			sumC += 1;
		} else {
			stepC += step;
//			float	frac = stepC - floor(stepC);
// FIX: Unverified change
			float	frac = 1 - (stepC - floor(stepC));
			float	leftOver = src[from] * frac;
			
			sum += leftOver;
			sumC += frac;
			
			if (sumC > 1) dest[to] = sum / sumC;
			else dest[to] = sum;

			sum = src[from] - leftOver;
			sumC = 1 - frac;
			stepC = (1 - frac) * step;
			
			to++;
			if (to >= destSize) break;
		}
	}
}
