#include <be/support/Autolock.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpBitmap.h>
#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlNodeVisual.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlImageNodes.h>

static const int32		WIDTH_KEY		= 'wdth';
static const int32		HEIGHT_KEY		= 'hght';
static const int32		DEF_WIDTH		= 50;
static const int32		DEF_HEIGHT		= 50;

/***************************************************************************
 * _GL-CACHED-IMAGE-ALGO
 ***************************************************************************/
class _GlCachedImageAlgo : public GlAlgoIm
{
public:
	_GlCachedImageAlgo(gl_node_id nid, gl_image_id imageId);
	_GlCachedImageAlgo(const _GlCachedImageAlgo& o);

	virtual GlAlgo*		Clone() const;

protected:
	virtual status_t	Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgoIm	inherited;
	gl_image_id			mImageId;
};

/* _GL-IMAGE-VISUAL
 * This visual is a little more complicated than some of the others --
 * it stores the preview in the node its based on, so that I don't
 * have to recompute it every time the node is inspected.  This is
 * because in this case, calculating the preview is a potentially
 * expensive step.
 */
class _GlImageVisual : public GlNodeVisual
{
public:
	_GlImageVisual(const GlRootRef& ref, GlImageNode& node);
	virtual ~_GlImageVisual();

protected:
	virtual status_t		LockedVisual(	GlNode* n, int32 w, int32 h,
											ArpBitmap** outBm, int32 index);

private:
	typedef GlNodeVisual	inherited;
	GlImageNode&			mN;

	status_t				CachePreview(int32 w, int32 h);
};

/***************************************************************************
 * GL-IMAGE-NODE
 ***************************************************************************/
GlImageNode::GlImageNode(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config), mImageId(0), mPreview(0),
		  mPreviewX(-1), mPreviewY(-1)
{
}

GlImageNode::GlImageNode(const GlImageNode& o)
		: inherited(o), mImageId(0), mPreview(0), mPreviewX(-1), mPreviewY(-1)
{
	if (o.mImageId) mImageId = GlGlobals().AcquireImage(o.mImageId);
}

GlImageNode::~GlImageNode()
{
	UncacheImages();
}

status_t GlImageNode::ParamChanged(gl_param_key key)
{
	inherited::ParamChanged(key);
	/* Every node gets pulsed with a ParamChanged() message when
	 * it is made visible to the user.  Some nodes need this --
	 * they depend on data from previous nodes and need to grab
	 * it each time.  Some nodes, like this one, don't.  You
	 * can tell if a param ACTUALLY changed because the key will
	 * be set, otherwise it's 0.
	 */
	if (key.key != 0) UncacheImages();
	return B_OK;
}

GlAlgo* GlImageNode::Generate(const gl_generate_args& args) const
{
	if (IsDirty() || !mImageId) {
		/* This lock is necessary because I'm being called within
		 * a read lock -- don't want to risk stepping on my own
		 * toes.
		 */
		BAutolock			l(mAccess);
		((GlImageNode*)this)->UncacheImages();
		if (!mImageId) ((GlImageNode*)this)->CacheImage();
	}
	return new _GlCachedImageAlgo(Id(), mImageId);
}

BView* GlImageNode::NewView(gl_new_view_params& params) const
{
	if (params.viewType == GL_INSPECTOR_VIEW) params.flags |= GL_NEW_VIEW_PREVIEW;
	return inherited::NewView(params);
}

GlNodeVisual* GlImageNode::NewVisual(const GlRootRef& ref) const
{
	return new _GlImageVisual(ref, (GlImageNode&)(*this));
}

bool GlImageNode::IsDirty() const
{
	return false;
}

void GlImageNode::UncacheImages()
{
	GlGlobals().ReleaseImage(mImageId);
	mImageId = 0;
	delete mPreview;
	mPreview = 0;
}

// #pragma mark -

/***************************************************************************
 * GL-LOAD-IMAGE
 ***************************************************************************/
