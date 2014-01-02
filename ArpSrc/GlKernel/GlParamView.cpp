#include <stdio.h>
#include <ArpCore/StlVector.h>
#include <ArpInterface/ArpPrefs.h>
#include <ArpSupport/ArpUniversalStringMachine.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlControlTarget.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlNodeVisual.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlRecorder.h>
#include <GlPublic/GlRootNode.h>
#include <GlPublic/GlStrainedParamList.h>
#include <GlKernel/GlDefs.h>
#include <GlKernel/GlParamViewAux.h>
#include <GlKernel/GlRecorderHolder.h>

#define WM_USER 'WMUR'

static const uint32		WHAT_INC		= 2;
static const char*		FN_CTRL			= NULL;
static const char*		FLOAT_CTRL		= NULL;
static const char*		FONT_CTRL		= NULL;
static const char*		INT_CTRL		= NULL;
static const char*		MENU_CTRL		= NULL;
static const char*		TEXT_CTRL		= NULL;
/*
static const char*		FN_CTRL			= "ParamFn";
static const char*		FLOAT_CTRL		= "ParamFloat";
static const char*		FONT_CTRL		= "ParamFont";
static const char*		INT_CTRL		= "ParamInt";
static const char*		MENU_CTRL		= "ParamMenu";
static const char*		TEXT_CTRL		= "ParamText";
*/
class _ParamVisualEntry
{
public:
	GlRecorderBitmapView*	v;
	int32					index;
	_ParamVisualEntry(GlRecorderBitmapView* inV, int32 inI) : v(inV), index(inI)	{ }
	_ParamVisualEntry(const _ParamVisualEntry& o) : v(o.v), index(o.index)	{ }
	_ParamVisualEntry&	operator=(const _ParamVisualEntry& o)	{ v = o.v; index = o.index; return *this; }
};

class _ParamViewData
{
public:
	vector<_ParamViewEntry*>	entries;
	vector<_ParamVisualEntry>	visuals;
	
	_ParamViewData()			{ }

	~_ParamViewData()
	{
		for (uint32 k = 0; k < entries.size(); k++) delete entries[k];
		entries.resize(0);
	}

	status_t RightEdgeForColumn(int32 column, float* outEdge) const
	{
		ArpASSERT(outEdge);
		status_t		err = B_ERROR;
		*outEdge = 0;
		for (uint32 k = 0; k < entries.size(); k++) {
			if (entries[k]->mColumn == column) {
				float	edge = entries[k]->RightEdge();
				if (edge > *outEdge) *outEdge = edge;
				err = B_OK;
			}
		}
		return err;
	}

	void SetLeftEdgeForColumn(int32 column, float left)
	{
		for (uint32 k = 0; k < entries.size(); k++) {
			if (entries[k]->mColumn == column)
				entries[k]->SetLeftEdge(left);
		}
	}

	void UpdateVisuals(GlNodeVisual* visual)
	{
		ArpVALIDATE(visual, return);
		for (uint32 k = 0; k < visuals.size(); k++) {
			ArpBitmapView*		v = visuals[k].v;
			if (v) {
				ArpBitmap*		bm = 0;
				BRect			f(v->Bounds());
//	n->ParamChanged(0, 0);
				visual->Visual(int32(f.Width()), int32(f.Height()), &bm, visuals[k].index);
				if (bm) v->TakeBitmap(bm);
			}
		}
	}

};

static bool _affect_label(const GlParam* p, const GlParamType* pt)
{
	if (!pt && p) pt = p->ParamType();
	if (!pt) return true;
	if (pt->Type() == GL_BOOL_TYPE) return false;
	return true;
}

static float get_divider(const GlStrainedParamList& params, BView* view)
{
	float					div = 0;
	/* Get the label width for all controls, and hence the divider.  The
	 * label is in the param if it has one, or the param type.  Certain controls
	 * won't affect the divider.
	 */
	GlStrainedParam			p;
	uint32					size = params.Size();
	for (uint32 k = 0; k < size; k++) {
		if (params.At(k, p) == B_OK) {
			if (p.label && _affect_label(p.p, p.pt)) {
				float		lw = arp_get_string_width(view, p.label);
				if (lw > div) div = lw;
			}
		}
	}
	return div;
}

