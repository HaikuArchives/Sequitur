#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlSawtooth1d.h>

/***************************************************************************
 * _GL-SAWTOOTH-1D
 ***************************************************************************/
class _GlSawtooth1d : public GlAlgo1d
{
public:
	_GlSawtooth1d(gl_node_id nid, float c, float p);
	_GlSawtooth1d(const _GlSawtooth1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d	inherited;
	float				mCycle, mPhase;
};

/***************************************************************************
 * GL-SAWTOOTH-1D
 ***************************************************************************/
GlSawtooth1d::GlSawtooth1d(const GlSawtooth1dAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlSawtooth1d::GlSawtooth1d(const GlSawtooth1d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlSawtooth1d::Clone() const
{
	return new GlSawtooth1d(*this);
}

GlAlgo* GlSawtooth1d::Generate(const gl_generate_args& args) const
{
	return new _GlSawtooth1d(	Id(), Params().Float(GL_CYCLE_KEY),
								Params().Float(GL_PHASE_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-SAWTOOTH-1D-ADD-ON
 ***************************************************************************/
GlSawtooth1dAddOn::GlSawtooth1dAddOn()
		: inherited(SZI[SZI_arp], GL_SAWTOOTH_KEY, SZ(SZ_1D), SZ(SZ_Sawtooth), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_SAWTOOTH_KEY));
	mCycle		= AddParamType(new GlFloatParamType(GL_CYCLE_KEY, SZ(SZ_Cycle), -100.0f, 100.0f, 1.0f, 0.1f));
	mPhase		= AddParamType(new GlFloatParamType(GL_PHASE_KEY, SZ(SZ_Phase), 0.0f, 1.0f, 0.0f, 0.1f));
}

GlNode* GlSawtooth1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlSawtooth1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SAWTOOTH-1D
 ***************************************************************************/
_GlSawtooth1d::_GlSawtooth1d(gl_node_id nid, float c, float p)
		: inherited(GL_SAWTOOTH_KEY, nid), mCycle(c), mPhase(p)
{
}

_GlSawtooth1d::_GlSawtooth1d(const _GlSawtooth1d& o)
		: inherited(o), mCycle(o.mCycle), mPhase(o.mPhase)
{
}

GlAlgo* _GlSawtooth1d::Clone() const
{
	return new _GlSawtooth1d(*this);
}

status_t _GlSawtooth1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	float		v;
	for (int32 step = 0; step < size; step++) {
		if (at) v = at[step]; else v = GL_1D_LFO_STEP(step, size);
		v = mPhase + (v * mCycle);
		v = v - floor(v);
		ArpASSERT(v >= 0 && v <= 1);
		if (flags&ALGO_HEAD_F) line[step] = (1 - v);
		else line[step] *= (1 - v);
	}
	return B_OK;
}

status_t _GlSawtooth1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == GL_CYCLE_KEY) mCycle = ((const GlFloatWrap&)wrap).v;
	else if (key.key == GL_PHASE_KEY) mPhase = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlSawtooth1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == GL_CYCLE_KEY) ((GlFloatWrap&)wrap).v = mCycle;
	else if (key.key == GL_PHASE_KEY) ((GlFloatWrap&)wrap).v = mPhase;
	else return B_ERROR;
	return B_OK;
}
