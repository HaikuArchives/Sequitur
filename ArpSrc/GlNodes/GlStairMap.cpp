#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlStairMap.h>

static const int32			STEPS_KEY		= '#___';
static const int32			MAX_STEPS		= 255;

/***************************************************************************
 * GL-STAIR-ALGO
 ***************************************************************************/
class GlStairAlgo : public GlAlgo1d
{
public:
	GlStairAlgo(gl_node_id nid, float s);
	GlStairAlgo(const GlStairAlgo& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;
	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo1d	inherited;
	float				mSteps;
};

/***************************************************************************
 * GL-STAIR-MAP
 ***************************************************************************/
GlStairMap::GlStairMap(const GlStairMapAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlStairMap::GlStairMap(const GlStairMap& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlStairMap::Clone() const
{
	return new GlStairMap(*this);
}

GlAlgo* GlStairMap::Generate(const gl_generate_args& args) const
{
	return new GlStairAlgo(Id(), float(Params().Int32(STEPS_KEY)));
}

// #pragma mark -

/***************************************************************************
 * GL-STAIR-MAP-ADD-ON
 ***************************************************************************/
GlStairMapAddOn::GlStairMapAddOn()
		: inherited(SZI[SZI_arp], GL_STAIR_KEY, SZ(SZ_1D), SZ(SZ_Stair), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_STAIR_KEY));
	mSteps		= AddParamType(new GlInt32ParamType(STEPS_KEY, SZ(SZ_Steps), 1, MAX_STEPS, 10));
}

GlNode* GlStairMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlStairMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * GL-STAIR-ALGO
 ***************************************************************************/
GlStairAlgo::GlStairAlgo(gl_node_id nid, float s)
		: inherited(GL_STAIR_KEY, nid), mSteps(s)
{
}

GlStairAlgo::GlStairAlgo(const GlStairAlgo& o)
		: inherited(o), mSteps(o.mSteps)
{
}

GlAlgo* GlStairAlgo::Clone() const
{
	return new GlStairAlgo(*this);
}

static inline int32 _float_mod(float v, float scale)
{
	int32	c = 0;
	while ((v -= scale) > 0) c++;
	return c;
}

status_t GlStairAlgo::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	float		scale = 1 / mSteps;
	for (int32 step = 0; step < size; step++) {
		line[step] = float(arp_clip_1(_float_mod(line[step], scale) / mSteps));
	}
	return B_OK;
}

status_t GlStairAlgo::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_INT32_TYPE) return B_ERROR;
	
	if (key.key == STEPS_KEY) mSteps = float(((const GlInt32Wrap&)wrap).v);
	else return B_ERROR;
	return B_OK;
}

status_t GlStairAlgo::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_INT32_TYPE) return B_ERROR;

	if (key.key == STEPS_KEY) ((GlInt32Wrap&)wrap).v = ARP_ROUND(mSteps);
	else return B_ERROR;
	return B_OK;
}
