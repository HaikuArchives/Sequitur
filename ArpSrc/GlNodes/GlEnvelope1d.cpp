#include <ArpMath/ArpDefs.h>
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlCache1d.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlRootNode.h>
#include "GlPublic/GlStrainedParamList.h"
#include "GlNodes/GlEnvelope1d.h"

static const int32			START_KEY			= 'strt';		// The start level
static const int32			END_KEY				= 'end ';		// The end level
static const int32			STAGE_KEY			= 'stag';
static const int32			CURVE_KEY			= 'curv';

static const uint32			STAGES_CHANGED_MSG	= '_stc';
static const char*			STAGE_STR			= "sc";
static const uint32			STAGE_VAL_MSG		= '_stv';
static const uint32			CURVE_VAL_MSG		= '_crv';
static const char*			VAL_STR				= "v";

// _GL-ENVELOPE-VIEW
class _GlEnvelopeView : public GlParamView
{
public:
	_GlEnvelopeView(const gl_new_view_params& params, gl_chain_id cid,
					GlStrainedParamList& list, const GlNode& node);

	virtual void		AttachedToWindow();
	virtual status_t	ParamChanged(	gl_param_key key, uint32 code,
										const GlParamWrap& wrap);

	status_t			UpdateVisual();

protected:
	virtual status_t	UpdateControls();

//	virtual	status_t	ControlMessage(uint32 what);

private:
	typedef GlParamView	inherited;
	ArpBitmapView*		mBmView;

//	status_t			SendStageControl(gl_node_id nid, int32 ptKey, BMessage& msg);

//	status_t			GetCurrentStage(uint32* stage);
//	status_t			GetCurrentValue(uint32 stage, BPoint* outLoc, float* outCurve);
};

/***************************************************************************
 * _GL-ENVELOPE-1D
 ***************************************************************************/
class _GlEnvelope1d : public GlAlgo1d
{
public:
	_GlEnvelope1d(float s, float e, float c, const GlNode* node);
	_GlEnvelope1d(const _GlEnvelope1d& o);
	virtual ~_GlEnvelope1d();

	virtual GlAlgo*		Clone() const;
	virtual status_t	Algo(float* line, float* at, int32 size, uint32 flags) const;

protected:
	virtual uint32		properties() const;

private:
	typedef GlAlgo1d	inherited;
	float				mStart, mEnd, mCurve;
	ArpPoint3d*			mStages;
	uint32				mCount;

	status_t			AlgoLinear(float* line, float* at, int32 size, uint32 flags) const;

	status_t			MakeStages(const GlParamList& params, bool replace);
	void				FreeStages();
	
protected:
	virtual void		_print() const;
};

/***************************************************************************
  * GL-ENVELOPE-1D
 ***************************************************************************/
GlEnvelope1d::GlEnvelope1d(const GlEnvelope1dAddOn* addon, const BMessage* config)
		: inherited(addon, config), mAddOn(addon)
{
}

GlEnvelope1d::GlEnvelope1d(const GlEnvelope1d& o)
		: inherited(o), mAddOn(o.mAddOn)
{
}

GlNode* GlEnvelope1d::Clone() const
{
	return new GlEnvelope1d(*this);
}

BView* GlEnvelope1d::NewView(gl_new_view_params& params) const
{
	if (params.viewType == GL_INSPECTOR_VIEW) {
		if (!Parent()) return 0;
		GlStrainedParamList		list;
		if (GetParams(list) != B_OK) return 0;
		return new _GlEnvelopeView(params, Parent()->Id(), list, *this);
	}
	return inherited::NewView(params);
}

#if 0
//	virtual status_t			Preview(int32 width, int32 height, ArpBitmap** outBm);
status_t GlEnvelope1d::Preview(int32 w, int32 h, ArpBitmap** outBm)
{
	ArpVALIDATE(outBm && w > 0 && h > 0, return B_ERROR);
	GlImage*				img = new GlImage(w, h);
	if (!img || img->InitCheck() != B_OK) {
		delete img;
		return B_NO_MEMORY;
	}

	GlAlgo*					a = 0;
	gl_generate_args		args;
	if (Parent()) a = Parent()->Generate(args);
	else a = Generate(args);
	
	if (a) {
		GlAlgo1d*			a1d = a->As1d();
		if (a1d) {
			GlCache1d		cache;
			cache.Render(a1d, img, GlRect(0, 0, w, h), GlRect(0, 0, w, h));
//			a1d->Render(img, GlRect(0, 0, w, h), GlRect(0, 0, w, h));
		}
	}
	delete a;
	
	*outBm = img->AsBitmap();
	delete img;
	if (*outBm) return B_OK;
	return B_ERROR;
}
#endif

