#include <GlPublic/GlImage.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlAlgoNbrInput.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlParamType.h>
#include <GlNodes/GlInNodes.h>

/***************************************************************************
 * _GL-IN-ALGO-IM
 ***************************************************************************/
class _GlInAlgoIm : public GlAlgo
{
public:
	_GlInAlgoIm(gl_node_id nid, uint32 io);
	_GlInAlgoIm(const _GlInAlgoIm& o);

	virtual uint32			Io() const;
	virtual GlAlgo*			Clone() const;

protected:
	virtual status_t		Perform(GlNodeDataList& list, const gl_process_args* args);

private:
	typedef GlAlgo			inherited;
	uint32					mIo;
};

/***************************************************************************
 * GL-IN
 ***************************************************************************/
GlIn::GlIn(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
}

GlIn::GlIn(const GlIn& o)
		: inherited(o)
{
}

GlNode* GlIn::Clone() const
{
	return new GlIn(*this);
}

GlAlgo* GlIn::Generate(const gl_generate_args& args) const
{
	ArpVALIDATE(AddOn(), return 0);
	return new _GlInAlgoIm(Id(), AddOn()->Io());
}

// #pragma mark -

/***************************************************************************
 * GL-IN-ADD-ON
 ***************************************************************************/
static int32 _key_for_type(uint32 type)
{
	if (type == GL_1D_IO)				return 'AmIn';
	else if (type == GL_2D_IO)			return 'AsIn';
	else if (type == GL_IMAGE_IO)		return GL_IMAGE_IN_KEY;
	else if (type == GL_TEXT_IO)		return 'AtIn';
	ArpASSERT(false);
	return 0;
}

static const BString16* _label_for_type(uint32 type)
{
	if (type == GL_1D_IO)				return SZ(SZ_Map_In);
	else if (type == GL_2D_IO)			return SZ(SZ_Surface_In);
	else if (type == GL_IMAGE_IO)		return SZ(SZ_Image_In);
	else if (type == GL_TEXT_IO)		return SZ(SZ_Text_In);
	ArpASSERT(false);
	return NULL;
}

GlInAddOn::GlInAddOn(uint32 io)
		: inherited(SZI[SZI_arp], _key_for_type(io), SZ(SZ_Flow),
					_label_for_type(io), 1, 0),
		  mIo(io)
{
	ArpASSERT(Key() != 0);
}

GlNode* GlInAddOn::NewInstance(const BMessage* config) const
{
	return new GlIn(this, config);
}

uint32 GlInAddOn::Io() const
{
	return mIo;
}

#if 0
GlImage* GlInAddOn::NewImage() const
{
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;
	
	GlImage*		img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}

	/* FIX: Draw +, - etc.
	 */
	GlPlanes*		p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p) {
		int32		size = p->w * p->h, x, y, pix, cen;
		memset(p->a, 0, size);

		cen = int32(w * 0.5);
		for (y = 2; y < p->h - 2; y++) {
			for (x = cen - 1; x <= cen + 1; x++) {
				pix = ARP_PIXEL(x, y, p->w);
				p->r[pix] = p->g[pix] = p->b[pix] = 0;
				p->a[pix] = 255;
			}
		}

		cen = int32(h * 0.5);
		for (x = 2; x < p->w - 2; x++) {
			for (y = cen - 1; y <= cen + 1; y++) {
				pix = ARP_PIXEL(x, y, p->w);
				p->r[pix] = p->g[pix] = p->b[pix] = 0;
				p->a[pix] = 255;
			}
		}
	}
	img->UnlockPixels(p);
	
	return img;
}
#endif

// #pragma mark -

/***************************************************************************
 * _GL-IN-ALGO-IM
 ***************************************************************************/
_GlInAlgoIm::_GlInAlgoIm(gl_node_id nid, uint32 io)
		: inherited(nid), mIo(io)
{
}

_GlInAlgoIm::_GlInAlgoIm(const _GlInAlgoIm& o)
		: inherited(o), mIo(o.mIo)
{
}

uint32 _GlInAlgoIm::Io() const
{
	return mIo;
}

GlAlgo* _GlInAlgoIm::Clone() const
{
	return new _GlInAlgoIm(*this);
}

status_t _GlInAlgoIm::Perform(GlNodeDataList& list, const gl_process_args* args)
{
	GlNodeData*		data;
	for (uint32 k = 0; (data = list.DataAt(k, list.BACKPOINTER_TYPE)) != 0; k++) {
		if (data->Type() == list.BACKPOINTER_TYPE) {
			GlNodeDataBackPointer*	bp = (GlNodeDataBackPointer*)data;
			if (mIo&GL_IMAGE_IO && bp->image) {
				GlImage*			img = bp->image->Clone();
				if (img) list.AddImage(img);
			}
		}
	}
	list.DeleteContents(list.BACKPOINTER_TYPE);
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * _GL-IN-ALGO-NBR
 ***************************************************************************/
class _GlInAlgoNbr : public GlAlgoNbrIn
{
public:
	_GlInAlgoNbr(gl_node_id nid, const BString16& key)
			: inherited(GL_NUMBER_IN_KEY, nid, key)			{ }
	_GlInAlgoNbr(const _GlInAlgoNbr& o) : inherited(o)		{ }

	virtual GlAlgo*		Clone() const { return new _GlInAlgoNbr(*this); }

	virtual status_t	SetParam(const gl_param_key& k, const GlParamWrap& wrap)
	{
		if (k.key != GL_KEY_PARAM || wrap.Type() != GL_TEXT_TYPE) return B_ERROR;
		key = ((const GlTextWrap&)wrap).v;
		return B_OK;
	}
	
	virtual status_t	GetParam(const gl_param_key& k, GlParamWrap& wrap) const
	{
		if (k.key != GL_KEY_PARAM || wrap.Type() != GL_TEXT_TYPE) return B_ERROR;
		((GlTextWrap&)wrap).v = key;
		return B_OK;
	}

private:
	typedef GlAlgoNbrIn		inherited;
};

/***************************************************************************
 * GL-IN-NBR
 ***************************************************************************/
class GlInNbr : public GlNode
{
public:
	GlInNbr(const GlNodeAddOn* addon, const BMessage* config) : inherited(addon, config) { }
	GlInNbr(const GlIn& o) : inherited(o) { }
	
	virtual GlNode*			Clone() const { return new GlInNbr(*this); }
	virtual GlAlgo*			Generate(const gl_generate_args& args) const
	{
		return new _GlInAlgoNbr(Id(), Params().Text(GL_KEY_PARAM));
	}

private:
	typedef GlNode			inherited;
};

// #pragma mark -

/***************************************************************************
 * GL-IN-NBR-ADD-ON
 ***************************************************************************/
GlInNbrAddOn::GlInNbrAddOn()
		: inherited(SZI[SZI_arp], GL_NUMBER_IN_KEY, SZ(SZ_Flow), SZ(SZ_Number_In), 1, 0)
{
	ArpASSERT(GlAlgoNbr::RegisterKey(GL_NUMBER_IN_KEY));

	AddParamType(new GlTextParamType(GL_KEY_PARAM, SZ(SZ_Key)));
}

GlNode* GlInNbrAddOn::NewInstance(const BMessage* config) const
{
	return new GlInNbr(this, config);
}
