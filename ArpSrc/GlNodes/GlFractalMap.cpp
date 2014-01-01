#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamList.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPseudoRandomizer.h>
#include <GlNodes/GlFractalMap.h>

static const int32			_GL_FRACTAL_KEY	= 'AmFr';
static const int32			START_KEY		= 'strt';
static const int32			END_KEY			= 'end_';
static const int32			RUG_KEY			= 'rugd';

static bool _in_range(	float* line, uint32 size);
static void _frac_line(	float* line, int32 a, int32 b, float rug, float size,
						GlPseudoRandomizer& pr);
static void _frac_mask(	float* mask, int32 w, int32 h, int32 il, int32 jl,
						int32 ih, int32 jh, GlPseudoRandomizer& pr, float rug);
static float _find_frac(float pos1, float pos2, float val1, float val2,
						float rug, GlPseudoRandomizer& pr,
						float end, float tolerance);

/***************************************************************************
 * _GL-FRACTAL-1D
 ***************************************************************************/
class _GlFractalMap : public GlAlgo1d
{
public:
	_GlFractalMap(	float s, float e, float rug, float rnd, float detail,
					gl_node_id nid);
	_GlFractalMap(const _GlFractalMap& o);
	virtual ~_GlFractalMap();

	virtual GlAlgo*		Clone() const;

	virtual status_t	Promote2d(	uint8* mask, int32 w, int32 h,
									const GlAlgo1d* y1d) const;

	float				Start() const;
	float				End() const;

	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

protected:
	virtual uint32		properties() const					{ return INFINITE_F; }

private:
	typedef GlAlgo1d		inherited;
	float				mStart, mEnd, mRug, mRnd, mDetail;

	/* This gets called when the at arg is present in Algo().  It goes
	 * through and it finds the value at each at point -- really ugly,
	 * but only intended to be used when the user is requesting a single
	 * at value -- i.e., a single point in the fractal line.
	 */
	status_t			AlgoAt(float* line, float* at, int32 size, uint32 flags) const;
};

/***************************************************************************
  * GL-FRACTAL-1D
 ***************************************************************************/