/***************************************************************************
 * GL-PARAM-VIEW
 ***************************************************************************/
GlParamView::GlParamView(	const gl_new_view_params& params,
							gl_chain_id cid, gl_node_id nid)
		: inherited(params.frame), mRect(0, 0, 152, 0), mRef(params.ref),
		  mCid(cid), mNid(nid), mChannel(params.channel),
		  mWhat(WM_USER + 1), mData(0), mVisual(0),
		  mRow(-1), mRowNid(0), mRowF(START_ROW), mRowColumn(-1),
		  mTarget(0)
{
	mData = new _ParamViewData();
}

GlParamView::~GlParamView()
{
	delete mData;
	delete mVisual;
	delete mTarget;
}

void GlParamView::AttachedToWindow()
{
	inherited::AttachedToWindow();
/* FIX: The superclass is doing something weird, setting the color to
 * some scale of the BG_C.  Why?
 */
//	SetHighColor(Prefs().GetColor(ARP_BG_C));
	/* This is a bit of a hack -- I can't get the preview bitmap
	 * in the constructor because the preview methods on the node are
	 * non-const, but I can't get a write lock in the constructor
	 * because it's being called from a read lock (which is necessary
	 * because class construction is a result of asking a node to
	 * construct the class).
	 */
	if (mData && mVisual) mData->UpdateVisuals(mVisual);
	mRef.AddObserver(GlNotifier::NODE_CODE, BMessenger(this));

	/* If I've got a control channel, then populate it with
	 * any control info it wants.
	 */
	if (mChannel) mChannel->Populate(*this);
}

void GlParamView::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
	mRef.RemoveObserver(GlNotifier::NODE_CODE, BMessenger(this));
}

void GlParamView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
//		case GlNotifier::NODE_CODE:
		case GlNotifier::CHANGE_MSG:
			/* Unfortuntely the key info is not available in this
			 * new style of doing things -- might even remove that
			 * arg from the visual.  No one was using it anyway.
			 */
			if (mVisual) mVisual->ParamChanged(gl_param_key(0, 0, 0));
			if (mVisual && mData) mData->UpdateVisuals(mVisual);
			{
				void*		sender;
				if (msg->FindPointer(GL_SENDER_STR, &sender) != B_OK) sender = 0;
				if (sender != (void*)this) UpdateControls();
//				if (sender != (void*)this) printf("UPDATE CONTROLS\n");
			}
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}

GlRecorderBitmapView* GlParamView::RecorderView(const char* name)
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->visuals.size(); k++) {
		GlRecorderBitmapView*	v = mData->visuals[k].v;
		if (v) {
			if (!name) return v;
			const char*			n = v->Name();
			if (n && strcmp(name, n) == 0) return v;
		}
	}
	return 0;
}

status_t GlParamView::AddParamControls(	const GlStrainedParamList& params,
										BRect* f, GlAddParamFilter* filter)
{
	if (f) mRect = *f;
	float				div = get_divider(params, this);

	GlStrainedParam		p;
	uint32				size = params.Size();
	for (uint32 k = 0; k < size; k++) {
		if (params.At(k, p) == B_OK) {
			ArpASSERT(p.nid && p.pt && p.label);
			uint32		flags = p.pt->StateFlags();
			if (!(flags&GL_ROOT_INFO_F)) {
				if (!filter || filter->Allow(p.nid, p.p, p.pt, p.index))
					AddParamControl(p, div);
			}
		}
	}
	return AddParamControlsFinished();
}

ArpBitmapView* GlParamView::AddVisualView(	const BRect& frame, const char* name,
											GlNodeVisual* visual, int32 visualIndex)
{
	ArpVALIDATE(mData, return 0);
	GlRecorderBitmapView*	v = new GlRecorderBitmapView(frame, name);
	if (!v) return v;
	if (AddView(v) != B_OK) {
		delete v;
		return 0;
	}
	if (visual) {
		delete mVisual;
		mVisual = visual;
	}
	mData->visuals.push_back(_ParamVisualEntry(v, visualIndex));
	return v;
}

