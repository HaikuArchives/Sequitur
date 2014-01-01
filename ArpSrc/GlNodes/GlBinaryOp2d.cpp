#include <stdio.h>
#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include "GlPublic/GlPixel.h"
#include "GlPublic/GlPlanes.h"
#include <GlNodes/GlBinaryOp2d.h>

static const int32		GL_BINARY_2D_KEY	= 'AsBO';
static const int32		_OP_KEY				= 'op__';
static const uint32		_LH_INDEX			= 0;
static const uint32		_RH_INDEX			= 1;

/***************************************************************************
 * _GL-BINARY-OP-2D
 ***************************************************************************/
class _GlBinaryOp2d : public GlAlgo2d
{
public:
	_GlBinaryOp2d(gl_node_id nid, int32 op);
	_GlBinaryOp2d(const _GlBinaryOp2d& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* src, GlPlanes& dest,
								GlProcessStatus* status = 0);
	virtual status_t	AssignBinary(GlAlgo* lh, GlAlgo* rh);

private:
	typedef GlAlgo2d	inherited;
	int32				mOp;

	status_t			Add(GlPlanes& lh, GlPlanes& rh) const;
	status_t			Sub(GlPlanes& lh, GlPlanes& rh) const;
	status_t			Mult(GlPlanes& lh, GlPlanes& rh) const;
	status_t			Div(GlPlanes& lh, GlPlanes& rh) const;
	status_t			Min(GlPlanes& lh, GlPlanes& rh) const;
	status_t			Max(GlPlanes& lh, GlPlanes& rh) const;
};

/***************************************************************************
  * GL-BINARY-OP-2D
 ***************************************************************************/
