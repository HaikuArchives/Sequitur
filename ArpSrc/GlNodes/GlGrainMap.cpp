#include <stdio.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamList.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPseudoRandomizer.h>
#include <GlNodes/GlGrainMap.h>

static const int32		GL_GRAIN_KEY	= 'AmGr';
static const int32		_SIZE_KEY		= 'size';
static const uint32		_SIZE_INDEX		= 0;

/***************************************************************************
 * _GL-GRAIN-MAP
 ***************************************************************************/
class _GlGrainMap : public GlAlgo1d
{
public:
	_GlGrainMap(gl_node_id nid, float min, float max, GlAlgo* m);
	_GlGrainMap(const _GlGrainMap& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d	inherited;
	float				mMin, mMax;

	status_t			InitLengths(float* line, int32 size) const;
	void				DrawGrain(	float* src, int32 srcSize, float* dest,
									int32 destFrom, int32 destTo) const;
};

/***************************************************************************
 * GL-GRAIN-MAP
 ***************************************************************************/
GlGrainMap::GlGrainMap(const GlGrainMapAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_SIZE_KEY, GL_1D_IO, SZ(SZ_Size), this), &added);
	if (chain && added) {
		GlNode*		node = GlGlobals().NewNode(GL_PINK_NOISE_KEY, 0);
		if (node) chain->AddNode(node);
	}
}

GlGrainMap::GlGrainMap(const GlGrainMap& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlGrainMap::Clone() const
{
	return new GlGrainMap(*this);
}

GlAlgo* GlGrainMap::Generate(const gl_generate_args& args) const
{
	GlAlgo*			map = GenerateChainAlgo(_SIZE_KEY, args);
	return new _GlGrainMap(	Id(), Params().Float(GL_MIN_PARAM_KEY),
							Params().Float(GL_MAX_PARAM_KEY), map);
}

// #pragma mark -

/***************************************************************************
 * GL-GRAIN-MAP-ADD-ON
 ***************************************************************************/
GlGrainMapAddOn::GlGrainMapAddOn()
		: inherited(SZI[SZI_arp], GL_GRAIN_KEY, SZ(SZ_1D), SZ(SZ_Grain), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_GRAIN_KEY));
	mMin		= AddParamType(new GlFloatParamType(GL_MIN_PARAM_KEY, SZ(SZ_Small), 0, 1, 0.2f, 0.1f));
	mMax		= AddParamType(new GlFloatParamType(GL_MAX_PARAM_KEY, SZ(SZ_Large), 0, 1, 0.4f, 0.1f));
}

GlNode* GlGrainMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlGrainMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-GRAIN-MAP
 ***************************************************************************/
_GlGrainMap::_GlGrainMap(gl_node_id nid, float min, float max, GlAlgo* m)
		: inherited(GL_GRAIN_KEY, nid), mMin(min), mMax(max)
{
	if (m) SetChain(m, _SIZE_INDEX);
}

_GlGrainMap::_GlGrainMap(const _GlGrainMap& o)
		: inherited(o), mMin(o.mMin), mMax(o.mMax)		  
{
}

GlAlgo* _GlGrainMap::Clone() const
{
	return new _GlGrainMap(*this);
}

status_t _GlGrainMap::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	if (size <= 1) return B_OK;
	GlArrayF			lengths;
	status_t			err = lengths.Resize(size);
	if (err != B_OK) return err;
	if ((err = InitLengths(lengths.n, lengths.size)) != B_OK) return err;

	GlArrayF			lineSrc;
	if ((err = lineSrc.Resize(size)) != B_OK) return err;
	memcpy(lineSrc.n, line, size * 4);

	float				r = ARP_ABS(mMax - mMin);
	float				min = (mMin < mMax) ? mMin : mMax;

	for (int32 start = 0; start < size; start++) {
		int32				nextSize = int32( (min + (lengths.n[start] * r)) * size);
		if (nextSize <= 0) nextSize = 1;
		else if (nextSize > size) nextSize = size;
		int32				stop = start + nextSize - 1;
		if (stop >= size) stop = size - 1;

		DrawGrain(lineSrc.n, size, line, start, stop);
		start = stop;	
	}
	return B_OK;
}

status_t _GlGrainMap::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == GL_MIN_PARAM_KEY) mMin = ((const GlFloatWrap&)wrap).v;
	else if (key.key == GL_MAX_PARAM_KEY) mMax = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlGrainMap::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == GL_MIN_PARAM_KEY) ((GlFloatWrap&)wrap).v = mMin;
	else if (key.key == GL_MAX_PARAM_KEY) ((GlFloatWrap&)wrap).v = mMax;
	else return B_ERROR;
	return B_OK;
}

status_t _GlGrainMap::InitLengths(float* line, int32 size) const
{
	ArpASSERT(line);
	GlAlgo1d*				m = (GlAlgo1d*)(ChainAt(_SIZE_INDEX));
	if (m) return m->Run(line, 0, size);

	float				mRnd = 0.1f;
	GlPseudoRandomizer	pr(uint32(mRnd * 10000));
	for (int32 k = 0; k < size; k++) line[k] = pr.Next();
	return B_OK;
}

void _GlGrainMap::DrawGrain(float* src, int32 srcSize, float* dest,
							int32 destFrom, int32 destTo) const
{
	int32				destSize = (destTo - destFrom) + 1;
if (srcSize <= destSize) {
	printf("src %ld dest %ld\n", srcSize, destSize);
ArpASSERT(false);
}
	ArpVALIDATE(srcSize > destSize, return);
	float				sum = 0;
	float				sumC = 0;
	float				step = destSize / float(srcSize),
						stepC = 0;
	int32				to = destFrom;
	for (int32 from = 0; from < srcSize; from++) {
		float			prevStepC = stepC;
		stepC += step;
		if (stepC < 1.0) {
			sum += src[from];
			sumC += 1;
		} else {
			float		leftover = 1 - prevStepC;
			float		frac = leftover * (1 / step);
			float		sumFrac = src[from] * frac;
			sum += sumFrac;
			sumC += frac;
			
			ArpASSERT(to <= destTo);
			if (sumC > 1) dest[to] = sum / sumC;
			else dest[to] = sum;

			stepC = stepC - 1.0f;
			sum = src[from] * (1 - frac);
			sumC = 1 - frac;

			to++;
			if (to > destTo) break;
		}
	}
	/* HACK!  Sometimes it doesn't stretch out quiiite far
	 * enough, so fill in the final value if necessary.
	 */
	if (to <= destTo) dest[destTo] = src[srcSize -1];
}