status_t GlParamView::ControlMessage(uint32 what)
{
	ArpVALIDATE(mChannel, return B_ERROR);
	if (!mData) return B_ERROR;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k]) {
			uint32			code = mData->entries[k]->MatchesWhat(what);
			if (code != GL_NO_PARAM_CHANGE) {
				mData->entries[k]->Callback(mChannel, code);
				return B_OK;
			}
		}
	}
	return B_ERROR;
}

status_t GlParamView::UpdateControls()
{
	if (!mData) return B_ERROR;
	const GlRootNode*		root = mRef.ReadLock();
	if (!root) return B_ERROR;

	for (uint32 k = 0; k < mData->entries.size(); k++) {
		_ParamViewEntry*	e = mData->entries[k];
		const GlNode*		node;
		if (e && (node = root->FindNode(0, e->key.nid)) != 0)
			e->UpdateControl(*node);
	}		
		
	mRef.ReadUnlock(root);

	if (mVisual) mData->UpdateVisuals(mVisual);

	return B_OK;
}

status_t GlParamView::ParamControlKeyAt(uint32 index, gl_param_key* outKey) const
{
	if (!mData) return B_ERROR;
	if (index >= mData->entries.size()) return B_ERROR;
	if (outKey) *outKey = mData->entries[index]->key;
	return B_OK;
}

status_t GlParamView::EnableParamControl(gl_param_key key, bool enable)
{
	if (!mData) return B_ERROR;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k] && key == mData->entries[k]->key) {
			return mData->entries[k]->EnableControl(enable);
		}
	}
	return B_ERROR;
}

status_t GlParamView::SetParamControl(gl_param_key key, GlParamWrap& wrap)
{
	if (!mData) return B_ERROR;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k] && key == mData->entries[k]->key) {
			return mData->entries[k]->SetControl(wrap);
		}
	}
	return B_ERROR;
}

status_t GlParamView::AddParamControl(GlStrainedParam& p, float div)
{
	ArpVALIDATE(p.nid && p.pt && p.label, return B_ERROR);
	if (!mData) return B_ERROR;

//if (param) { printf("Add Param "); param->Print(); }
//else printf("Add Param Type %s:%s\n", pt->Name(), pt->Label());

	/* If the pt has no row or is the first of its kind, start a
	 * new row, otherwise stack onto the current.  We can only stack
	 * if we're on the same nid as the previous row.
	 */
	if (p.pt->Row() >= 0 && mRow == p.pt->Row() && mRowNid == p.nid) {
		mRowF = STACK_ROW;
		mRowColumn++;
	} else {
		mRowF = START_ROW;
		mRowLeft = float(Prefs().GetInt32(ARP_PADX));
		if (p.pt->Row() >= 0) mRowColumn = 0;
		else mRowColumn = -1;
	}
	mRow = p.pt->Row();
	mRowNid = p.nid;
	
	gl_param_key			key(p.nid, p.pt->Key(), p.index);
	if (p.pt->Type() == GL_BOOL_TYPE)
		return AddBoolParam(p, key, div);
	else if (p.pt->Type() == GL_COLOR_TYPE)
		return AddColorParam(p, key, div);
	else if (p.pt->Type() == GL_FILENAME_TYPE)
		return AddFileNameParam(p, key, div);
	else if (p.pt->Type() == GL_FLOAT_TYPE)
		return AddFloatParam(p, key, div);
	else if (p.pt->Type() == GL_FONT_TYPE)
		return AddFontParam(p, key, div);
	else if (p.pt->Type() == GL_INT32_TYPE)
		return AddInt32Param(p, key, div);
	else if (p.pt->Type() == GL_MENU_TYPE)
		return AddMenuParam(p, key, div);
	else if (p.pt->Type() == GL_POINT_TYPE)
		return AddPointParam(p, key, div);
	else if (p.pt->Type() == GL_POINT_3D_TYPE)
		return AddPoint3dParam(p, key, div);
	else if (p.pt->Type() == GL_REL_ABS_TYPE)
		return AddRelAbsParam(p, key, div);
	else if (p.pt->Type() == GL_TEXT_TYPE)
		return AddTextParam(p, key, div);
	ArpASSERT(false);
	return B_ERROR;
}

