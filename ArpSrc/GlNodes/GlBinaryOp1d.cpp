#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamList.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlNodes/GlBinaryOp1d.h>

static const int32		GL_BINARY_1D_KEY	= '1BOp';
static const int32		_BINARY_1D_KEY		= '1BOp';
static const int32		_OP_KEY				= 'op__';
static const uint32		_LH_INDEX			= 0;
static const uint32		_RH_INDEX			= 1;

/***************************************************************************
 * _GL-BINARY-OP-1D
 ***************************************************************************/
class _GlBinaryOp1d : public GlAlgo1d
{
public:
	_GlBinaryOp1d(gl_node_id nid, int32 op);
	_GlBinaryOp1d(const _GlBinaryOp1d& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	AssignBinary(GlAlgo* lh, GlAlgo* rh);
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

private:
	typedef GlAlgo1d	inherited;
	int32				mOp;
	GlAlgo1dWrap		mLh, mRh;

	status_t			Add(float* lh, float* rh, int32 size) const;
	status_t			Sub(float* lh, float* rh, int32 size) const;
	status_t			Mult(float* lh, float* rh, int32 size) const;
	status_t			Div(float* lh, float* rh, int32 size) const;
	status_t			Min(float* lh, float* rh, int32 size) const;
	status_t			Max(float* lh, float* rh, int32 size) const;

protected:
	virtual void		_print() const;
};

/***************************************************************************
  * GL-BINARY-OP-1D
 ***************************************************************************/
GlBinaryOp1d::GlBinaryOp1d(const GlBinaryOp1dAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlBinaryOp1d::GlBinaryOp1d(const GlBinaryOp1d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlBinaryOp1d::Clone() const
{
	return new GlBinaryOp1d(*this);
}

GlAlgo* GlBinaryOp1d::Generate(const gl_generate_args& args) const
{
	return new _GlBinaryOp1d(Id(), Params().Menu(_OP_KEY));
}

// #pragma mark -

/***************************************************************************
 * GL-BINARY-OP-1D-ADD-ON
 ***************************************************************************/
GlBinaryOp1dAddOn::GlBinaryOp1dAddOn()
		: inherited(SZI[SZI_arp], GL_BINARY_1D_KEY, SZ(SZ_Flow), SZ(SZ_Binary), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_BINARY_1D_KEY));

	BMessage		msg;
	msg.AddString("item", "Add");			msg.AddInt32("i", GL_ADD_BINARY_1D_KEY);
	msg.AddString("item", "Subtract");		msg.AddInt32("i", GL_SUB_BINARY_1D_KEY);
	msg.AddString("item", "Multiply");		msg.AddInt32("i", GL_MULT_BINARY_1D_KEY);
	msg.AddString("item", "Divide");		msg.AddInt32("i", GL_DIV_BINARY_1D_KEY);
	msg.AddString("item", "Min");			msg.AddInt32("i", GL_MIN_BINARY_1D_KEY);
	msg.AddString("item", "Max");			msg.AddInt32("i", GL_MAX_BINARY_1D_KEY);
	mOp		= AddParamType(new GlMenuParamType(_OP_KEY, SZ(SZ_Mode), msg, GL_ADD_BINARY_1D_KEY));
}

GlNode* GlBinaryOp1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlBinaryOp1d(this, config);
}

GlImage* GlBinaryOp1dAddOn::NewImage() const
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

// #pragma mark -

/***************************************************************************
 * _GL-BINARY-OP-1D
 ***************************************************************************/
_GlBinaryOp1d::_GlBinaryOp1d(gl_node_id nid, int32 op)
		: inherited(_BINARY_1D_KEY, nid, GL_BINARY), mOp(op)
{
	if (mOp == GL_ADD_BINARY_1D_KEY || mOp == GL_SUB_BINARY_1D_KEY)
		mPrecedence = 0.66f;
	else mPrecedence = 0.33f;
}

_GlBinaryOp1d::_GlBinaryOp1d(const _GlBinaryOp1d& o)
		: inherited(o), mOp(o.mOp)
{
}

GlAlgo* _GlBinaryOp1d::Clone() const
{
	return new _GlBinaryOp1d(*this);
}