GlBinaryOp2d::GlBinaryOp2d(	const GlBinaryOp2dAddOn* addon,
								const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlBinaryOp2d::GlBinaryOp2d(const GlBinaryOp2d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlBinaryOp2d::Clone() const
{
	return new GlBinaryOp2d(*this);
}

GlAlgo* GlBinaryOp2d::Generate(const gl_generate_args& args) const
{
	return new _GlBinaryOp2d(Id(), Params().Menu(_OP_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-BINARY-OP-2D-ADD-ON
 ***************************************************************************/
GlBinaryOp2dAddOn::GlBinaryOp2dAddOn()
		: inherited(SZI[SZI_arp], GL_BINARY_2D_KEY, SZ(SZ_Flow), SZ(SZ_Binary), 1, 0)
{
	BMessage		msg;
	msg.AddString("item", "Add");			msg.AddInt32("i", GL_ADD_BINARY_SRF_KEY);
	msg.AddString("item", "Subtract");		msg.AddInt32("i", GL_SUB_BINARY_SRF_KEY);
	msg.AddString("item", "Multiply");		msg.AddInt32("i", GL_MULT_BINARY_SRF_KEY);
	msg.AddString("item", "Divide");		msg.AddInt32("i", GL_DIV_BINARY_SRF_KEY);
	msg.AddString("item", "Min");			msg.AddInt32("i", GL_MIN_BINARY_SRF_KEY);
	msg.AddString("item", "Max");			msg.AddInt32("i", GL_MAX_BINARY_SRF_KEY);
	mOp		= AddParamType(new GlMenuParamType(_OP_KEY, SZ(SZ_Mode), msg, GL_ADD_BINARY_SRF_KEY));
}

GlNode* GlBinaryOp2dAddOn::NewInstance(const BMessage* config) const
{
	return new GlBinaryOp2d(this, config);
}

GlImage* GlBinaryOp2dAddOn::NewImage() const
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
 * _GL-BINARY-OP-2D
 ***************************************************************************/
_GlBinaryOp2d::_GlBinaryOp2d(gl_node_id nid, int32 op)
		: inherited(nid, 0, GL_FILL_BLACK, GL_BINARY), mOp(op)
{
	if (mOp == GL_ADD_BINARY_SRF_KEY || mOp == GL_SUB_BINARY_SRF_KEY)
		mPrecedence = 0.66f;
	else mPrecedence = 0.33f;
}

_GlBinaryOp2d::_GlBinaryOp2d(const _GlBinaryOp2d& o)
		: inherited(o), mOp(o.mOp)
{
}

GlAlgo* _GlBinaryOp2d::Clone() const
{
	return new _GlBinaryOp2d(*this);
}

status_t _GlBinaryOp2d::AssignBinary(GlAlgo* lh, GlAlgo* rh)
{
	SetChain(lh, _LH_INDEX);
	SetChain(rh, _RH_INDEX);
	return B_OK;
}

status_t _GlBinaryOp2d::Process(	const GlPlanes* src, GlPlanes& dest,
									GlProcessStatus* status)
{
	if (dest.size < 1) return B_OK;

	GlAlgo2d*		lh = Algo2dAt(_LH_INDEX);
	GlAlgo2d*		rh = Algo2dAt(_RH_INDEX);

	if (lh && rh) {
		GlPlanes*	c = dest.Clone();
		if (c && c->size == dest.size) {
			lh->Process(src, dest, status);
			rh->Process(src, *c, status);
			if (mOp == GL_ADD_BINARY_SRF_KEY) Add(dest, *c);
			else if (mOp == GL_SUB_BINARY_SRF_KEY) Sub(dest, *c);
			else if (mOp == GL_MULT_BINARY_SRF_KEY) Mult(dest, *c);
			else if (mOp == GL_DIV_BINARY_SRF_KEY) Div(dest, *c);
			else if (mOp == GL_MIN_BINARY_SRF_KEY) Min(dest, *c);
			else if (mOp == GL_MAX_BINARY_SRF_KEY) Max(dest, *c);
			else ArpASSERT(false);
		}
		delete c;
	} else if (lh) {
		lh->Process(src, dest, status);
	} else if (rh) {
		rh->Process(src, dest, status);
	}
	return B_OK;
}

status_t _GlBinaryOp2d::Add(GlPlanes& lh, GlPlanes& rh) const
{
	for (int32 pix = 0; pix < lh.w * lh.h; pix++)
		for (uint32 k = 0; k < lh.size; k++)
			lh.plane[k][pix] = arp_clip_255(lh.plane[k][pix] + int32(rh.plane[k][pix]));
	return B_OK;
}

status_t _GlBinaryOp2d::Sub(GlPlanes& lh, GlPlanes& rh) const
{
	for (int32 pix = 0; pix < lh.w * lh.h; pix++)
		for (uint32 k = 0; k < lh.size; k++)
			lh.plane[k][pix] = arp_clip_255(lh.plane[k][pix] - int32(rh.plane[k][pix]));
	return B_OK;
}

status_t _GlBinaryOp2d::Mult(GlPlanes& lh, GlPlanes& rh) const
{
	for (int32 pix = 0; pix < lh.w * lh.h; pix++)
		for (uint32 k = 0; k < lh.size; k++)
			lh.plane[k][pix] = arp_clip_255(lh.plane[k][pix] * float(rh.plane[k][pix]));
	return B_OK;
}

status_t _GlBinaryOp2d::Div(GlPlanes& lh, GlPlanes& rh) const
{
	for (int32 pix = 0; pix < lh.w * lh.h; pix++)
		for (uint32 k = 0; k < lh.size; k++)
			lh.plane[k][pix] = arp_clip_255(lh.plane[k][pix] / float(rh.plane[k][pix]));
	return B_OK;
}

status_t _GlBinaryOp2d::Min(GlPlanes& lh, GlPlanes& rh) const
{
	for (int32 pix = 0; pix < lh.w * lh.h; pix++) {
		for (uint32 k = 0; k < lh.size; k++) {
			if (rh.plane[k][pix] < lh.plane[k][pix]) lh.plane[k][pix] = rh.plane[k][pix];
		}
	}
	return B_OK;
}

status_t _GlBinaryOp2d::Max(GlPlanes& lh, GlPlanes& rh) const
{
	for (int32 pix = 0; pix < lh.w * lh.h; pix++) {
		for (uint32 k = 0; k < lh.size; k++) {
			if (rh.plane[k][pix] > lh.plane[k][pix]) lh.plane[k][pix] = rh.plane[k][pix];
		}
	}
	return B_OK;
}
