#include <cstring>
#include <ArpInterface/ArpBitmap.h>
#include <GlPublic/GlDefs.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlNodeData.h>
#include <GlPublic/GlParamWrap.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlPixel.h>

static int32 _COLOR_PLANES		= 5;
static int32 _LIGHT_PLANES		= 2;
static int32 _MATERIAL_PLANES	= 3;

static uint8 _INIT_DIFF			= 255;
static uint8 _INIT_SPEC			= 0;
static uint8 _INIT_D			= 32;
static uint8 _INIT_C			= 127;
static uint8 _INIT_F			= 8;

/***************************************************************************
 * GL-IMAGE
 ****************************************************************************/
GlImage::GlImage()
		: mColor(0), mLight(0), mMaterial(0), mSolidCache(0)
{
	Free();
	InitProperties();
}

GlImage::GlImage(	int32 width, int32 height, const ArpVoxel* bg,
					const ArpVoxel* fg, uint32 fields)
		: mColor(0), mLight(0), mMaterial(0), mSolidCache(0)
{
	Free();
	InitProperties();
	
	if (bg) SetProperty(ARP_BACKGROUND_COLOUR, *bg);
	if (fg) SetProperty(ARP_FOREGROUND_COLOUR, *fg);

	int32			size = width * height;
	if (size <= 0) {
		mIsSolid = true;
		mStatus = B_OK;
		return;
	}
	mColor = new uint8[size * _COLOR_PLANES];
	bool		light = (fields&GL_PIXEL_DIFF_MASK || fields&GL_PIXEL_SPEC_MASK),
				material = (fields&GL_PIXEL_D_MASK || fields&GL_PIXEL_C_MASK || fields&GL_PIXEL_F_MASK);
	if (light) mLight = new uint8[size * _LIGHT_PLANES];
	if (material) mMaterial = new uint8[size * _MATERIAL_PLANES];

	if (!mColor || (light && !mLight) || (material && !mMaterial)) {
		Free();
		return;
	} 
	mW = width;
	mH = height;
	mStatus = B_OK;

	if (bg) {
		memset(mColor,					bg->r,		size);
		memset(mColor + size,			bg->g,		size);
		memset(mColor + (size * 2),		bg->b,		size);
		memset(mColor + (size * 3),		bg->a,		size);
		memset(mColor + (size * 4),		bg->z,		size);
	}
	if (mLight) {
		memset(mLight,					_INIT_DIFF,	size);
		memset(mLight + size,			_INIT_SPEC,	size);
	}
	if (mMaterial) {
		memset(mMaterial,				_INIT_D,	size);
		memset(mMaterial + size,		_INIT_C,	size);
		memset(mMaterial + (size * 2),	_INIT_F,	size);
	}
}

GlImage::GlImage(const ArpVoxel& bg)
		: mColor(0), mLight(0), mMaterial(0), mSolidCache(0)
{
	Free();
	InitProperties();
	mBg = bg;
	mIsSolid = true;
	mStatus = B_OK;
}

GlImage::GlImage(const GlImage& o)
		: mStatus(B_ERROR), mW(0), mH(0), mColor(0), mLight(0), mMaterial(0),
		  mBounding(o.mBounding), mX(o.mX), mY(o.mY), mIsSolid(o.mIsSolid),
		  mBg(o.mBg), mFg(o.mFg), mSolidCache(0)
{
	int32			size = o.mW * o.mH;
	if (size > 0) {
		if (o.mColor) mColor = new uint8[size * _COLOR_PLANES];
		if (mColor) memcpy(mColor, o.mColor, size * _COLOR_PLANES);
		if (o.mLight) mLight = new uint8[size * _LIGHT_PLANES];
		if (mLight) memcpy(mLight, o.mLight, size * _LIGHT_PLANES);
		if (o.mMaterial) mMaterial = new uint8[size * _MATERIAL_PLANES];
		if (mMaterial) memcpy(mMaterial, o.mMaterial, size * _MATERIAL_PLANES);
	}

	if (mIsSolid) mStatus = B_OK;
	if (mColor || mLight || mMaterial) {
		mW = o.mW;
		mH = o.mH;
		mStatus = B_OK;
	}
}

