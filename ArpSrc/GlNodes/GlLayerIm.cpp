#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpMenuControl.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlCache2d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlLayerIm.h>

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

static const int32		_IMAGE_KEY			= 'img_';
static const int32		_IMAGE_INDEX		= 0;
static const int32		_MASK_KEY			= 'mask';
static const int32		_MASK_INDEX			= 1;
static const int32		_LAYER_KEY			= 'lyr_';

static void _verify_dest(	std::vector<GlImage*>& destImgs, std::vector<GlImage*>& layers,
							GlNodeDataList& list);
static void _align_sources(std::vector<GlImage*>& layers);

/***************************************************************************
 * _GL-LAYER-ALGO
 ***************************************************************************/
class _GlLayerAlgo : public GlAlgoIm
{
public:
	_GlLayerAlgo(	gl_node_id nid, int32 mode, int32 onMask, bool align,
					GlAlgo* image, GlAlgo* mask, std::vector<GlAlgo*>& layers);
	_GlLayerAlgo(const _GlLayerAlgo& o);

	virtual GlAlgo*		Clone() const;

//	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
//	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
	int32				mMode, mOnMask;
	bool				mAlign;

	GlCache2d			mDestCache;

	status_t			Perform(GlImage* srcImage, GlPlanes& destPixels,
								uint8* destMask, int32 destOriginX,
								int32 destOriginY);
	status_t			PerformReplaceAlpha(GlImage* srcImage, GlPlanes& dest,
											uint8* destMask, int32 destOriginX,
											int32 destOriginY);

	status_t			PerformMask(GlNodeDataList& list, const gl_process_args* args,
									GlAlgo2dWrap& maskWrap);

	status_t			GetLayerImages(	GlNodeDataList& inList,
										GlNodeDataList& layerList,
										const gl_process_args* args);
};

/***************************************************************************
 * GL-LAYER-IM
 ***************************************************************************/
GlLayerIm::GlLayerIm(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	VerifyChain(new GlChain(_IMAGE_KEY, GL_IMAGE_IO, SZ(SZ_Image), this));
	VerifyChain(new GlChain(_MASK_KEY, GL_2D_IO, SZ(SZ_Mask), this));
	VerifyChain(new GlChain(_LAYER_KEY, GL_IMAGE_IO, SZ(SZ_Layer), this, 1));
}

GlLayerIm::GlLayerIm(const GlLayerIm& o)
		: inherited(o)
{
}

GlNode* GlLayerIm::Clone() const
{
	return new GlLayerIm(*this);
}

GlAlgo* GlLayerIm::Generate(const gl_generate_args& args) const
{
	const GlParamList&	params = Params();
	int32				mode = params.Menu(GL_MODE_PARAM_KEY);
	int32				onMask = 0;
	if (params.Bool(_R_ON, true))		onMask |= GL_PIXEL_R_MASK;
	if (params.Bool(_G_ON, true))		onMask |= GL_PIXEL_G_MASK;
	if (params.Bool(_B_ON, true))		onMask |= GL_PIXEL_B_MASK;
	if (params.Bool(_A_ON, false))		onMask |= GL_PIXEL_A_MASK;
	if (params.Bool(_Z_ON, false))		onMask |= GL_PIXEL_Z_MASK;
	if (params.Bool(_DIFF_ON, false))	onMask |= GL_PIXEL_DIFF_MASK;
	if (params.Bool(_SPEC_ON, false))	onMask |= GL_PIXEL_SPEC_MASK;
	if (params.Bool(_D_ON, false))		onMask |= GL_PIXEL_D_MASK;
	if (params.Bool(_C_ON, false))		onMask |= GL_PIXEL_C_MASK;
	if (params.Bool(_F_ON, false))		onMask |= GL_PIXEL_F_MASK;
	bool				align = Params().Bool(GL_COMPOSITE_ALIGN_SOURCES_KEY, false);

	GlAlgo*				image = GenerateChainAlgo(_IMAGE_KEY, args);
	GlAlgo*				mask = GenerateChainAlgo(_MASK_KEY, args);
	/* Since I can have a variable number of chains I'm a little more
	 * complicated than the typical Generate().  Make an algo for each
	 * chain and put it in the correct container.
	 */
	std::vector<GlAlgo*>		layers;
	uint32				size = ChainSize();
	for (uint32 k = 0; k < size; k++) {
		const GlChain*	c = ChainAt(k);
		if (c && c->Key() == _LAYER_KEY) {
			GlAlgo*		a = c->Generate(args);
			if (a) layers.push_back(a);
		}
	}

	return new _GlLayerAlgo(Id(), mode, onMask, align, image, mask, layers);
}

