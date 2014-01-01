#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlParenNodes.h>

static GlImage*		_new_open_image(int32 w, int32 h);
static GlImage*		_new_close_image(int32 w, int32 h);

/***************************************************************************
 * _GL-PAREN-SURFACE
 ****************************************************************************/
class _GlParenSurface : public GlAlgo2d
{
public:
	_GlParenSurface(int32 token) : inherited(0, 0, GL_FILL_BLACK, token), mV(0)	{ }
	_GlParenSurface(const _GlParenSurface& o) : inherited(o), mV(0)				{ }
	virtual ~_GlParenSurface()	{ delete mV; }
	
	virtual GlAlgo*			Clone() const { return new _GlParenSurface(*this); }
	virtual status_t		Process(const GlPlanes* src, GlPlanes& dest,
									GlProcessStatus* status)
	{
		if (!mV) return B_OK;
		return mV->ProcessAll(src, dest, status);
	}

	virtual status_t		AssignUnary(GlAlgo* v)
	{
		/* In the current model, the open paren is treated as the unary
		 * operator and everything gets assigned to its value, so enforce that.
		 */
		ArpASSERT(Token() == GL_OPEN_PAREN);
		delete mV;
		mV = (GlAlgo2d*)v;
		return B_OK;
	}
	
protected:
	virtual void			_print() const
	{
		if (Token() == GL_OPEN_PAREN) printf("(");
		else if (Token() == GL_CLOSE_PAREN) printf(")");
		else printf("<_GlParen2d>");
	}

private:
	typedef GlAlgo2d		inherited;
	GlAlgo2d*				mV;
};

/***************************************************************************
 * GL-PAREN-SRF
 ***************************************************************************/
class GlParenSrf : public GlNode2d
{
public:
	GlParenSrf(const GlNodeAddOn* addon, const BMessage* config, int32 token)
			: inherited(addon, config), mToken(token) { }
	
	GlParenSrf(const GlParenSrf& o)
			: inherited(o), mToken(o.mToken) { }
	
	virtual GlNode*			Clone() const { return new GlParenSrf(*this); }
	virtual GlAlgo*			Generate(const gl_generate_args& args) const
	{
		return new _GlParenSurface(mToken);
	}

private:
	typedef GlNode2d		inherited;
	int32					mToken;
};

// #pragma mark -

/***************************************************************************
 * GL-PAREN-SRF-ADD-ON
 ***************************************************************************/
GlParenSrfAddOn::GlParenSrfAddOn(int32 key, const BString16* label, int32 token)
		: inherited(SZI[SZI_arp], key, SZ(SZ_Flow), label, 1, 0),
		  mToken(token)
{
}

GlNode* GlParenSrfAddOn::NewInstance(const BMessage* config) const
{
	return new GlParenSrf(this, config, mToken);
}

GlImage* GlParenSrfAddOn::NewImage() const
{
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;

	if (mToken == GL_OPEN_PAREN) return _new_open_image(w, h);
	else if (mToken == GL_CLOSE_PAREN) return _new_close_image(w, h);
	ArpASSERT(false);
	return 0;
}

// #pragma mark -

/***************************************************************************
 * _GL-PAREN-NBR
 ****************************************************************************/
class _GlParenNbr : public GlAlgoNbr
{
public:
	_GlParenNbr(int32 key, int32 token) : inherited(key, 0, token)	{ }
	_GlParenNbr(const _GlParenNbr& o) : inherited(o)				{ }
	
	virtual GlAlgo*			Clone() const { return new _GlParenNbr(*this); }
	virtual status_t		Process(GlArrayF& set)
	{
		return mWrap.Process(set);
	}