GlLoadImage::GlLoadImage(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlLoadImage::GlLoadImage(const GlLoadImage& o)
		: inherited(o)
{
}

GlNode* GlLoadImage::Clone() const
{
	return new GlLoadImage(*this);
}

void GlLoadImage::CacheImage()
{
	if (mImageId) UncacheImages();
	BString16		fn = Params().FileName('file');
	if (fn.Length() < 1) return;

	mImageId = GlGlobals().AcquireImage(fn);
}

// #pragma mark -

/***************************************************************************
 * GL-LOAD-IMAGE-ADD-ON
 ***************************************************************************/
GlLoadImageAddOn::GlLoadImageAddOn()
		: inherited(SZI[SZI_arp], 'AiLo', SZ(SZ_Images), SZ(SZ_Load_Image), 1, 0)
{
	AddParamType(new GlFileNameParamType('file', SZ(SZ_File)));
}

GlNode* GlLoadImageAddOn::NewInstance(const BMessage* config) const
{
	return new GlLoadImage(this, config);
}

GlImage* GlLoadImageAddOn::NewImage() const
{
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;
	
	GlImage*		img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	/* FIX: Load or draw a folder icon.
	 */
	GlPlanes*		p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32		size = p->w * p->h;
		memset(p->r,	180,	size);
		memset(p->g,	200,	size);
		memset(p->b,	220,	size);
		memset(p->a,	255,	size);
		img->UnlockPixels(p);
	}
	return img;
}

// #pragma mark -

/***************************************************************************
 * GL-NEW-IMAGE
 ***************************************************************************/
GlNewImage::GlNewImage(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config), mLastSize(-1, -1)
{
}

GlNewImage::GlNewImage(const GlNewImage& o)
		: inherited(o), mLastSize(-1, -1)
{
}

GlNode* GlNewImage::Clone() const
{
	return new GlNewImage(*this);
}

bool GlNewImage::IsDirty() const
{
	if (mLastSize.x < 0 || mLastSize.y < 0) return true;
	BPoint			size(	float(this->Params().Int32(WIDTH_KEY)),
							float(this->Params().Int32(HEIGHT_KEY)));
	if (size != mLastSize) return true;
	return false;
}

static ArpVoxel _voxel(const ArpVoxel& v, int32 z)
{
	return ArpVoxel(v.r, v.g, v.b, v.a, arp_clip_255(z));
}

void GlNewImage::CacheImage()
{
	BPoint			size(	float(this->Params().Int32(WIDTH_KEY)),
							float(this->Params().Int32(HEIGHT_KEY)));
	int32			fgZ = Params().Int32(GL_FG_Z),
					bgZ = Params().Int32(GL_BG_Z);
	ArpVoxel		fg = _voxel(Params().Color(GL_FG_RGBA), fgZ),
					bg = _voxel(Params().Color(GL_BG_RGBA), bgZ);

	if (mImageId) UncacheImages();
	GlImage*		img = new GlImage(ARP_ROUND(size.x), ARP_ROUND(size.y), &bg, &fg);

	if (!img || img->InitCheck() != B_OK) {
		UncacheImages();
		delete img;
		return;
	}
	mImageId = GlGlobals().AcquireImage(img);
	mLastSize = size;
}

// #pragma mark -

/***************************************************************************
 * GL-NEW-IMAGE-ADD-ON
 ***************************************************************************/
GlNewImageAddOn::GlNewImageAddOn()
		: inherited(SZI[SZI_arp], GL_NEW_IMAGE_KEY, SZ(SZ_Images), SZ(SZ_New_Image), 1, 0)
{
	ArpVoxel		fg(0, 0, 0, 255, 255);
	ArpVoxel		bg(255, 255, 255, 255, 0);

	AddParamType(new GlInt32ParamType(WIDTH_KEY,	SZ(SZ_Width), 0, 4096, DEF_WIDTH));
	AddParamType(new GlInt32ParamType(HEIGHT_KEY,	SZ(SZ_Height), 0, 4096, DEF_HEIGHT));
	AddParamType(new GlColorParamType(GL_FG_RGBA,	SZ(SZ_FG), &fg));
	AddParamType(new GlInt32ParamType(GL_FG_Z,		SZ(SZ_FG_depth), 0, 255, fg.z));
	AddParamType(new GlColorParamType(GL_BG_RGBA,	SZ(SZ_BG), &bg));
	AddParamType(new GlInt32ParamType(GL_BG_Z,		SZ(SZ_BG_depth), 0, 255, bg.z));
}

GlNode* GlNewImageAddOn::NewInstance(const BMessage* config) const
{
	return new GlNewImage(this, config);
}

