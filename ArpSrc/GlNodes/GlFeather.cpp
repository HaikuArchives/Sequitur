#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlNodeVisual.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlProcessStatus.h>
#include <GlNodes/GlFeather.h>

static const int32		GL_FEATHER_KEY		= 'AFtr';

static const int32		_FILTER_KEY			= 'filt';
static const uint32		_FILTER_INDEX		= 0;
static const int32		_ATTENUATION_KEY	= 'attn';
static const uint32		_ATTENUATION_INDEX	= 1;

static const float		_INIT_REL			= 0.0;
static const int32		_INIT_ABS			= 4;
static const float		_INIT_ABS_F			= float(_INIT_ABS);

static status_t _make_kernel(	GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
								GlFilterKernel& outKernel);

/***************************************************************************
 * GL-FEATHER-ALGO
 ***************************************************************************/
class GlFeatherAlgo : public GlAlgo2d
{
public:
	GlFeatherAlgo(	gl_node_id nid, uint32 targets, float lr, float tr,
					float rr, float br, int32 la, int32 ta, int32 ra, int32 ba,
					GlAlgo* filter, GlAlgo* atten);
	GlFeatherAlgo(const GlFeatherAlgo& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* src, GlPlanes& planes,
								GlProcessStatus* status = 0);

private:
	typedef GlAlgo2d	inherited;
	float				mLr, mTr, mRr, mBr;		// Relative coords
	int32				mLa, mTa, mRa, mBa;		// Absolute coords
	
	status_t			ProcessAlgo(const GlPlanes& src, GlPlanes& dest,
									float* taps, int32 tapW, int32 tapH,
									int32 tapL, int32 tapT, int32 tapR, int32 tapB,
									uint8* atten, GlProcessStatus* status);
};

/***************************************************************************
 * GL-FEATHER
 ***************************************************************************/
GlFeather::GlFeather(const GlFeatherAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	bool			added = false;
	GlChain*		chain = VerifyChain(new GlChain(_FILTER_KEY, GL_2D_IO, SZ(SZ_Filter), this), &added);
	if (chain && added) {
		/* FIX:  It should be an ellipse shape.
		 */
		GlNode*		node = GlGlobals().NewNode(GL_ELLIPSE_KEY, 0);
		if (node) chain->AddNode(node);
	}
	VerifyChain(new GlChain(_ATTENUATION_KEY, GL_2D_IO, SZ(SZ_Attenuation), this));
}