GlImage::GlImage(const ArpBitmap& bm)
		: mColor(0), mLight(0), mMaterial(0), mSolidCache(0)
{
	Free();
	InitProperties();
	int32			size = bm.Width() * bm.Height();
	if (size > 0) {
		mColor = new uint8[size * _COLOR_PLANES];
		if (mColor) {
			mW = bm.Width();
			mH = bm.Height();
			mStatus = bm.Get(mColor, mColor + size, mColor + (size * 2), mColor + (size * 3), mW, mH);
		}
	}
}

GlImage::~GlImage()
{
	Free();
	delete[] mSolidCache;
}

gl_image_id GlImage::Id() const
{
	return (gl_image_id)this;
}

GlImage* GlImage::Clone() const
{
	return new GlImage(*this);
}

status_t GlImage::InitCheck() const
{
	return mStatus;
}

int32 GlImage::Width() const
{
	return mW;
}

int32 GlImage::Height() const
{
	return mH;
}

status_t GlImage::SetSize(int32 w, int32 h)
{
	if (mIsSolid) return B_ERROR;
	ArpVALIDATE(w > 0 && h > 0, return B_ERROR);
	/* FIX:  Make sure pixels aren't locked, as soon as the
	 * locking actually does something.
	 */
	uint8*			newColor = new uint8[(w * h) * _COLOR_PLANES];
	if (!newColor) return B_NO_MEMORY;
	uint8*			newLight = (mLight) ? (new uint8[(w * h) * _LIGHT_PLANES]) : 0;
	uint8*			newMaterial = (mMaterial) ? (new uint8[(w * h) * _MATERIAL_PLANES]) : 0;
	if ( (mLight && !newLight) || (mMaterial && !newMaterial) ) {
		delete[] newColor;
		delete[] newLight;
		delete[] newMaterial;
		return B_NO_MEMORY;
	}
	delete[] mColor;
	delete[] mLight;
	delete[] mMaterial;
	mColor = newColor;
	mLight = newLight;
	mMaterial = newMaterial;
	mW = w;
	mH = h;
	mStatus = B_OK;
	return mStatus;
}

uint32 GlImage::Fields() const
{
	uint32			f = 0;
	if (mColor)		f |= GL_PIXEL_COLOR;
	if (mLight)		f |= GL_PIXEL_LIGHT;
	if (mMaterial)	f |= GL_PIXEL_MATERIAL;
	return f;
}

ArpPixelRules GlImage::Bounding() const
{
	return mBounding;
}

void GlImage::SetBounding(ArpPixelRules bounding)
{
	mBounding = bounding;
}

void GlImage::GetOrigin(int32* x, int32* y)
{
	ArpASSERT(x && y);
	*x = mX;
	*y = mY;
}

void GlImage::SetOrigin(int32 x, int32 y)
{
	mX = x;
	mY = y;
}

ArpVoxel GlImage::Property(ArpColourConstant constant) const
{
	if (constant == ARP_FOREGROUND_COLOUR) return mFg;
	return mBg;
}

void GlImage::SetProperty(ArpColourConstant constant, const ArpVoxel& p)
{
	if (constant == ARP_BACKGROUND_COLOUR) mBg = p;
	else if (constant == ARP_FOREGROUND_COLOUR) mFg = p;
}

bool GlImage::IsSolid() const
{
	return mIsSolid;
}

GlPlanes* GlImage::LockPixels(uint32 targets, bool enforce)
{
	if (mStatus != B_OK) return 0;
	if (mIsSolid) {
		/* FIX: need to change the solid to a GlPixel and supply
		 * all its values.  Ugh.  That doesn't square with the other
		 * make pixels method, maybe I should be supplying each plane
		 * separately here.
		 */
		if (CacheSolid() != B_OK) return 0;
		return MakePixels(targets, mSolidCache, 0, 0, 1, 1);
	}
	if (mW <= 0 || mH <= 0) return 0;
	if (enforce) {
		if (targets&GL_PIXEL_R_MASK || targets&GL_PIXEL_G_MASK || targets&GL_PIXEL_B_MASK || targets&GL_PIXEL_A_MASK || targets&GL_PIXEL_Z_MASK) {
			ArpVALIDATE(mColor, return 0);
		}
		if ( (targets&GL_PIXEL_DIFF_MASK || targets&GL_PIXEL_SPEC_MASK) && !mLight) {
			mLight = new uint8[(mW * mH) * _LIGHT_PLANES];
			if (!mLight) return 0;
			memset(mLight, _INIT_DIFF, mW * mH);
			memset(mLight + (mW * mH), _INIT_SPEC, mW * mH);
		}
		if ( (targets&GL_PIXEL_D_MASK || targets&GL_PIXEL_C_MASK || targets&GL_PIXEL_F_MASK) && !mMaterial) {
			mMaterial = new uint8[(mW * mH) * _MATERIAL_PLANES];
			if (!mMaterial) return 0;
			memset(mMaterial, _INIT_D, mW * mH);
			memset(mMaterial + (mW * mH), _INIT_C, mW * mH);
			memset(mMaterial + ((mW * mH) * 2), _INIT_F, mW * mH);
		}
	}
	return MakePixels(targets, mColor, mLight, mMaterial, mW, mH);
}

