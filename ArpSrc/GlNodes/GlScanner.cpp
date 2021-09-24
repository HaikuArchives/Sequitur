#include <cstdio>
#include <support/Autolock.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpBitmap.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlDegreeLine.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlLineCache.h>
#include <GlPublic/GlLineFiller.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlMask.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlPixel.h>
#include <GlPublic/GlPlanes.h>
#include <GlPublic/GlRecorder.h>
#include <GlPublic/GlRealtimeParamList.h>
#include <GlPublic/GlRootNode.h>
#include <GlPublic/GlRootRef.h>
#include <GlPublic/GlAlgo2d.h>
#include <GlNodes/GlScanner.h>

static const int32		GL_SCANNER_KEY	= 'ApSc';
static const int32		MAGNIFY_MODE	= 'magn';
static const int32		ADD_MODE		= 'add_';
static const int32		SUB_MODE		= 'sub_';
static const int32		SHIFT_MODE		= 'shft';

static const float		INIT_ANGLE		= 0;
static const int32		ANGLE_KEY		= 'angl';

static const int32		_WARP_KEY		= 'warp';
static const uint32		_WARP_INDEX		= 0;

/***************************************************************************
 * _GL-SCANNER-ALGO
 ***************************************************************************/
class _GlScannerAlgo : public GlAlgo2d
{
public:
	_GlScannerAlgo(	gl_node_id nid, uint32 targets, int32 mode,
					float angle, int32 lengthAbs, float lengthRel, GlAlgo* warp);
	_GlScannerAlgo(const _GlScannerAlgo& o);
	
	virtual GlAlgo*		Clone() const;
	virtual status_t	Process(const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
								GlProcessStatus* status = 0);

	virtual status_t	SetParam(const gl_param_key& key, const GlParamWrap& wrap);
	virtual status_t	GetParam(const gl_param_key& key, GlParamWrap& wrap) const;

private:
	typedef GlAlgo2d	inherited;
	int32				mMode;
	float				mAngle;
	int32				mLengthAbs;
	float				mLengthRel;
};

/***************************************************************************
 * _GL-SCANNER-FILLER
 ***************************************************************************/
class _GlScannerFiller : public GlLineScanner
{
public:
	_GlScannerFiller(uint8* mask, int32 length);
	virtual ~_GlScannerFiller();

	/* Caller owns the curve.
	 */
	status_t			Init(int32 w, int32 h, GlAlgo1d* lens);

protected:
	uint8*				mMask;
	int32				mLength;
	GlAlgo1d*				mLens;
	float*				mCache;
	uint32				mCacheSize;

	virtual void		InitLineVal(uint32 size, gl_filler_cell* cells);
	virtual void		DrawLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame);
};

// Magnify scanner
class _GlScannerMagnify : public _GlScannerFiller
{
public:
	_GlScannerMagnify(uint8* mask, int32 length);

	virtual void		FillLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame);

private:
	typedef _GlScannerFiller inherited;
};

// Add scanner
class _GlScannerAdd : public _GlScannerFiller
{
public:
	_GlScannerAdd(uint8* mask, int32 length);

	virtual void		FillLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame);

private:
	typedef _GlScannerFiller inherited;
};

// Subtract scanner
class _GlScannerSub : public _GlScannerFiller
{
public:
	_GlScannerSub(uint8* mask, int32 length);

	virtual void		FillLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame);

private:
	typedef _GlScannerFiller inherited;
};

// Shift scanner
class _GlScannerShift : public _GlScannerFiller
{
public:
	_GlScannerShift(uint8* mask, int32 length);

	virtual void		FillLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame);

private:
	typedef _GlScannerFiller inherited;
};

/***************************************************************************
  * GL-SCANNER
 ***************************************************************************/
GlScanner::GlScanner(const GlScannerAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
	VerifyChain(new GlChain(_WARP_KEY, GL_1D_IO, SZ(SZ_Warp), this));
}

