#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlResizeIm.h>

static const int32		_BGC_KEY		= 'bgc_';
static const uint32		_BGC_INDEX		= 0;
static const float		_INIT_REL		= 0.0f;
static const int32		_INIT_ABS		= 0;
static const float		_INIT_ABS_F		= float(_INIT_ABS);

/***************************************************************************
 * _GL-RESIZE-ALGO
 ***************************************************************************/
class _GlResizeAlgo : public GlAlgoIm
{
public:
	_GlResizeAlgo(	gl_node_id nid, const GlRelAbs& l, const GlRelAbs& t,
					const GlRelAbs& r, const GlRelAbs& b, GlAlgo* bgc);
	_GlResizeAlgo(const _GlResizeAlgo& o);

	virtual GlAlgo*		Clone() const;

//	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
//	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
	GlRelAbs			mL, mT, mR, mB;
	bool				mMaintainOrigin;

	GlImage*			Resize(GlImage* srcIm, ArpVoxel* bgC);
	status_t			Draw(GlPlanes* src, GlPlanes* dest, int32 left, int32 top);
	status_t			GetSize(BPoint lt, BPoint rb, int32* w, int32* h);
	ArpVoxel*			GetBgC(	GlNodeDataList& list, const gl_process_args* args,
								ArpVoxel& v);
};

/***************************************************************************
 * GL-RESIZE-IM
 ***************************************************************************/
GlResizeIm::GlResizeIm(const GlResizeImAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	VerifyChain(new GlChain(_BGC_KEY, GL_IMAGE_IO, SZ(SZ_Color), this));
}