status_t GlParamView::AddParamControlsFinished()
{
	ArpVALIDATE(mData, return B_ERROR);
	/* Line up the columns.
	 */
	float		right;
	for (int32 column = 0; mData->RightEdgeForColumn(column, &right) == B_OK; column++)
		mData->SetLeftEdgeForColumn(column + 1, right + Prefs().GetInt32(ARP_PADX));

	if (mChannel) {
		if (!mTarget) mTarget = new _ParamViewTarget(mRef, this);
		if (mTarget) mChannel->Add(mTarget);
		mTarget = 0;
	}
	delete mTarget;
	mTarget = 0;
	
	return B_OK;
}

float GlParamView::FloatW(const BString16* label, const GlFloatParamType* pt)
{
	float		w = 0;
	if (label) w += StringWidth(label->String()) + 5;
	// FIX: Figure width based on min max and steps
	w += 60;
	return w;
}

status_t GlParamView::AddBoolParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlBoolParamType*		bpt = (const GlBoolParamType*)p.pt;
	if (!bpt) return B_ERROR;
	float						w = CheckBoxW(p.label);
	BRect						f(mRect);
	if (mRowF == START_ROW)	{
		ShiftCheckBoxDown(f);
		mRect = f;
	} else {
		f.left = (mRowLeft);
		f.right = (mRowLeft + w);
		if (f.bottom > mRect.bottom) mRect.bottom = (f.bottom);
	}
	f.right = f.left + w;
	GlBoolWrap					wrap(bpt->Init());
	if (p.p) wrap.GetValue(p.p);
	mWhat += WHAT_INC;
	// FIX: Ya know, maintaining unique names for the views is
	// probably a total waste, they never get looked up by name
	// anyway.  So I'll do the first 2 like this, but the others
	// get generic names for now.
	ArpUniversalStringMachine	usm;
	BCheckBox*					ctrl = AddCheckBox(f, usm.String(p.label), p.label, mWhat, wrap.v);
	if (!ctrl) return B_NO_MEMORY;
	if (bpt->Row() >= 0) {
		mRowLeft = f.right + Prefs().GetInt32(ARP_PADX);
	}
	_ParamViewEntry*			entry = new _ParamViewBoolEntry(ctrl, key, mWhat);
	if (!entry) return B_ERROR;
	entry->mColumn = mRowColumn;
	mData->entries.push_back(entry);
	return B_OK;
}

status_t GlParamView::AddColorParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlColorParamType*		cpt = (const GlColorParamType*)p.pt;
	if (!cpt) return B_ERROR;
	ShiftIntDown(mRect);
	GlColorWrap					wrap(cpt->Init());
	if (p.p) wrap.GetValue(p.p);
	mWhat += WHAT_INC;
	ArpUniversalStringMachine	usm;
	ArpColourControl*			ctrl = AddColourControl(mRect, usm.String(p.label), p.label, wrap.v, 0, mWhat, div);
	if (!ctrl) return B_NO_MEMORY;
	_ParamViewEntry*			entry = new _ParamViewColorEntry(ctrl, key, mWhat);
	if (!entry) return B_ERROR;
	entry->mColumn = mRowColumn;
	mData->entries.push_back(entry);
	return B_OK;
}

status_t GlParamView::AddFileNameParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlFileNameParamType*	fpt = (const GlFileNameParamType*)p.pt;
	if (!fpt) return B_ERROR;
	ShiftTextDown(mRect);
	GlTextWrap			twrap;
	if (p.p) twrap.GetValue(p.p);
	mWhat += WHAT_INC;
//	ArpFileNameControl*	ctrl = AddFileNameControl(mRect, p.label, twrap.v, mWhat);
	ArpFileNameControl*	ctrl = AddFileNameControl(mRect, FN_CTRL, twrap.v, mWhat);
	if (!ctrl) return B_NO_MEMORY;
	_ParamViewEntry*	e = new _ParamViewFileNameEntry(ctrl, key, mWhat);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddFloatParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlFloatParamType*	fpt = (const GlFloatParamType*)p.pt;
	if (!fpt) return B_ERROR;
	const GlFloatParam*		fp = (const GlFloatParam*)p.p;
