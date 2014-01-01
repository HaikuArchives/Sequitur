#include <ArpCore/StlVector.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlCompositeImg.h>

/* MODE constants.
 */
enum {
	REPLACE_MODE		= GL_COMPOSITE_MODE_REPLACE,
	REPLACE_ALPHA_MODE	= 'rplA',
	AVERAGE_MODE		= GL_COMPOSITE_MODE_AVERAGE,
	MIN_MODE			= 'min_',
	MAX_MODE			= 'max_',
	ADD_MODE			= 'add_',
	SUB_MODE			= 'sub_',
	CENTER_MODE			= 'cent'
};
/* SRC constants.
 */
enum {
	ONE_D_SRC			= '1d__',
	TWO_D_SRC			= '2d__',
	RED_SRC				= 'r___',
	GREEN_SRC			= 'g___',
	BLUE_SRC			= 'b___',
	ALPHA_SRC			= 'a___',
	HUE_SRC				= 'hue_',
	SATURATION_SRC		= 'sat_',
	VALUE_SRC			= 'val_',
	DEPTH_SRC			= 'z___',
	DIFFUSION_SRC		= 'diff',
	SPECULARITY_SRC		= 'spec',
	DENSITY_SRC			= 'd___',
	COHESION_SRC		= 'c___',
	FLUIDITY_SRC		= 'f___'
};

#define _COMP_R		(0)
#define _COMP_G		(1)
#define _COMP_B		(2)
#define _COMP_A		(3)
#define _COMP_Z		(4)
#define _COMP_DIFF	(5)
#define _COMP_SPEC	(6)
#define _COMP_D		(7)
#define _COMP_C		(8)
#define _COMP_F		(9)
#define _COMP_LAST	(10)

static const int32		_R_ON				= 'r_on';
static const int32		_G_ON				= 'g_on';
static const int32		_B_ON				= 'b_on';
static const int32		_A_ON				= 'a_on';
static const int32		_Z_ON				= 'z_on';
static const int32		_DIFF_ON			= 'i_on';
static const int32		_SPEC_ON			= 's_on';
static const int32		_D_ON				= 'd_on';
static const int32		_C_ON				= 'c_on';
static const int32		_F_ON				= 'f_on';

static const int32		_R_SRC				= 'rsrc';
static const int32		_G_SRC				= 'gsrc';
static const int32		_B_SRC				= 'bsrc';
static const int32		_A_SRC				= 'asrc';
static const int32		_Z_SRC				= 'zsrc';
static const int32		_DIFF_SRC			= 'isrc';
static const int32		_SPEC_SRC			= 'ssrc';
static const int32		_D_SRC				= 'dsrc';
static const int32		_C_SRC				= 'csrc';
static const int32		_F_SRC				= 'fsrc';

static const int32		_SRC_IMG_KEY		= 'simg';
static const uint32		_SRC_IMG_INDEX		= 1;
static const int32		_DEST_MASK_KEY		= 'dmsk';
static const uint32		_DEST_2D_INDEX		= 0;
static const int32		_SRC_MASK_KEY		= 'smsk';
static const uint32		_SRC_2D_INDEX		= 3;
static const int32		_MAP_KEY			= 'map_';
static const uint32		_SRC_1D_INDEX		= 2;

static void _verify_dest(	vector<GlImage*>& destImgs, vector<GlImage*>& srcImgs,
							GlNodeDataList& list);
static void _align_sources(vector<GlImage*>& srcImgs);
static void _override_plane_src(GlPlanes& pixels, int32* srcMode);

/***************************************************************************
 * _GL-COMPOSITE-ALGO
 ***************************************************************************/
class _GlCompositeAlgo : public GlAlgoIm
{
public:
	_GlCompositeAlgo(	gl_node_id nid, int32 mode, int32 onMask, int32* srcMode,
						bool align, GlAlgo* dest2d, GlAlgo* srcIm,
						GlAlgo* src1d, GlAlgo* src2d);
	_GlCompositeAlgo(const _GlCompositeAlgo& o);

	virtual GlAlgo*		Clone() const;

//	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
//	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
	int32				mMode, mOnMask, mSrcMode[_COMP_LAST];
	bool				mAlign;

	GlMask				mSrcMask, mDestMask;