void GlImage::UnlockPixels(GlPlanes* p) const
{
	/* Technically there should always be pixels, but some clients are lazy
	 * and I'm OK with that.
	 */
	if (!p) return;
//	ArpVALIDATE(p, return);
	if (!mIsSolid) ArpASSERT(mColor == p->r && mLight == p->diff && mMaterial == p->d);
	else ArpASSERT(mSolidCache == p->r);
	/* Always free before deleting -- deleting will also free, but right
	 * now there's a check to make sure I've been free'd, which means the
	 * unlock happen.  Assert gets hit, that means someone forgot to unlock.
	 */
	p->Free();
	delete p;
}

status_t GlImage::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	if (!mColor) return B_ERROR;
	/* FIX: Check to see if I'm locked, as soon as I have a mechanism for that.
	 */
	int32				size = mW * mH;
	memset(mColor,				r, size);
	memset(mColor + size,		g, size);
	memset(mColor + (size * 2),	b, size);
	memset(mColor + (size * 3),	a, size);
	return B_OK;
}

GlImage* GlImage::SourceClone(int32 width, int32 height)
{
	GlImage*		src = new GlImage();
	if (!src) return 0;
	uint8*			newColor = 0;
	uint8*			newLight = 0;
	uint8*			newMaterial = 0;
	if (width > 0 && height > 0) {
		int32		size = width * height;
		if ((newColor = new uint8[size * _COLOR_PLANES]) == 0) {
			delete src;
			return 0;
		}
		if (mLight && ((newLight = new uint8[size * _LIGHT_PLANES]) == 0)) {
			delete src;
			delete newColor;
			return 0;
		}
		if (mMaterial && ((newMaterial = new uint8[size * _MATERIAL_PLANES]) == 0)) {
			delete src;
			delete newColor;
			delete newLight;
			return 0;
		}
	}
	
	src->mStatus = mStatus;
	src->mColor = mColor;
	src->mLight = mLight;
	src->mMaterial = mMaterial;
	src->mW = mW;
	src->mH = mH;
	src->mBounding = mBounding;
	src->mX = mX;
	src->mY = mY;
	src->mIsSolid = mIsSolid;
	src->mBg = mBg;
	src->mFg = mFg;
	/* Invalidate me
	 */
	mColor = newColor;
	mLight = newLight;
	mMaterial = newMaterial;
	mW = width;
	mH = height;
	mStatus = (mColor) ? B_OK : B_ERROR;
	return src;
}

status_t GlImage::Take(GlImage& o)
{
	delete[] mColor;
	delete[] mLight;
	delete[] mMaterial;

	mStatus = o.mStatus;
	mW = o.mW;
	mH = o.mH;
	mColor = o.mColor;
	mLight = o.mLight;
	mMaterial = o.mMaterial;
	mBounding = o.mBounding;
	mX = o.mX;
	mY = o.mY;
	mIsSolid = o.mIsSolid;
	mBg = o.mBg;
	mFg = o.mFg;

	o.mStatus = B_ERROR;
	o.mW = o.mH = 0;	
	o.mColor = o.mLight = o.mMaterial = 0;	

	return B_OK;
}