#if 0
status_t GlEnvelope1d::Control(const BMessage& msg)
{
	if (msg.what == STAGES_CHANGED_MSG) {
		int32			n;
		if (msg.FindInt32(STAGE_STR, &n) != B_OK) return B_ERROR;
		while (Params().EraseValue(STAGE_KEY) == B_OK) ;
		for (int32 k = 0; k < n; k++) {
			float		x = (k + 1) / float(n + 1);
			GlParam*	p = new GlPointParam((GlPointParamType*)mAddOn->mStage, Id(), BPoint(x, 0.5));
			if (p) Params().AddParam(p, true);
			p = new GlFloatParam((GlFloatParamType*)mAddOn->mCurve, Id(), 1.0);
			if (p) Params().AddParam(p, true);
		}
		return B_OK;
	} else if (msg.what == STAGE_VAL_MSG) {
		int32			n;
		BPoint			pt;
		if (msg.FindInt32(STAGE_STR, &n) != B_OK
				|| msg.FindPoint(VAL_STR, &pt) != B_OK) return B_ERROR;
		uint32			c = 0;
		GlParam*		p;
		for (uint32 k = 0; (p = Params().At(k)) != 0; k++) {
			if (p->ParamType()->Key() == STAGE_KEY) {
				if (int32(c) == n) {
					GlPointWrap		wrap(pt);
					wrap.SetValue(p);
					return B_OK;
				}
				c++;
			}
		}
	} else if (msg.what == CURVE_VAL_MSG) {
		int32			n;
		float			f;
		if (msg.FindInt32(STAGE_STR, &n) != B_OK
				|| msg.FindFloat(VAL_STR, &f) != B_OK) return B_ERROR;
		uint32			c = 0;
		GlParam*		p;
		for (uint32 k = 0; (p = Params().At(k)) != 0; k++) {
			if (p->ParamType()->Key() == CURVE_KEY) {
				if (int32(c) == n) {
					GlFloatWrap		wrap(f);
					wrap.SetValue(p);
					return B_OK;
				}
				c++;
			}
		}

	}
	return B_ERROR;
}
#endif

GlAlgo* GlEnvelope1d::Generate(const gl_generate_args& args) const
{
	return new _GlEnvelope1d(	Params().Float(START_KEY),
								Params().Float(END_KEY),
								Params().Float(CURVE_KEY),
								this);
}

// #pragma mark -

/***************************************************************************
 * GL-ENVELOPE-1D-ADD-ON
 ***************************************************************************/
GlEnvelope1dAddOn::GlEnvelope1dAddOn()
		: inherited(SZI[SZI_arp], GL_ENVELOPE_KEY, SZ(SZ_1D), SZ(SZ_Envelope), 1, 0)
{
	ArpASSERT(GlAlgo1d::RegisterKey(GL_ENVELOPE_KEY));
	mStart		= AddParamType(new GlFloatParamType(START_KEY, SZ(SZ_Start), 0, 1, 0, 0.1f));
	mEnd		= AddParamType(new GlFloatParamType(END_KEY, SZ(SZ_End), 0, 1, 1, 0.1f));
	mStage		= AddParamType(	new GlPoint3dParamType(STAGE_KEY, SZ(SZ_Stage), 0, 0, SZ(SZ_Curve),
								ArpPoint3d(0, 0, 0), ArpPoint3d(1, 1, 100), ArpPoint3d(0.2f, 1, 1), 0.1f));
	mCurve		= AddParamType(new GlFloatParamType(CURVE_KEY, SZ(SZ_Curve),  0, 100, 1, 0.1f));
}

GlNode* GlEnvelope1dAddOn::NewInstance(const BMessage* config) const
{
	return new GlEnvelope1d(this, config);
}

// #pragma mark -

/***************************************************************************
 * _GL-ENVELOPE-1D
 ***************************************************************************/
_GlEnvelope1d::_GlEnvelope1d(float s, float e, float c, const GlNode* node)
		: inherited(GL_ENVELOPE_KEY, (node) ? node->Id() : 0),
		  mStart(s), mEnd(e), mCurve(c), mStages(0), mCount(0)
{
	if (node) MakeStages(node->Params(), true);
}