status_t GlLayerIm::Process(	GlNodeDataList& list,
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
 * GL-LAYER-IM-ADD-ON
 ***************************************************************************/
static GlParamType* new_mode_type()
{
	BMessage		msg;
	msg.AddString(ARP_MENU_ITEM_STR, "Replace");			msg.AddInt32(ARP_MENU_I_STR, REPLACE_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, "Replace (Alpha)");	msg.AddInt32(ARP_MENU_I_STR, REPLACE_ALPHA_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, *SZ(SZ_Average));		msg.AddInt32(ARP_MENU_I_STR, AVERAGE_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, *SZ(SZ_Min));			msg.AddInt32(ARP_MENU_I_STR, MIN_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, *SZ(SZ_Max));			msg.AddInt32(ARP_MENU_I_STR, MAX_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, *SZ(SZ_Add));			msg.AddInt32(ARP_MENU_I_STR, ADD_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, *SZ(SZ_Subtract));	msg.AddInt32(ARP_MENU_I_STR, SUB_MODE);
	msg.AddString(ARP_MENU_ITEM_STR, "Center");				msg.AddInt32(ARP_MENU_I_STR, CENTER_MODE);
	return new GlMenuParamType(GL_MODE_PARAM_KEY, SZ(SZ_Mode), msg, REPLACE_MODE);
}

GlLayerImAddOn::GlLayerImAddOn()
		: inherited(SZI[SZI_arp], GL_LAYER_KEY, SZ(SZ_Combine), SZ(SZ_Layer), 1, 0)
{
	AddParamType(new_mode_type());
	// Color
	AddParamType(new GlBoolParamType(	_R_ON, SZ(SZ_Red), true));
	AddParamType(new GlBoolParamType(	_G_ON, SZ(SZ_Green), true));
	AddParamType(new GlBoolParamType(	_B_ON, SZ(SZ_Blue), true));
	AddParamType(new GlBoolParamType(	_A_ON, SZ(SZ_Alpha), false));
	// Light
	AddParamType(new GlBoolParamType(	_Z_ON, SZ(SZ_Depth), false));
	AddParamType(new GlBoolParamType(	_DIFF_ON, SZ(SZ_Diffusion), false));
	AddParamType(new GlBoolParamType(	_SPEC_ON, SZ(SZ_Specularity), false));
	// Material
	AddParamType(new GlBoolParamType(	_D_ON, SZ(SZ_Density), false));
	AddParamType(new GlBoolParamType(	_C_ON, SZ(SZ_Cohesion), false));
	AddParamType(new GlBoolParamType(	_F_ON, SZ(SZ_Fluidity), false));

	AddParamType(new GlBoolParamType(GL_COMPOSITE_ALIGN_SOURCES_KEY, SZ(SZ_Align_sources), false));
}

GlNode* GlLayerImAddOn::NewInstance(const BMessage* config) const
{
	return new GlLayerIm(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-LAYER-ALGO
 ***************************************************************************/
_GlLayerAlgo::_GlLayerAlgo(	gl_node_id nid, int32 mode, int32 onMask, bool align,
							GlAlgo* image, GlAlgo* mask, std::vector<GlAlgo*>& layers)
		: inherited(nid), mMode(mode), mOnMask(onMask), mAlign(align)
{
	for (uint32 k = 0; k < layers.size(); k++) {
		if (layers[k]) SetChain(layers[k], k + 2);
	}
	if (image) SetChain(image, _IMAGE_INDEX);
	if (mask) SetChain(mask, _MASK_INDEX);
}

_GlLayerAlgo::_GlLayerAlgo(const _GlLayerAlgo& o)
		: inherited(o), mMode(o.mMode), mOnMask(o.mOnMask), mAlign(o.mAlign)
{
}

GlAlgo* _GlLayerAlgo::Clone() const
{
	return new _GlLayerAlgo(*this);
}

#define DO_OP(destV, srcV, mode, srcMode, srcM, destM) \
	(mode == AVERAGE_MODE) ? (destV + ((int16(srcV) - destV) * (srcM * destM))) \
		: (mode == MIN_MODE) ? ((destV < srcV) ? destV : (destV - ((destV - srcV) * destM))) \
		: (mode == MAX_MODE) ? ((destV > srcV) ? destV : (destV + ((srcV - destV) * destM))) \
		: (mode == ADD_MODE) ? (destV + (srcV * destM)) \
		: (mode == SUB_MODE) ? (destV - (srcV * destM)) \
		: (mode == CENTER_MODE) ? ( (srcV < 128) ? (destV - srcV) : (destV + (srcV - 128)) ) \
		: (srcV * (srcM * destM))
		// Default to pixel REPLACE

//		: (dest + ((src - dest) * (srcM * destM)))

//		: arp_clip_255(src * destM)
		// Default to REPLACE

status_t _GlLayerAlgo::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	GlAlgo*				imageAlgo = ChainAt(_IMAGE_INDEX);
	GlAlgo2dWrap		maskWrap(AlgoAt(_MASK_INDEX));
	/* First get any images generated by the layers.  This has to be done before
	 * I process my image because the layers might do their own processing, and
	 * they want the original image.
	 */
	GlNodeDataList		layerList;
	GetLayerImages(list, layerList, args);
	/* Now process my image, if I can.  I can take care of some simple cases
	 * here, too, namely when there are no layers: If there's a mask, use it
	 * as the source and return, otherwise just return.
	 */
	if (imageAlgo) imageAlgo->PerformAll(list, args);
	if (list.Size(list.IMAGE_TYPE) < 1) return list.DeleteContents();
	if (layerList.Size(list.IMAGE_TYPE) < 1) {
		if (maskWrap.InitCheck() == B_OK) return PerformMask(list, args, maskWrap);
		return B_OK;
	}
	
	GlImage*			destImg;
	GlImage*			srcImg;
	uint32				k;
	std::vector<GlImage*>	destImgs;
	std::vector<GlImage*>	layers;
	for (k = 0; (destImg = list.ImageAt(k)) != 0; k++) destImgs.push_back(destImg);
	while ((srcImg = layerList.DetachImage()) != 0) layers.push_back(srcImg);
	layerList.DeleteContents();
	
	if (destImgs.size() < 1 && layers.size() > 1 && mAlign)
		_align_sources(layers);

	/* If there are no destination images, one of two things happen:  Either there is
	 * a single source image, and it just becomes the destination, or there are multiple
	 * sources, and I create a new destination large enough for all of them.
	 */
	_verify_dest(destImgs, layers, list);

	for (k = 0; k < destImgs.size(); k++) {
		destImg = destImgs[k];
		int32			destOriginX, destOriginY;
		destImg->GetOrigin(&destOriginX, &destOriginY);
		GlPlanes*		destPixels;
		if ((destPixels = destImg->LockPixels(mOnMask)) != 0) {
			uint8*		destMask = 0;
			if (maskWrap.InitCheck() == B_OK)
				destMask = maskWrap.Cache(mDestCache, destPixels, (args) ? args->status : 0);
			for (uint32 j = 0; j < layers.size(); j++) {
				srcImg = layers[j];
				if (srcImg->InitCheck() == B_OK) {
					Perform(srcImg, *destPixels, destMask, destOriginX, destOriginY);
				}
			}
			destImg->UnlockPixels(destPixels);
		}
	}

	for (k = 0; k < layers.size(); k++) delete layers[k];
	return B_OK;
}

status_t _GlLayerAlgo::Perform(	GlImage* srcImage, GlPlanes& dest,
								uint8* destMask, int32 destOriginX,
								int32 destOriginY)
{
	if (dest.size < 1) return B_ERROR;
	if (mMode == REPLACE_ALPHA_MODE) return PerformReplaceAlpha(srcImage, dest, destMask,
																destOriginX, destOriginY);
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
					srcM = glTable256[src->a[srcPix]];
				}
				if (destMask) destM = glTable256[destMask[destPix]];
				else destM = 1.0;
				
				for (k = 0; k < dest.size; k++) {
//					uint8		srcV = GET_SRC(dest.planeSrc[k], src, srcPix, dest.plane[k][destPix]);
					uint8		srcV = src->plane[k][srcPix];
					float		v = DO_OP(dest.plane[k][destPix], srcV, mMode, dest.planeSrc[k], srcM, destM);
					dest.plane[k][destPix] = arp_clip_255(v);
				}
			}
			destPix++;
		}
	}
	if (srcImage) srcImage->UnlockPixels(src);
	else delete src;
	return B_OK;
}

