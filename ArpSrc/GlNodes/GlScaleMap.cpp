#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlScaleMap.h>

static const int32		GL_SCALE_KEY	= 'AmSc';
static const int32		LOW_KEY			= 'low_';
static const int32		HIGH_KEY		= 'high';

/***************************************************************************
 * _GL-SCALE-MAP
 ***************************************************************************/
class _GlScaleMap : public GlAlgo1d
{
public:
	_GlScaleMap(gl_node_id nid, float l, float h);
	_GlScaleMap(const _GlScaleMap& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;
	virtual status_t	ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual uint32		properties() const			{ return APPLY_TO_2D_F; }

private:
	typedef GlAlgo1d	inherited;
	float				mLow, mHigh;
};

/***************************************************************************
  * GL-SCALE-MAP
 ***************************************************************************/
GlScaleMap::GlScaleMap(const GlScaleMapAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlScaleMap::GlScaleMap(const GlScaleMap& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlScaleMap::Clone() const
{
	return new GlScaleMap(*this);
}

GlAlgo* GlScaleMap::Generate(const gl_generate_args& args) const
{
	return new _GlScaleMap(	Id(), Params().Float(LOW_KEY),
							Params().Float(HIGH_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-SCALE-MAP-ADD-ON
 ***************************************************************************/
GlScaleMapAddOn::GlScaleMapAddOn()
		: inherited(SZI[SZI_arp], GL_SCALE_KEY, SZ(SZ_1D), SZ(SZ_Scale), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_SCALE_KEY));
	mLow		= AddParamType(new GlFloatParamType(LOW_KEY, SZ(SZ_Low), 0.0f, 1.0f, 0.2f, 0.1f));
	mHigh		= AddParamType(new GlFloatParamType(HIGH_KEY, SZ(SZ_High), 0.0f, 1.0f, 0.8f, 0.1f));
}

GlNode* GlScaleMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlScaleMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SCALE-MAP
 ***************************************************************************/
_GlScaleMap::_GlScaleMap(gl_node_id nid, float l, float h)
		: inherited(GL_SCALE_KEY, nid), mLow(l), mHigh(h)
{
}

_GlScaleMap::_GlScaleMap(const _GlScaleMap& o)
		: inherited(o), mLow(o.mLow), mHigh(o.mHigh)
{
}

GlAlgo* _GlScaleMap::Clone() const
{
	return new _GlScaleMap(*this);
}

status_t _GlScaleMap::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	float		min = 2, max = -2;
	int32		step;

	for (step = 0; step < size; step++) {
		if (line[step] < min) min = line[step];
		if (line[step] > max) max = line[step];
	}

	float		oldR = max - min, newR = mHigh - mLow;
	if (oldR <= 0) {
		float	newV = (min < mLow) ? mLow : (max > mHigh) ? mHigh : min;
		for (step = 0; step < size; step++) line[step] = newV;
		return B_OK;
	}
	
	for (step = 0; step < size; step++) {
		line[step] = arp_clip_1(mLow + (( (line[step] - min) * newR) / oldR));
	}

	return B_OK;
}

status_t _GlScaleMap::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == LOW_KEY) mLow = ((const GlFloatWrap&)wrap).v;
	else if (key.key == HIGH_KEY) mHigh = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlScaleMap::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == LOW_KEY) ((GlFloatWrap&)wrap).v = mLow;
	else if (key.key == HIGH_KEY) ((GlFloatWrap&)wrap).v = mHigh;
	else return B_ERROR;
	return B_OK;
}

status_t _GlScaleMap::ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const
{
	ArpASSERT(dest.w > 0 && dest.h > 0 && dest.size > 0);
	/* Right now I'll scale based on all the planes -- maybe I'll want to
	 * do it per plane in the end, not sure yet.
	 */
	int32		pix, size = dest.w * dest.h;
	uint8		min = 255, max = 0;
	uint32		k;
	for (pix = 0; pix < size; pix++) {
		for (k = 0; k < dest.size; k++) {
			if (dest.plane[k][pix] < min) min = dest.plane[k][pix];
			if (dest.plane[k][pix] > max) max = dest.plane[k][pix];
		}
	}

	float		low = mLow * 255, high = mHigh * 255;
	float		oldR = float(max - min), newR = float(high - low);

	for (pix = 0; pix < size; pix++) {
		for (k = 0; k < dest.size; k++) {
			dest.plane[k][pix] = arp_clip_255(low + (((dest.plane[k][pix] - min) * newR) / oldR));
		}
	}
	return B_OK;
}