_GlEnvelope1d::_GlEnvelope1d(const _GlEnvelope1d& o)
		: inherited(o), mStart(o.mStart), mEnd(o.mEnd), mCurve(o.mCurve),
		  mStages(0), mCount(0)
{
	if (o.mStages && o.mCount > 0) {
		if ((mStages = new ArpPoint3d[o.mCount]) != 0) {
			mCount = o.mCount;
			for (uint32 k = 0; k < mCount; k++) mStages[k] = o.mStages[k];
		}
	}
}

_GlEnvelope1d::~_GlEnvelope1d()
{
	FreeStages();
}

GlAlgo* _GlEnvelope1d::Clone() const
{
	return new _GlEnvelope1d(*this);
}

status_t _GlEnvelope1d::Algo(float* line, float* at, int32 size, uint32 flags) const
{
	if (mCount < 1) return AlgoLinear(line, at, size, flags);

	uint32			stage = 0;
	float			v, startX = 0, startY = mStart,
					endX = mStages[stage].x, endY = mStages[stage].y;
	float			pos = 0;
	for (int32 step = 0; step < size; step++) {
		pos =  GL_1D_ENV_STEP(step, size);
		if (at) v = at[step];
		else v = pos;
		/* Change stage if necessary.
		 */
		if (stage < mCount && pos >= mStages[stage].x) {
			startX = endX;
			startY = endY;
			stage++;
			if (stage >= mCount) {
				endX = 1;
				endY = mEnd;
			} else {
				endX = mStages[stage].x;
				endY = mStages[stage].y;
			}
		}
		if (endX <= startX) v = startY;
		else if (stage < mCount && mStages[stage].z != 1) {
			float	f;
			if (endY < startY)	f = 1 - pow( 1 - ((v - startX) / (endX - startX)), mStages[stage].z);
			else				f = pow( (v - startX) / (endX - startX), mStages[stage].z);
			f = startY + (f * (endY - startY));
			v = arp_clip_1(f);
		} else
			v = arp_clip_1(startY + ( ((v - startX) * (endY - startY)) / (endX - startX) ) );

		if (flags&ALGO_HEAD_F) line[step] = v;
		else line[step] *= v;
		ArpASSERT(line[step] >= 0 && line[step] <= 1);
	}
	return B_OK;				
}

uint32 _GlEnvelope1d::properties() const
{
	if (mCount < 1 && mStart == mEnd) return CONSTANT_F;
	return 0;
}

status_t _GlEnvelope1d::AlgoLinear(float* line, float* at, int32 size, uint32 flags) const
{
	float		r = mEnd - mStart;
	for (int32 step = 0; step < size; step++) {
		float	v = mStart;
		if (mStart != mEnd) {
			if (at) v = mStart + (at[step] * r);
			else v = mStart + (GL_1D_ENV_STEP(step, size) * r);
		}
		ArpASSERT(v >= 0 && v <= 1);
		if (flags&ALGO_HEAD_F) line[step] = v;
		else line[step] *= v;
	}
	return B_OK;
}

status_t _GlEnvelope1d::MakeStages(const GlParamList& params, bool replace)
{
	uint32			c = 0;
	const GlParam*	p;
	if (replace) {
		FreeStages();
		for (uint32 sp = 0; (p = params.At(sp)) != 0; sp++) {
			if (p->ParamType()->Key() == STAGE_KEY) c++;
		}

		if (c < 1) return B_OK;
		if ((mStages = new ArpPoint3d[c]) == 0) return B_NO_MEMORY;
		mCount = c;
		c = 0;
	}
	
	float				lastX = 0;
	GlPoint3dWrap		w;
	for (int32 k = 0; params.GetValueNoInit(0, STAGE_KEY, w, k) == B_OK; k++) {
		if (c >= mCount) { ArpASSERT(false); break; }
		mStages[c] = w.v;
		if (mStages[c].x < lastX) mStages[c].x = lastX;
		mStages[c].x = arp_clip_1(mStages[c].x);
		mStages[c].y = arp_clip_1(mStages[c].y);

		lastX = mStages[c].x;		
		c++;
	}
	return B_OK;
}

void _GlEnvelope1d::FreeStages()
{
	delete[] mStages;
	mStages = 0;
	mCount = 0;
}

void _GlEnvelope1d::_print() const
{
	printf("Envelope start %f end %f curve %f (size %ld)", mStart, mEnd, mCurve, mCount);
#if 0
	if (mCount > 0) {
		for (uint32 k = 0; k < mCount; k++) {
			for (uint32 t = 0; t < tabs + 2; t++) printf("\t");
			printf("%ld: (%f, %f) curve %f\n", k, mStages[k].x, mStages[k].y, mStages[k].z);
		}
	}
#endif
}