status_t _GlLayerAlgo::PerformReplaceAlpha(	GlImage* srcImage, GlPlanes& dest,
											uint8* destMask, int32 destOriginX,
											int32 destOriginY)
{
/* FIX: THIS IS A TOTAL HACK UNTIL I FIGURE OUT HOW TO GET THIS @#$EWR#R
 * COMPOSITE NODE FULL FEATURED BUT EASY
 */
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
//					srcM = glTable256[srcMask[srcPix]];
					srcM = 1;
				}
				if (destMask) destM = glTable256[destMask[destPix]];
				else if (dest.a) destM = glTable256[dest.a[destPix]];
				
				for (k = 0; k < dest.size; k++) {
//					uint8		srcV = GET_SRC(dest.planeSrc[k], src, srcPix, dest.plane[k][destPix]);
					uint8		srcV = src->plane[k][srcPix];
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
	return B_OK;
}

/* FIX:  Now that I look at it, this should probably just be a different node.
 */
status_t _GlLayerAlgo::PerformMask(	GlNodeDataList& list, const gl_process_args* args,
									GlAlgo2dWrap& maskWrap)
{
	ArpVALIDATE(maskWrap.InitCheck() == B_OK, return B_ERROR);
	if (mOnMask == 0) return B_OK;
	GlCache2d			cache;
	GlImage*			img;
	for (uint32 k = 0; (img = list.ImageAt(k)) != 0; k++) {
		GlPlanes*		p = img->LockPixels(mOnMask);
		if (p && p->HasPlanes(mOnMask)) {
			uint8*		mask = maskWrap.Cache(cache, p, (args) ? args->status : 0);
			if (mask) {
				int32	size = p->w * p->h;
				for (int32 pix = 0; pix < size; pix++) {
					for (uint32 t = 0; t < p->size; t++) {
						if (mMode == REPLACE_MODE) p->plane[t][pix] = mask[pix];
						else {
							float	v = float(DO_OP(p->plane[t][pix], mask[pix], mMode, p->planeSrc[t], 1, 1));
							p->plane[t][pix] = arp_clip_255(v);
						}
					}
				}
			}
		}
		img->UnlockPixels(p);
	}
	return B_OK;
}

