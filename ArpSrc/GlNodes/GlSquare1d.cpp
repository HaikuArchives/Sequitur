#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlSquare1d.h>

static const int32		WIDTH_KEY	= 'wdth';

/***************************************************************************
 * _GL-SQUARE-1D
 ***************************************************************************/
class _GlSquare1d : public GlAlgo1d
{
public:
	_GlSquare1d(gl_node_id nid, float c, float p, float w);
	_GlSquare1d(const _GlSquare1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d	inherited;
	float				mCycle, mPhase, mWidth;

};

/***************************************************************************
  * GL-SQUARE-1D
 ***************************************************************************/
GlSquare1d::GlSquare1d(const GlSquare1dAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlSquare1d::GlSquare1d(const GlSquare1d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlSquare1d::Clone() const
{
	return new GlSquare1d(*this);
}

GlAlgo* GlSquare1d::Generate(const gl_generate_args& args) const
{
	return new _GlSquare1d(	Id(), Params().Float(GL_CYCLE_KEY),
							Params().Float(GL_PHASE_KEY),
							Params().Float(WIDTH_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-SQUARE-1D-ADD-ON
 ***************************************************************************/
GlSquare1dAddOn::GlSquare1dAddOn()
		: inherited(SZI[SZI_arp], GL_SQUARE_KEY, SZ(SZ_1D), SZ(SZ_Square), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_SQUARE_KEY));
	mCycle		= AddParamType(new GlFloatParamType(GL_CYCLE_KEY, SZ(SZ_Cycle),	0.0f,	100.0f,	1.0f,		0.1f));
	mPhase		= AddParamType(new GlFloatParamType(GL_PHASE_KEY, SZ(SZ_Phase),	0.0f,	1.0f,	0.0f,		0.1f));
	mWidth		= AddParamType(new GlFloatParamType(WIDTH_KEY, SZ(SZ_Width),	0.0f,	1.0f,	0.5f,		0.1f));
}

GlNode* GlSquare1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlSquare1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SQUARE-1D
 ***************************************************************************/
_GlSquare1d::_GlSquare1d(gl_node_id nid, float c, float p, float w)
		: inherited(GL_SQUARE_KEY, nid), mCycle(c), mPhase(p), mWidth(w)
{
}

_GlSquare1d::_GlSquare1d(const _GlSquare1d& o)
		: inherited(o), mCycle(o.mCycle), mPhase(o.mPhase), mWidth(o.mWidth)
{
}

GlAlgo* _GlSquare1d::Clone() const
{
	return new _GlSquare1d(*this);
}

status_t _GlSquare1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	float		v;
	for (int32 step = 0; step < size; step++) {
		if (at) v = mPhase + (at[step] * mCycle);
		else v = mPhase + (GL_1D_LFO_STEP(step, size) * mCycle);
		v = v - floor(v);
		if (v > mWidth) line[step] = 0;
		else if (flags&ALGO_HEAD_F) line[step] = 1;
	}
	return B_OK;
}

status_t _GlSquare1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == GL_CYCLE_KEY) mCycle = ((const GlFloatWrap&)wrap).v;
	else if (key.key == GL_PHASE_KEY) mPhase = ((const GlFloatWrap&)wrap).v;
	else if (key.key == WIDTH_KEY) mWidth = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlSquare1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == GL_CYCLE_KEY) ((GlFloatWrap&)wrap).v = mCycle;
	else if (key.key == GL_PHASE_KEY) ((GlFloatWrap&)wrap).v = mPhase;
	else if (key.key == WIDTH_KEY) ((GlFloatWrap&)wrap).v = mWidth;
	else return B_ERROR;
	return B_OK;
}