// #pragma mark -

/***************************************************************************
 * _GL-ENVELOPE-CONTROL
 ***************************************************************************/
class _GlEnvelopeControl : public ArpBitmapView
{
public:
	_GlEnvelopeControl(	BRect frame, const GlRootRef& ref, const GlNode& node,
						_GlEnvelopeView* parent);
	virtual ~_GlEnvelopeControl();

	virtual	void			MouseDown(BPoint where);
	virtual	void			MouseMoved(	BPoint where, uint32 code,
										const BMessage* dragMessage);
	virtual	void			MouseUp(BPoint where);

protected:
	virtual void			DrawOn(BView* view, BRect clip);

private:
	typedef ArpBitmapView	inherited;
	GlRootRef				mRef;
	gl_node_id				mNid;
	GlNode*					mNode;
vector<ArpPoint3d>		mStages;
	BPoint					mRad;
	int32					mMouse;
	bool					mChanges;
	_GlEnvelopeView*		mParent;
		
	status_t				InstallStages();
	int32					PointAt(const BPoint& pt) const;

	status_t				Cache(const GlParamList& params);
};

_GlEnvelopeControl::_GlEnvelopeControl(	BRect frame, const GlRootRef& ref,
										const GlNode& node, _GlEnvelopeView* parent)
		: inherited(frame, "env"), mRef(ref), mNid(node.Id()), mNode(0),
		  mRad(2, 2), mMouse(-1), mChanges(true), mParent(parent)
{
	Cache(node.Params());
}

_GlEnvelopeControl::~_GlEnvelopeControl()
{
	if (mNode) mNode->DecRefs();
}

void _GlEnvelopeControl::MouseDown(BPoint where)
{
	mMouse = PointAt(where);
	if (mMouse < 0) {
		BRect	b(Bounds());
		ArpPoint3d		stage(where.x / b.Width(), where.y / b.Height(), 1);
		mStages.push_back(stage);
		this->Invalidate();
		mMouse = int32(mStages.size()) - 1;
	}
}

void _GlEnvelopeControl::MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage)
{
	int32	buttons = arp_get_mouse_buttons(*this);
	if (buttons&B_PRIMARY_MOUSE_BUTTON && mMouse >= 0) {
		if (mMouse < int32(mStages.size())) {
			BRect		b(Bounds());
			mStages[mMouse].x = arp_clip_1(where.x / b.Width());
			mStages[mMouse].y = arp_clip_1(where.y / b.Height());
			InstallStages();
			if (mParent) mParent->UpdateVisual();
//			this->Invalidate();
		}
	}
}

void _GlEnvelopeControl::MouseUp(BPoint where)
{
	mMouse = -1;
ArpASSERT(false);
//	if (Window()) Window()->PostMessage(GL_PARAM_CHANGED);
}

void _GlEnvelopeControl::DrawOn(BView* view, BRect clip)
{
	inherited::DrawOn(view, clip);
	BRect			b = Bounds();
	float			w = b.Width(), h = b.Height();
	view->SetHighColor(255, 0, 0);
	for (uint32 k = 0; k < mStages.size(); k++) {
		BPoint		loc(mStages[k].x * w, mStages[k].y * h);
		view->FillRect(BRect(loc.x - mRad.x, loc.y - mRad.y, loc.x + mRad.x, loc.y + mRad.y));
	}
}

status_t _GlEnvelopeControl::InstallStages()
{
	GlRootNode*			root = mRef.WriteLock();
	if (!root) return B_ERROR;
	if (!mNode) {
		mNode = root->FindNode(0, mNid);
		if (mNode) mNode->IncRefs();
	}
	if (mNode) {
		int32			size = int32(mStages.size());
		while (mNode->Params().EraseValue(STAGE_KEY, size) == B_OK) ;
		GlPoint3dWrap	w;
		for (int32 k = 0; k < size; k++) {
			w.v = mStages[k];
			w.v.y = 1 - w.v.y;
			mNode->Params().SetValue(STAGE_KEY, w, k);
		}
	}
	
	mRef.WriteUnlock(root);
	return B_OK;
}

int32 _GlEnvelopeControl::PointAt(const BPoint& pt) const
{
	BRect		b = Bounds();
	float		w = b.Width(), h = b.Height();

	for (uint32 k = 0; k < mStages.size(); k++) {
		BPoint		loc(mStages[k].x * w, mStages[k].y * h);
		if (pt.x >= loc.x - mRad.x && pt.x <= loc.x + mRad.x
				&& pt.y >= loc.y - mRad.y && pt.y <= loc.y + mRad.y)
			return k; 
	}
	return -1;
}

