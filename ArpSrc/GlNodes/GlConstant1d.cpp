#include "GlPublic/GlAlgo1d.h"
#include "GlPublic/GlParamType.h"
#include "GlNodes/GlConstant1d.h"

static const float			_DEF_VALUE			= 0.5f;

/***************************************************************************
 * _GL-CONSTANT-1D
 ***************************************************************************/
class _GlConstant1d : public GlAlgo1d
{
public:
	_GlConstant1d(gl_node_id nid, float v);
	_GlConstant1d(const _GlConstant1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual uint32		properties() const;

private:
	typedef GlAlgo1d	inherited;
	float				mValue;
};

/***************************************************************************
  * GL-CONSTANT-1D
 ***************************************************************************/
GlConstant1d::GlConstant1d(const GlNode1dAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlConstant1d::GlConstant1d(const GlConstant1d& o)
		: inherited(o)
{
}

GlNode* GlConstant1d::Clone() const
{
	return new GlConstant1d(*this);
}

GlAlgo* GlConstant1d::Generate(const gl_generate_args& args) const
{
	return new _GlConstant1d(Id(), Params().Float(GL_VALUE_PARAM_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-CONSTANT-ADD-ON
 ***************************************************************************/
GlConstant1dAddOn::GlConstant1dAddOn()
		: inherited(SZI[SZI_arp], GL_CONSTANT_KEY, SZ(SZ_1D), SZ(SZ_Constant), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_CONSTANT_KEY));
	AddParamType(new GlFloatParamType(GL_VALUE_PARAM_KEY, SZ(SZ_Value),	0, 1, _DEF_VALUE, 0.1f));
}

GlNode* GlConstant1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlConstant1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-CONSTANT-1D
 ***************************************************************************/
_GlConstant1d::_GlConstant1d(gl_node_id nid, float v)
		: inherited(GL_CONSTANT_KEY, nid), mValue(v)
{
}

_GlConstant1d::_GlConstant1d(const _GlConstant1d& o)
		: inherited(o), mValue(o.mValue)
{
}

GlAlgo* _GlConstant1d::Clone() const
{
	return new _GlConstant1d(*this);
}

status_t _GlConstant1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	for (int32 step = 0; step < size; step++)
		line[step] = mValue;
	return B_OK;
}

status_t _GlConstant1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == GL_VALUE_PARAM_KEY) mValue = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlConstant1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == GL_VALUE_PARAM_KEY) ((GlFloatWrap&)wrap).v = mValue;
	else return B_ERROR;
	return B_OK;
}

uint32 _GlConstant1d::properties() const
{
	return CONSTANT_F;
}
