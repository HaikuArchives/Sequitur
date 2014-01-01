#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlTriangle1d.h>

/***************************************************************************
 * _GL-TRIANGLE-1D
 ***************************************************************************/
class _GlTriangle1d : public GlAlgo1d
{
public:
	_GlTriangle1d(gl_node_id nid, float c, float p);
	_GlTriangle1d(const _GlTriangle1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d	inherited;
	float				mCycle, mPhase;
};

/***************************************************************************
 * GL-TRIANGLE-1D
 ***************************************************************************/
GlTriangle1d::GlTriangle1d(const GlTriangle1dAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlTriangle1d::GlTriangle1d(const GlTriangle1d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlTriangle1d::Clone() const
{
	return new GlTriangle1d(*this);
}

GlAlgo* GlTriangle1d::Generate(const gl_generate_args& args) const
{
	return new _GlTriangle1d(	Id(), Params().Float(GL_CYCLE_KEY),
								Params().Float(GL_PHASE_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-TRIANGLE-1D-ADD-ON
 ***************************************************************************/
GlTriangle1dAddOn::GlTriangle1dAddOn()
		: inherited(SZI[SZI_arp], GL_TRIANGLE_KEY, SZ(SZ_1D), SZ(SZ_Triangle), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_TRIANGLE_KEY));
	mCycle		= AddParamType(new GlFloatParamType(GL_CYCLE_KEY, SZ(SZ_Cycle), 0, 100, 1, 0.1f));
	mPhase		= AddParamType(new GlFloatParamType(GL_PHASE_KEY, SZ(SZ_Phase), 0, 1, 0, 0.1f));
}

GlNode* GlTriangle1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlTriangle1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-TRIANGLE-1D
 ***************************************************************************/
_GlTriangle1d::_GlTriangle1d(gl_node_id nid, float c, float p)
		: inherited(GL_TRIANGLE_KEY, nid), mCycle(c), mPhase(p)
{
}

_GlTriangle1d::_GlTriangle1d(const _GlTriangle1d& o)
		: inherited(o), mCycle(o.mCycle), mPhase(o.mPhase)
{
}

GlAlgo* _GlTriangle1d::Clone() const
{
	return new _GlTriangle1d(*this);
}

status_t _GlTriangle1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	float		v;
	for (int32 step = 0; step < size; step++) {
		if (at) v = mPhase + (at[step] * mCycle);
		else v = mPhase + (GL_1D_LFO_STEP(step, size) * mCycle);
		v = v - floor(v);
		ArpASSERT(v >= 0 && v <= 1);
		if (v <= 0.5) v *= 2;
		else v = 1 - ((v - 0.5f) * 2);
		if (flags&ALGO_HEAD_F) line[step] = v;
		else line[step] *= v;
	}
	return B_OK;
}

status_t _GlTriangle1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == GL_CYCLE_KEY) mCycle = ((const GlFloatWrap&)wrap).v;
	else if (key.key == GL_PHASE_KEY) mPhase = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlTriangle1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == GL_CYCLE_KEY) ((GlFloatWrap&)wrap).v = mCycle;
	else if (key.key == GL_PHASE_KEY) ((GlFloatWrap&)wrap).v = mPhase;
	else return B_ERROR;
	return B_OK;
}