static GlImage* _low_quality_scale(	uint8* srcR, uint8* srcG, uint8* srcB, uint8* srcA,
									int32 srcW, int32 srcH, int32 destW, int32 destH)
{
	ArpASSERT(srcR && srcG && srcB && srcA);
	GlImage*		newImg = new GlImage(destW, destH);
	if (!newImg) return 0;

	GlPlanes*		dest = newImg->LockPixels(GL_PIXEL_RGBA);
	if (!dest) {
		delete newImg;
		return 0;
	}
	ArpASSERT(dest->HasColor());
	
	double			dx = srcW / double(dest->w),
					dy = srcH / double(dest->h),
					py = 0.0;
	for (int32 y = 0; y < dest->h; y++) {
		double	px = 0.0;
		for (int32 x = 0; x < dest->w; x++) {
			int32	srcPix = ARP_PIXEL(int32(px), int32(py), srcW),
					destPix = ARP_PIXEL(x, y, dest->w);
			dest->r[destPix] = srcR[srcPix];
			dest->g[destPix] = srcG[srcPix];
			dest->b[destPix] = srcB[srcPix];
			dest->a[destPix] = srcA[srcPix];
			px += dx;
		}
		py += dy;
	}

	newImg->UnlockPixels(dest);	
	return newImg;
}

GlImage* GlImage::AsScaledImage(int32 newW, int32 newH, float quality) const
{
	ArpVALIDATE(mColor && mW > 0 && mH > 0 && newW > 0 && newH > 0, return 0);
	if (mW == newW && mH == newH) return Clone();

	int32				s = mW * mH;
	if (quality <= 0.5) return _low_quality_scale(	mColor, mColor + s, mColor + (s * 2),
													mColor + (s * 3), mW, mH, newW, newH);

	float				x = float(newW) / mW, y = float(newH) / mH;
	GlAlgo*				a = GlGlobals().NewAlgo(SZI[SZI_arp], GL_STRETCH_KEY);
	if (!a) return 0;

	GlImage*			newImg = Clone();
	if (!newImg) {
		delete a;
		return 0;
	}
	
	GlPointWrap			w(BPoint(x, x));
	a->SetParam(gl_param_key(0, GL_HRZ_KEY, 0), w);
	w.v.x = w.v.y = y;
	a->SetParam(gl_param_key(0, GL_VRT_KEY, 0), w);
	GlNodeDataList		list;
	list.AddImage(newImg);
	a->PerformAll(list, 0);
	return list.DetachImage();
}

ArpBitmap* GlImage::AsBitmap(int32 w, int32 h) const
{
	if (!mIsSolid) {
		if (!mColor) return 0;
		int32			s = mW * mH;
		return new ArpBitmap(mColor, mColor + s, mColor + (s * 2), mColor + (s * 3), mW, mH);
	}
	if (w < 1 || h < 1) return 0;
	return new ArpBitmap(mBg.r, mBg.g, mBg.b, mBg.a, w, h);
}

status_t GlImage::FromTextHack(	const ArpBitmap& bm,
								const ArpVoxel& bg,
								const ArpVoxel& fg)
{
	int32		w = bm.Width(), h = bm.Height();
	int32		size = w * h;
	if (size < 1) return B_ERROR;
	if (w != mW || h != mH) {
		Free();
		mColor = new uint8[size * _COLOR_PLANES];
		if (!mColor) return B_NO_MEMORY;
		mW = w;
		mH = h;
	}
	mStatus = bm.GetTextHack(	mColor, mColor + size, mColor + (size * 2),
								mColor + (size * 3), mColor + (size * 4),
								mW, mH, bg, fg);
	return mStatus;
}

status_t GlImage::LockColorHack(const uint8** r, const uint8** g,
								const uint8** b, const uint8** a,
								const uint8** z) const
{
	if (!mColor) return B_ERROR;
	if (r) *r = mColor;
	if (g) *g = mColor + (mW * mH);
	if (b) *b = mColor + (mW * mH) * 2;
	if (a) *a = mColor + (mW * mH) * 3;
	if (z) *z = mColor + (mW * mH) * 4;
	return B_OK;
}

void GlImage::UnlockColorHack(const uint8* r) const
{
}

GlPlanes* GlImage::NewPlanes() const
{
	return new GlPlanes();
}

status_t GlImage::CacheSolid()
{
	if (!mSolidCache) mSolidCache = new uint8[_COLOR_PLANES + _LIGHT_PLANES + _MATERIAL_PLANES];
	if (!mSolidCache) return B_NO_MEMORY;
	mSolidCache[0] = mBg.r;
	mSolidCache[1] = mBg.g;
	mSolidCache[2] = mBg.b;
	mSolidCache[3] = mBg.a;
	mSolidCache[4] = mBg.z;
	return B_OK;
}