GlFractalMap::GlFractalMap(const GlFractalMapAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlFractalMap::GlFractalMap(const GlFractalMap& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlFractalMap::Clone() const
{
	return new GlFractalMap(*this);
}

GlAlgo* GlFractalMap::Generate(const gl_generate_args& args) const
{
	return new _GlFractalMap(Params().Float(START_KEY),
							Params().Float(END_KEY),
							Params().Float(RUG_KEY),
							Params().Float('rndm'),
							Params().Float('detl'),
							Id());
}

// #pragma mark -

/***************************************************************************
 * GL-FRACTAL-ADD-ON
 ***************************************************************************/
GlFractalMapAddOn::GlFractalMapAddOn()
		: inherited(SZI[SZI_arp], _GL_FRACTAL_KEY, SZ(SZ_1D), SZ(SZ_Fractal), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(_GL_FRACTAL_KEY));
	mStart		= AddParamType(new GlFloatParamType(START_KEY, SZ(SZ_Start),		0, 1, 0.5f,  0.1f));
	mEnd		= AddParamType(new GlFloatParamType(END_KEY, SZ(SZ_End),		0, 1, 0.5f,  0.1f));
	mRug		= AddParamType(new GlFloatParamType(RUG_KEY, SZ(SZ_Ruggedness), 0, 1, 0.25f, 0.01f));
	mRnd		= AddParamType(new GlFloatParamType('rndm', SZ(SZ_Random), 	0, 1, 0.1f,  0.01f));
	mDetail		= AddParamType(new GlFloatParamType('detl', SZ(SZ_Detail), 	0, 0.1f, 0.001f,  0.01f));
}

GlNode* GlFractalMapAddOn::NewInstance(const BMessage* config) const
{
	return new GlFractalMap(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-FRACTAL-1D
 ***************************************************************************/
_GlFractalMap::_GlFractalMap(	float s, float e, float rug, float rnd,
								float detail, gl_node_id nid)
		: inherited(_GL_FRACTAL_KEY, nid), mStart(s), mEnd(e), mRug(rug),
		  mRnd(rnd), mDetail(detail)
{
}

_GlFractalMap::_GlFractalMap(const _GlFractalMap& o)
		: inherited(o), mStart(o.mStart), mEnd(o.mEnd), mRug(o.mRug),
		  mRnd(o.mRnd), mDetail(o.mDetail)
{
}

_GlFractalMap::~_GlFractalMap()
{
}

GlAlgo* _GlFractalMap::Clone() const
{
	return new _GlFractalMap(*this);
}

status_t _GlFractalMap::Promote2d(	uint8* mask, int32 w, int32 h,
									const GlAlgo1d* y1d) const
{
	ArpVALIDATE(mask && y1d, return B_ERROR);
	if (Key() != y1d->Key()) return B_ERROR;
	
	float*				m = new float[w * h];
	int32				pix;
	float				sy = ((const _GlFractalMap*)y1d)->Start(),
						ey = ((const _GlFractalMap*)y1d)->End();
	if (!m) return B_NO_MEMORY;
	for (pix = 0; pix < w * h; pix++) m[pix] = -10000;
	m[0]							= (mStart + sy) / 2;
	m[w - 1]						= (mEnd + sy) / 2;
	m[ARP_PIXEL(0, h - 1, w)]		= (mStart + ey) / 2;
	m[ARP_PIXEL(w - 1, h - 1, w)]	= (mEnd + ey) / 2;
	GlPseudoRandomizer	pr(uint32(mRnd * 10000));
	_frac_mask(m, w, h, 0, 0, w - 1, h - 1, pr, mRug);

	for (pix = 0; pix < w * h; pix++) {
		if (m[pix] < 0) mask[pix] = 0;
		else if (m[pix] > 1) mask[pix] = ARP_MIN(mask[pix], 255);
		else {
			uint8		v = arp_clip_255(m[pix] * 255);
			mask[pix] = ARP_MIN(mask[pix], v);
		}
	}
	delete[] m;	
	return B_OK;
}

float _GlFractalMap::Start() const
{
	return mStart;
}

float _GlFractalMap::End() const
{
	return mEnd;
}

status_t _GlFractalMap::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	ArpVALIDATE(line && size > 0, return B_OK);
	if (at) return AlgoAt(line, at, size, flags);
	if (size == 1) {
		line[0] = (mStart + mEnd) * 0.5f;
		return B_OK;
	}
	
	int32				k;
	if (mRug <= 0) {
		for (k = 0; k < size; k++) line[k] *= 0.5;
		return B_OK;
	}
	GlPseudoRandomizer	pr(uint32(mRnd * 10000));
	line[0] *= mStart;
	line[size - 1] *= mEnd;
	_frac_line(line, 0, size - 1, mRug, float(size), pr);

	for (k = 0; k < size; k++) {
		if (line[k] < 0) line[k] = 0.;
		else if (line[k] > 1) line[k] = 1.;
	}
	ArpASSERT(_in_range(line, size));
	return B_OK;
}

#if 0
	virtual void		UpdateFrom(GlParamList& params);

void _GlFractalMap::UpdateFrom(GlParamList& params)
{
	GlFloatWrap		wrap;
	if (params.GetValue(0, START_KEY, wrap) == B_OK) mStart = wrap.f;
	if (params.GetValue(0, END_KEY, wrap) == B_OK) mEnd = wrap.f;
	if (params.GetValue(0, RUG_KEY, wrap) == B_OK) mRug = wrap.f;
}
#endif

status_t _GlFractalMap::AlgoAt(float* line, float* at, int32 size, uint32 flags) const
{
	GlPseudoRandomizer	pr(uint32(mRnd * 10000));
	for (int32 step = 0; step < size; step++) {
		float		v = _find_frac(0, 1, mStart, mEnd, mRug, pr, at[step], mDetail);
		line[step] *= v;
	}
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * Fractal algo
 ***************************************************************************/
static bool _in_range(float* line, uint32 size)
{
	for (uint32 k = 0; k < size; k++) {
		if (line[k] < 0 || line[k] > 1) {
			printf("** %ld: %f\n", k, line[k]);
			return false;
		}
	}
	return true;
}

static void _frac_line(float* line, int32 a, int32 b, float rug, float size, GlPseudoRandomizer& pr)
{
	if (b - a <= 0) return;
	int32		mid = (a + b) / 2;
	if (mid <= a || mid >= b) return;
	line[mid] *= (line[a] + line[b]) / 2 + ((b / size - a / size) * ((pr.Next() * 2 - 1) * rug));
	_frac_line(line, a, mid, rug, size, pr);
	_frac_line(line, mid, b, rug, size, pr);
}

// l = low, m = mid, h = high
static void _frac_mask(	float* mask, int32 w, int32 h, int32 il, int32 jl, int32 ih, int32 jh,
						GlPseudoRandomizer& pr, float rug)
{
	int32		im = (il + ih + 1) / 2, jm = (jl + jh + 1) / 2;
	int32		iljl = ARP_PIXEL(il, jl, w), iljh = ARP_PIXEL(il, jh, w),
				ihjl = ARP_PIXEL(ih, jl, w), ihjh = ARP_PIXEL(ih, jh, w);
	if (jm < jh) {
		int32	iljm = ARP_PIXEL(il, jm, w), ihjm = ARP_PIXEL(ih, jm, w);
		if (mask[iljm] < -9000)
			mask[iljm] = (mask[iljl] + mask[iljh]) / 2 + (float(jh) / h - float(jl) / h) * ((pr.Next() * 2 - 1) * rug);
		if (il < ih)
			mask[ihjm] = (mask[ihjl] + mask[ihjh]) / 2 + (float(jh) / h - float(jl) / h) * ((pr.Next() * 2 - 1) * rug);
	}
	if (im < ih) {
		int32	imjl = ARP_PIXEL(im, jl, w), imjh = ARP_PIXEL(im, jh, w);
		if (mask[imjl] < -9000)
			mask[imjl] = (mask[iljl] + mask[ihjl]) / 2 + (float(ih) / w - float(il) / w) * ((pr.Next() * 2 - 1) * rug);
		if (jl < jh)
			mask[imjh] = (mask[iljh] + mask[ihjh]) / 2 + (float(ih) / w - float(il) / w) * ((pr.Next() * 2 - 1) * rug);
	}
	if (im < ih && jm < jh) {
		int32	imjm = ARP_PIXEL(im, jm, w);
		mask[imjm] = (mask[iljl] + mask[ihjl] + mask[iljh] + mask[ihjh]) / 4
						+ (ARP_ABS(float(ih) / w - float(il) / w) + ARP_ABS(float(jh) / h - float(jh) / h)) * ((pr.Next() * 2 - 1) * rug);
	}
	if (im < ih || jm < jh) {
		_frac_mask(mask, w, h, il, jl, im, jm, pr, rug);
		_frac_mask(mask, w, h, il, jm, im, jh, pr, rug);
		_frac_mask(mask, w, h, im, jl, ih, jm, pr, rug);
		_frac_mask(mask, w, h, im, jm, ih, jh, pr, rug);
	}
}

static float _find_frac(float pos1, float pos2, float val1, float val2,
						float rug, GlPseudoRandomizer& pr,
						float end, float tolerance)
{
	ArpASSERT(pos2 - pos1 > 0);
	float		m = (pos1 + pos2) / 2;
	float		v = (val1 + val2) / 2 + ((pos2 - pos1) * ((pr.Next() * 2 - 1) * rug));
	if (ARP_ABS(m - end) <= tolerance) return v;
	if (end < m) return _find_frac(pos1, m, val1, v, rug, pr, end, tolerance);
	else return _find_frac(m, pos2, v, val2, rug, pr, end, tolerance);
}