status_t _GlLayerAlgo::GetLayerImages(	GlNodeDataList& inList,
										GlNodeDataList& layerList,
										const gl_process_args* args)
{
	/* Get layer images, if any.  For each layer, I have to stuff all
	 * my input list images as backpointers in the list to be performed.
	 */
	uint32					size = ChainSize();
	for (uint32 k = 2; k < size; k++) {
		GlAlgo*				a = ChainAt(k);
		if (a) {
			GlImage*		img;
			GlNodeDataList	l;
			for (uint32 i = 0; (img = inList.ImageAt(i)) != 0; i++) {
				GlNodeDataBackPointer*	bp = new GlNodeDataBackPointer(img);
				if (bp) l.AddData(bp);
			}
			a->PerformAll(l, args);
			while ((img = l.DetachImage()) != 0) layerList.AddImage(img);
		}
	}
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * Misc
 ***************************************************************************/
static void _verify_dest(	std::vector<GlImage*>& destImgs, std::vector<GlImage*>& layers,
							GlNodeDataList& list)
{
	if (destImgs.size() > 0) return;
	if (layers.size() == 1) {
		destImgs.push_back(layers[0]);
		list.AddImage(layers[0]);	
		layers.resize(0);
	} else if (layers.size() > 1) {
		int32		l = 0, t = 0, r = 0, b = 0;
		for (uint32 k = 0; k < layers.size(); k++) {
			int32	w = layers[k]->Width(), h = layers[k]->Height();
			int32	originX, originY;
			layers[k]->GetOrigin(&originX, &originY);
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

static void _align_sources(std::vector<GlImage*>& layers)
{
	if (layers.size() < 2) return;
	int32			originX, originY, x, y;
	uint32			k;
	layers[0]->GetOrigin(&originX, &originY);
	for (k = 1; k < layers.size(); k++) {
		layers[k]->GetOrigin(&x, &y);
		if (x < originX) originX = x;
		if (y < originY) originY = y;
	}

	for (k = 0; k < layers.size(); k++) {
		layers[k]->GetOrigin(&x, &y);
		layers[k]->SetOrigin(x - originX, y - originY);
	}
}
