#include <stdio.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpMenuControl.h>
#include <GlPublic/GlAlgoNbr.h>
#include <GlPublic/GlArrayF.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include "GlPublic/GlPixel.h"
#include "GlPublic/GlPlanes.h"
#include <GlNodes/GlBinaryOpNbr.h>

static const uint32		_LH_INDEX			= 0;
static const uint32		_RH_INDEX			= 1;

/***************************************************************************
 * _GL-BINARY-OP-NBR
 ***************************************************************************/
class _GlBinaryOpNbr : public GlAlgoNbr
{
public:
	_GlBinaryOpNbr(gl_node_id nid, int32 op);
	_GlBinaryOpNbr(const _GlBinaryOpNbr& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	AssignBinary(GlAlgo* lh, GlAlgo* rh);
	virtual status_t	Process(GlArrayF& set);

private:
	typedef GlAlgoNbr	inherited;
	int32				mOp;
	GlAlgoNbrWrap		mLh, mRh;
	GlArrayF			mLhCache, mRhCache;
};

/***************************************************************************
  * GL-BINARY-OP-NBR
 ***************************************************************************/
GlBinaryOpNbr::GlBinaryOpNbr(	const GlNodeAddOn* addon,
								const BMessage* config)
		: inherited(addon, config)
{
}

GlBinaryOpNbr::GlBinaryOpNbr(const GlBinaryOpNbr& o)
		: inherited(o)
{
}

GlNode* GlBinaryOpNbr::Clone() const
{
	return new GlBinaryOpNbr(*this);
}

GlAlgo* GlBinaryOpNbr::Generate(const gl_generate_args& args) const
{
	return new _GlBinaryOpNbr(Id(), Params().Menu(GL_BINARY_OP_PARAM_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-BINARY-OP-NBR-ADD-ON
 ***************************************************************************/
GlBinaryOpNbrAddOn::GlBinaryOpNbrAddOn()
		: inherited(SZI[SZI_arp], GL_BINARY_NBR_KEY, SZ(SZ_Flow), SZ(SZ_Binary), 1, 0)
{
	ArpASSERT(GlAlgoNbr::RegisterKey(GL_BINARY_NBR_KEY));

	BMessage		msg;
	msg.AddString(ARP_MENU_ITEM_STR, "Add");		msg.AddInt32(ARP_MENU_I_STR, GL_ADD_PARAM_KEY);
	msg.AddString(ARP_MENU_ITEM_STR, "Subtract");	msg.AddInt32(ARP_MENU_I_STR, GL_SUB_PARAM_KEY);
	msg.AddString(ARP_MENU_ITEM_STR, "Multiply");	msg.AddInt32(ARP_MENU_I_STR, GL_MULT_PARAM_KEY);
	msg.AddString(ARP_MENU_ITEM_STR, "Divide");		msg.AddInt32(ARP_MENU_I_STR, GL_DIV_PARAM_KEY);
	AddParamType(new GlMenuParamType(GL_BINARY_OP_PARAM_KEY, SZ(SZ_Mode), msg, GL_ADD_PARAM_KEY));
}

GlNode* GlBinaryOpNbrAddOn::NewInstance(const BMessage* config) const
{
	return new GlBinaryOpNbr(this, config);
}

GlImage* GlBinaryOpNbrAddOn::NewImage() const
{
	int32			w = Prefs().GetInt32(GL_NODE_IMAGE_X),
					h = Prefs().GetInt32(GL_NODE_IMAGE_Y);
	if (w < 1 || h < 1) return 0;
	
	GlImage*		img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return 0;
	}

	/* FIX: Draw +, - etc. -- Share code with the Binary Map.
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

// #pragma mark -

/***************************************************************************
 * _GL-BINARY-OP-NBR
 ***************************************************************************/
_GlBinaryOpNbr::_GlBinaryOpNbr(gl_node_id nid, int32 op)
		: inherited(GL_BINARY_NBR_KEY, nid, GL_BINARY), mOp(op)
{
	if (mOp == GL_ADD_PARAM_KEY || mOp == GL_SUB_PARAM_KEY)
		mPrecedence = 0.66f;
	else mPrecedence = 0.33f;
}

_GlBinaryOpNbr::_GlBinaryOpNbr(const _GlBinaryOpNbr& o)
		: inherited(o), mOp(o.mOp)
{
}

GlAlgo* _GlBinaryOpNbr::Clone() const
{
	return new _GlBinaryOpNbr(*this);
}

status_t _GlBinaryOpNbr::AssignBinary(GlAlgo* lh, GlAlgo* rh)
{
	SetChain(lh, _LH_INDEX);
	SetChain(rh, _RH_INDEX);
	mLh.SetAlgo(lh);
	mRh.SetAlgo(rh);
	return B_OK;
}

status_t _GlBinaryOpNbr::Process(GlArrayF& set)
{
	if (mLh.InitCheck() != B_OK || mRh.InitCheck() != B_OK) return B_ERROR;

	if (mLhCache.Resize(0) != B_OK) return B_NO_MEMORY;
	if (mRhCache.Resize(0) != B_OK) return B_NO_MEMORY;
	
	if (mLh.Process(mLhCache) != B_OK) return B_ERROR;
	if (mRh.Process(mRhCache) != B_OK) return B_ERROR;

	uint32					min = ARP_MIN(mLhCache.size, mRhCache.size);
	if (min < 1) return B_OK;
	if (set.Resize(min) != B_OK) return B_NO_MEMORY;
	
	uint32					k;
	if (mOp == GL_ADD_PARAM_KEY) {
		for (k = 0; k < set.size; k++) set.n[k] = mLhCache.n[k] + mRhCache.n[k];
	} else if (mOp == GL_SUB_PARAM_KEY) {
		for (k = 0; k < set.size; k++) set.n[k] = mLhCache.n[k] - mRhCache.n[k];
	} else if (mOp == GL_MULT_PARAM_KEY) {
		for (k = 0; k < set.size; k++) set.n[k] = mLhCache.n[k] * mRhCache.n[k];
	} else if (mOp == GL_DIV_PARAM_KEY) {
		for (k = 0; k < set.size; k++) set.n[k] = mLhCache.n[k] / mRhCache.n[k];
	}

	return B_OK;
}
