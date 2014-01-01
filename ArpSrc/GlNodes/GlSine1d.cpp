#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlSine1d.h>

static const float		CYCLE_MAX	= 100;

/***************************************************************************
 * _GL-SINE-1D
 ***************************************************************************/
class _GlSine1d : public GlAlgo1d
{
public:
	_GlSine1d(gl_node_id nid, float c, float p);
	_GlSine1d(const _GlSine1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual void		_print() const;

private:
	typedef GlAlgo1d	inherited;
	float				mCycle, mPhase;
};

/***************************************************************************
 * GL-SINE-1D
 ***************************************************************************/
GlSine1d::GlSine1d(const GlNode1dAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlSine1d::GlSine1d(const GlSine1d& o)
		: inherited(o)
{
}

GlNode* GlSine1d::Clone() const
{
	return new GlSine1d(*this);
}

GlAlgo* GlSine1d::Generate(const gl_generate_args& args) const
{
	return new _GlSine1d(	Id(), Params().Float(GL_CYCLE_KEY),
							Params().Float(GL_PHASE_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-SINE-1D-ADD-ON
 ***************************************************************************/
GlSine1dAddOn::GlSine1dAddOn()
		: inherited(SZI[SZI_arp], GL_SINE_KEY, SZ(SZ_1D), SZ(SZ_Sine), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_SINE_KEY));
	AddParamType(new GlFloatParamType(GL_CYCLE_KEY, SZ(SZ_Cycle), 0.0f, CYCLE_MAX, 1.0f, 0.1f));
	AddParamType(new GlFloatParamType(GL_PHASE_KEY, SZ(SZ_Phase), 0.0f, 1.0f, 0.0f, 0.1f));
}

GlNode* GlSine1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlSine1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SINE-1D
 ***************************************************************************/
_GlSine1d::_GlSine1d(gl_node_id nid, float c, float p)
		: inherited(GL_SINE_KEY, nid), mCycle(M_PI * 2 * c), mPhase(M_PI * 2 * p)
{
}

_GlSine1d::_GlSine1d(const _GlSine1d& o)
		: inherited(o), mCycle(o.mCycle), mPhase(o.mPhase)
{
}

GlAlgo* _GlSine1d::Clone() const
{
	return new _GlSine1d(*this);
}

status_t _GlSine1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
//printf("GlSineAlgo\n");
	float		v;
	for (int32 step = 0; step < size; step++) {
		if (at) v = at[step]; else v = GL_1D_LFO_STEP(step, size);
		v = sin( (v * mCycle) + mPhase);
		if (v < -1) v = -1;
		else if (v > 1) v = 1;
		v = (v + 1) * 0.5f;
		ArpASSERT(v >= 0 && v <= 1);
		if (flags&ALGO_HEAD_F) line[step] = v;
		else line[step] *= v;
	}
	return B_OK;
}

status_t _GlSine1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == GL_CYCLE_KEY) mCycle = ((const GlFloatWrap&)wrap).v;
	else if (key.key == GL_PHASE_KEY) mPhase = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlSine1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == GL_CYCLE_KEY) ((GlFloatWrap&)wrap).v = mCycle;
	else if (key.key == GL_PHASE_KEY) ((GlFloatWrap&)wrap).v = mPhase;
	else return B_ERROR;
	return B_OK;
}

void _GlSine1d::_print() const
{
	printf(	"_GlSine1d key %ld cycle %f phase %f\n", mKey, mCycle, mPhase);
}