	status_t			Perform(GlImage* srcImage, GlAlgo2d* src2d, GlPlanes& destPixels,
								uint8* destMask, int32 destOriginX,
								int32 destOriginY, GlAlgo1d* src1d);
	status_t			PerformReplaceAlpha(GlImage* srcImage, GlAlgo2d* src2d, GlPlanes& dest,
											uint8* destMask, int32 destOriginX,
											int32 destOriginY, GlAlgo1d* src1d);

	status_t			GetSourceImages(	GlNodeDataList& list,
											GlNodeDataList& srcList,
											const gl_process_args* args);
	bool				AllFromConstruct() const;
	bool				MissingSource(GlImage* srcIm, GlAlgo1d* src1d, uint8* src2d) const;
};

/***************************************************************************
 * GL-COMPOSITE
 ***************************************************************************/
GlCompositeImg::GlCompositeImg(	const GlCompositeImgAddOn* addon,
							const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	VerifyChain(new GlChain(_SRC_IMG_KEY, GL_IMAGE_IO, SZ(SZ_Source_Image), this));
	VerifyChain(new GlChain(_DEST_MASK_KEY, GL_2D_IO, SZ(SZ_Destination_Mask), this));
	VerifyChain(new GlChain(_SRC_MASK_KEY, GL_2D_IO, SZ(SZ_Source_Mask), this));
	VerifyChain(new GlChain(_MAP_KEY, GL_1D_IO, SZ(SZ_Map), this));
}