status_t _GlEnvelopeControl::Cache(const GlParamList& params)
{
	mStages.resize(0);
	GlPoint3dWrap		w;
	for (int32 k = 0; params.GetValueNoInit(0, STAGE_KEY, w, k) == B_OK; k++) {
		w.v.y = 1 - w.v.y;
		mStages.push_back(w.v);
	}
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * _GL-ENVELOPE-VIEW
 ***************************************************************************/
class _GlEnvParamFilter : public GlAddParamFilter
{
public:
	_GlEnvParamFilter(gl_node_id nid) : mNid(nid)	{ }

	/* Only let through the start, end, curve -- no stages.
	 */
	virtual bool		AllowKey(gl_param_key key)
	{
		ArpVALIDATE(key.nid == mNid, return true);		
		if (key.key == START_KEY) return true;
		if (key.key == END_KEY) return true;
		if (key.key == CURVE_KEY) return true;
		return false;
	}

private:
	gl_node_id			mNid;
};

_GlEnvelopeView::_GlEnvelopeView(	const gl_new_view_params& params, gl_chain_id cid,
									GlStrainedParamList& list, const GlNode& node)
		: inherited(params, cid, node.Id()), mBmView(0)
{
//debugger("dsds");
	float					sx = float(Prefs().GetInt32(ARP_PADX)),
							sy = float(Prefs().GetInt32(ARP_PADY));
	float					fh = ViewFontHeight();
//	float					div = get_divider(list, this);
	mRect.Set(sx, sy, 152, sy + fh);
	AddLabel(mRect, "label", node.AddOn()->Label());

	mRect.Set(mRect.left, mRect.bottom + sy, mRect.right, mRect.bottom + sy + 50);

	mBmView = new _GlEnvelopeControl(mRect, params.ref, node, this);
	if (mBmView) AddView(mBmView);

	_GlEnvParamFilter*		filter = new _GlEnvParamFilter(node.Id());
	AddParamControls(list, 0, filter);
	delete filter;
}

void _GlEnvelopeView::AttachedToWindow()
{
	inherited::AttachedToWindow();
	SetHighColor(Prefs().GetColor(ARP_BG_C));
	/* This is a bit of a hack -- I can't get the preview bitmap
	 * in the constructor because the preview methods on the node are
	 * non-const, but I can't get a write lock in the constructor
	 * because it's being called from a read lock (which is necessary
	 * because class construction is a result of asking a node to
	 * construct the class).
	 */
	UpdateVisual();
}

/* FIX:  Hopefully this method has been obsoleted, because it's not
 * being called anymore.  If not, it's possible it could be handled
 * by installing a ControlTarget, although that might be a little
 * sketchy.
 */
status_t _GlEnvelopeView::ParamChanged(	gl_param_key key, uint32 code,
										const GlParamWrap& wrap)
{
ArpASSERT(false);
	status_t		err = B_OK;
//	status_t		err = inherited::ParamChanged(key, code, wrap);
	if (err != B_OK) return err;
	/* FIX: Probably, even if they're different I still want to update
	 * the preview -- the assumption is someone I depend on changed.
	 */
	ArpASSERT(key.nid == mNid);
	return UpdateVisual();
}

status_t _GlEnvelopeView::UpdateVisual()
{
	if (!mBmView) return B_NO_MEMORY;
	ArpBitmap*			bm = 0;
	GlRootNode*			r = mRef.WriteLock();
	if (r) {
		GlNode*			n = r->FindNode(mCid, mNid);
		ArpASSERT(n);
		if (n) {
			/* To clear out any cached preview, in case a previous
			 * node this one depends on changed (I wouldn't know about that).
			 */
			BRect		f(mBmView->Bounds());
//			n->ParamChanged(0, 0);
// FIX			n->Preview(f.Width(), f.Height(), &bm);
		}
	}
	mRef.WriteUnlock(r);
	if (bm) mBmView->TakeBitmap(bm);
	return B_OK;
}

status_t _GlEnvelopeView::UpdateControls()
{
	inherited::UpdateControls();
	UpdateVisual();
	return B_OK;
}

// #pragma mark -

/***************************************************************************
 * Misc functions
 ***************************************************************************/
GlAlgo1d*	gl_new_linear_envelope(float start, float end)
{
	return new _GlEnvelope1d(start, end, 1, 0);
}
