#include <cstdio>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlNodeVisual.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlPlaneNode.h>
#include <GlKernel/GlPixelTargetView.h>

static const char*		PIXEL_TARGETS_IO		= "pt";

/***************************************************************************
  * GL-PLANE-NODE
 ***************************************************************************/
GlPlaneNode::GlPlaneNode(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, 0), mPixelTargets(GL_PIXEL_RGB)
{
	if (config) ReadFrom(*config);
}

GlPlaneNode::GlPlaneNode(const GlPlaneNode& o)
		: inherited(o), mPixelTargets(o.mPixelTargets)
{
}

uint32 GlPlaneNode::PixelTargets() const
{
	return mPixelTargets;
}

void GlPlaneNode::SetPixelTargets(uint32 targets)
{
	mPixelTargets = targets;
}

status_t GlPlaneNode::GetProperty(int32 code, GlParamWrap& wrap) const
{
	if (code == GL_PIXEL_TARGET_PROP) {
		if (wrap.Type() != GL_INT32_TYPE) return B_ERROR;
		((GlInt32Wrap&)wrap).v = mPixelTargets;
		return B_OK;
	}
	return inherited::GetProperty(code, wrap);
}

status_t GlPlaneNode::SetProperty(int32 code, const GlParamWrap& wrap)
{
	if (code == GL_PIXEL_TARGET_PROP) {
		if (wrap.Type() != GL_INT32_TYPE) return B_ERROR;
		mPixelTargets = uint32( ((GlInt32Wrap&)wrap).v );
		/* Always call the notification mechanism -- right now it's
		 * obviously the generic param changed, but it might get its
		 * own at some point.
		 */
		if (mParent) mParent->ParamChanged(gl_param_key(0, 0, 0));
		return B_OK;
	}
	return inherited::SetProperty(code, wrap);
}

status_t GlPlaneNode::ReadFrom(const BMessage& config)
{
	status_t		err = inherited::ReadFrom(config);
	int32			i32;
	if (config.FindInt32(PIXEL_TARGETS_IO, &i32) == B_OK) mPixelTargets = i32;
	return err;
}

status_t GlPlaneNode::WriteTo(BMessage& config) const
{
	status_t		err = inherited::WriteTo(config);
	if (err != B_OK) return err;
	if ((err = config.AddInt32(PIXEL_TARGETS_IO, mPixelTargets)) != B_OK) return err;
	return B_OK;
}

BView* GlPlaneNode::NewView(gl_new_view_params& params) const
{
	if (params.viewType == GL_PIXEL_TARGET_VIEW)
		return new GlPixelTargetView(params.frame, params.ref, *this);
	return inherited::NewView(params);
}

// #pragma mark -

/***************************************************************************
  * _GL-PLANE-VISUAL
 ***************************************************************************/
class _GlPlaneVisual : public GlNodeVisual
{
public:
	_GlPlaneVisual(const GlRootRef& ref, gl_node_id nid) : inherited(ref, nid),
			mS(0), mChanged(false), mImage(0)				{ }
	virtual ~_GlPlaneVisual()	{ delete mS; delete mImage; }

	virtual void			ParamChanged(gl_param_key key);

protected:
	virtual status_t		PreVisual(GlNode* node);
	virtual status_t		LockedVisual(	GlNode* n, int32 w, int32 h,
											ArpBitmap** outBm, int32 index);
	virtual void			PostVisual(GlNode* node);

private:
	typedef GlNodeVisual	inherited;
	GlAlgo2d*				mS;
	bool					mChanged;
	GlImage*				mImage;
};

void _GlPlaneVisual::ParamChanged(gl_param_key key)
{
	inherited::ParamChanged(key);
	mChanged = true;
}

status_t _GlPlaneVisual::PreVisual(GlNode* n)
{
	ArpVALIDATE(n, return B_ERROR);
	delete mS;
	mS = 0;

// How do I want to do this?  It doesn't make sense to get the
// 2D just for my node.  But getting it for the entire chain, like
// the 1D, is a little weird, too.  I could go back to the old-style,
// and get it up to my current node, but then that's at odds with
// the 1D.  I could do something like the 1D, where it does a color
// overlay of the final, but with the 2D that might be real visual
// overload.
#if 0
	gl_generate_args	args;
	GlAlgo*				a = n->Generate(args);
	if (a) mS = a->As2d();
	if (!mS) delete a;
#endif

	const GlChain*		parent = n->Parent();
	ArpASSERT(parent);
	if (!parent) return B_ERROR;

	gl_generate_args	args;
	GlAlgo*				a = parent->Generate(args);
	if (a) mS = a->As2d();
	if (!mS) delete a;

	return B_OK;
}

status_t _GlPlaneVisual::LockedVisual(	GlNode* n, int32 w, int32 h,
										ArpBitmap** outBm, int32 index)
{
	ArpVALIDATE(n && outBm && w > 0 && h > 0, return B_ERROR);
	/* Make sure there's an image to draw on, and it's the right size.
	 */
	if (!mImage || w != mImage->Width() || h != mImage->Height()) {
		delete mImage;
		mImage = 0;
	}
	if (!mImage) mImage = new GlImage(w, h);
	if (!mImage || mImage->InitCheck() != B_OK) return B_NO_MEMORY;
	ArpASSERT(mImage && w == mImage->Width() && h == mImage->Height());

	if (mS) {
		GlPlanes*		p = mImage->LockPixels(GL_PIXEL_RGBA, true);
		if (p) {
			GlMask		mask;
			uint8*		data;
			p->Fill(mS->FillType());
			if ((data = mask.Make(*p, mS)) != 0) {
				/* Render the surface.
				 */
				for (int32 pix = 0; pix < p->w * p->h; pix++) {
					if (p->a[pix] > 0) p->a[pix] = data[pix];
				}
			}
		}
		mImage->UnlockPixels(p);
	}
		
	*outBm = mImage->AsBitmap();
	if (*outBm) return B_OK;
	return B_ERROR;
}

void _GlPlaneVisual::PostVisual(GlNode* node)
{
	delete mS;
	mS = 0;
}

GlNodeVisual* GlPlaneNode::NewVisual(const GlRootRef& ref) const
{
	return new _GlPlaneVisual(ref, Id());
}