GlScanner::GlScanner(const GlScanner& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlScanner::Clone() const
{
	return new GlScanner(*this);
}

GlAlgo* GlScanner::Generate(const gl_generate_args& args) const
{
	GlAlgo*			warp = GenerateChainAlgo(_WARP_KEY, args);
	return new _GlScannerAlgo(	Id(), PixelTargets(),
								Params().Menu(GL_MODE_PARAM_KEY),
								Params().Float(ANGLE_KEY),
								Params().Int32('labs'),
								Params().Float('lrel'),
								warp);
}

// #pragma mark -

/***************************************************************************
 * GL-SCANNER-ADD-ON
 ***************************************************************************/
static GlParamType* _new_mode_type()
{
	BMessage		msg;
	msg.AddString("item", "Magnify");		msg.AddInt32("i", MAGNIFY_MODE);
	msg.AddString("item", "Add");			msg.AddInt32("i", ADD_MODE);
	msg.AddString("item", "Subtract");		msg.AddInt32("i", SUB_MODE);
	msg.AddString("item", "Shift");			msg.AddInt32("i", SHIFT_MODE);
	return new GlMenuParamType(GL_MODE_PARAM_KEY, SZ(SZ_Mode), msg, MAGNIFY_MODE);
}

GlScannerAddOn::GlScannerAddOn()
		: inherited(SZI[SZI_arp], GL_SCANNER_KEY, SZ(SZ_Distort), SZ(SZ_Scanner), 1, 0)
{
	mMode			= AddParamType(_new_mode_type());
	mAngle			= AddParamType(new GlFloatParamType(ANGLE_KEY, SZ(SZ_Angle), 0, 359, INIT_ANGLE, 1));
	mLengthAbs		= AddParamType(new GlInt32ParamType('labs', SZ(SZ_Abs_length), -1024, 1024, 0));
	mLengthRel		= AddParamType(new GlFloatParamType('lrel', SZ(SZ_Rel_length), 0, 1, 0.2f, 0.01f));
	mResolution		= AddParamType(new GlInt32ParamType('res_', SZ(SZ_Resolution), 1, 1000, 100));
}

GlNode* GlScannerAddOn::NewInstance(const BMessage* config) const
{
	return new GlScanner(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-ALGO
 ***************************************************************************/
_GlScannerAlgo::_GlScannerAlgo(	gl_node_id nid, uint32 targets, int32 mode,
								float angle, int32 lengthAbs, float lengthRel,
								GlAlgo* warp)
		: inherited(nid, targets), mMode(mode), mAngle(angle), mLengthAbs(lengthAbs),
		  mLengthRel(lengthRel)
{
	if (warp) SetChain(warp, _WARP_INDEX);
}

_GlScannerAlgo::_GlScannerAlgo(const _GlScannerAlgo& o)
		: inherited(o), mMode(o.mMode), mAngle(o.mAngle),
		  mLengthAbs(o.mLengthAbs), mLengthRel(o.mLengthRel)
{
}

GlAlgo* _GlScannerAlgo::Clone() const
{
	return new _GlScannerAlgo(*this);
}

status_t _GlScannerAlgo::Process(	const GlPlanes* pixels, uint8* mask, int32 w, int32 h,
									GlProcessStatus* status)
{
	if (!mask) return B_OK;
	int32				length = mLengthAbs + int32(((w + h) / 2.0) * mLengthRel);
	_GlScannerFiller*	scanner = 0;
	if (mMode == MAGNIFY_MODE) scanner = new _GlScannerMagnify(mask, length);
	else if (mMode == ADD_MODE) scanner = new _GlScannerAdd(mask, length);
	else if (mMode == SUB_MODE) scanner = new _GlScannerSub(mask, length);
	else if (mMode == SHIFT_MODE) scanner = new _GlScannerShift(mask, length);

	if (scanner && scanner->Init(w, h, (GlAlgo1d*)(ChainAt(_WARP_INDEX))) == B_OK)
		scanner->Perform(w, h, mAngle);
	delete scanner;

	return B_OK;
}

status_t _GlScannerAlgo::SetParam(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;
	
	if (key.key == ANGLE_KEY) mAngle = ((const GlFloatWrap&)wrap).v;
	else return B_ERROR;
	return B_OK;
}

status_t _GlScannerAlgo::GetParam(const gl_param_key& key, GlParamWrap& wrap) const
{
	if (wrap.Type() != GL_FLOAT_TYPE) return B_ERROR;

	if (key.key == ANGLE_KEY) ((GlFloatWrap&)wrap).v = mAngle;
	else return B_ERROR;
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-FILLER
 ***************************************************************************/
_GlScannerFiller::_GlScannerFiller(uint8* mask, int32 length)
		: mMask(mask), mLength(length), mLens(0), mCache(0),
		  mCacheSize(0)
{
}

_GlScannerFiller::~_GlScannerFiller()
{
	delete[] mCache;
}

status_t _GlScannerFiller::Init(int32 w, int32 h, GlAlgo1d* lens)
{
	delete[] mCache;
	mCache = 0;
	mCacheSize = 0;
	
	if (!lens) return B_ERROR;
	mLens = lens;

	uint32		cacheSize = (w > h) ? w : h;
	if (cacheSize < 1) return B_ERROR;
	mCache = new float[cacheSize];
	if (!mCache) return B_NO_MEMORY;
	mCacheSize = cacheSize;
	return B_OK;
}

void _GlScannerFiller::InitLineVal(uint32 size, gl_filler_cell* cells)
{
	ArpVALIDATE(cells, return);
	for (uint32 k = 0; k < size; k++) cells[k].val = 255;
}

void _GlScannerFiller::DrawLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame)
{
	ArpVALIDATE(mMask && cells, return);
	for (uint32 k = start; k < end; k++) {
		if (GL_IN_BOUNDS(cells[k].x, cells[k].y, w, h)) {
			int32	pix = ARP_PIXEL(cells[k].x, cells[k].y, w);
			mMask[pix] = arp_clip_255(cells[k].val);
		}
	}
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-MAGNIFY
 ***************************************************************************/
_GlScannerMagnify::_GlScannerMagnify(uint8* mask, int32 length)
		: inherited(mask, length)
{
}

static bool _get_cell_bounds(	int32 w, int32 h, gl_filler_cell* cells,
								uint32 start, uint32 end,
								uint32* outStart, uint32* outEnd)
{
	bool		foundStart = false;
	for (uint32 k = start; k < end; k++) {
		if (GL_IN_BOUNDS(cells[k].x, cells[k].y, w, h)) {
			if (!foundStart) {
				*outStart = k;
				foundStart = true;
			}
			*outEnd = k + 1;
		}
	}
	return foundStart;
}

/* I use the mLens as a magnifying lens -- the higher the value, the more
 * that pixel gets stretched out.  All the pixel stretching is summed to
 * zero so the new line is the same length as the old.
 */
void _GlScannerMagnify::FillLine(	int32 w, int32 h, gl_filler_cell* cells,
									uint32 start, uint32 end, float frame)
{
	ArpVALIDATE(mCache && cells && mMask, return);
	/* Temp until the LineFiller is reporting the in-bounds start and end
	 */
	uint32			startTmp = 0, endTmp = 0;
	if (!_get_cell_bounds(w, h, cells, start, end, &startTmp, &endTmp)) return;
	ArpASSERT(endTmp > startTmp);
	if (endTmp <= startTmp) return;
	
	uint32			size = endTmp - startTmp;
	ArpASSERT(size <= mCacheSize);
	float			sum = 0;
	uint32			k;
//printf("Frame is %f\n", frame);
	mLens->SetStep(frame);
	mLens->Run(mCache, 0, size);
	/* Figure how much each cell contributes to the total.
	 */
	float			tmpMag = (w + h) / 2.0f;
	for (k = 0; k < size; k++) {
//		float		v = 1 + (mLens->At(_curve_index(k, size)) * tmpMag);
		float		v = 1 + (mCache[k] * tmpMag);
		mCache[k] = v;
		sum += v;
	}
	float			scale = size / sum;
	for (k = 0; k < size; k++) mCache[k] *= scale;

	/* Caclucalate the new values.
	 */
	uint32			runStart = 0, i;
	sum = 0;
	uint32			target = startTmp;
	for (k = 0; k < size; k++) {
		if (sum + mCache[k] >= 1 || (k == size - 1 && target < endTmp && mCache[k] > 0.0)) {
			float	v = 0;
			/* Add up all previous cells.
			 */
			for (uint32 k2 = runStart; k2 < k; k2++) {
				i = startTmp + k2;
				ArpASSERT(i >= start && i < end);
				ArpASSERT(GL_IN_BOUNDS(cells[i].x, cells[i].y, w, h));
				v += (mMask[ARP_PIXEL(cells[i].x, cells[i].y, w)] * mCache[k2]);
			}
			/* Add in the current cell, and if it's got any left over
			 * make sure we stay on it.
			 */
			i = startTmp + k;
			ArpASSERT(i >= start && i <= end);
			ArpASSERT(GL_IN_BOUNDS(cells[i].x, cells[i].y, w, h));
			float	remainder = (1 - sum);
			v += (mMask[ARP_PIXEL(cells[i].x, cells[i].y, w)] * remainder);
			mCache[k] -= remainder;

			ArpASSERT(target >= start && target < end);
			ArpASSERT(GL_IN_BOUNDS(cells[target].x, cells[target].y, w, h));
			cells[target].val = v;
			target++;
			if (mCache[k] <= 0) {
				runStart = k + 1;
			} else {
				runStart = k;
				k--;
			}
			sum = 0.0;
		} else sum += mCache[k];
	}
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-2D-ADD
 ***************************************************************************/
_GlScannerAdd::_GlScannerAdd(uint8* mask, int32 length)
		: inherited(mask, length)
{
}

/* Add the mLens value to each cell.
 */
void _GlScannerAdd::FillLine(	int32 w, int32 h, gl_filler_cell* cells,
								uint32 start, uint32 end, float frame)
{
	ArpVALIDATE(mCache && cells && mMask, return);
	/* Temp until the LineFiller is reporting the in-bounds start and end
	 */
	uint32			startTmp = 0, endTmp = 0;
	if (!_get_cell_bounds(w, h, cells, start, end, &startTmp, &endTmp)) return;
	ArpASSERT(endTmp > startTmp);
	if (endTmp <= startTmp) return;
	
	uint32			scanSize = endTmp - startTmp;
	ArpASSERT(mCacheSize >= scanSize);
	mLens->Run(mCache, 0, scanSize);

	for (uint32 k = startTmp; k < endTmp; k++) {
		ArpASSERT(GL_IN_BOUNDS(cells[k].x, cells[k].y, w, h));
		int32		pix = ARP_PIXEL(cells[k].x, cells[k].y, w);
		float		v = mCache[k - startTmp] * 255;
		cells[k].val = arp_clip_255(mMask[pix] + v);
	}

	mLens->SetStep(frame);
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-SUB
 ***************************************************************************/
_GlScannerSub::_GlScannerSub(uint8* mask, int32 length)
		: inherited(mask, length)
{
}

/* Subtract the mLens value from each cell.
 */
void _GlScannerSub::FillLine(	int32 w, int32 h, gl_filler_cell* cells,
								uint32 start, uint32 end, float frame)
{
	ArpVALIDATE(mCache && cells && mMask, return);
	/* Temp until the LineFiller is reporting the in-bounds start and end
	 */
	uint32			startTmp = 0, endTmp = 0;
	if (!_get_cell_bounds(w, h, cells, start, end, &startTmp, &endTmp)) return;
	ArpASSERT(endTmp > startTmp);
	if (endTmp <= startTmp) return;
	
	uint32			scanSize = endTmp - startTmp;
	ArpASSERT(mCacheSize >= scanSize);
	mLens->Run(mCache, 0, scanSize);
	
	for (uint32 k = startTmp; k < endTmp; k++) {
		ArpASSERT(GL_IN_BOUNDS(cells[k].x, cells[k].y, w, h));
		int32		pix = ARP_PIXEL(cells[k].x, cells[k].y, w);
		float		v = mCache[k - startTmp] * 255;
//		float		v = mLens->At(GL_1D_STEP(k - startTmp, scanSize)) * 255;
		cells[k].val = arp_clip_255(mMask[pix] - v);
	}

	mLens->SetStep(frame);
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-SHIFT
 ***************************************************************************/
_GlScannerShift::_GlScannerShift(uint8* mask, int32 length)
		: inherited(mask, length)
{
}

/* Shift each cell -- mLens of 0 means shift hard left, 0.5 stays the same,
 * 1 shift hard right.
 */
void _GlScannerShift::FillLine(	int32 w, int32 h, gl_filler_cell* cells,
								uint32 start, uint32 end, float frame)
{
	ArpVALIDATE(mCache && cells && mMask, return);
	/* Temp until the LineFiller is reporting the in-bounds start and end
	 */
	uint32			startTmp = 0, endTmp = 0;
	if (!_get_cell_bounds(w, h, cells, start, end, &startTmp, &endTmp)) return;
	ArpASSERT(endTmp > startTmp);
	if (endTmp <= startTmp) return;
	
	uint32			scanSize = endTmp - startTmp, k;
	ArpASSERT(mCacheSize >= scanSize);
	mLens->Run(mCache, 0, scanSize);

	for (k = startTmp; k < endTmp; k++) cells[k].val = -1;
	
	for (k = startTmp; k < endTmp; k++) {
		ArpASSERT(GL_IN_BOUNDS(cells[k].x, cells[k].y, w, h));
		int32		pix = ARP_PIXEL(cells[k].x, cells[k].y, w);
		float		v = (mCache[k - startTmp] * 2) - 1;
		int32		shift = int32(k + (v * mLength));
		uint8		src = mMask[pix];
		if (shift == int32(k)) {
			if (cells[k].val < 0) cells[k].val = src;
			else cells[k].val = (cells[k].val + src) / 2;
		} else if (shift < int32(k)) {
			if (shift < int32(startTmp)) shift = startTmp;
			for (int32 j = shift; j <= int32(k); j++) {
				if (cells[j].val < 0) cells[j].val = src;
				else cells[j].val = (cells[j].val + src) / 2;
			}
		} else if (shift > int32(k)) {
			if (shift >= int32(endTmp)) shift = endTmp - 1;
			for (int32 j = k; j <= shift; j++) {
				if (cells[j].val < 0) cells[j].val = src;
				else cells[j].val = (cells[j].val + src) / 2;
			}
		}
	}

	mLens->SetStep(frame);
}

// #pragma mark -

/***************************************************************************
 * _GL-SCANNER-RECORDER
 ***************************************************************************/
class _GlScannerRecorder : public GlRecorder
{
public:
	_GlScannerRecorder(	GlAlgo2d* algo, GlRealtimeParamList* params,
						const GlRootRef& ref);
	virtual ~_GlScannerRecorder();

	virtual void			SetState(GlControlState& s) const;
	virtual void			LockedSetStep(float step);
	virtual status_t		Draw(ArpBitmap** bm);
	virtual void			ParamEvent(gl_param_key key, const GlParamWrap& wrap);
	virtual void			MidiEvent(GlMidiEvent event, int32 letter);
	virtual status_t		UpdateSource();

private:
	GlRootRef				mRef;
	GlAlgo2d*				mAlgo;
	GlRealtimeParamList*	mParams;
	float					mAngle;

	GlImage*				mOrig;
	GlImage*				mEdit;
	GlImage*				mAngleIm;

	GlMask					mOrigMask;
	GlMask					mEditMask;

	GlLineCache				mLine;		// A cache of the line scanning through the image
	
	void					DrawLine(GlPlanes& p, float step);

	/* Get the current step as part of getting the mask because the
	 * lock is already being acquired there.  No reason to duplicate that.
	 */
	uint8*					GetMask(GlPlanes& p, float* step);
	status_t				CacheLine(GlPlanes& p);

	GlImage*				GetImage(int32 w, int32 h);
};

_GlScannerRecorder::_GlScannerRecorder(	GlAlgo2d* algo, GlRealtimeParamList* params,
										const GlRootRef& ref)
		: mRef(ref), mAlgo(algo), mParams(params), mAngle(-1),
		  mOrig(0), mEdit(0), mAngleIm(0)
{
}

_GlScannerRecorder::~_GlScannerRecorder()
{
	delete mOrig;
	delete mEdit;
	delete mAngleIm;
	delete mAlgo;
	delete mParams;
}

void _GlScannerRecorder::SetState(GlControlState& s) const
{
	if (!mParams) return;
	BAutolock		l(mAccess);
	mParams->SetState(s);
}

void _GlScannerRecorder::LockedSetStep(float step)
{
	mStep = step;
}

status_t _GlScannerRecorder::Draw(ArpBitmap** bm)
{
	if (!mAlgo) return B_ERROR;
	int32				w = (*bm)->Width(), h = (*bm)->Height();
	if (w < 1 || h < 1) return B_ERROR;
	GlImage*			img = GetImage(w, h);
	if (!img) return B_ERROR;
	ArpASSERT(mOrig);

	GlPlanes*			p = img->LockPixels(GL_PIXEL_RGBA, true);
	if (p && p->w == w && p->h == h) {
		ArpASSERT(p->r && p->g && p->b && p->a);
		float			step = -1.0;
		uint8*			mask = GetMask(*p, &step);
		if (mask) {
			memcpy(p->r,	mask,		p->w * p->h);
			memcpy(p->g,	mask,		p->w * p->h);
			memcpy(p->b,	mask,		p->w * p->h);
		}
		if (step >= 0.0) DrawLine(*p, step);
	}
	img->UnlockPixels(p);
	
	*bm = img->AsBitmap();
	
	return B_OK;
}

void _GlScannerRecorder::ParamEvent(gl_param_key key, const GlParamWrap& wrap)
{
	if (!mParams || !mAlgo) return;
	BAutolock		l(mAccess);
	/* FIX:  I should check and see if the step has actually changed.
	 */
	mAlgo->SetStep(mStep);
	mParams->SetValue(key, wrap);
}

void _GlScannerRecorder::MidiEvent(GlMidiEvent event, int32 letter)
{
	if (!mParams || !mAlgo) return;
	BAutolock		l(mAccess);
	/* FIX:  I should check and see if the step has actually changed.
	 */
	mAlgo->SetStep(mStep);
//mAlgo->PrintTree();
//debugger("dsds");
	mParams->SetValue(letter, event);
//mAlgo->PrintTree();
}

status_t _GlScannerRecorder::UpdateSource()
{
	if (!mParams) return B_ERROR;
	BAutolock		l(mAccess);
	status_t		err = B_ERROR;
	GlRootNode*		root = mRef.WriteLock();
	if (root) err = mParams->UpdateSource(root);
	mRef.WriteUnlock(root);
	return err;
}

void _GlScannerRecorder::DrawLine(GlPlanes& p, float step)
{
	if (CacheLine(p) != B_OK) return;
	int32		s = int32(step * p.h);
	if (s < 0) s = 0; else if (s >= p.h) s = p.h - 1;

	int32		stop = (s + 1) * p.w;
	for (int32 pix = s * p.w; pix < stop; pix++)
		p.r[pix] = 255;
}

class _GlScannerValueSrf : public GlAlgo2d
{
public:
	_GlScannerValueSrf() : GlAlgo2d(0, 0)							{ }
	_GlScannerValueSrf(const _GlScannerValueSrf& o) : GlAlgo2d(o)	{ }

	virtual GlAlgo* Clone() const { return new _GlScannerValueSrf(*this); }
	
	virtual void Process(const GlPlanes* src, GlPlanes& dest)
	{
		if (!src || dest.size < 1) return;
		ArpVALIDATE(src->HasColor(), return);
	
		for (int32 pix = 0; pix < dest.w * dest.h; pix++) {
			bool		needsValue = true;
			uint8		val = 0;
			for (uint32 k = 0; k < dest.size; k++) {
				if (dest.plane[k][pix] > 0) {
					if (needsValue) { val = src->Value(pix); needsValue = false; }
					dest.plane[k][pix] = ARP_MIN(dest.plane[k][pix], val);
				}
			}
		}
	}
};

uint8* _GlScannerRecorder::GetMask(GlPlanes& p, float* step)
{
	ArpASSERT(p.w > 0 && p.h > 0);
	int32			w, h;
	uint8*			orig = mOrigMask.Data(&w, &h);
	if (!orig || w != p.w || h != p.h) {
		/* FIX:  What's the best way to initialize this?
		 * Hopefully using it will reveal.
		 */
		GlAlgo2d*	s = new _GlScannerValueSrf();
		if (!s) return 0;
		orig = mOrigMask.Make(p, s);
		delete s;
		if (!orig) return 0;
	}

	uint8*			edit = mEditMask.Data(&w, &h);
	if (!edit || w != p.w || h != p.h) {
		if (mEditMask.Make(orig, p.w, p.h) != B_OK) return 0;
		edit = mEditMask.Data(&w, &h);
		if (!edit || w != p.w || h != p.h) return 0;
	}

	memcpy(edit,	orig,		p.w * p.h);

	GlPlanes		dest(p.w, p.h);
	if (dest.SetSize(1) != B_OK) return 0;
	dest.plane[0] = edit;
	
	BAutolock		l(mAccess);
	mAlgo->ProcessAll(&p, dest);
	*step = mStep;
	return edit;
}

static inline bool _in_tolerance(float f1, float f2)
{
	if (ARP_ABS(f2 - f1) <= 0.1) return true;
	return false;
}

status_t _GlScannerRecorder::CacheLine(GlPlanes& p)
{
	float				a;
	{
		BAutolock		l(mAccess);
		GlFloatWrap		w;
		if (mAlgo->GetParam(gl_param_key(0, ANGLE_KEY), w) != B_OK) return B_ERROR;
		a = w.v;
	}
	int32			cacheSize = (p.w > p.h) ? p.w : p.h;
	if (cacheSize < 1) return B_ERROR;
	if (_in_tolerance(a, mAngle) && uint32(cacheSize) == mLine.size) return B_OK;

	if (mLine.Resize(cacheSize) != B_OK) return B_ERROR;
	ArpVALIDATE(mLine.size == uint32(cacheSize), return B_ERROR);

	mAngle = a;
	if (mAngle < 0 || mAngle > 359) mAngle = 0;

	GlDegreeLine		deg(mAngle);
	deg.CacheLine(mLine);
//	mLine.Print();
	return B_OK;
}

GlImage* _GlScannerRecorder::GetImage(int32 w, int32 h)
{
	ArpVALIDATE(w > 0 && h > 0, return 0);
	ArpVoxel			v(0, 0, 0, 255);
	/* Make sure I've got an edit image.
	 */
	if (!mEdit || mEdit->Width() != w || mEdit->Height() != h) {
		delete mEdit;
		mEdit = 0;
	}
	if (!mEdit) mEdit = new GlImage(w, h, &v);
	if (!mEdit || mEdit->InitCheck() != B_OK) {
		delete mEdit;
		mEdit = 0;
		return 0;
	}
	/* Make sure I've got an orig image.
	 */
	bool				origNeedsInit = false;
	if (!mOrig || mOrig->Width() != w || mOrig->Height() != h) {
		delete mOrig;
		mOrig = 0;
	}
	if (!mOrig) {
		mOrig = new GlImage(w, h, &v);
		origNeedsInit = true;
	}
	if (!mOrig || mOrig->InitCheck() != B_OK) {
		delete mOrig;
		mOrig = 0;
		return 0;
	}
	/* Copy the orig into the edit.
	 */
	GlPlanes*		src = mOrig->LockPixels(GL_PIXEL_RGB, true);
	GlPlanes*		dest = mEdit->LockPixels(GL_PIXEL_RGB, true);
	if (src && dest && src->w == dest->w && src->h == dest->h) {
		int32		size = src->w * src->h;
		if (origNeedsInit) src->ColorWheel(0, 0, src->w - 1, src->h - 1);
		memcpy(dest->r, src->r, size);
		memcpy(dest->g, src->g, size);
		memcpy(dest->b, src->b, size);
	}
	mOrig->UnlockPixels(src);
	mEdit->UnlockPixels(dest);

	return mEdit;
}

/* FIX: This is temp until I figure out how to construct
 * new ControlTargets based on recordable parse nodes.
 */
#include <GlKernel/GlRecorderHolder.h>

BView* GlScanner::NewView(gl_new_view_params& params) const
{
	BView*						v = inherited::NewView(params);
	if (v && params.viewType == GL_INSPECTOR_VIEW && params.channel) {
		GlRecorder*				r = ((GlScanner*)this)->NewRecorder(params.ref);
		if (r) {
			GlRecorderHolder*	rh = new GlRecorderHolder(r);
			if (rh) params.channel->Add(rh);
			else r->DecRefs();
		}
	}
	return v;
}

GlRecorder* GlScanner::NewRecorder(const GlRootRef& ref)
{
	gl_generate_args		args;
	GlAlgo*					a = Generate(args);
	GlAlgo2d*				s = (a) ? a->As2d() : 0;
	if (!s) {
		delete a;
		return 0;
	}
	GlRealtimeParamList*	list = new GlRealtimeParamList();
	if (!list) {
		delete s;
		return 0;
	}
	if (GetParams(*list) != B_OK) {
		delete s;
		delete list;
		return 0;
	}
	list->Build(s);
//	list->Print();
	return new _GlScannerRecorder(s, list, ref);
}
