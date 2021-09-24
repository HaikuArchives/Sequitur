#include <cstdio>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlPinkNoise1d.h>

static const int32			AMOUNT_KEY		= 'amnt';

/***************************************************************************
 * _GL-PINK-NOISE-1D
 ***************************************************************************/
class _GlPinkNoise1d : public GlAlgo1d
{
public:
	float		amount;

	_GlPinkNoise1d(gl_node_id nid, float a);
	_GlPinkNoise1d(const _GlPinkNoise1d& o);

	virtual GlAlgo*		Clone() const;
	virtual status_t	Promote2d(	uint8* mask, int32 w, int32 h,
									const GlAlgo1d* y1d) const;

	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;
	virtual status_t	ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const;

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual bool		set_step(float v);
	virtual uint32		properties() const			{ return RANDOM_F | APPLY_TO_2D_F; }

private:
	typedef GlAlgo1d	inherited;
	int32				mMaxKey;
	mutable int32		mKey;
	mutable int32		mWhiteValues[5];
	int32				mRange;

	void SetRange(int32 range);
	int32 GetNextValue() const;
};

/***************************************************************************
  * GL-PINK-NOISE-1D
 ***************************************************************************/
GlPinkNoise1d::GlPinkNoise1d(	const GlNode1dAddOn* addon,
								const BMessage* config)
		: inherited(addon, config)
{
}

GlPinkNoise1d::GlPinkNoise1d(const GlPinkNoise1d& o)
		: inherited(o)
{
}

GlNode* GlPinkNoise1d::Clone() const
{
	return new GlPinkNoise1d(*this);
}

GlAlgo* GlPinkNoise1d::Generate(const gl_generate_args& args) const
{
	return new _GlPinkNoise1d(Id(), Params().Float(AMOUNT_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-PINK-NOISE-1D-ADD-ON
 ***************************************************************************/
GlPinkNoise1dAddOn::GlPinkNoise1dAddOn()
		: inherited(SZI[SZI_arp], GL_PINK_NOISE_KEY, SZ(SZ_1D), SZ(SZ_Pink_Noise), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_PINK_NOISE_KEY));
	AddParamType(new GlFloatParamType(AMOUNT_KEY, SZ(SZ_Amount), 0, 1, 0.5f, 0.1f));
}

GlNode* GlPinkNoise1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlPinkNoise1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-PINK-NOISE-MAP
 ***************************************************************************/
_GlPinkNoise1d::_GlPinkNoise1d(gl_node_id nid, float a)
		: inherited(GL_PINK_NOISE_KEY, nid, GL_NO_TOKEN, 0.5), amount(a)
{
	SetRange(256);
}

_GlPinkNoise1d::_GlPinkNoise1d(const _GlPinkNoise1d& o)
		: inherited(o), amount(o.amount)
{
	SetRange(o.mRange);
}

GlAlgo* _GlPinkNoise1d::Clone() const
{
	return new _GlPinkNoise1d(*this);
}

status_t _GlPinkNoise1d::Promote2d(uint8* mask, int32 w, int32 h,
									const GlAlgo1d* y1d) const
{
	ArpVALIDATE(mask && y1d, return B_ERROR);
	if (Key() != y1d->Key()) return B_ERROR;

	int32		size = w * h;
	for (int32 pix = 0; pix < size; pix++) {
		if (mask[pix] > 0) {
			uint8		val = arp_clip_255(GetNextValue());
			mask[pix] = ARP_MIN(mask[pix], val);
		}
	}
	return B_OK;
}

status_t _GlPinkNoise1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	for (int32 step = 0; step < size; step++) {
		float		pink = ((GetNextValue() / 255) - 0.5f) * 2 * amount;
		if (line[step] + pink < 0) line[step] -= pink;
		else if (line[step] + pink > 1) line[step] -= pink;
		else line[step] += pink;
		line[step] = arp_clip_1(line[step]);

	}
	return B_OK;
}

status_t _GlPinkNoise1d::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == AMOUNT_KEY) amount = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlPinkNoise1d::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == AMOUNT_KEY) ((GlFloatWrap&)wrap).v = amount;
	else return B_ERROR;
	return B_OK;
}

bool _GlPinkNoise1d::set_step(float v)
{
	return true;
}

status_t _GlPinkNoise1d::ApplyTo2d(GlPlanes& dest, GlProcessStatus* status) const
{
	int32			pix, size = dest.w * dest.h;
	for (pix = 0; pix < size; pix++) {
		float		pink = ((GetNextValue() / 255.0f) - 0.5f) * 2 * amount;
		int32		offset = int32(pink * 255);
		if (offset != 0) {
			for (uint32 k = 0; k < dest.size; k++) {
				dest.plane[k][pix] = arp_clip_255(dest.plane[k][pix] + offset);
			}
		}
	}
	return B_OK;
}

void _GlPinkNoise1d::SetRange(int32 range)
{
	mMaxKey = 0x1f; // Five bits set
	this->mRange = range;
	mKey = 0;
	for (int32 i = 0; i < 5; i++)
		mWhiteValues[i] = rand() % (mRange/5);
}

int32 _GlPinkNoise1d::GetNextValue() const
{
	int32			last_key = mKey;
	int32			sum;
	mKey++;
	if (mKey > mMaxKey) mKey = 0;
	// Exclusive-Or previous value with current value. This gives
	// a list of bits that have changed.
	int32			diff = last_key ^ mKey;
	sum = 0;
	for (int32 i = 0; i < 5; i++) {
		// If bit changed get new random number for corresponding
		// white_value
		if (diff & (1 << i))
			mWhiteValues[i] = rand() % (mRange/5);
		sum += mWhiteValues[i];
	}
	return sum;
}
