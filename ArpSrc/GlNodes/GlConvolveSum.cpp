#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlAlgoNbrInput.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
//#include "GlNodes/GlBinaryOpNbr.h"
#include <GlNodes/GlConvolveSum.h>
//#include "GlNodes/GlSumNmb.h"

static const int32		GL_CONVOLVE_KEY		= 'ApCs';
static const int32		_LT_KEY				= 'lt__';
static const int32		_RB_KEY				= 'rb__';
static const int32		_LOW_KEY			= 'low_';
static const int32		_HIGH_KEY			= 'hi__';

static const uint32		_VALUES_INPUT		= 1;
static const uint32		_WIDTH_INPUT		= 2;
static const uint32		_HEIGHT_INPUT		= 3;

static const int32		_VALUE_KEY			= 'val_';
static const int32		_VALUE_INDEX		= 0;
static const int32		_FILTER_KEY			= 'fltr';

/***************************************************************************
 * GL-CONVOLVE-ALGO
 ***************************************************************************/
class GlConvolveAlgo : public GlAlgo2d
{
public:
	GlConvolveAlgo(	gl_node_id nid, uint32 targets, GlFilterKernel& kernel,
					int32 macro, GlAlgo* value);
	GlConvolveAlgo(const GlConvolveAlgo& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status = 0);

private:
	typedef GlAlgo2d	inherited;
	GlFilterKernel		mKernel;
	int32				mMacro;

	status_t			ProcessAlgo(const GlPlanes& src, GlPlanes& dest,
									float* taps, int32 tapW, int32 tapH,
									int32 tapL, int32 tapT, int32 tapR, int32 tapB);
	status_t			ProcessBlur(const GlPlanes& src, GlPlanes& dest,
									float* taps, int32 tapW, int32 tapH,
									int32 tapL, int32 tapT, int32 tapR, int32 tapB);
};

/***************************************************************************
 * GL-CONVOLVE-SUM
 ***************************************************************************/
GlConvolveSum::GlConvolveSum(const GlConvolveSumAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon), mChanged(false)
{
	bool				added = false;
	GlChain*			chain = VerifyChain(new GlChain(_VALUE_KEY, GL_NUMBER_IO, SZ(SZ_Value), this), &added);
	if (chain && added && addon) addon->mBlur1.Install(chain);

	chain = VerifyChain(new GlChain(_FILTER_KEY, GL_2D_IO, SZ(SZ_Filter), this), &added);
	if (chain && added) {
		GlChainMacro	m;
		m.Add(GL_INVERT_KEY).Sub(0).Add(GL_CONSTANT_KEY).SetF(GL_VALUE_PARAM_KEY, 1.0);
		m.Install(chain);
	}
}