GlResizeIm::GlResizeIm(const GlResizeIm& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlResizeIm::Clone() const
{
	return new GlResizeIm(*this);
}

GlAlgo* GlResizeIm::Generate(const gl_generate_args& args) const
{
	GlRelAbs		l, t, r, b;
	GlAlgo*			a = 0;
	GetChainParams(args, l, t, r, b, &a);
	return new _GlResizeAlgo(Id(), l, t, r, b, a);
}

status_t GlResizeIm::GetChainParams(const gl_generate_args& args,
									GlRelAbs& left, GlRelAbs& top,
									GlRelAbs& right, GlRelAbs& bottom,
									GlAlgo** outIm) const
{
	if (args.flags&GL_NODE_ICON_F) {
		int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
						h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
		left.rel = top.rel = right.rel = bottom.rel = 0;
		float			rat = 0.25;
		left.abs = int32(w * rat);
		top.abs = int32(h * rat);
		right.abs = -int32(w * rat);
		bottom.abs = -int32(h * rat);
		return B_OK;
	}

	BPoint				l(Params().Point(GL_LEFT_KEY)),
						t(Params().Point(GL_TOP_KEY)),
						r(Params().Point(GL_RIGHT_KEY)),
						b(Params().Point(GL_BOTTOM_KEY));

	left.Set(	l.x, ARP_ROUND(l.y));
	top.Set(	t.x, ARP_ROUND(t.y));
	right.Set(	r.x, ARP_ROUND(r.y));
	bottom.Set(	b.x, ARP_ROUND(b.y));

	if (outIm) *outIm = GenerateChainAlgo(_BGC_KEY, args);
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * GL-RESIZE-IM-ADD-ON
 ***************************************************************************/
GlResizeImAddOn::GlResizeImAddOn()
		: inherited(SZI[SZI_arp], GL_RESIZE_KEY, SZ(SZ_Images), SZ(SZ_Resize), 1, 0)
{
	AddParamType(new GlPointParamType(GL_LEFT_KEY, SZ(SZ_Left), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1024), BPoint(10, 1024), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
	AddParamType(new GlPointParamType(GL_TOP_KEY, SZ(SZ_Top), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1024), BPoint(10, 1024), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
	AddParamType(new GlPointParamType(GL_RIGHT_KEY, SZ(SZ_Right), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1024), BPoint(10, 1024), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
	AddParamType(new GlPointParamType(GL_BOTTOM_KEY, SZ(SZ_Bottom), SZ(SZ_Rel), SZ(SZ_Abs), BPoint(-10, -1024), BPoint(10, 1024), BPoint(_INIT_REL, _INIT_ABS_F), 0.1f));
}

GlNode* GlResizeImAddOn::NewInstance(const BMessage* config) const
{
	return new GlResizeIm(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-RESIZE-ALGO
 ***************************************************************************/
_GlResizeAlgo::_GlResizeAlgo(	gl_node_id nid, const GlRelAbs& l, const GlRelAbs& t,
								const GlRelAbs& r, const GlRelAbs& b, GlAlgo* bgc)
		: inherited(nid), mL(l), mT(t), mR(r), mB(b)
{
	if (bgc) SetChain(bgc, _BGC_INDEX);
}

_GlResizeAlgo::_GlResizeAlgo(const _GlResizeAlgo& o)
		: inherited(o), mL(o.mL), mT(o.mT), mR(o.mR), mB(o.mB)
{
}

GlAlgo* _GlResizeAlgo::Clone() const
{
	return new _GlResizeAlgo(*this);
}

status_t _GlResizeAlgo::Perform(GlNodeDataList& list,
								const gl_process_args* args)
{
	GlNodeDataList			destList;
	ArpVoxel				voxel;
	ArpVoxel*				bgC = GetBgC(list, args, voxel);
	GlImage*				srcIm;
	uint32					k;
	for (k = 0; (srcIm = list.ImageAt(k)) != 0; k++) {
		GlImage*			destIm = Resize(srcIm, bgC);
		if (destIm) {
			if (destList.AddImage(destIm) != B_OK) delete destIm;
		}
	}
	list.DeleteContents();
	list.MergeList(&destList);
	return B_OK;
}

GlImage* _GlResizeAlgo::Resize(GlImage* srcIm, ArpVoxel* bgC)
{
	ArpVALIDATE(srcIm, return 0);
	if (srcIm->IsSolid()) return srcIm->Clone();

	int32				w = srcIm->Width(), h = srcIm->Height();
	BRect				r(	mL.rel * w + mL.abs, mT.rel * h + mT.abs,
							mR.rel * w + mR.abs, mB.rel * h + mB.abs);
	int32				oldW = w, oldH = h;
	int32				newW = oldW, newH = oldH;
	if (GetSize(r.LeftTop(), r.RightBottom(), &newW, &newH) != B_OK) return 0;

	ArpVoxel			bg = srcIm->Property(ARP_BACKGROUND_COLOUR),
						fg = srcIm->Property(ARP_FOREGROUND_COLOUR);
	GlImage*			destIm = new GlImage(newW, newH, (bgC) ? bgC : &bg, &fg, srcIm->Fields());
	if (!destIm || destIm->InitCheck() != B_OK) {
		delete destIm;
		return 0;
	}

	GlPlanes*			src = srcIm->LockPixels(GL_PIXEL_ALL);
	GlPlanes*			dest = destIm->LockPixels(GL_PIXEL_ALL);
	if (src && dest) Draw(src, dest, 0 - int32(r.left), 0 - int32(r.top));
	srcIm->UnlockPixels(src);
	destIm->UnlockPixels(dest);

	return destIm;
}

status_t _GlResizeAlgo::Draw(GlPlanes* src, GlPlanes* dest, int32 left, int32 top)
{
	ArpVALIDATE(src && dest, return B_ERROR);
	ArpVALIDATE(src->size == dest->size, return B_ERROR);

	int32			srcY = (top < 0) ? (0-top) : 0;
	for (int32 y = (top > 0) ? top : 0; y < dest->h; y++) {
		int32		srcX = (left < 0) ? (0-left) : 0;
		for (int32 x = (left > 0) ? left : 0; x < dest->w; x++) {
			int32	destPix = ARP_PIXEL(x, y, dest->w),
					srcPix = ARP_PIXEL(srcX, srcY, src->w);
			for (uint32 k = 0; k < src->size; k++)
				dest->plane[k][destPix] = src->plane[k][srcPix];
			srcX++;
			if (srcX >= src->w) break;
		}
		srcY++;
		if (srcY >= src->h) break;
	}
	return B_OK;
}

status_t _GlResizeAlgo::GetSize(BPoint lt, BPoint rb, int32* w, int32* h)
{
	*w = *w - int32(lt.x - rb.x);
	*h = *h - int32(lt.y - rb.y);
	if (*w < 1 || *h < 1) return B_ERROR;
	return B_OK;
}

ArpVoxel* _GlResizeAlgo::GetBgC(GlNodeDataList& list, const gl_process_args* args,
								ArpVoxel& v)
{
	GlAlgo*		a = AlgoAt(_BGC_INDEX);
	if (!a) return 0;

	/* Get bg image, if any.  First I have to stuff all my
	 * list images as backpointers in the supplied list.
	 */
	GlNodeDataList		imgList;
	GlImage*			img;
	for (uint32 k = 0; (img = list.ImageAt(k)) != 0; k++) {
		GlNodeDataBackPointer*	bp = new GlNodeDataBackPointer(img);
		if (bp) imgList.AddData(bp);
	}
	a->PerformAll(imgList, args);
	img = imgList.ImageAt(0);
	if (!img) return 0;

	GlPlanes*			p = img->LockPixels(GL_PIXEL_RGBAZ, true);
	if (!p) return 0;
	ArpVoxel*			ans = 0;
	if (p->r && p->g && p->b && p->a && p->z && p->w > 0 && p->h > 0) {
		v.r = p->r[0];
		v.g = p->g[0];
		v.b = p->b[0];
		v.a = p->a[0];
		v.z = p->z[0];
		ans = &v;
	}
	img->UnlockPixels(p); 
	return ans;
}
