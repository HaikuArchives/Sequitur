#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlPivot1d.h>

static const int32		GL_PIVOT_KEY	= 'AmPv';
static const int32		ANGLE_KEY		= '<___';
static const int32		LOC_KEY			= 'loc_';

/***************************************************************************
 * _GL-PIVOT-1D
 ***************************************************************************/
class _GlPivot1d : public GlAlgo1d
{
public:
	_GlPivot1d(gl_node_id nid, float a, const BPoint& pt);
	_GlPivot1d(const _GlPivot1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;
	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d		inherited;
	float				mAngle, mSlope;
	BPoint				mPt;
};

/***************************************************************************
  * GL-PIVOT-1D
 ***************************************************************************/
GlPivot1d::GlPivot1d(const GlPivot1dAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlPivot1d::GlPivot1d(const GlPivot1d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlPivot1d::Clone() const
{
	return new GlPivot1d(*this);
}

GlAlgo* GlPivot1d::Generate(const gl_generate_args& args) const
{
	return new _GlPivot1d(	Id(), Params().Float(ANGLE_KEY),
							Params().Point(LOC_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-PIVOT-ADD-ON
 ***************************************************************************/
GlPivot1dAddOn::GlPivot1dAddOn()
		: inherited(SZI[SZI_arp], GL_PIVOT_KEY, SZ(SZ_1D), SZ(SZ_Pivot), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_PIVOT_KEY));
	mAngle		= AddParamType(new GlFloatParamType(ANGLE_KEY, SZ(SZ_Angle), 0.0f, 180.0f, 45.0f, 0.1f));
	mLoc		= AddParamType(new GlPointParamType(LOC_KEY, SZ(SZ_Fulcrum), SZ(SZ_x), SZ(SZ_y), BPoint(0, 0), BPoint(1, 1), BPoint(0.5f, 0.5f), 0.1f));
}

GlNode* GlPivot1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlPivot1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-PIVOT-1D
 ***************************************************************************/
#define DTOR		0.017453f	/* convert degrees to radians */

_GlPivot1d::_GlPivot1d(gl_node_id nid, float a, const BPoint& pt)
		: inherited(GL_PIVOT_KEY, nid), mAngle(a), mPt(pt)
{
	if (mAngle <= 0 || mAngle >= 180) mSlope = 0;
	else if (mAngle == 45) mSlope = 1;
	else {
		float		radians = mAngle * DTOR;	// Find radians
		mSlope = tan(radians);					// Find tangent
	}
}

_GlPivot1d::_GlPivot1d(const _GlPivot1d& o)
		: inherited(o), mAngle(o.mAngle), mSlope(o.mSlope), mPt(o.mPt)
{
}

GlAlgo* _GlPivot1d::Clone() const
{
	return new _GlPivot1d(*this);
}

status_t _GlPivot1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	for (int32 step = 0; step < size; step++) {
		float	v = (flags&ALGO_HEAD_F) ? GL_1D_ENV_STEP(step, size) : line[step];
		if (mAngle == 90) {
			if (v >= mPt.x) v = 1;
			else v = 0;
		} else {
			v += 0.5f - mPt.x;
			v = 0.5f + mSlope * (v - 0.5f);
			v += 0.5f - mPt.y;
			if (v < 0) v = 0;
			else if (v > 1) v = 1;
		}
		line[step] = v;
	}
	return B_OK;
}

status_t _GlPivot1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (key.key == ANGLE_KEY && wrap.Type() == GL_FLOAT_TYPE)
		mAngle = ((const GlFloatWrap&)wrap).v;
	else if (key.key == LOC_KEY && wrap.Type() == GL_POINT_TYPE)
		mPt = ((const GlPointWrap&)wrap).v;
	else return B_ERROR;

	return B_OK;
}

status_t _GlPivot1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (key.key == ANGLE_KEY && wrap.Type() == GL_FLOAT_TYPE)
		((GlFloatWrap&)wrap).v = mAngle;
	else if (key.key == LOC_KEY && wrap.Type() == GL_POINT_TYPE)
		((GlPointWrap&)wrap).v = mPt;
	else return B_ERROR;

	return B_OK;
}
