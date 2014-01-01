#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpMenuControl.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlValue.h>

static const int32		GL_VALUE_SRF_KEY	= 'AsVa';

enum {
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

/***************************************************************************
 * _GL-VALUE-2D
 ****************************************************************************/
class _GlValue2d : public GlAlgo2d
{
public:
	_GlValue2d(gl_node_id nid, uint32 targets, int32 src);
	_GlValue2d(const _GlValue2d& o);

	virtual GlAlgo*			Clone() const;
	virtual status_t		Process(const GlPlanes* src, GlPlanes& dest,
									GlProcessStatus* status = 0);

protected:
	virtual status_t		PerformImage(GlImage* img, const gl_process_args* args);
	status_t				ProcessImage(GlPlanes* p, GlProcessStatus* status);

private:
	typedef GlAlgo2d		inherited;
	int32					mSrc;

	status_t				Validate(const GlPlanes& p) const;
};

/***************************************************************************
 * GL-VALUE
 ***************************************************************************/
GlValue::GlValue(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlValue::GlValue(const GlValue& o)
		: inherited(o)
{
}

GlNode* GlValue::Clone() const
{
	return new GlValue(*this);
}

GlAlgo* GlValue::Generate(const gl_generate_args& args) const
{
	return new _GlValue2d(Id(), PixelTargets(), Params().Menu('sr'));
}

// #pragma mark -

/***************************************************************************
 * GL-VALUE-ADD-ON
 ***************************************************************************/
GlValueAddOn::GlValueAddOn()
		: inherited(SZI[SZI_arp], GL_VALUE_SRF_KEY, SZ(SZ_Color), SZ(SZ_Value), 1, 0)
{
	BMessage		msg;
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Red));			msg.AddInt32(ARP_MENU_I_STR, RED_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Green));		msg.AddInt32(ARP_MENU_I_STR, GREEN_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Blue));		msg.AddInt32(ARP_MENU_I_STR, BLUE_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Alpha));		msg.AddInt32(ARP_MENU_I_STR, ALPHA_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Hue));			msg.AddInt32(ARP_MENU_I_STR, HUE_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Saturation));	msg.AddInt32(ARP_MENU_I_STR, SATURATION_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Value));		msg.AddInt32(ARP_MENU_I_STR, VALUE_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Depth));		msg.AddInt32(ARP_MENU_I_STR, DEPTH_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Diffusion));	msg.AddInt32(ARP_MENU_I_STR, DIFFUSION_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Specularity));	msg.AddInt32(ARP_MENU_I_STR, SPECULARITY_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Density));		msg.AddInt32(ARP_MENU_I_STR, DENSITY_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Cohesion));	msg.AddInt32(ARP_MENU_I_STR, COHESION_SRC);
	msg.AddString16(ARP_MENU_ITEM_STR, SZ(SZ_Fluidity));	msg.AddInt32(ARP_MENU_I_STR, FLUIDITY_SRC);
	AddParamType(new GlMenuParamType('src_', SZ(SZ_Source), msg, RED_SRC));
}

GlNode* GlValueAddOn::NewInstance(const BMessage* config) const
{
	return new GlValue(this, config);
}

GlImage* GlValueAddOn::NewImage() const
{
	int32				w = Prefs().GetInt32(GL_NODE_IMAGE_X),
						h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;
	
	GlImage*			img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	GlNode*				node = NewInstance(0);
	if (!node) return img;

	node->IncRefs();
	gl_generate_args	args;
	args.flags = GL_NODE_ICON_F;
	GlAlgo*				a = node->Generate(args);
	node->MakeEmpty();
	node->DecRefs();

	GlAlgo2d*			s = (a) ? a->As2d() : 0;
	GlPlanes*			p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		GlFillType		fill = GL_FILL_NONE;
		if (s) fill = s->FillType();
		if (fill == GL_FILL_BLACK) p->Black();
		else if (fill == GL_FILL_COLORWHEEL) p->ColorWheel(0, 0, w, h);

		GlMask			mask;
		uint8*			data;
		if (s && (data = mask.Make(*p, s)) != 0) {
			/* Render the surface.
			 */
			for (int32 pix = 0; pix < p->w * p->h; pix++) {
				if (p->a[pix] > 0) p->a[pix] = data[pix];
			}
		}
		img->UnlockPixels(p);
	}

	delete a;
	return img;
}