GlFeather::GlFeather(const GlFeather& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlFeather::Clone() const
{
	return new GlFeather(*this);
}

GlAlgo* GlFeather::Generate(const gl_generate_args& args) const
{
	if (!mAddOn) return 0;
	float					lr, tr, rr, br;
	int32					la, ta, ra, ba;
	GlAlgo*					filterAlgo = 0;
	GlAlgo*					attenAlgo = 0;
	GetChainParams(args, &lr, &la, &tr, &ta, &rr, &ra, &br, &ba, &filterAlgo, &attenAlgo);
	if (!filterAlgo) {
		printf("GlFeather::Generate() no filter algo\n");
		delete attenAlgo;
		return 0;
	}
	return new GlFeatherAlgo(	Id(), PixelTargets(), lr, tr, rr, br, la, ta, ra, ba,
								filterAlgo, attenAlgo);
}

status_t GlFeather::GetChainParams(	const gl_generate_args& args,
									float* lr, int32* la, float* tr, int32* ta,
									float* rr, int32* ra, float* br, int32* ba,
									GlAlgo** filter,
									GlAlgo** attenuation) const
{
	ArpVALIDATE(lr && la && tr && ta && rr && ra && br && ba, return B_ERROR);
	ArpVALIDATE(filter, return B_ERROR);
	*filter = GenerateChainAlgo(_FILTER_KEY, args);
	if (attenuation) *attenuation = GenerateChainAlgo(_ATTENUATION_KEY, args);

	BPoint				l(_INIT_REL, _INIT_ABS_F), t(_INIT_REL, _INIT_ABS_F),
						r(_INIT_REL, _INIT_ABS_F), b(_INIT_REL, _INIT_ABS_F);
	if (mAddOn) {
		l = Params().Point(GL_LEFT_KEY);
		t = Params().Point(GL_TOP_KEY);
		r = Params().Point(GL_RIGHT_KEY);
		b = Params().Point(GL_BOTTOM_KEY);
	}
	*lr = l.x;
	*la = ARP_ROUND(l.y);
	*tr = t.x;
	*ta = ARP_ROUND(t.y);
	*rr = r.x;
	*ra = ARP_ROUND(r.y);
	*br = b.x;
	*ba = ARP_ROUND(b.y);

	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-CONVOLVE-SUM-ADD-ON
 ***************************************************************************/
GlFeatherAddOn::GlFeatherAddOn()
		: inherited(SZI[SZI_arp], GL_FEATHER_KEY, SZ(SZ_Distort), SZ(SZ_Feather), 1, 0)
{
	mL			= AddParamType(new GlPointParamType(GL_LEFT_KEY, SZ(SZ_Left), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(0, 0), BPoint(1, 128), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
	mT			= AddParamType(new GlPointParamType(GL_TOP_KEY, SZ(SZ_Top), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(0, 0), BPoint(1, 128), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
	mR			= AddParamType(new GlPointParamType(GL_RIGHT_KEY, SZ(SZ_Right), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(0, 0), BPoint(1, 128), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
	mB			= AddParamType(new GlPointParamType(GL_BOTTOM_KEY, SZ(SZ_Bottom), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(0, 0), BPoint(1, 128), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
}

GlNode* GlFeatherAddOn::NewInstance(const BMessage* config) const
{
	return new GlFeather(this, config);
}

// #pragma mark -

/***************************************************************************
 * GL-FEATHER-ALGO
 ***************************************************************************/
GlFeatherAlgo::GlFeatherAlgo(	gl_node_id nid, uint32 targets, float lr,
								float tr, float rr, float br,
								int32 la, int32 ta, int32 ra, int32 ba,
								GlAlgo* filter, GlAlgo* atten)
		: inherited(nid, targets), mLr(lr), mTr(tr), mRr(rr), mBr(br),
		  mLa(la), mTa(ta), mRa(ra), mBa(ba)
{
	ArpASSERT(filter);
	if (filter) SetChain(filter, _FILTER_INDEX);
	if (atten) SetChain(atten, _ATTENUATION_INDEX);
}

GlFeatherAlgo::GlFeatherAlgo(const GlFeatherAlgo& o)
		: inherited(o), mLr(o.mLr), mTr(o.mTr), mRr(o.mRr), mBr(o.mBr),
		  mLa(o.mLa), mTa(o.mTa), mRa(o.mRa), mBa(o.mBa)
{
}

GlAlgo* GlFeatherAlgo::Clone() const
{
	return new GlFeatherAlgo(*this);
}

status_t GlFeatherAlgo::Process(const GlPlanes* s, GlPlanes& dest,
								GlProcessStatus* status)
{
	GlAlgo2d*			filter = Algo2dAt(_FILTER_INDEX);
	GlAlgo2d*			attenAlgo = Algo2dAt(_ATTENUATION_INDEX);

	ArpVALIDATE(filter, return B_OK);
	if (dest.size < 1) return B_OK;

	uint8*				atten = 0;
	GlMask				mask;
	if (s && attenAlgo) {
		ArpVALIDATE(s->w == dest.w && s->h == dest.h, return B_OK);
		atten = mask.Make(*s, attenAlgo);
	}

	int32				l = ARP_ROUND(mLr * dest.w + mLa), t = ARP_ROUND(mTr * dest.h + mTa),
						r = ARP_ROUND(mRr * dest.w + mRa), b = ARP_ROUND(mBr * dest.h + mBa);
//printf("w %ld h %ld,  lr %f tr %f rr %f br %f,  la %ld ta %ld ra %ld ba %ld,  l %ld t %ld r %ld b %ld\n",
//		dest.w, dest.h, mLr, mTr, mRr, mBr, mLa, mTa, mRa, mBa, l, t, r, b);
	ArpVALIDATE(l >= 0 && t >= 0 && r >= 0 && b >= 0, return B_OK);
	GlFilterKernel		kernel;
	status_t			err = _make_kernel(filter, l, t, r, b, kernel);
	if (err != B_OK) return B_OK;
//kernel.Print();

	int32				tapW, tapH;
	float*				taps = kernel.LockTaps(&tapW, &tapH);
	if (!taps) return B_OK;
	int32				tapL = kernel.Left(), tapT = kernel.Top(),
						tapR = kernel.Right(), tapB = kernel.Bottom();

	GlPlanes*			src = dest.Clone();
	if (src && src->size == dest.size && src->w == dest.w && src->h == dest.h) {
		err = ProcessAlgo(*src, dest, taps, tapW, tapH, tapL, tapT, tapR, tapB, atten, status);
		delete src;
	}
	kernel.UnlockTaps(taps);
	return err;
}

status_t GlFeatherAlgo::ProcessAlgo(const GlPlanes& src, GlPlanes& dest,
									float* taps, int32 tapW, int32 tapH,
									int32 tapL, int32 tapT, int32 tapR, int32 tapB,
									uint8* attenMap, GlProcessStatus* status)
{
	ArpVALIDATE(src.size == dest.size && src.size > 0, return B_ERROR);
//printf("GlFeather algo (size %ld %ld)\n", dest.w, dest.h);
//bigtime_t		start = system_time();

	uint32				k, i;
	int32				w = dest.w, h = dest.h, srcPix = 0;

	uint32*				kArray = new uint32[dest.size];
	if (!kArray) return B_NO_MEMORY;
	uint32				kSize;
	uint8				atten = 255;
	status_t			err = B_OK;
	/* For each pixel...
	 */
//printf("Feather Algo start %p\n", status);
	if (status) status->PushPartition(h);
	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			/* Spread the source to each destination tap, scaled by the
			 * tap.  First determine which planes actually need to be
			 * operated on -- if the src is 0, it's not necessary.
			 */
			kSize = 0;
			for (k = 0; k < dest.size; k++) {
				if (src.plane[k][srcPix] > 0) {
					kArray[kSize] = k;
					kSize++;
				}
			}
			if (attenMap) atten = attenMap[srcPix];
			if (kSize > 0) {
				uint32		tap = 0;
				for (int32 tapY = -tapT; tapY <= tapB; tapY++) {
					for (int32 tapX = -tapL; tapX <= tapR; tapX++) {
						int32		destX = x + tapX, destY = y + tapY;
						ARP_SYMMETRIC_EXTENSION(destX, destY, w, h);
						ArpASSERT(int32(tap) == ((tapY + tapT) * tapW + (tapX + tapL)));
						int32		destPix = ARP_PIXEL(destX, destY, w);
						for (k = 0; k < kSize; k++) {
							i = kArray[k];
							float	t = taps[tap];
							/* If the attenuation chain had a surface,
							 * and the current surface value is less than 255,
							 * then scale the value.
							 */
							if (atten < 255) t = t * (glTable256[atten]);
							uint8	val = ARP_CLIP_255(src.plane[i][srcPix] * t);
							if (val > dest.plane[i][destPix])
								dest.plane[i][destPix] = val;
						}
						tap++;
					}
				}
			}
			srcPix++;
		}
		if (status) {
			err = status->IncSegment();
			if (err != B_OK) return err;
		}
	}
//printf("	algo: %f sec\n", double(system_time() - start) / 1000000);
	delete[] kArray;
	if (status) status->PopPartition();
//printf("Feather Algo end\n");

	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * _GL-FEATHER-VISUAL
 ***************************************************************************/
class _GlFeatherVisual : public GlNodeVisual
{
public:
	_GlFeatherVisual(	const GlRootRef& ref, GlFeather& node,
						const GlFeatherAddOn* addon);
	virtual ~_GlFeatherVisual();

	virtual void			ParamChanged(gl_param_key key);

protected:
	virtual status_t		LockedVisual(	GlNode* n, int32 w, int32 h,
											ArpBitmap** outBm, int32 index);

private:
	typedef GlNodeVisual	inherited;
	GlFeather&				mN;
	const GlFeatherAddOn*	mAddOn;
	bool					mChanged;

	status_t				MakePreview(GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
										int32 viewW, int32 viewH, ArpBitmap** outBm) const;
};

_GlFeatherVisual::_GlFeatherVisual(	const GlRootRef& ref, GlFeather& node,
									const GlFeatherAddOn* addon)
		: inherited(ref, node.Id()), mN(node), mAddOn(addon), mChanged(false)
{
	mN.IncRefs();
}

_GlFeatherVisual::~_GlFeatherVisual()
{
	mN.DecRefs();
}

void _GlFeatherVisual::ParamChanged(gl_param_key key)
{
	inherited::ParamChanged(key);
	mChanged = true;
}

status_t _GlFeatherVisual::LockedVisual(GlNode* n, int32 w, int32 h,
										ArpBitmap** outBm, int32 index)
{
	ArpVALIDATE(n && outBm && w > 0 && h > 0, return B_ERROR);

	float				lr, tr, rr, br;
	int32				la, ta, ra, ba;
	GlAlgo*				filterAlgo = 0;
	gl_generate_args	args;
	mN.GetChainParams(args, &lr, &la, &tr, &ta, &rr, &ra, &br, &ba, &filterAlgo, 0);
	if (!filterAlgo) return B_ERROR;
	status_t			err = B_ERROR;
	/* Default to a size of 50 x 50, in case coords are only virtual.
	 */
	int32				vw = 50, vh = 50;
	int32				l = int32(lr * vw + la), t = int32(tr * vh + ta),
						r = int32(rr * vw + ra), b = int32(br * vh + ba);
// FIX: Temp!  Get the wrap written!
	GlAlgo2d*			a2d = filterAlgo->As2d();
	if (a2d) err = MakePreview(a2d, l, t, r, b, w, h, outBm);
	delete filterAlgo;
	mChanged = false;
	return err;
}

status_t _GlFeatherVisual::MakePreview(	GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
										int32 viewW, int32 viewH, ArpBitmap** outBm) const
{
	ArpVALIDATE(viewW > 0 && viewH > 0, return B_ERROR);
	GlFilterKernel	kernel;
	status_t		err = _make_kernel(s, l, t, r, b, kernel);
	if (err != B_OK) return err;
	int32			tapW, tapH, pix;
	float*			taps = kernel.LockTaps(&tapW, &tapH);
	if (!taps) return B_ERROR;
	int32			newW, newH;
	gl_scale_to_dimens(tapW, tapH, viewW, viewH, &newW, &newH);
	ArpASSERT(newW > 0 && newH > 0);
	if (newW <= 0 || newH <= 0) {
		kernel.UnlockTaps(taps);
		return B_ERROR;
	}
	/* Actually, this really shouldn't be necessary, low and high
	 * are supplied as args.
	 */
	float			low, high;
	low = high = taps[0];
	for (pix = 0; pix < tapW * tapH; pix++) {
		if (taps[pix] > high) high = taps[pix];
		if (taps[pix] < low) low = taps[pix];
	}
	float			range = high - low;
	
	GlImage*		img = new GlImage(newW, newH);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return B_NO_MEMORY;
	}

	GlPlanes*		pixels = img->LockPixels(GL_PIXEL_RGBA);
	if (pixels && pixels->w == newW && pixels->h == newH) {
		double		dx = tapW / double(newW),
					dy = tapH / double(newH),
					py = 0.0, scale = 255 / range;
		for (int32 y = 0; y < newH; y++) {
			double	px = 0.0;
			for (int32 x = 0; x < newW; x++) {
				int32	srcPix = ARP_PIXEL(int32(px), int32(py), tapW),
						destPix = ARP_PIXEL(x, y, newW);
				ArpASSERT(srcPix >=0 && srcPix < tapW * tapH);
				uint8	val = arp_clip_255(0 - low + (taps[srcPix] * scale));
				pixels->r[destPix] = pixels->g[destPix] = pixels->b[destPix] = val;
				pixels->a[destPix] = 255;
				px += dx;
			}
			py += dy;
		}
	}
	img->UnlockPixels(pixels);
	
	kernel.UnlockTaps(taps); 
	*outBm = img->AsBitmap();
	delete img;
	if (!(*outBm)) return B_NO_MEMORY;
	return B_OK;
}

// #pragma mark -

GlNodeVisual* GlFeather::NewVisual(const GlRootRef& ref) const
{
	return new _GlFeatherVisual(ref, (GlFeather&)(*this), mAddOn);
}

// #pragma mark -

/***************************************************************************
 * Misc
 ***************************************************************************/

static status_t _make_kernel(	GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
								GlFilterKernel& outKernel)
{
	ArpVALIDATE(s, return B_ERROR);
	/* Process 2D.
	 */
	status_t		err = outKernel.Init(l, t, r, b);
	if (err != B_OK) return err;
	int32			w = l + r + 1,
					h = t + b + 1;
	
	GlPlanes		dest(w, h, 1);
	if (dest.size < 1) return B_NO_MEMORY;
	memset(dest.plane[0], 255, w * h);
	s->Process((const GlPlanes*)0, dest);
	/* Translate into floating kernel.
	 */
	int32			kW, kH;
	float*			taps = outKernel.LockTaps(&kW, &kH);
	if (!taps || w != kW || h != kH) return B_ERROR;

	for (int32 pix = 0; pix < w * h; pix++) taps[pix] = glTable256[dest.plane[0][pix]];
#if 0
	float			scale = ARP_ABS(high - low) / 256.;
	for (int32 pix = 0; pix < w * h; pix++) taps[pix] = low + (dest.plane[0][pix] * scale);
#endif	
	outKernel.UnlockTaps(taps);
	return B_OK;
}