status_t _GlBinaryOp1d::AssignBinary(GlAlgo* lh, GlAlgo* rh)
{
	SetChain(lh, _LH_INDEX);
	SetChain(rh, _RH_INDEX);
	mLh.SetAlgo(lh);
	mRh.SetAlgo(rh);
	return B_OK;
}

status_t _GlBinaryOp1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	ArpVALIDATE(line && size > 0, return B_ERROR);

	if (mLh.InitCheck() == B_OK && mRh.InitCheck() == B_OK) {
		/* Setup
		 */
		int32			k;

		float*			rhLine = new float[size];
		if (!rhLine) return B_NO_MEMORY;
		for (k = 0; k < size; k++) rhLine[k] = line[k];

		float*			rhAt = 0;
		if (at) {
			rhAt = new float[size];
			if (!rhAt) {
				delete[] rhLine;
				return B_NO_MEMORY;
			}
			for (k = 0; k < size; k++) rhAt[k] = at[k];
		}
		/* Processing
		 */
		mLh.Process(line, at, size, flags);
		mRh.Process(rhLine, rhAt, size, flags);
		if (mOp == GL_ADD_BINARY_1D_KEY) Add(line, rhLine, size);
		else if (mOp == GL_SUB_BINARY_1D_KEY) Sub(line, rhLine, size);
		else if (mOp == GL_MULT_BINARY_1D_KEY) Mult(line, rhLine, size);
		else if (mOp == GL_DIV_BINARY_1D_KEY) Div(line, rhLine, size);
		else if (mOp == GL_MIN_BINARY_1D_KEY) Min(line, rhLine, size);
		else if (mOp == GL_MAX_BINARY_1D_KEY) Max(line, rhLine, size);
		/* Cleanup
		 */
		delete[] rhLine;
		delete[] rhAt;
	} else if (mLh.InitCheck() == B_OK) {
		mLh.Process(line, at, size, flags);
	} else if (mRh.InitCheck() == B_OK) {
		mRh.Process(line, at, size, flags);
	}
	
	return B_OK;
}

status_t _GlBinaryOp1d::Add(float* lh, float* rh, int32 size) const
{
	for (int32 k = 0; k < size; k++)
		lh[k] = arp_clip_1(lh[k] + rh[k]); 
	return B_OK;
}

status_t _GlBinaryOp1d::Sub(float* lh, float* rh, int32 size) const
{
	for (int32 k = 0; k < size; k++)
		lh[k] = arp_clip_1(lh[k] - rh[k]); 
	return B_OK;
}

status_t _GlBinaryOp1d::Mult(float* lh, float* rh, int32 size) const
{
	for (int32 k = 0; k < size; k++)
		lh[k] = arp_clip_1(lh[k] * rh[k]); 
	return B_OK;
}

status_t _GlBinaryOp1d::Div(float* lh, float* rh, int32 size) const
{
	for (int32 k = 0; k < size; k++)
		lh[k] = arp_clip_1(lh[k] / rh[k]); 
	return B_OK;
}

status_t _GlBinaryOp1d::Min(float* lh, float* rh, int32 size) const
{
	for (int32 k = 0; k < size; k++) {
		if (rh[k] < lh[k]) lh[k] = rh[k];
	}
	return B_OK;
}

status_t _GlBinaryOp1d::Max(float* lh, float* rh, int32 size) const
{
	for (int32 k = 0; k < size; k++) {
		if (rh[k] > lh[k]) lh[k] = rh[k];
	}
	return B_OK;
}

void _GlBinaryOp1d::_print() const
{
	if (mOp == GL_ADD_BINARY_1D_KEY) printf("+");
	else if (mOp == GL_SUB_BINARY_1D_KEY) printf("-");
	else if (mOp == GL_MULT_BINARY_1D_KEY) printf("*");
	else if (mOp == GL_DIV_BINARY_1D_KEY) printf("/");
	else if (mOp == GL_MIN_BINARY_1D_KEY) printf("min");
	else if (mOp == GL_MAX_BINARY_1D_KEY) printf("max");
	else inherited::_print();
}