GlConvolveSum::GlConvolveSum(const GlConvolveSum& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlConvolveSum::Clone() const
{
	return new GlConvolveSum(*this);
}

status_t GlConvolveSum::ParamChanged(gl_param_key key)
{
	mChanged = true;
	return inherited::ParamChanged(key);
}

GlAlgo* GlConvolveSum::Generate(const gl_generate_args& args) const
{
	/* Make the filter kernel
	 */
	GlAlgo2d*				filter = GenerateChain2d(_FILTER_KEY, args);
	if (!filter) return 0;

	BPoint				lt = Params().Point(_LT_KEY),
						rb = Params().Point(_RB_KEY);
	float				low = Params().Float(_LOW_KEY),
						high = Params().Float(_HIGH_KEY);

	if (args.flags&GL_NODE_ICON_F) lt.x = lt.y = rb.x = rb.y = 4;

	GlFilterKernel		kernel;
	status_t			err = MakeKernel(filter, int32(lt.x), int32(lt.y), int32(rb.x), int32(rb.y), low, high, kernel);
	delete filter;
	if (err != B_OK) return 0;

	/* Make the value algo (or find a macro it matches)
	 */	
	int32				macro = GL_CONVOLVE_SUM_NONE;
	GlAlgo*				value = 0;
	if (mAddOn) macro = mAddOn->ChainMacro(FindChain(_VALUE_KEY));
	if (macro == GL_CONVOLVE_SUM_NONE) {
		value = GenerateChainAlgo(_VALUE_KEY, args);
		if (!value) return 0;
	}

	return new GlConvolveAlgo(Id(), PixelTargets(), kernel, macro, value);
}

#if 0
status_t GlConvolveSum::Preview(int32 w, int32 h, ArpBitmap** outBm)
{
	GlAlgo2d*			filterAlgo = 0;
	GetMatrixParams(0, &filterAlgo, 0, 0);
	if (!filterAlgo) return B_ERROR;
	BPoint				lt = Params().Point(0, mAddOn->mLtType),
						rb = Params().Point(0, mAddOn->mRbType);
	float				low = Params().Float(0, mAddOn->mMapLType),
						high = Params().Float(0, mAddOn->mMapHType);
	status_t			err = B_ERROR;
	err = MakePreview(filterAlgo, int32(lt.x), int32(lt.y), int32(rb.x), int32(rb.y), low, high, w, h, outBm);
	delete filterAlgo;
	mChanged = false;
	return err;
}

status_t GlConvolveSum::MakePreview(GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
									float low, float high, int32 viewW, int32 viewH,
									ArpBitmap** outBm) const
{
	ArpVALIDATE(viewW > 0 && viewH > 0, return B_ERROR);
	GlFilterKernel	kernel;
	status_t		err = MakeKernel(s, l, t, r, b, low, high, kernel);
	if (err != B_OK) return err;
	int32			tapW, tapH, pix;
	float*			taps = kernel.LockTaps(&tapW, &tapH);
	if (!taps) return B_ERROR;
	int32			newW, newH;
	scale_to_dimens(tapW, tapH, viewW, viewH, &newW, &newH);
	ArpASSERT(newW > 0 && newH > 0);
	if (newW <= 0 || newH <= 0) {
		kernel.UnlockTaps(taps);
		return B_ERROR;
	}
	/* Actually, this really shouldn't be necessary, low and high
	 * are supplied as args.
	 */
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
#endif

status_t GlConvolveSum::MakeKernel(	GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
									float low, float high, GlFilterKernel& outKernel) const
{
	ArpVALIDATE(s, return B_ERROR);
	/* Process 2D.
	 */
	status_t		err = outKernel.Init(l, t, r, b);
	if (err != B_OK) return err;
	int32			w = l + r + 1,
					h = t + b + 1, pix;
	GlPlanes		dest(w, h, 1);
	ArpASSERT(dest.size == 1 && dest.w == w && dest.h == h);
	if (dest.size < 1) return B_NO_MEMORY;
	memset(dest.plane[0], 255, dest.w * dest.h);
	if (s->ProcessAll((const GlPlanes*)0, dest, 0) != B_OK) return B_ERROR;
	
	/* Translate into floating kernel.
	 */
	int32			kW, kH;
	float*			taps = outKernel.LockTaps(&kW, &kH);
	if (!taps || w != kW || h != kH) return B_ERROR;

	float			scale = ARP_ABS(high - low) / 256.0f;
	for (pix = 0; pix < w * h; pix++) taps[pix] = low + (dest.plane[0][pix] * scale);
	
	outKernel.UnlockTaps(taps);
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-CONVOLVE-SUM-ADD-ON
 ***************************************************************************/
GlConvolveSumAddOn::GlConvolveSumAddOn()
		: inherited(SZI[SZI_arp], GL_CONVOLVE_KEY, SZ(SZ_Distort), SZ(SZ_Blur), 1, 0)
{
	AddParamType(new GlPointParamType(_LT_KEY, SZ(SZ_LT), SZ(SZ_Left), SZ(SZ_Top), BPoint(0, 0), BPoint(128, 128), BPoint(2, 2), 1));
	AddParamType(new GlPointParamType(_RB_KEY, SZ(SZ_RB), SZ(SZ_Right), SZ(SZ_Bottom), BPoint(0, 0), BPoint(128, 128), BPoint(2, 2), 1));
	AddParamType(new GlFloatParamType(_LOW_KEY, SZ(SZ_Low), -256.0f, 256.0f, 0.0f, 0.1f));
	AddParamType(new GlFloatParamType(_HIGH_KEY, SZ(SZ_High), -256.0f, 256.0f, 1.0f, 0.1f));

	/* Init the blur1 chain
	 */
	mBlur1.Add(GL_OPEN_PAREN_NBR_KEY);
	mBlur1.Add(GL_NUMBER_IN_KEY).Set(GL_KEY_PARAM, "v").Set(GL_KEY_PARAM, "values");
	mBlur1.Add(GL_SUM_KEY).Add(GL_CLOSE_PAREN_NBR_KEY);
	mBlur1.Add(GL_BINARY_NBR_KEY).Set(GL_BINARY_OP_PARAM_KEY, GL_DIV_PARAM_KEY);
	mBlur1.Add(GL_OPEN_PAREN_NBR_KEY);
	mBlur1.Add(GL_NUMBER_IN_KEY).Set(GL_KEY_PARAM, "w").Set(GL_KEY_PARAM, "width");
	mBlur1.Add(GL_BINARY_NBR_KEY).Set(GL_BINARY_OP_PARAM_KEY, GL_MULT_PARAM_KEY);
	mBlur1.Add(GL_NUMBER_IN_KEY).Set(GL_KEY_PARAM, "h").Set(GL_KEY_PARAM, "height");
	mBlur1.Add(GL_CLOSE_PAREN_NBR_KEY);
//mBlur1.Print();
}

GlNode* GlConvolveSumAddOn::NewInstance(const BMessage* config) const
{
	return new GlConvolveSum(this, config);
}

int32 GlConvolveSumAddOn::ChainMacro(const GlChain* c) const
{
	if (!c) return GL_CONVOLVE_SUM_NONE;
	if (mBlur1.Matches(c)) return GL_CONVOLVE_SUM_BLUR;
	return GL_CONVOLVE_SUM_NONE;
}

// #pragma mark -

/***************************************************************************
 * GL-CONVOLVE-ALGO
 ***************************************************************************/
GlConvolveAlgo::GlConvolveAlgo(	gl_node_id nid, uint32 targets, GlFilterKernel& kernel,
								int32 macro, GlAlgo* value)
		: inherited(nid, targets), mKernel(kernel), mMacro(macro)
{
	if (value) SetChain(value, _VALUE_INDEX);
}

GlConvolveAlgo::GlConvolveAlgo(const GlConvolveAlgo& o)
		: inherited(o), mKernel(o.mKernel), mMacro(o.mMacro)
{
}
	
GlAlgo* GlConvolveAlgo::Clone() const
{
	return new GlConvolveAlgo(*this);
}

status_t GlConvolveAlgo::Process(	const GlPlanes*, GlPlanes& dest,
									GlProcessStatus* status)
{
	if (dest.size < 1) return B_OK;
// mKernel.Print();
	int32				tapW, tapH;
	float*				taps = mKernel.LockTaps(&tapW, &tapH);
	if (!taps) return B_OK;
	int32				tapL = mKernel.Left(), tapT = mKernel.Top(),
						tapR = mKernel.Right(), tapB = mKernel.Bottom();

	GlPlanes*			src = dest.Clone();
	if (src && src->size == dest.size && src->w == dest.w && src->h == dest.h) {
		if (mMacro == GL_CONVOLVE_SUM_BLUR)
			ProcessBlur(*src, dest, taps, tapW, tapH, tapL, tapT, tapR, tapB);
		else
			ProcessAlgo(*src, dest, taps, tapW, tapH, tapL, tapT, tapR, tapB);
		delete src;
	}
	mKernel.UnlockTaps(taps);
	return B_OK;
}

class _Convolve2dArray
{
public:
	GlArrayF*				a;
	uint32					size;

	_Convolve2dArray() : a(0), size(0)	{ }
	~_Convolve2dArray()					{ Free(); }
	
	status_t	Init(uint32 count, uint32 depth)
	{
		Free();
		a = new GlArrayF[count];
		if (!a) return B_NO_MEMORY;

		size = count;
		for (uint32 k = 0; k < size; k++) {
			if (a[k].Resize(depth) != B_OK) return B_ERROR;
		}
		return B_OK;
	}

	void		Free()
	{
		delete[] a;
		a = 0;
		size = 0;
	}
};

#define _SET_VALUES \
	for (k = 0; k < dest.size; k++) { \
		sums.a[k].Resize(0); \
		for (k2 = 0; k2 < ins.size; k2++) { \
			ArpASSERT(ins.ins[k2]); \
			if (ins.ins[k2]->index == 0) ins.ins[k2]->values = &(values.a[k]); \
		} \
		wrap.Process(sums.a[k]); \
		if (sums.a[k].size > 0) \
			dest.plane[k][destPix] = arp_clip_255(sums.a[k].n[0]); \
	} \
	destPix++;

#if 1
status_t GlConvolveAlgo::ProcessAlgo(const GlPlanes& src, GlPlanes& dest,
									float* taps, int32 tapW, int32 tapH,
									int32 tapL, int32 tapT, int32 tapR, int32 tapB)
{
printf("GlConvolveSum algo\n");
//wrap.Algo()->Print();
bigtime_t		start = system_time();

	GlAlgoNbrWrap				wrap(AlgoAt(_VALUE_INDEX));
	ArpVALIDATE(wrap.InitCheck() == B_OK, return B_ERROR);
	GlAlgoNbrInList				ins;
	if (wrap.GetInputs(ins) != B_OK) return B_ERROR;
	
	GlArrayF					inW(1), inH(1);
	if (inW.size != 1 || inH.size != 1) return B_NO_MEMORY;
	inW.n[0] = float(tapW);
	inH.n[0] = float(tapH);
	ins.SetValues(&inW, 2, "width", "w");
	ins.SetValues(&inH, 2, "height", "h");
	ins.SetIndex(0, 2, "values", "v");

	_Convolve2dArray			values;
	int32						tapSize = tapW * tapH;
	if (values.Init(dest.size, tapSize) != B_OK) return B_ERROR;
	_Convolve2dArray			sums;
	if (sums.Init(dest.size, tapSize) != B_OK) return B_ERROR;

	uint32						k, k2;
	int32						w = dest.w, h = dest.h, destPix = 0,
								x, y;
	/* Track an 2x2 grid of values -- the original source values for
	 * each plane.  At the start of each row, fill this grid in with
	 * all the values surrounding the current pixel.  Then, for each
	 * x move, only fill in the slice that's been newly revealed.
	 */
	for (y = 0; y < h; y++) {
		// Load up the initial values using symmetric extension
		x = 0;
		int32				tap = 0, tapX, tapY;
		for (tapY = -tapT; tapY <= tapB; tapY++) {
			for (tapX = -tapL; tapX <= tapR; tapX++) {
				int32		srcX = x + tapX, srcY = y + tapY;
				ArpASSERT(int32(tap) == ((tapY + tapT) * tapW + (tapX + tapL)));
				ArpASSERT(tap < tapSize);
				int32		srcPix = ARP_PIXEL_SE(srcX, srcY, w, h);
				for (k = 0; k < dest.size; k++) {
					ArpASSERT(uint32(tap) < values.a[k].size);
					values.a[k].n[tap] = (src.plane[k][srcPix] * taps[tap]);
				}
				tap++;
			}
		}
#if 0
	for (k = 0; k < dest.size; k++) { 
		sums.a[k].Resize(0); 
		for (k2 = 0; k2 < ins.size; k2++) { 
			ArpASSERT(ins.ins[k2]); 
			if (ins.ins[k2]->index == 0) ins.ins[k2]->values = &(values.a[k]); 
		} 
debugger("dsds");
		wrap.Process(sums.a[k]); 
		if (sums.a[k].size > 0) 
			dest.plane[k][destPix] = arp_clip_255(sums.a[k].n[0]); 
	} 
	destPix++;
#endif
		_SET_VALUES

		int32			slice = 0;
		for (x = 1; x < w; x++) {
			// Load a new slice of the array using symmetric extension
			tap = slice;
			for (tapY = -tapT; tapY <= tapB; tapY++) {
				int32		srcX = x + tapR, srcY = y + tapY;
				ArpASSERT(tap < tapSize);
				int32		srcPix = ARP_PIXEL_SE(srcX, srcY, w, h);
				for (k = 0; k < dest.size; k++) {
					ArpASSERT(uint32(tap) < values.a[k].size);
					values.a[k].n[tap] = (src.plane[k][srcPix] * taps[tap]);
				}
				tap += tapW;
			}
			slice++;
			if (slice >= tapW) slice = 0;

			_SET_VALUES
		}
	}
printf("	algo: %f sec\n", double(system_time() - start) / 1000000);
	return B_OK;
}
#endif

#undef _SET_VALUES

#if 0
status_t GlConvolveAlgo::ProcessAlgo(const GlPlanes& src, GlPlanes& dest,
									float* taps, int32 tapW, int32 tapH,
									int32 tapL, int32 tapT, int32 tapR, int32 tapB)
{
printf("GlConvolveSum algo\n");
//wrap.Algo()->Print();
bigtime_t		start = system_time();

	GlAlgoNbrWrap			wrap(AlgoAt(_VALUE_INDEX));
	ArpVALIDATE(wrap.InitCheck() == B_OK, return B_ERROR);
	GlAlgoNbrInList			ins;
	if (wrap.GetInputs(ins) != B_OK) return B_ERROR;
	
	GlArrayF				inW(1), inH(1);
	if (inW.size != 1 || inH.size != 1) return B_NO_MEMORY;
	inW.n[0] = tapW;
	inH.n[0] = tapH;
	ins.SetValues(&inW, 2, "width", "w");
	ins.SetValues(&inH, 2, "height", "h");
	ins.SetIndex(0, 2, "values", "v");

	_Convolve2dArray		values;
	if (values.Init(dest.size, tapW * tapH) != B_OK) return B_ERROR;
	_Convolve2dArray		sums;
	if (sums.Init(dest.size, tapW * tapH) != B_OK) return B_ERROR;

	uint32				k, k2;
	int32				w = dest.w, h = dest.h, destPix = 0;
//	int32				tapSize = tapW * tapH;
	// For each pixel...
	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			// For each tap...
			uint32		tap = 0;
			for (int32 tapY = -tapT; tapY <= tapB; tapY++) {
				for (int32 tapX = -tapL; tapX <= tapR; tapX++) {
					int32		srcX = x + tapX, srcY = y + tapY;
					ArpASSERT(int32(tap) == ((tapY + tapT) * tapW + (tapX + tapL)));
					int32		srcPix = ARP_PIXEL_SE(srcX, srcY, w, h);
					for (k = 0; k < dest.size; k++) {
						ArpASSERT(tap < values.a[k].size);
						values.a[k].n[tap] = (src.plane[k][srcPix] * taps[tap]);
					}
					tap++;
				}
			}
			for (k = 0; k < dest.size; k++) {
				sums.a[k].Resize(0);
				for (k2 = 0; k2 < ins.size; k2++) {
					ArpASSERT(ins.ins[k2]);
					if (ins.ins[k2]->index == 0) ins.ins[k2]->values = &(values.a[k]);
				}
//debugger("sdsds");
				wrap.Process(sums.a[k]);
				if (sums.a[k].size > 0)
					dest.plane[k][destPix] = arp_clip_255(sums.a[k].n[0]);
			}
			destPix++;
		}
	}
printf("	algo: %f sec\n", double(system_time() - start) / 1000000);
	return B_OK;
}
#endif

status_t GlConvolveAlgo::ProcessBlur(	const GlPlanes& src, GlPlanes& dest,
										float* taps, int32 tapW, int32 tapH,
										int32 tapL, int32 tapT, int32 tapR, int32 tapB)
{
printf("GlConvolveSum blur\n");
bigtime_t		start = system_time();
	uint32				k;
	int32				w = dest.w, h = dest.h, destPix = 0;
	float				tapSize = float(tapW * tapH);
	float*				sums = new float[dest.size];
	if (!sums) return B_NO_MEMORY;
	// For each pixel...
	for (int32 y = 0; y < h; y++) {
		for (int32 x = 0; x < w; x++) {
			for (k = 0; k < dest.size; k++) sums[k] = 0.0;
			// For each tap...
			uint32		tap = 0;
			for (int32 tapY = -tapT; tapY <= tapB; tapY++) {
				for (int32 tapX = -tapL; tapX <= tapR; tapX++) {
					int32		srcX = x + tapX, srcY = y + tapY;
					ArpASSERT(int32(tap) == ((tapY + tapT) * tapW + (tapX + tapL)));
					int32		srcPix = ARP_PIXEL_SE(srcX, srcY, w, h);
					for (k = 0; k < dest.size; k++) {
						sums[k] += src.plane[k][srcPix] * taps[tap];
					}
					tap++;
				}
			}
			for (k = 0; k < dest.size; k++) {
				dest.plane[k][destPix] = arp_clip_255(sums[k] / tapSize);
			}
			destPix++;
		}
	}
	delete[] sums;
printf("	blur: %f sec\n", double(system_time() - start) / 1000000);
	return B_OK;
}