//	ShiftIntDown(mRect);
#if 1
	float				w = FloatW(p.label, fpt);
	BRect				f(mRect);
	ShiftIntDown(f);
	if (mRowF == START_ROW)	{
		mRect = f;
		f.right = f.left + w;
	} else {
		f.left = (mRowLeft);
		f.right = (mRowLeft + w);
		float			h = f.Height();
		f.top = (mRect.top);
		f.bottom = (f.top + h);
		if (f.bottom > mRect.bottom) mRect.bottom = (f.bottom);
	}
#endif

	GlFloatWrap			fwrap(fpt->Init());
	if (p.p) fwrap.GetValue(p.p);
	mWhat += WHAT_INC;
	float				min = fpt->Min(), max = fpt->Max();
	if (fp) fp->GetRange(&min, &max);	
//printf("Add float (%f, %f - %f, %f) div %f\n", mRect.L(), mRect.top, mRect.right, mRect.bottom, div);
//	ArpFloatControl*	ctrl = AddFloatControl(	f, p.label, p.label, mWhat, mWhat + 1, min,
	ArpFloatControl*	ctrl = AddFloatControl(	f, FLOAT_CTRL, p.label, mWhat, mWhat + 1, min,
												max, fwrap.v, fpt->Steps(), div);
	if (!ctrl) return B_NO_MEMORY;
	if (fpt->Row() >= 0) mRowLeft = f.right + Prefs().GetInt32(ARP_PADX);
	_ParamViewEntry*	e = new _ParamViewFloatEntry(ctrl, key, mWhat, mWhat + 1);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	AddMidiParam(p.midi, key, p.p, p.pt);
	return B_OK;
}

status_t GlParamView::AddFontParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlFontParamType*	fpt = (const GlFontParamType*)p.pt;
	if (!fpt) return B_ERROR;
	ShiftMenuDown(mRect);

	GlFontWrap			fwrap(fpt->Init());
	if (p.p) fwrap.GetValue(p.p);
	mWhat += WHAT_INC;
//	ArpFontControl*		ctrl = AddFontControl(mRect, p.label, p.label, mWhat, fwrap.v, div);
	ArpFontControl*		ctrl = AddFontControl(mRect, FONT_CTRL, p.label, mWhat, fwrap.v, div);
	if (!ctrl) return B_NO_MEMORY;