GlCompositeImg::GlCompositeImg(const GlCompositeImg& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlCompositeImg::Clone() const
{
	return new GlCompositeImg(*this);
}

GlAlgo* GlCompositeImg::Generate(const gl_generate_args& args) const
{
	const GlParamList&	params = Params();
	int32				mode = params.Menu(GL_MODE_PARAM_KEY);
	int32				onMask = 0;
	if (params.Bool(_R_ON))		onMask |= GL_PIXEL_R_MASK;
	if (params.Bool(_G_ON))		onMask |= GL_PIXEL_G_MASK;
	if (params.Bool(_B_ON))		onMask |= GL_PIXEL_B_MASK;
	if (params.Bool(_A_ON))		onMask |= GL_PIXEL_A_MASK;
	if (params.Bool(_Z_ON))		onMask |= GL_PIXEL_Z_MASK;
	if (params.Bool(_DIFF_ON))	onMask |= GL_PIXEL_DIFF_MASK;
	if (params.Bool(_SPEC_ON))	onMask |= GL_PIXEL_SPEC_MASK;
	if (params.Bool(_D_ON))		onMask |= GL_PIXEL_D_MASK;
	if (params.Bool(_C_ON))		onMask |= GL_PIXEL_C_MASK;
	if (params.Bool(_F_ON))		onMask |= GL_PIXEL_F_MASK;
	int32				srcMode[_COMP_LAST];
	srcMode[_COMP_R] = params.Menu(_R_SRC);
	srcMode[_COMP_G] = params.Menu(_G_SRC);
	srcMode[_COMP_B] = params.Menu(_B_SRC);
	srcMode[_COMP_A] = params.Menu(_A_SRC);
	srcMode[_COMP_Z] = params.Menu(_Z_SRC);
	srcMode[_COMP_SPEC] = params.Menu(_DIFF_SRC);
	srcMode[_COMP_DIFF] = params.Menu(_SPEC_SRC);
	srcMode[_COMP_D] = params.Menu(_D_SRC);
	srcMode[_COMP_C] = params.Menu(_C_SRC);
	srcMode[_COMP_F] = params.Menu(_F_SRC);
	bool				align = Params().Bool(GL_COMPOSITE_ALIGN_SOURCES_KEY);
	GlAlgo*				srcIm = GenerateChainAlgo(_SRC_IMG_KEY, args);
	GlAlgo*				src1d = GenerateChainAlgo(_MAP_KEY, args);
	GlAlgo*				dest2d = GenerateChainAlgo(_DEST_MASK_KEY, args);
	GlAlgo*				src2d = GenerateChainAlgo(_SRC_MASK_KEY, args);
	return new _GlCompositeAlgo(Id(), mode, onMask, srcMode, align, dest2d, srcIm, src1d, src2d);
}

status_t GlCompositeImg::Process(	GlNodeDataList& list,
									const gl_process_args* args)
{
	gl_generate_args	gargs;
	GlAlgo*				a = Generate(gargs);
	if (!a) return B_NO_MEMORY;
	status_t			err = a->PerformAll(list, args);
	delete a;
	return err;
}

// #pragma mark -

/***************************************************************************
 * GL-COMPOSITE-COLOR-ADD-ON
 ***************************************************************************/
static GlParamType* new_mode_type()
{
	BMessage		msg;
	msg.AddString("item", "Replace");			msg.AddInt32("i", REPLACE_MODE);
	msg.AddString("item", "Replace (Alpha)");	msg.AddInt32("i", REPLACE_ALPHA_MODE);
	msg.AddString("item", "Average");			msg.AddInt32("i", AVERAGE_MODE);
	msg.AddString("item", "Min");				msg.AddInt32("i", MIN_MODE);
	msg.AddString("item", "Max");				msg.AddInt32("i", MAX_MODE);
	msg.AddString("item", "Add");				msg.AddInt32("i", ADD_MODE);
	msg.AddString("item", "Subtract");			msg.AddInt32("i", SUB_MODE);
	msg.AddString("item", "Center");			msg.AddInt32("i", CENTER_MODE);
	return new GlMenuParamType(GL_MODE_PARAM_KEY, SZ(SZ_Mode), msg, REPLACE_MODE);
}

static GlParamType* new_src_type(int32 key, int32 init, int32 row)
{
	BMessage		msg;
//	msg.AddString("item", "1D");				msg.AddInt32("i", ONE_D_SRC);
	msg.AddString("item", "2D");				msg.AddInt32("i", TWO_D_SRC);
	msg.AddString("item", "Red");				msg.AddInt32("i", RED_SRC);
	msg.AddString("item", "Green");				msg.AddInt32("i", GREEN_SRC);
	msg.AddString("item", "Blue");				msg.AddInt32("i", BLUE_SRC);
	msg.AddString("item", "Alpha");				msg.AddInt32("i", ALPHA_SRC);
	msg.AddString("item", "Hue");				msg.AddInt32("i", HUE_SRC);
	msg.AddString("item", "Saturation");		msg.AddInt32("i", SATURATION_SRC);
	msg.AddString("item", "Value");				msg.AddInt32("i", VALUE_SRC);
	msg.AddString("item", "Depth");				msg.AddInt32("i", DEPTH_SRC);
	msg.AddString("item", "Diffusion");			msg.AddInt32("i", DIFFUSION_SRC);
	msg.AddString("item", "Specularity");		msg.AddInt32("i", SPECULARITY_SRC);
	msg.AddString("item", "Density");			msg.AddInt32("i", DENSITY_SRC);
	msg.AddString("item", "Cohesion");			msg.AddInt32("i", COHESION_SRC);
	msg.AddString("item", "Fluidity");			msg.AddInt32("i", FLUIDITY_SRC);
	return new GlMenuParamType(key, SZ(SZ_from), msg, init, row);
}

GlCompositeImgAddOn::GlCompositeImgAddOn()
		: inherited(SZI[SZI_arp], GL_COMPOSITE_KEY, SZ(SZ_Combine), SZ(SZ_Composite), 1, 0)
{
	int32			row = -1;
		
	mMode		= AddParamType(new_mode_type());
	// Color
	mROn		= AddParamType(new GlBoolParamType(	_R_ON, SZ(SZ_Red), true,				++row));
	mRSrc		= AddParamType(new_src_type(		_R_SRC, RED_SRC,				row));
	mGOn		= AddParamType(new GlBoolParamType(	_G_ON, SZ(SZ_Green), true,			++row));
	mGSrc		= AddParamType(new_src_type(		_G_SRC, GREEN_SRC,				row));
	mBOn		= AddParamType(new GlBoolParamType(	_B_ON, SZ(SZ_Blue), true,			++row));
	mBSrc		= AddParamType(new_src_type(		_B_SRC, BLUE_SRC,				row));
	mAOn		= AddParamType(new GlBoolParamType(	_A_ON, SZ(SZ_Alpha), false,			++row));
	mASrc		= AddParamType(new_src_type(		_A_SRC, ALPHA_SRC,				row));
	// Material
	mZOn		= AddParamType(new GlBoolParamType(	_Z_ON, SZ(SZ_Depth), false,			++row));
	mZSrc		= AddParamType(new_src_type(		_Z_SRC, DEPTH_SRC,				row));
	mDiffOn		= AddParamType(new GlBoolParamType(	_DIFF_ON, SZ(SZ_Diffusion), false,	++row));
	mDiffSrc	= AddParamType(new_src_type(		_DIFF_SRC, DIFFUSION_SRC,		row));
	mSpecOn		= AddParamType(new GlBoolParamType(	_SPEC_ON, SZ(SZ_Specularity), false,	++row));
	mSpecSrc	= AddParamType(new_src_type(		_SPEC_SRC, SPECULARITY_SRC,		row));
	mDOn		= AddParamType(new GlBoolParamType(	_D_ON, SZ(SZ_Density), false,		++row));
	mDSrc		= AddParamType(new_src_type(		_D_SRC, DENSITY_SRC,			row));
	mCOn		= AddParamType(new GlBoolParamType(	_C_ON, SZ(SZ_Cohesion), false,		++row));
	mCSrc		= AddParamType(new_src_type(		_C_SRC, COHESION_SRC,			row));
	mFOn		= AddParamType(new GlBoolParamType(	_F_ON, SZ(SZ_Fluidity), false,		++row));
	mFSrc		= AddParamType(new_src_type(		_F_SRC, FLUIDITY_SRC,			row));

	mAlignSources	= AddParamType(new GlBoolParamType(GL_COMPOSITE_ALIGN_SOURCES_KEY, SZ(SZ_Align_sources), false, ++row));
}

GlNode* GlCompositeImgAddOn::NewInstance(const BMessage* config) const
{
	return new GlCompositeImg(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-COMPOSITE-ALGO
 ***************************************************************************/
_GlCompositeAlgo::_GlCompositeAlgo(	gl_node_id nid, int32 mode, int32 onMask,
									int32* srcMode, bool align, GlAlgo* dest2d,
									GlAlgo* srcIm, GlAlgo* src1d, GlAlgo* src2d)
		: inherited(nid), mMode(mode), mOnMask(onMask), mAlign(align)
{
	ArpASSERT(srcMode);
	for (uint32 k = 0; k < _COMP_LAST; k++) mSrcMode[k] = srcMode[k];
	if (dest2d) SetChain(dest2d, _DEST_2D_INDEX);
	if (srcIm) SetChain(srcIm, _SRC_IMG_INDEX);
	if (src1d) SetChain(src1d, _SRC_1D_INDEX);
	if (src2d) SetChain(src2d, _SRC_2D_INDEX);
}

_GlCompositeAlgo::_GlCompositeAlgo(const _GlCompositeAlgo& o)
		: inherited(o), mMode(o.mMode), mOnMask(o.mOnMask), mAlign(o.mAlign)
{
	for (uint32 k = 0; k < _COMP_LAST; k++) mSrcMode[k] = o.mSrcMode[k];
}

GlAlgo* _GlCompositeAlgo::Clone() const
{
	return new _GlCompositeAlgo(*this);
}

#if 0
#define GET_SRC(srcMode, src, srcPix, destV) 
	(srcMode == ONE_D_SRC)				? (arp_clip_255((cache1d ? cache1d->n[destV] : src1d->At(glTable256[destV], glTable256[destV])) * 255)) 
		: (srcMode == TWO_D_SRC)		? (255) 
#endif

#define GET_SRC(srcMode, src, srcPix, destV) \
	(srcMode == TWO_D_SRC)				? (255) \
		: (srcMode == RED_SRC)			? (src->r[srcPix]) \
		: (srcMode == GREEN_SRC)		? (src->g[srcPix]) \
		: (srcMode == BLUE_SRC)			? (src->b[srcPix]) \
		: (srcMode == ALPHA_SRC)		? (src->a[srcPix]) \
		: (srcMode == HUE_SRC)			? (src->Hue(srcPix)) \
		: (srcMode == SATURATION_SRC)	? (src->Saturation(srcPix)) \
		: (srcMode == VALUE_SRC)		? (src->Value(srcPix)) \
		: (srcMode == DEPTH_SRC)		? (src->z[srcPix]) \
		: (srcMode == DIFFUSION_SRC)	? (src->diff[srcPix]) \
		: (srcMode == SPECULARITY_SRC)	? (src->spec[srcPix]) \
		: (srcMode == DENSITY_SRC)		? (src->d[srcPix]) \
		: (srcMode == COHESION_SRC)		? (src->c[srcPix]) \
		: (srcMode == FLUIDITY_SRC)		? (src->f[srcPix]) \
		: 0

#define DO_OP(destV, srcV, mode, srcMode, srcM, destM) \
	(mode == AVERAGE_MODE) ? (destV + ((int16(srcV) - destV) * (srcM * destM))) \
		: (mode == MIN_MODE) ? ((destV < srcV) ? destV : (destV - ((destV - srcV) * destM))) \
		: (mode == MAX_MODE) ? ((destV > srcV) ? destV : (destV + ((srcV - destV) * destM))) \
		: (mode == ADD_MODE) ? (destV + (srcV * destM)) \
		: (mode == SUB_MODE) ? (destV - (srcV * destM)) \
		: (mode == CENTER_MODE) ? ( (srcV < 128) ? (destV - srcV) : (destV + (srcV - 128)) ) \
		: (mode == REPLACE_MODE && srcMode == TWO_D_SRC) ? (destMask[destPix]) \
		: (srcV * (srcM * destM))
		// Default to pixel REPLACE

//		: (dest + ((src - dest) * (srcM * destM)))

//		: arp_clip_255(src * destM)
		// Default to REPLACE

status_t _GlCompositeAlgo::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	GlAlgo1d*			src1d = Algo1dAt(_SRC_1D_INDEX);
	GlAlgo2d*			dest2d = Algo2dAt(_DEST_2D_INDEX);
	GlAlgo2d*			src2d = Algo2dAt(_SRC_2D_INDEX);

	GlNodeDataList		srcImgList;
	GetSourceImages(list, srcImgList, args);
	
	GlImage*			destImg;
	GlImage*			srcImg;
	uint32				k;
	vector<GlImage*>	destImgs;
	vector<GlImage*>	srcImgs;
	for (k = 0; (destImg = list.ImageAt(k)) != 0; k++) destImgs.push_back(destImg);
	while ((srcImg = srcImgList.DetachImage()) != 0) srcImgs.push_back(srcImg);
	srcImgList.DeleteContents();
	
	if (destImgs.size() < 1 && srcImgs.size() > 1 && mAlign)
		_align_sources(srcImgs);

	/* If there are no destination images, one of two things happen:  Either there is
	 * a single source image, and it just becomes the destination, or there are multiple
	 * sources, and I create a new destination large enough for all of them.
	 */
	_verify_dest(destImgs, srcImgs, list);

	for (k = 0; k < destImgs.size(); k++) {
		destImg = destImgs[k];
		int32			destOriginX, destOriginY;
		destImg->GetOrigin(&destOriginX, &destOriginY);
		GlPlanes*		destPixels;
		if ((destPixels = destImg->LockPixels(mOnMask)) != 0) {
			_override_plane_src(*destPixels, mSrcMode);
			uint8*		destMask = 0;
			if (dest2d) destMask = mDestMask.Make(*destPixels, dest2d, (args) ? args->status : 0);
			uint32		srcCount = 0;
			for (uint32 j = 0; j < srcImgs.size(); j++) {
				srcImg = srcImgs[j];
				if (srcImg->InitCheck() == B_OK) {
					Perform(srcImg, src2d, *destPixels, destMask, destOriginX, destOriginY,
							src1d);
				}
				srcCount++;
			}
			/* If there were no src images, then check to see if every
			 * flag that's on is getting data from the 1d or 2d object -- if that's
			 * the case, then I can be run.
			 */
			if (srcCount < 1 && AllFromConstruct())
				Perform(0, 0, *destPixels, destMask, destOriginX, destOriginY, src1d);
			destImg->UnlockPixels(destPixels);
		}
	}

	for (k = 0; k < srcImgs.size(); k++) delete srcImgs[k];
	return B_OK;
}

status_t _GlCompositeAlgo::Perform(	GlImage* srcImage, GlAlgo2d* src2d, GlPlanes& dest,
									uint8* destMask, int32 destOriginX,
									int32 destOriginY, GlAlgo1d* src1d)
{
	ArpASSERT(!src1d);
	if (dest.size < 1) return B_ERROR;
	if (mMode == REPLACE_ALPHA_MODE) return PerformReplaceAlpha(srcImage, src2d, dest, destMask,
																destOriginX, destOriginY, src1d);
	if (MissingSource(srcImage, src1d, destMask)) {
		printf("_GlCompositeAlgo::Perform() Missing source\n");
		return B_ERROR;
	}
//	ArpVALIDATE(!MissingSource(srcImage, src1d, destMask), return B_ERROR);
	GlPlanes*		src;
	if (srcImage) {
		if ((src = srcImage->LockPixels(mOnMask)) == 0) return B_ERROR;
		if (!src->HasPlanes(mOnMask)) {
			ArpASSERT(false);
			srcImage->UnlockPixels(src);
			return B_ERROR;
		}
	} else {
		if ((src = new GlPlanes()) == 0) return B_NO_MEMORY;
		src->w = dest.w;
		src->h = dest.h;
	}

//	float*			cache1d = 0;
	GlCache1d*		cache1d = 0;
	if (src1d && !(src1d->Properties()&src1d->RANDOM_F)) {
//		cache1d = src1d->Run(256);
		cache1d = src1d->NewCache(256);
//		if (!cache1d) return B_NO_MEMORY;
	}

	uint8*			srcMask = 0;
	if (src2d) srcMask = mSrcMask.Make(*src, src2d);

	int32			srcX, srcY;
	float			srcM = 1, destM;
	int32			srcPix = 0, destPix = 0;
	uint32			k;
	
	for (int32 y = 0; y < dest.h; y++) {
		for (int32 x = 0; x < dest.w; x++) {
			srcX = x;
			srcY = y;
			if (srcImage) {
				srcX += destOriginX;
				srcY += destOriginY;
				srcImage->GetLocation(&srcX, &srcY);
			}
			if (srcX >= 0 && srcX < src->w && srcY >= 0 && srcY < src->h) {
				if (srcImage) {
					srcPix = ARP_PIXEL(srcX, srcY, src->w);
					if (srcMask) srcM = glTable256[srcMask[srcPix]];
					else srcM = glTable256[src->a[srcPix]];
				}
				if (destMask) destM = glTable256[destMask[destPix]];
				else destM = 1.0;
				
				for (k = 0; k < dest.size; k++) {
					uint8		srcV = GET_SRC(dest.planeSrc[k], src, srcPix, dest.plane[k][destPix]);
					float		v = DO_OP(dest.plane[k][destPix], srcV, mMode, dest.planeSrc[k], srcM, destM);
					dest.plane[k][destPix] = arp_clip_255(v);
				}
			}
			destPix++;
		}
	}
	if (srcImage) srcImage->UnlockPixels(src);
	else delete src;
	delete cache1d;
	return B_OK;
}

status_t _GlCompositeAlgo::PerformReplaceAlpha(	GlImage* srcImage, GlAlgo2d* src2d, GlPlanes& dest,
												uint8* destMask, int32 destOriginX,
												int32 destOriginY, GlAlgo1d* src1d)
{
/* FIX: THIS IS A TOTAL HACK UNTIL I FIGURE OUT HOW TO GET THIS @#$EWR#R
 * COMPOSITE NODE FULL FEATURED BUT EASY
 */
	if (MissingSource(srcImage, src1d, destMask)) {
		printf("_GlCompositeAlgo::PerformReplaceAlpha() Missing source\n");
		return B_ERROR;
	}
	GlPlanes*		src;
	if (srcImage) {
		if ((src = srcImage->LockPixels(mOnMask)) == 0) return B_ERROR;
		if (!src->HasPlanes(mOnMask)) {
			ArpASSERT(false);
			srcImage->UnlockPixels(src);
			return B_ERROR;
		}
	} else {
		if ((src = new GlPlanes()) == 0) return B_NO_MEMORY;
		src->w = dest.w;
		src->h = dest.h;
	}

//	float*			cache1d = 0;
	GlCache1d*		cache1d = 0;
	if (src1d && !(src1d->Properties()&src1d->RANDOM_F)) {
//		cache1d = src1d->Run(256);
		cache1d = src1d->NewCache(256);
//		if (!cache1d) return B_NO_MEMORY;
	}
	
	uint8*			srcMask = 0;
	if (src2d) srcMask = mSrcMask.Make(*src, src2d);

	int32			srcX, srcY;
	float			srcM = 1, destM = 1;
	int32			srcPix = 0, destPix = 0;
	uint32			k, alphaK = dest.size + 10;
	
	for (k = 0; k < dest.size; k++) {
		if (dest.a && dest.plane[k] == dest.a) alphaK = k;
	}
	
	for (int32 y = 0; y < dest.h; y++) {
		for (int32 x = 0; x < dest.w; x++) {
			srcX = x;
			srcY = y;
			if (srcImage) {
				srcX += destOriginX;
				srcY += destOriginY;
				srcImage->GetLocation(&srcX, &srcY);
			}
			if (srcX >= 0 && srcX < src->w && srcY >= 0 && srcY < src->h) {
				if (srcImage) {
					srcPix = ARP_PIXEL(srcX, srcY, src->w);
					if (srcMask) srcM = glTable256[srcMask[srcPix]];
					else srcM = glTable256[src->a[srcPix]];
				}
				if (destMask) destM = glTable256[destMask[destPix]];
				else if (dest.a) destM = glTable256[dest.a[destPix]];
				
				for (k = 0; k < dest.size; k++) {
					uint8		srcV = GET_SRC(dest.planeSrc[k], src, srcPix, dest.plane[k][destPix]);
					if (k == alphaK) {
						if (srcV > dest.plane[k][destPix]) dest.plane[k][destPix] = srcV;
					} else {
//						if ( (x == 0 && y == 0) || (x == 100 && y == 100)) {
//							printf("(%ld, %ld) destM %f\n", x, y, destM);
//						}
						if (destM <= 0) dest.plane[k][destPix] = srcV;
						else {
							float	v = DO_OP(dest.plane[k][destPix], srcV, AVERAGE_MODE, dest.planeSrc[k], srcM, destM);
							dest.plane[k][destPix] = arp_clip_255(v);
						}	
					}
				}
			}
			destPix++;
		}
	}
	if (srcImage) srcImage->UnlockPixels(src);
	else delete src;
	delete cache1d;
	return B_OK;
}

status_t _GlCompositeAlgo::GetSourceImages(	GlNodeDataList& list,
											GlNodeDataList& srcList,
											const gl_process_args* args)
{
	GlAlgo*				a = ChainAt(_SRC_IMG_INDEX);
	if (!a) return B_OK;
	/* Get source images, if any.  First I have to stuff all my
	 * list images as backpointers in the supplied list.
	 */
	GlImage*			img;
	for (uint32 k = 0; (img = list.ImageAt(k)) != 0; k++) {
		GlNodeDataBackPointer*	bp = new GlNodeDataBackPointer(img);
		if (bp) srcList.AddData(bp);
	}
	status_t			err = a->PerformAll(srcList, args);
	srcList.DeleteContents(srcList.BACKPOINTER_TYPE);
	return err;
}

bool _GlCompositeAlgo::AllFromConstruct() const
{
	if (mOnMask&GL_PIXEL_R_MASK		&& mSrcMode[_COMP_R] != ONE_D_SRC		&& mSrcMode[_COMP_R] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_G_MASK		&& mSrcMode[_COMP_G] != ONE_D_SRC		&& mSrcMode[_COMP_G] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_B_MASK		&& mSrcMode[_COMP_B] != ONE_D_SRC		&& mSrcMode[_COMP_B] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_A_MASK		&& mSrcMode[_COMP_A] != ONE_D_SRC		&& mSrcMode[_COMP_A] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_Z_MASK		&& mSrcMode[_COMP_Z] != ONE_D_SRC		&& mSrcMode[_COMP_Z] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_DIFF_MASK	&& mSrcMode[_COMP_DIFF] != ONE_D_SRC	&& mSrcMode[_COMP_DIFF] != TWO_D_SRC)	return false;
	if (mOnMask&GL_PIXEL_SPEC_MASK	&& mSrcMode[_COMP_SPEC] != ONE_D_SRC	&& mSrcMode[_COMP_SPEC] != TWO_D_SRC)	return false;
	if (mOnMask&GL_PIXEL_D_MASK		&& mSrcMode[_COMP_D] != ONE_D_SRC		&& mSrcMode[_COMP_D] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_C_MASK		&& mSrcMode[_COMP_C] != ONE_D_SRC		&& mSrcMode[_COMP_C] != TWO_D_SRC)		return false;
	if (mOnMask&GL_PIXEL_F_MASK		&& mSrcMode[_COMP_F] != ONE_D_SRC		&& mSrcMode[_COMP_F] != TWO_D_SRC)		return false;
	return true;
}

static bool missing_source(	int32 onMask, int32 onFlag, int32 src,
							GlAlgo1d* src1d, uint8* srcSurface, GlImage* srcImage)
{
	if (onMask&onFlag) {
		if (src == ONE_D_SRC && !src1d) return true;
		if (src == TWO_D_SRC && !srcSurface) return true;
		if (src != ONE_D_SRC && src != TWO_D_SRC && !srcImage) return true;
	}
	return false;
}

bool _GlCompositeAlgo::MissingSource(GlImage* srcIm, GlAlgo1d* src1d, uint8* src2d) const
{
	if (missing_source(mOnMask, GL_PIXEL_R_MASK, mSrcMode[_COMP_R], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_G_MASK, mSrcMode[_COMP_G], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_B_MASK, mSrcMode[_COMP_B], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_A_MASK, mSrcMode[_COMP_A], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_Z_MASK, mSrcMode[_COMP_Z], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_DIFF_MASK, mSrcMode[_COMP_DIFF], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_SPEC_MASK, mSrcMode[_COMP_SPEC], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_D_MASK, mSrcMode[_COMP_D], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_C_MASK, mSrcMode[_COMP_C], src1d, src2d, srcIm)) return true;
	if (missing_source(mOnMask, GL_PIXEL_F_MASK, mSrcMode[_COMP_F], src1d, src2d, srcIm)) return true;
	return false;
}

// #pragma mark -

/***************************************************************************
 * Misc
 ***************************************************************************/
static void _verify_dest(	vector<GlImage*>& destImgs, vector<GlImage*>& srcImgs,
							GlNodeDataList& list)
{
	if (destImgs.size() > 0) return;
	if (srcImgs.size() == 1) {
		destImgs.push_back(srcImgs[0]);
		list.AddImage(srcImgs[0]);	
		srcImgs.resize(0);
	} else if (srcImgs.size() > 1) {
		int32		l = 0, t = 0, r = 0, b = 0;
		for (uint32 k = 0; k < srcImgs.size(); k++) {
			int32	w = srcImgs[k]->Width(), h = srcImgs[k]->Height();
			int32	originX, originY;
			srcImgs[k]->GetOrigin(&originX, &originY);
			int32	cornerX = originX + w, cornerY = originY + h;
			if (k == 0) {
				l = originX; t = originY; r = cornerX; b = cornerY;
			} else {
				if (originX < l) l = originX;
				if (originY < t) t = originY;
				if (cornerX > r) r = cornerX;
				if (cornerY > b) b = cornerY;
			}
		}
		if (l >= r || t >= b) return;
		ArpVoxel	bg(255, 255, 255, 0, 0), fg(0, 0, 0, 255, 255);
		GlImage*	img = new GlImage(r - l, b - t, &bg, &fg);
		if (!img) return;
		destImgs.push_back(img);
		list.AddImage(img);
	}
}

static void _align_sources(vector<GlImage*>& srcImgs)
{
	if (srcImgs.size() < 2) return;
	int32			originX, originY, x, y;
	uint32			k;
	srcImgs[0]->GetOrigin(&originX, &originY);
	for (k = 1; k < srcImgs.size(); k++) {
		srcImgs[k]->GetOrigin(&x, &y);
		if (x < originX) originX = x;
		if (y < originY) originY = y;
	}

	for (k = 0; k < srcImgs.size(); k++) {
		srcImgs[k]->GetOrigin(&x, &y);
		srcImgs[k]->SetOrigin(x - originX, y - originY);
	}
}

/* I replace the planeSrc values in the GlPlanes with my own src info.
 */
static void _override_plane_src(GlPlanes& pixels, int32* srcMode)
{
	for (uint32 k = 0; k < pixels.size; k++) {
		if (pixels.planeSrc[k] == GL_PIXEL_R_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_R]);
		else if (pixels.planeSrc[k] == GL_PIXEL_G_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_G]);
		else if (pixels.planeSrc[k] == GL_PIXEL_B_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_B]);
		else if (pixels.planeSrc[k] == GL_PIXEL_A_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_A]);
		else if (pixels.planeSrc[k] == GL_PIXEL_Z_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_Z]);
		else if (pixels.planeSrc[k] == GL_PIXEL_DIFF_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_DIFF]);
		else if (pixels.planeSrc[k] == GL_PIXEL_SPEC_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_SPEC]);
		else if (pixels.planeSrc[k] == GL_PIXEL_D_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_D]);
		else if (pixels.planeSrc[k] == GL_PIXEL_C_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_C]);
		else if (pixels.planeSrc[k] == GL_PIXEL_F_MASK) pixels.planeSrc[k] = uint32(srcMode[_COMP_F]);
		else { ArpASSERT(false); pixels.planeSrc[k] = 0; }
	}
}