// #pragma mark -

/***************************************************************************
 * _GL-VALUE-2D
 ****************************************************************************/
_GlValue2d::_GlValue2d(gl_node_id nid, uint32 targets, int32 src)
		: inherited(nid, targets, GL_FILL_COLORWHEEL), mSrc(src)
{
}

_GlValue2d::_GlValue2d(const _GlValue2d& o)
		: inherited(o), mSrc(o.mSrc)
{
}

GlAlgo* _GlValue2d::Clone() const
{
	return new _GlValue2d(*this);
}

#define _GET_SRC(src, pixels, pix) \
	(src == RED_SRC)				? (pixels->r[pix]) \
		: (src == GREEN_SRC)		? (pixels->g[pix]) \
		: (src == BLUE_SRC)			? (pixels->b[pix]) \
		: (src == ALPHA_SRC)		? (pixels->a[pix]) \
		: (src == HUE_SRC)			? (pixels->Hue(pix)) \
		: (src == SATURATION_SRC)	? (pixels->Saturation(pix)) \
		: (src == VALUE_SRC)		? (pixels->Value(pix)) \
		: (src == DEPTH_SRC)		? (pixels->z[pix]) \
		: (src == DIFFUSION_SRC)	? (pixels->diff[pix]) \
		: (src == SPECULARITY_SRC)	? (pixels->spec[pix]) \
		: (src == DENSITY_SRC)		? (pixels->d[pix]) \
		: (src == COHESION_SRC)		? (pixels->c[pix]) \
		: (src == FLUIDITY_SRC)		? (pixels->f[pix]) \
		: 0

status_t _GlValue2d::Process(	const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status)
{
	if (!src || dest.size < 1) return B_OK;
	if (Validate(*src) != B_OK) return B_OK;
	
	for (int32 pix = 0; pix < dest.w * dest.h; pix++) {
		if (dest.size > 1 || dest.plane[0][pix] > 0) {
			uint8		val = _GET_SRC(mSrc, src, pix);
			for (uint32 k = 0; k < dest.size; k++)
				dest.plane[k][pix] = ARP_MIN(dest.plane[k][pix], val);
		}
	}
	return B_OK;
}

status_t _GlValue2d::PerformImage(GlImage* img, const gl_process_args* args)
{
	if (mTargets == 0) return B_OK;
	if (img->InitCheck() != B_OK) return B_ERROR;
	GlPlanes*		pixels = img->LockPixels(mTargets, true);
	ArpASSERT(pixels);
	status_t		err = B_OK;
	if (pixels) err = ProcessImage(pixels, (args) ? args->status : 0);
	img->UnlockPixels(pixels);
	return err;
}

status_t _GlValue2d::ProcessImage(GlPlanes* p, GlProcessStatus* status)
{
	if (p->size < 1) return B_OK;
	if (Validate(*p) != B_OK) return B_OK;
	
	for (int32 pix = 0; pix < p->w * p->h; pix++) {
		if (p->size > 1 || p->plane[0][pix] > 0) {
			uint8		val = _GET_SRC(mSrc, p, pix);
			for (uint32 k = 0; k < p->size; k++)
				p->plane[k][pix] = val;
		}
	}
	return B_OK;
}

status_t _GlValue2d::Validate(const GlPlanes& p) const
{
	ArpVALIDATE(p.HasColor(), return B_ERROR);
	if ( (mSrc == DIFFUSION_SRC && !(p.diff))
			|| (mSrc == SPECULARITY_SRC && !(p.spec))
			|| (mSrc == DENSITY_SRC && !(p.d))
			|| (mSrc == COHESION_SRC && !(p.c))
			|| (mSrc == FLUIDITY_SRC && !(p.f)) )
		return B_ERROR;
	return B_OK;
}