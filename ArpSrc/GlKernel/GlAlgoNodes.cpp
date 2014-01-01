#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlAlgoNodes.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlNodeVisual.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlStrainedParamList.h>

// _GL-NODE-1D-VISUAL
class _GlNode1dVisual : public GlNodeVisual
{
public:
	_GlNode1dVisual(const GlRootRef& ref, gl_node_id nid);
	virtual ~_GlNode1dVisual();

	virtual void			ParamChanged(gl_param_key key);

protected:
	virtual status_t		PreVisual(GlNode* node);
	virtual status_t		LockedVisual(	GlNode* n, int32 w, int32 h,
											ArpBitmap** outBm, int32 index);
	virtual void			PostVisual(GlNode* node);

private:
	typedef GlNodeVisual	inherited;
	GlAlgo1d*				mAlgo;
	uint32					mNodeCount;
	bool					mChanged;
	GlImage*				mImage;
};

/***************************************************************************
 * GL-NODE-1D
 ***************************************************************************/
GlNode1d::GlNode1d(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlNode1d::GlNode1d(const GlNode1d& o)
		: inherited(o)
{
}

BView* GlNode1d::NewView(gl_new_view_params& params) const
{
	if (params.viewType != GL_INSPECTOR_VIEW) return inherited::NewView(params);
	ArpVALIDATE(Parent(), return 0);

	GlStrainedParamList		list;
	status_t				err = GetParams(list);
	if (err != B_OK) return 0;
//	if (GetParams(params) != B_OK) return 0;

	GlParamView*		v = new GlParamView(params, Parent()->Id(), Id());
	if (!v) return 0;

	float					sx = float(Prefs().GetInt32(ARP_PADX)),
							sy = float(Prefs().GetInt32(ARP_PADY));
	float					fh = v->ViewFontHeight();
//	float					div = get_divider(params, this);
	v->mRect.Set(sx, sy, 152, sy + fh);
	v->AddLabel(v->mRect, "label", AddOn()->Label());

	v->mRect.Set(v->mRect.left, v->mRect.bottom + sy, v->mRect.right, v->mRect.bottom + sy + 50);
	v->AddVisualView(v->mRect, "bm", NewVisual(params.ref), 0);

	v->AddParamControls(list);

	return v;
}

GlNodeVisual* GlNode1d::NewVisual(const GlRootRef& ref) const
{
	return new _GlNode1dVisual(ref, Id());
}

// #pragma mark -

/***************************************************************************
 * GL-NODE-1D-ADD-ON
 ***************************************************************************/
GlNode1dAddOn::GlNode1dAddOn(	const BString16& creator, int32 key,
								const BString16* category, const BString16* label,
								int32 majorVersion, int32 minorVersion)
		: inherited(creator, key, category, label, majorVersion, minorVersion)
{
}

GlImage* GlNode1dAddOn::NewImage() const
{
	int32				w = Prefs().GetInt32(GL_NODE_IMAGE_X),
						h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;
	
	GlImage*			img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	img->SetColor(0, 0, 0, 0);
	
	GlNode*				node = NewInstance(0);
	if (!node) return img;

	node->IncRefs();
	gl_generate_args	args;
	args.flags = GL_NODE_ICON_F;
	GlAlgo*				a = node->Generate(args);
	node->MakeEmpty();
	node->DecRefs();

	GlAlgo1d*			a1d = (a) ? a->As1d() : 0;
	if (a1d) {
		GlCache1d		cache;
		cache.Render(a1d, img, GlRect(0, 0, w, h), GlRect(0, 0, w, h));
	}
	delete a;
	
	return img;
}

// #pragma mark -

/***************************************************************************
 * _GL-NODE-1D-VISUAL
 ***************************************************************************/
_GlNode1dVisual::_GlNode1dVisual(const GlRootRef& ref, gl_node_id nid)
		: inherited(ref, nid), mAlgo(0), mChanged(false), mImage(0)
{
}

_GlNode1dVisual::~_GlNode1dVisual()
{
	delete mAlgo;
	delete mImage;
}

void _GlNode1dVisual::ParamChanged(gl_param_key key)
{
	inherited::ParamChanged(key);
	mChanged = true;
}

status_t _GlNode1dVisual::PreVisual(GlNode* n)
{
	ArpVALIDATE(n, return B_ERROR);
	delete mAlgo;
	mAlgo = 0;
	mNodeCount = 0;
	
	/* Always generate the algo for my parent -- from
	 * that, I can find my own if it's different.
	 */
	const GlChain*		parent = n->Parent();
	ArpASSERT(parent);
	if (!parent) return B_ERROR;
	
	mNodeCount = parent->NodeCount();
	gl_generate_args	args;
	GlAlgo*				a = parent->Generate(args);
	mAlgo = (a) ? a->As1d() : 0;
	if (!mAlgo) delete a;
	return B_OK;
}

class _GlFindMapNodeAction : public GlAlgoAction
{
public:
	gl_node_id			nid;
	GlAlgo1d*			map;
	
	_GlFindMapNodeAction(gl_node_id inNid) : nid(inNid), map(0)	{ }
	
	virtual int32		Perform(GlAlgo1d* algo)
	{
		ArpASSERT(algo);
		if (algo->NodeId() == nid) {
			map = algo;
			return GL_STOP_OPERATION;
		}
		return GL_CONTINUE;
	}
};

status_t _GlNode1dVisual::LockedVisual(	GlNode* n, int32 w, int32 h,
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

	GlAlgo1d*			a = 0;
	GlAlgo1d*			parentA = 0;
	uint32				flags = GlAlgo1d::INIT_LINE_F;
	/* I want to always draw my algo (so I should have it), and
	 * also overlay my parent on top of there are multiple nodes
	 * in the parent chain.
	 */
	if (mNodeCount <= 1) a = mAlgo;
	else if (mAlgo) {
		parentA = mAlgo;
		if (n->Id() == mAlgo->NodeId()) a = mAlgo;
		else {
			_GlFindMapNodeAction	action(n->Id());
			mAlgo->Walk(action, GL_1D_IO);
			a = action.map;
		}
		flags |= GlAlgo1d::SINGLE_ALGO_F;
	}

	mImage->SetColor(0, 0, 0, 0);
	GlCache1d		cache;
	if (a) cache.Render(a, mImage, GlRect(0, 0, w, h), GlRect(0, 0, w, h), 255, flags);
	if (parentA) cache.Render(parentA, mImage, GlRect(0, 0, w, h), GlRect(0, 0, w, h), 200);
	
	*outBm = mImage->AsBitmap();
	if (*outBm) return B_OK;
	return B_ERROR;
}

void _GlNode1dVisual::PostVisual(GlNode* node)
{
	delete mAlgo;
	mAlgo = 0;
	mNodeCount = 0;
}

// #pragma mark -

// _GL-NODE-2D-VISUAL
class _GlNode2dVisual : public GlNodeVisual
{
public:
	_GlNode2dVisual(const GlRootRef& ref, gl_node_id nid);
	virtual ~_GlNode2dVisual();

protected:
	virtual status_t		PreVisual(GlNode* node);
	virtual status_t		LockedVisual(	GlNode* n, int32 w, int32 h,
											ArpBitmap** outBm, int32 index);
	virtual void			PostVisual(GlNode* node);

private:
	typedef GlNodeVisual	inherited;
	GlAlgo2d*				mS;
};

/***************************************************************************
  * GL-NODE-2D
 ***************************************************************************/
GlNode2d::GlNode2d(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlNode2d::GlNode2d(const GlNode2d& o)
		: inherited(o)
{
}

BView* GlNode2d::NewView(gl_new_view_params& params) const
{
	if (params.viewType != GL_INSPECTOR_VIEW) return inherited::NewView(params);
	ArpVALIDATE(Parent(), return 0);

	GlStrainedParamList		list;
	status_t				err = GetParams(list);
	if (err != B_OK) return 0;
//	if (GetParams(list) != B_OK) return 0;

	GlParamView*			v = new GlParamView(params, Parent()->Id(), Id());
	if (!v) return 0;

	float					sx = float(Prefs().GetInt32(ARP_PADX)),
							sy = float(Prefs().GetInt32(ARP_PADY));
	float					fh = v->ViewFontHeight();
	v->mRect.Set(sx, sy, 152, sy + fh);
	v->AddLabel(v->mRect, "label", AddOn()->Label());

	v->mRect.Set(v->mRect.left, v->mRect.bottom + sy, v->mRect.right, v->mRect.bottom + sy + 50);
	v->AddVisualView(v->mRect, "bm", NewVisual(params.ref));

	v->AddParamControls(list);
	
	return v;
}

GlNodeVisual* GlNode2d::NewVisual(const GlRootRef& ref) const
{
	return new _GlNode2dVisual(ref, Id());
}

// #pragma mark -

/***************************************************************************
 * GL-NODE-2D-ADD-ON
 ***************************************************************************/
GlNode2dAddOn::GlNode2dAddOn(	const BString16& creator, int32 key,
								const BString16* category, const BString16* label,
								int32 majorVersion, int32 minorVersion)
		: inherited(creator, key, category, label, majorVersion, minorVersion)
{
}

GlImage* GlNode2dAddOn::NewImage() const
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
 * _GL-NODE-2D-VISUAL
 ***************************************************************************/
_GlNode2dVisual::_GlNode2dVisual(const GlRootRef& ref, gl_node_id nid)
		: inherited(ref, nid), mS(0)
{
}

_GlNode2dVisual::~_GlNode2dVisual()
{
	delete mS;
}

status_t _GlNode2dVisual::PreVisual(GlNode* n)
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

class _GlFind2dNodeAction : public GlAlgoAction
{
public:
	gl_node_id			nid;
	GlAlgo2d*			a;
	
	_GlFind2dNodeAction(gl_node_id inNid) : nid(inNid), a(0)	{ }
	
	virtual int32		Perform(GlAlgo2d* algo)
	{
		ArpASSERT(algo);
		if (algo->NodeId() == nid) {
			a = algo;
			return GL_STOP_OPERATION;
		}
		return GL_CONTINUE;
	}
};

status_t _GlNode2dVisual::LockedVisual(	GlNode* n, int32 w, int32 h,
										ArpBitmap** outBm, int32 index)
{
	ArpVALIDATE(n && outBm && w > 0 && h > 0, return B_ERROR);
	GlImage*					img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return B_NO_MEMORY;
	}

	GlAlgo2d*					s = mS;
	GlFillType					fill = GL_FILL_NONE;

	if (s) {
		fill = s->FillType();
		_GlFind2dNodeAction		action(n->Id());
		s->Walk(action, GL_2D_IO);
		if (action.a) fill = action.a->FillType();
	}

	GlPlanes*					p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		if (fill == GL_FILL_BLACK) p->Black();
		else if (fill == GL_FILL_COLORWHEEL) p->ColorWheel(0, 0, w - 1, h - 1);

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
	
	*outBm = img->AsBitmap();
	delete img;
	if (*outBm) return B_OK;
	return B_ERROR;
}

void _GlNode2dVisual::PostVisual(GlNode* node)
{
	delete mS;
	mS = 0;
}

// #pragma mark -

/***************************************************************************
 * GL-NODE-IM
 ***************************************************************************/
GlNodeIm::GlNodeIm(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

// #pragma mark -

/***************************************************************************
 * GL-NODE-IM-ADD-ON
 ***************************************************************************/
GlNodeImAddOn::GlNodeImAddOn(	const BString16& creator, int32 key,
								const BString16* category, const BString16* label,
								int32 majorVersion, int32 minorVersion)
		: inherited(creator, key, category, label, majorVersion, minorVersion)
{
}