//	if (pt->Row() >= 0) mRowLeft = f.right + Prefs().GetInt32(ARP_PADX);
	_ParamViewEntry*	e = new _ParamViewFontEntry(ctrl, key, mWhat);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddInt32Param(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlInt32ParamType*	ipt = (const GlInt32ParamType*)p.pt;
	if (!ipt) return B_ERROR;
	ShiftIntDown(mRect);
	GlInt32Wrap			wrap(ipt->Init());
	if (p.p) wrap.GetValue(p.p);
	mWhat += WHAT_INC;
//printf("Add int32 (%f, %f - %f, %f) div %f\n", mRect.L(), mRect.top, mRect.right, mRect.bottom, div);
//	ArpIntControl*		ctrl = AddIntControl(	mRect, p.label, p.label, mWhat, ipt->Min(),
	ArpIntControl*		ctrl = AddIntControl(	mRect, INT_CTRL, p.label, mWhat, ipt->Min(),
												ipt->Max(), wrap.v, div);
	if (!ctrl) return B_NO_MEMORY;
	_ParamViewEntry*	e = new _ParamViewInt32Entry(ctrl, key, mWhat);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddMenuParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlMenuParamType*	mpt = (const GlMenuParamType*)p.pt;
	if (!mpt) return B_ERROR;

	float				w = MenuW(p.label, mpt->Items());
	BRect				f(mRect);
	ShiftMenuDown(f);
	if (mRowF == START_ROW)	{
		mRect = f;
		f.right = f.left + w;
	} else {
		f.left = (mRowLeft);
		f.right = (mRowLeft + w);
		float			h = f.Height();
		f.top = (mRect.top);
		f.bottom = (f.top + h);
		if (f.bottom > mRect.bottom) mRect.bottom = (f.bottom);
	}

	GlInt32Wrap			wrap(mpt->Init());
	if (p.p) wrap.GetValue(p.p);
	mWhat += WHAT_INC;
//	ArpMenuControl*		ctrl = AddMenuControl(f, p.label, p.label, mWhat, mpt->Items(), div);
	ArpMenuControl*		ctrl = AddMenuControl(f, MENU_CTRL, p.label, mWhat, mpt->Items(), div);
	if (!ctrl) return B_NO_MEMORY;
	ctrl->SetCurrentIndex(wrap.v);
	if (mpt->Row() >= 0) mRowLeft = f.right + Prefs().GetInt32(ARP_PADX);
	_ParamViewEntry*	e = new _ParamViewMenuEntry(ctrl, key, mWhat);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddPointParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlPointParamType*	pt = (const GlPointParamType*)p.pt;
	if (!pt) return B_ERROR;
	ShiftIntDown(mRect);
	GlPointWrap			wrap(pt->Init());
	if (p.p) wrap.GetValue(p.p);
	uint32				xChanging =	++mWhat;
	uint32				xChanged =	++mWhat;
	uint32				yChanging =	++mWhat;
	uint32				yChanged =	++mWhat;
	float				half = ((mRect.Width()) / 2) - 2;
	BRect				xRect(mRect);
	xRect.right = xRect.left + half;
//	ArpFloatControl*	xCtrl = AddFloatControl(xRect, p.label, p.label, xChanging, xChanged, pt->Min().x,
	ArpFloatControl*	xCtrl = AddFloatControl(xRect, FLOAT_CTRL, p.label, xChanging, xChanged, pt->Min().x,
												pt->Max().x, wrap.v.x, pt->Steps(), div);
	if (!xCtrl) return B_NO_MEMORY;
	BRect				yRect(mRect);
	yRect.left = yRect.left + half + 4;
//	ArpFloatControl*	yCtrl = AddFloatControl(yRect, pt->YLabel(), pt->YLabel(), yChanging,
	ArpFloatControl*	yCtrl = AddFloatControl(yRect, FLOAT_CTRL, pt->YLabel(), yChanging,
												yChanged, pt->Min().y, pt->Max().y, wrap.v.y,
												pt->Steps(), div);
	if (!yCtrl) {
		delete xCtrl;
		return B_NO_MEMORY;
	}
	
	_ParamViewEntry*	e = new _ParamViewPointEntry(	xCtrl, xChanging, xChanged,
														yCtrl, yChanging, yChanged, key);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddPoint3dParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlPoint3dParamType*	pt = (const GlPoint3dParamType*)p.pt;
	if (!pt) return B_ERROR;
	ShiftIntDown(mRect);
	GlPoint3dWrap		wrap(pt->Init());
	if (p.p) wrap.GetValue(p.p);
	uint32				xChanging =	++mWhat;
	uint32				xChanged =	++mWhat;
	uint32				yChanging =	++mWhat;
	uint32				yChanged =	++mWhat;
	uint32				zChanging =	++mWhat;
	uint32				zChanged =	++mWhat;
	float				third = ((mRect.Width()) / 3) - 2;
	BRect				xRect(mRect);
	xRect.right = xRect.left + third;
//	ArpFloatControl*	xCtrl = AddFloatControl(xRect, p.label, p.label, xChanging, xChanged, pt->Min().x,
	ArpFloatControl*	xCtrl = AddFloatControl(xRect, FLOAT_CTRL, p.label, xChanging, xChanged, pt->Min().x,
												pt->Max().x, wrap.v.x, pt->Steps(), div);
	if (!xCtrl) return B_NO_MEMORY;
	BRect				yRect(mRect);
	yRect.left = yRect.left + third + 4;
//	ArpFloatControl*	yCtrl = AddFloatControl(yRect, pt->YLabel(), pt->YLabel(), yChanging,
	ArpFloatControl*	yCtrl = AddFloatControl(yRect, FLOAT_CTRL, pt->YLabel(), yChanging,
												yChanged, pt->Min().y, pt->Max().y, wrap.v.y,
												pt->Steps(), div);
	if (!yCtrl) {
		delete xCtrl;
		return B_NO_MEMORY;
	}
	BRect				zRect(mRect);
	zRect.left = zRect.left + third + 4;
//	ArpFloatControl*	zCtrl = AddFloatControl(zRect, pt->ZLabel(), pt->ZLabel(), zChanging,
	ArpFloatControl*	zCtrl = AddFloatControl(zRect, FLOAT_CTRL, pt->ZLabel(), zChanging,
												zChanged, pt->Min().z, pt->Max().z, wrap.v.z,
												pt->Steps(), div);
	if (!zCtrl) {
		delete xCtrl;
		delete zCtrl;
		return B_NO_MEMORY;
	}
	
	_ParamViewEntry*	e = new _ParamViewPoint3dEntry(	xCtrl, xChanging, xChanged,
														yCtrl, yChanging, yChanged,
														zCtrl, zChanging, zChanged, key);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddRelAbsParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlRelAbsParamType*	pt = (const GlRelAbsParamType*)p.pt;
	if (!pt) return B_ERROR;
	ShiftIntDown(mRect);
	GlRelAbsWrap		wrap(pt->Init());
	if (p.p) wrap.GetValue(p.p);
	uint32				relChanging =	++mWhat;
	uint32				relChanged =	++mWhat;
	uint32				absChanging =	++mWhat;
	uint32				absChanged =	++mWhat;
	float				half = ((mRect.Width()) / 2) - 2;
	BRect				xRect(mRect);
	xRect.right = xRect.left + half;
//	ArpFloatControl*	xCtrl = AddFloatControl(xRect, p.label, p.label, relChanging, relChanged, pt->Min().rel,
	ArpFloatControl*	xCtrl = AddFloatControl(xRect, FLOAT_CTRL, p.label, relChanging, relChanged, pt->Min().rel,
												pt->Max().rel, wrap.v.rel, pt->Steps(), div);
	if (!xCtrl) return B_NO_MEMORY;
	BRect				yRect(mRect);
	yRect.left = yRect.left + half + 4;
//	ArpIntControl*		yCtrl = AddIntControl(	yRect, p.label, 0, absChanged, pt->Min().abs,
	ArpIntControl*		yCtrl = AddIntControl(	yRect, INT_CTRL, 0, absChanged, pt->Min().abs,
												pt->Max().abs, wrap.v.abs, div);
	if (!yCtrl) {
		delete xCtrl;
		return B_NO_MEMORY;
	}
	
	_ParamViewEntry*	e = new _ParamViewRelAbsEntry(	xCtrl, relChanging, relChanged,
														yCtrl, absChanging, absChanged, key);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlParamView::AddTextParam(GlStrainedParam& p, gl_param_key key, float div)
{
	const GlTextParamType*	tpt = (const GlTextParamType*)p.pt;
	if (!tpt) return B_ERROR;
	ShiftTextDown(mRect);
	GlTextWrap			wrap;
	if (p.p) wrap.GetValue(p.p);
	mWhat += WHAT_INC;
//	BTextControl*		ctrl = AddTextControl(mRect, p.label, p.label, mWhat, mWhat, wrap.v, div);
	BTextControl*		ctrl = AddTextControl(mRect, TEXT_CTRL, p.label, mWhat, mWhat, wrap.v, div);
	if (!ctrl) return B_NO_MEMORY;
	_ParamViewEntry*	e = new _ParamViewTextEntry(ctrl, key, mWhat);
	if (!e) return B_ERROR;
	e->mColumn = mRowColumn;
	mData->entries.push_back(e);
	return B_OK;
}

void GlParamView::AddMidiParam(	int32 midi, gl_param_key key, const GlParam* p,
								const GlParamType* pt)
{
	ArpVALIDATE(p || pt, return);
	if (midi < GL_MIDI_A || midi > GL_MIDI_Z) return;
	
	if (!mTarget) mTarget = new _ParamViewTarget(mRef, this);
	if (!mTarget) return;
	if (p) mTarget->AddMidiParam(midi, key, p);
	else if (pt) mTarget->AddMidiParam(midi, key, pt);
}

// #pragma mark -

/***************************************************************************
 * GL-ADD-PARAM-FILTER
 ***************************************************************************/
bool GlAddParamFilter::Allow(	gl_node_id nid, const GlParam* p,
								const GlParamType* pt, int32 index)
{
	gl_param_key		key(nid, 0, index);
	if (pt) key.key = pt->Key();
	else if (p && p->ParamType()) key.key = p->ParamType()->Key();
	return AllowKey(key);
}

bool GlAddParamFilter::AllowKey(gl_param_key key)
{
	return true;
}