GlImage* GlNewImageAddOn::NewImage() const
{
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;
	
	GlImage*		img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	/* FIX: Load or draw a folder icon.
	 */
	GlPlanes*		p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32		size = p->w * p->h;
		memset(p->r,	180,	size);
		memset(p->g,	160,	size);
		memset(p->b,	140,	size);
		memset(p->a,	255,	size);
		img->UnlockPixels(p);
	}
	return img;
}

// #pragma mark -

/***************************************************************************
 * _GL-CACHED-IMAGE-ALGO
 ***************************************************************************/
_GlCachedImageAlgo::_GlCachedImageAlgo(gl_node_id nid, gl_image_id imageId)
		: inherited(nid), mImageId(imageId)
{
}

_GlCachedImageAlgo::_GlCachedImageAlgo(const _GlCachedImageAlgo& o)
		: inherited(o), mImageId(o.mImageId)
{
}

GlAlgo* _GlCachedImageAlgo::Clone() const
{
	return new _GlCachedImageAlgo(*this);
}

status_t _GlCachedImageAlgo::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	/* FIX:  Probably, I shouldn't do this.  For now I need to
	 * because the app is blindly giving me an image, but eventually
	 * it should check to see if the input is an image in before
	 * doing that.
* ALSO:  For the preview, there should be some way to signify that
* what I'm doing is processing for preview, and the size, so this
* node can scale down to that.
	 */
	list.DeleteContents();
	
	if (!mImageId) return list.AsError(BString16("No file name to load image"));
	GlImage*			image = GlGlobals().CloneImage(mImageId);
	if (!image) return list.AsNoMemoryError();
	if (list.AddImage(image) != B_OK) delete image;
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * _GL-IMAGE-VISUAL
 ***************************************************************************/
_GlImageVisual::_GlImageVisual(const GlRootRef& ref, GlImageNode& node)
		: inherited(ref, node.Id()), mN(node)
{
	mN.IncRefs();
}

_GlImageVisual::~_GlImageVisual()
{
	mN.DecRefs();
}

#if 0
	virtual void			ParamChanged(gl_param_key key);

void _GlImageVisual::ParamChanged(gl_param_key key)
{
	inherited::ParamChanged(key);
	mChanged = true;
}
#endif

status_t _GlImageVisual::LockedVisual(	GlNode* n, int32 w, int32 h,
										ArpBitmap** outBm, int32 index)
{
	ArpVALIDATE(outBm && w > 0 && h > 0, return B_ERROR);

	if (!(mN.mPreview) || mN.mPreviewX != w || mN.mPreviewY != h)
		CachePreview(w, h);
	if (!(mN.mPreview)) return B_ERROR;
	*outBm = mN.mPreview->AsBitmap();
	if (*outBm) return B_OK;
	return B_NO_MEMORY;
}

status_t _GlImageVisual::CachePreview(int32 w, int32 h)
{
	if (mN.mPreview) delete mN.mPreview;
	mN.mPreview = 0;
	if (mN.mImageId == 0) mN.CacheImage();
	if (mN.mImageId == 0) return B_ERROR;
	const GlImage*		img = GlGlobals().SourceImage(mN.mImageId);
	if (!img) return B_ERROR;

	mN.mPreviewX = w;
	mN.mPreviewY = h;

	/* Special case for if the image is a solid -- I want to cache a
	 * picture to represent this.
	 */
	if (img->IsSolid()) {
		ArpVoxel		bg = img->Property(ARP_BACKGROUND_COLOUR),
						fg = img->Property(ARP_FOREGROUND_COLOUR);
		mN.mPreview = new GlImage(10, 10, &bg, &fg);
		return B_OK;
	}

	if (w >= img->Width() && h >= img->Height()) {
		mN.mPreview = img->Clone();
		if (mN.mPreview) return B_OK;
		else return B_NO_MEMORY;
	}

	int32				newW, newH;
	gl_scale_to_dimens(img->Width(), img->Height(), w, h, &newW, &newH);
	ArpVALIDATE(newW > 0 && newH > 0 && newW <= w && newH <= h, return B_ERROR);

	/* FIX:  There should probably be a user preference to set the quality
	 * for the scaled images in the previews.
	 */
	mN.mPreview = img->AsScaledImage(newW, newH, 0);
	if (mN.mPreview == 0) return B_NO_MEMORY;
	return B_OK;	
}