GlPlanes* GlImage::MakePixels(	uint32 targets, uint8* color,
								uint8* light, uint8* material, int32 w, int32 h) const

{
	ArpVALIDATE(color, return 0);
	uint32			size = 0;
	if (targets&GL_PIXEL_R_MASK) size++;
	if (targets&GL_PIXEL_G_MASK) size++;
	if (targets&GL_PIXEL_B_MASK) size++;
	if (targets&GL_PIXEL_A_MASK) size++;
	if (targets&GL_PIXEL_Z_MASK) size++;
	if (light) {
		if (targets&GL_PIXEL_DIFF_MASK) size++;
		if (targets&GL_PIXEL_SPEC_MASK) size++;
	}
	if (material) {
		if (targets&GL_PIXEL_D_MASK) size++;
		if (targets&GL_PIXEL_C_MASK) size++;
		if (targets&GL_PIXEL_F_MASK) size++;
	}
	if (size < 1) return 0;

	GlPlanes*		p = NewPlanes();
	if (!p) return 0;
	p->plane = new uint8*[size];
	p->planeSrc = new uint32[size];
	if (!(p->plane) || !(p->planeSrc)) {
		p->Free();
		delete p;
		return 0;
	}
	p->w = w;
	p->h = h;

	p->r = color;
	p->g = color + (w * h);
	p->b = color + (w * h) * 2;
	p->a = color + (w * h) * 3;
	p->z = color + (w * h) * 4;
	if (targets&GL_PIXEL_R_MASK)		{ p->plane[p->size] = p->r;	p->planeSrc[p->size++] = GL_PIXEL_R_MASK; }
	if (targets&GL_PIXEL_G_MASK)		{ p->plane[p->size] = p->g;	p->planeSrc[p->size++] = GL_PIXEL_G_MASK; }
	if (targets&GL_PIXEL_B_MASK)		{ p->plane[p->size] = p->b;	p->planeSrc[p->size++] = GL_PIXEL_B_MASK; }
	if (targets&GL_PIXEL_A_MASK)		{ p->plane[p->size] = p->a;	p->planeSrc[p->size++] = GL_PIXEL_A_MASK; }
	if (targets&GL_PIXEL_Z_MASK)		{ p->plane[p->size] = p->z;	p->planeSrc[p->size++] = GL_PIXEL_Z_MASK; }
	if (light) {
		p->diff = light;
		p->spec = light + (w * h);
		if (targets&GL_PIXEL_DIFF_MASK)	{ p->plane[p->size] = p->diff;	p->planeSrc[p->size++] = GL_PIXEL_DIFF_MASK; }
		if (targets&GL_PIXEL_SPEC_MASK)	{ p->plane[p->size] = p->spec;	p->planeSrc[p->size++] = GL_PIXEL_SPEC_MASK; }
	}
	if (material) {
		p->d = material;
		p->c = material + (w * h);
		p->f = material + (w * h) * 2;
		if (targets&GL_PIXEL_D_MASK)	{ p->plane[p->size] = p->d;	p->planeSrc[p->size++] = GL_PIXEL_D_MASK; }
		if (targets&GL_PIXEL_C_MASK)	{ p->plane[p->size] = p->c;	p->planeSrc[p->size++] = GL_PIXEL_C_MASK; }
		if (targets&GL_PIXEL_F_MASK)	{ p->plane[p->size] = p->f;	p->planeSrc[p->size++] = GL_PIXEL_F_MASK; }
	}
	ArpASSERT(size == p->size);
	return p;
}

void GlImage::InitProperties()
{
	mBounding = ARP_NO_PIXEL_RULES;
	mX = 0;
	mY = 0;
	mBg = ArpVoxel(255, 255, 255, 255, 0);
	mFg = ArpVoxel(0, 0, 0, 255, 255);
}

void GlImage::Free()
{
	mStatus = B_ERROR;
	mW = 0;
	mH = 0;
	delete[] mColor;
	delete[] mLight;
	delete[] mMaterial;
	mColor = mLight = mMaterial = 0;
	mIsSolid = false;
}

void GlImage::Print(uint32 tabs) const
{
	for (uint32 k = 0; k < tabs; k++) printf("\t");
	printf("GlImage w %ld h %ld status %ld (%p)\n", mW, mH, mStatus, this);
}