	virtual status_t		AssignUnary(GlAlgo* v)
	{
		/* In the current model, the open paren is treated as the unary
		 * operator and everything gets assigned to its value, so enforce that.
		 */
		ArpASSERT(Token() == GL_OPEN_PAREN);
		SetChain(v, 0);
		mWrap.SetAlgo(v);
		return B_OK;
	}
	
protected:
	virtual void			_print() const
	{
		if (Token() == GL_OPEN_PAREN) printf("(");
		else if (Token() == GL_CLOSE_PAREN) printf(")");
		else printf("<_GlParen2d>");
	}

private:
	typedef GlAlgoNbr		inherited;
	GlAlgoNbrWrap			mWrap;
};

/***************************************************************************
 * GL-PAREN-NBR
 ***************************************************************************/
class GlParenNbr : public GlNode
{
public:
	GlParenNbr(const GlNodeAddOn* addon, const BMessage* config, int32 token)
			: inherited(addon, config), mToken(token) { }
	
	GlParenNbr(const GlParenNbr& o)
			: inherited(o), mToken(o.mToken) { }
	
	virtual GlNode*		Clone() const { return new GlParenNbr(*this); }
	virtual GlAlgo*		Generate(const gl_generate_args& args) const
	{
		ArpVALIDATE(AddOn(), return 0);
		return new _GlParenNbr(AddOn()->Key(), mToken);
	}

private:
	typedef GlNode		inherited;
	int32				mToken;
};

/***************************************************************************
 * GL-PAREN-NBR-ADD-ON
 ***************************************************************************/
GlParenNbrAddOn::GlParenNbrAddOn(int32 key, const BString16* label, int32 token)
		: inherited(SZI[SZI_arp], key, SZ(SZ_Flow), label, 1, 0),
		  mToken(token)
{
	ArpASSERT(GlAlgoNbr::RegisterKey(key));
}

GlNode* GlParenNbrAddOn::NewInstance(const BMessage* config) const
{
	return new GlParenNbr(this, config, mToken);
}

GlImage* GlParenNbrAddOn::NewImage() const
{
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;

	if (mToken == GL_OPEN_PAREN) return _new_open_image(w, h);
	else if (mToken == GL_CLOSE_PAREN) return _new_close_image(w, h);
	ArpASSERT(false);
	return 0;
}

// #pragma mark -

/***************************************************************************
 * Misc
 ***************************************************************************/
GlImage* _new_open_image(int32 w, int32 h)
{
	ArpVALIDATE(w > 0 && h > 0, return 0);

	GlImage*			img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	img->SetColor(0, 0, 0, 0);
	
	GlPlanes*			p = img->LockPixels(GL_PIXEL_A_MASK, true);
	if (p && p->a) {
		int32			cenX = w - 1, cenY = h / 2;
		int32			rad = (w / 2 < cenY) ? w / 2 : cenY;
		rad = int32(rad * 1.5);
		int32			pix = 0;
		for (int32 y = 0; y < h; y++) {
			for (int32 x = 0; x < w; x++) {
				float	v;
				if (rad <= 0) v = 0;
				else v = ARP_DISTANCE(cenX, cenY, x, y) / rad;
				if (v >= 0.84 && v < 1.0) p->a[pix] = 255;
				pix++;
			}
		}
	}
	img->UnlockPixels(p);
	
	return img;
}

GlImage* _new_close_image(int32 w, int32 h)
{
	ArpVALIDATE(w > 0 && h > 0, return 0);

	GlImage*			img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}
	img->SetColor(0, 0, 0, 0);
	
	GlPlanes*			p = img->LockPixels(GL_PIXEL_A_MASK, true);
	if (p && p->a) {
		int32			cenX = 0, cenY = h / 2;
		int32			rad = (w / 2 < cenY) ? w / 2 : cenY;
		rad = int32(rad * 1.5);
		int32			pix = 0;
		for (int32 y = 0; y < h; y++) {
			for (int32 x = 0; x < w; x++) {
				float	v;
				if (rad <= 0) v = 0;
				else v = ARP_DISTANCE(cenX, cenY, x, y) / rad;
				if (v >= 0.84 && v < 1.0) p->a[pix] = 255;
				pix++;
			}
		}
	}
	img->UnlockPixels(p);
	
	return img;
}
