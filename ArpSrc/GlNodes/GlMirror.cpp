#include <GlPublic/GlCache2d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlChainMacro.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlMirror.h>

static const int32		GL_MIRROR_KEY		= 'ApMr';
static const int32		_HRZ_KEY			= 'hrz_';
static const int32		_HRZ_INDEX			= 0;
static const int32		_VRT_KEY			= 'vrt_';
static const int32		_VRT_INDEX			= 1;

/***************************************************************************
 * GL-MIRROR-2D
 ****************************************************************************/
class _GlPtrnFade
{
public:
	GlAlgo2dWrap	wrap;
	GlCache2d		cache;
	uint8*			data;		// Just the data from the cache
	float			sum;
	
	_GlPtrnFade() : data(0)							{ }

	status_t	Make(const GlPlanes* src, int32 w, int32 h)
	{
		if (wrap.InitCheck() != B_OK) return B_OK;
		if (cache.SetDimensions(w, h) == 0) return B_ERROR;
		data = wrap.Cache(cache, src);
		if (data) return B_OK;
		return B_ERROR;
	}
};

class GlMirror2d : public GlAlgo2d
{
public:
	GlMirror2d(gl_node_id nid, uint32 targets, GlAlgo* hrz, GlAlgo* vrt);
	GlMirror2d(const GlMirror2d& o);

	virtual GlAlgo*			Clone() const;
	virtual status_t		Process(const GlPlanes* src, uint8* plane, int32 w, int32 h,
									GlProcessStatus* status = 0);

private:
	typedef GlAlgo2d		inherited;
	_GlPtrnFade				mFade[2];
};

/***************************************************************************
  * GL-MIRROR
 ***************************************************************************/
GlMirror::GlMirror(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	SetPixelTargets(GL_PIXEL_ALL);
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_HRZ_KEY, GL_2D_IO, SZ(SZ_Horizontal), this), &added);
	if (chain && added) {
		GlChainMacro	m;
		m.Add(GL_PROMOTE_KEY).Sub(0).Add(GL_CONSTANT_KEY).SetF(GL_VALUE_PARAM_KEY, 1.0);
		m.Install(chain);
	}
	VerifyChain(new GlChain(_VRT_KEY, GL_2D_IO, SZ(SZ_Vertical), this));
}

GlMirror::GlMirror(const GlMirror& o)
		: inherited(o)
{
}

GlNode* GlMirror::Clone() const
{
	return new GlMirror(*this);
}

GlAlgo* GlMirror::Generate(const gl_generate_args& args) const
{
	GlAlgo*			hrz = GenerateChainAlgo(_HRZ_KEY, args);
	GlAlgo*			vrt = GenerateChainAlgo(_VRT_KEY, args);
	return new GlMirror2d(Id(), PixelTargets(), hrz, vrt);
}

// #pragma mark -

/***************************************************************************
 * GL-MIRROR-ADD-ON
 ***************************************************************************/
GlMirrorAddOn::GlMirrorAddOn()
		: inherited(SZI[SZI_arp], GL_MIRROR_KEY, SZ(SZ_Combine), SZ(SZ_Mirror), 1, 0)
{
}

GlNode* GlMirrorAddOn::NewInstance(const BMessage* config) const
{
	return new GlMirror(this, config);
}

// #pragma mark -

/***************************************************************************
 * GL-MIRROR-2D
 ****************************************************************************/
GlMirror2d::GlMirror2d(gl_node_id nid, uint32 targets, GlAlgo* hrz, GlAlgo* vrt)
		: inherited(nid, targets)
{
	if (hrz) SetChain(hrz, _HRZ_INDEX);
	if (vrt) SetChain(vrt, _VRT_INDEX);

	mFade[0].wrap.SetAlgo(hrz);
	mFade[1].wrap.SetAlgo(vrt);
}

GlMirror2d::GlMirror2d(const GlMirror2d& o)
		: inherited(o)
{
	mFade[0].wrap.SetAlgo(AlgoAt(_HRZ_INDEX));
	mFade[1].wrap.SetAlgo(AlgoAt(_VRT_INDEX));
}

GlAlgo* GlMirror2d::Clone() const
{
	return new GlMirror2d(*this);
}

#define _GLPTRN_GET_PIX(k, x, y, w, h) \
	(k == 0 || k == 2)			? (ARP_PIXEL((w - 1) - x, y, w)) \
		: (k == 1 || k == 3)	? (ARP_PIXEL(x, (h - 1) - y, w)) \
		: 0

status_t GlMirror2d::Process(	const GlPlanes* img, uint8* plane, int32 w, int32 h,
								GlProcessStatus* status)
{
	if (!plane) return B_OK;

	/* Make all the fade masks.
	 */
	status_t			err = B_ERROR;
	int32				pix, fadePix;
	uint32				k;
	for (k = 0; k < 2; k++) {
		if (mFade[k].Make(img, w, h) == B_OK) err = B_OK;
		mFade[k].sum = 0;
	}
	if (err != B_OK) return B_OK;
	GlMask				srcMask(plane, w, h);
	uint8*				src = srcMask.Data();
	if (!src) return B_OK;

	/* Sum the fade mask at each pixel and blend it into the original.
	 */
	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			pix = ARP_PIXEL(x, y, w);
			float		origSum = 0;
			float		fadeSum = 0;
			uint8		count = 0;

			for (k = 0; k < 2; k++) {
				if (mFade[k].data && mFade[k].data[pix] > 0) {
					mFade[k].sum = glTable256[mFade[k].data[pix]];
					origSum += (1 - mFade[k].sum);
					fadeSum += mFade[k].sum;
					count++;
				} else mFade[k].sum = 0;
			}
			if (count > 0) {
				if (count > 1) origSum /= count;
				float	scale = 1 / (origSum + fadeSum);
				float	v = src[pix] * (origSum * scale);
				for (k = 0; k < 2; k++) {
					if (mFade[k].sum > 0) {
						fadePix = _GLPTRN_GET_PIX(k, x, y, w, h);
						v += src[fadePix] * (mFade[k].sum * scale);
					}
				}
				plane[pix] = arp_clip_255(v);
			}
		}
	}
	return B_OK;
}
