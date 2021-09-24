#include <ArpCore/StlVector.h>
#include <GlPublic/GlMidiEvent.h>
#include <GlPublic/GlStrainedParamList.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
// All for setting midi values.  Bleh!
#include <GlPublic/GlAlgo1d.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlRootNode.h>
#include <GlPublic/GlRootRef.h>

class _StrainedParamListEntry
{
public:
	gl_node_id				nid;
	const GlParam*			param;
	int32					index;
	const GlParamType*		paramType;
	int32					control, midi;
		
	_StrainedParamListEntry(gl_node_id n, const GlParam* p, int32 i32,
							const GlParamType* pt, const BString16* label,
							int32 c, int32 m)
			: nid(n), param(p), index(i32),
			  paramType(pt), control(c), midi(m)
	{
		if (label) mLabel = *label;
	}

	_StrainedParamListEntry(const _StrainedParamListEntry& o)
			: nid(o.nid), param(o.param), index(o.index), paramType(o.paramType),
			  control(o.control), midi(o.midi), mLabel(o.mLabel)
	{
	}

	~_StrainedParamListEntry()
	{
	}

	const BString16*		Label(uint32 flags) const
	{
		if (mLabel.Length() > 0) return &mLabel;
		if (flags&GlStrainedParamList::NO_DEFAULT_LABEL_F) return 0;
		if (paramType) return paramType->Label();
		if (param && param->ParamType()) return param->ParamType()->Label();
		return 0;
	}

private:
	BString16				mLabel;
};

class _StrainedParamListData
{
public:
	std::vector<_StrainedParamListEntry*>	entries;
	
	_StrainedParamListData()		{ }
	_StrainedParamListData(const _StrainedParamListData& o)
	{
		for (uint32 k = 0; k < o.entries.size(); k++) {
			if (o.entries[k]) {
				_StrainedParamListEntry*		e = new _StrainedParamListEntry(*(o.entries[k]));
				if (e) entries.push_back(e);
			}
		}
	}

	~_StrainedParamListData()
	{
		for (uint32 k = 0; k < entries.size(); k++) delete entries[k];
	}


	const _StrainedParamListEntry*	FindParam(	gl_node_id nid, const GlParam* p,
												int32 index, const GlParamType* pt) const
	{
		for (uint32 k = 0; k < entries.size(); k++) {
			const _StrainedParamListEntry*	e = entries[k];
			if (e) {
				if (nid == e->nid && p == e->param && index == e->index && pt == e->paramType)
					return e;
			}
		}
		return 0;
	}
};

/***************************************************************************
 * GL-STRAINED-PARAM-LIST
 ****************************************************************************/
GlStrainedParamList::GlStrainedParamList(uint32 flags)
		: mData(new _StrainedParamListData()), mFlags(flags)
{
}

GlStrainedParamList::GlStrainedParamList(const GlStrainedParamList& o)
		: mData(0), mFlags(o.mFlags)
{
	if (o.mData) mData = new _StrainedParamListData(*(o.mData));
}

GlStrainedParamList::~GlStrainedParamList()
{
	delete mData;
}

uint32 GlStrainedParamList::Size() const
{
	if (!mData) return 0;
	return uint32(mData->entries.size());
}

void GlStrainedParamList::SetFlags(uint32 flags)
{
	mFlags = flags;
}

status_t GlStrainedParamList::Add(	gl_node_id nid, const GlParam* param, int32 index,
									const BString16* label, int32 control, int32 midi)
{
	if (!mData) return B_NO_MEMORY;
	ArpVALIDATE(param, return B_ERROR);
//printf("add nid %p param %p index %ld\n", nid, param, index);
//if (mData->FindParam(nid, param, index, 0) != 0) debugger("Sdsd");
	ArpASSERT(mData->FindParam(nid, param, index, 0) == 0);
	_StrainedParamListEntry*	e = new _StrainedParamListEntry(nid, param, index, 0, label, control, midi);
	if (!e) return B_NO_MEMORY;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlStrainedParamList::Add(	gl_node_id nid, const GlParamType* paramType,
									const BString16* label, int32 control, int32 midi)
{
	if (!mData) return B_NO_MEMORY;
	ArpVALIDATE(paramType, return B_ERROR);
//printf("add nid %p paramType %p\n", nid, paramType);
//if (mData->FindParam(nid, 0, 0, paramType) != 0) debugger("Sdsd");
	ArpASSERT(mData->FindParam(nid, 0, 0, paramType) == 0);
	_StrainedParamListEntry*	e = new _StrainedParamListEntry(nid, 0, 0, paramType, label, control, midi);
	if (!e) return B_NO_MEMORY;
	mData->entries.push_back(e);
	return B_OK;
}

status_t GlStrainedParamList::At(uint32 index, GlStrainedParam& param) const
{
	if (!mData) return B_NO_MEMORY;
	if (index >= mData->entries.size()) return B_ERROR;
	param.Clear();
	
	param.nid = mData->entries[index]->nid;
	param.p = mData->entries[index]->param;
	param.index = mData->entries[index]->index;
	param.pt = mData->entries[index]->paramType;
	param.label = mData->entries[index]->Label(mFlags);
	param.control = mData->entries[index]->control;
	param.midi = mData->entries[index]->midi;
	if (param.pt == 0 && param.p) param.pt = param.p->ParamType();
	return B_OK;
}

const GlParam* GlStrainedParamList::At(const gl_param_key& key) const
{
	if (!mData) return 0;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		_StrainedParamListEntry*	e = mData->entries[k];
		if (e && key.nid == e->nid && key.index == e->index
				&& e->param && key.key == e->param->ParamType()->Key())
			return e->param;
	}
	return 0;
}

#if 0
	const GlParam*		At(gl_node_id nid, int32 paramTypeKey, int32 i = 0) const;

const GlParam* GlStrainedParamList::At(gl_node_id nid, int32 ptKey, int32 i) const
{
	if (!mData) return 0;
	int32			match = 0;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k] && nid == mData->entries[k]->nid
				&& mData->entries[k]->param
				&& mData->entries[k]->code != mData->entries[k]->NORMAL_CODE) {
			if (ptKey == mData->entries[k]->param->ParamType()->Key()) {
				if (match == i) return mData->entries[k]->param;
				match++;
			}
		}
	}
	return 0;
}
#endif



#if 0
	/* MIDI
	 * Set all parameters at the midi value.  Return B_OK if anything
	 * was changed.
	 */
	status_t			SetParams(	int32 midi, const GlMidiEvent& event,
									GlRootRef& ref) const;

static float _float_value(const GlParam* pArg, const GlParamType* ptArg, float v, GlAlgo1d* a)
{
	ArpASSERT(ptArg->Type() == GL_FLOAT_TYPE);
	const GlFloatParamType*	pt = (const GlFloatParamType*)ptArg;
	const GlFloatParam*		fp = (pArg) ? ((const GlFloatParam*)pArg) : 0;
	float					min = pt->Min(), max = pt->Max();
	if (fp) fp->GetRange(&min, &max);
	float					r = max - min;
//printf("v %f min %f max %f\n", v, min, max);
	if (!a) return min + (r * v);
//printf("%f becomes %f (min %f max %f r %f)\n", v, min + (a->At(v) * r), min, max, r);
//a->Print();
	return min + (a->At(v) * r);
}

static int32 _int32_value(const GlParam* pArg, const GlParamType* ptArg, float v, GlAlgo1d* a)
{
	ArpASSERT(ptArg->Type() == GL_INT32_TYPE);
	GlInt32ParamType*		pt = (GlInt32ParamType*)ptArg;
	int32					r = pt->Max() - pt->Min();
	if (!a) return int32(pt->Min() + (r * v));
	return int32(pt->Min() + (a->At(v) * r));
}

static int32 _menu_value(const GlParam* pArg, const GlParamType* ptArg, float v, GlAlgo1d* a)
{
	ArpASSERT(ptArg->Type() == GL_MENU_TYPE);
	GlMenuParamType*		pt = (GlMenuParamType*)ptArg;
	int32					r = pt->ItemSize() - 1;
	if (r <= 0) return 0;
	int32					index = 0;
	if (!a) index = int32(r * v);
	else index = int32(a->At(v) * r);
	ArpASSERT(index >= 0 && index <= r);
	if (index < 0) index = 0;
	else if (index > r) index = r;
	return pt->Selection(index);
}

/* Find any params bound to the midi event.  If I have any,
 * change the param.  The locking is a little tricky -- I only
 * lock if there's a param to change.
 */
status_t GlStrainedParamList::SetParams(int32 midi, const GlMidiEvent& event,
										GlRootRef& ref) const
{
	ArpVALIDATE(midi >= 0, return B_ERROR);
	if (!mData) return B_NO_MEMORY;
	status_t			err = B_ERROR;
	GlRootNode*			root = 0;
	GlNode*				node = 0;
	float				v = event.ScaledValue();
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		_StrainedParamListEntry*	e = mData->entries[k];
		if (e && midi == e->midi) {
			const GlParam*			p = e->param;
			const GlParamType*		pt = (p) ? p->ParamType() : e->paramType;
			ArpASSERT(pt);
			
			if (!root) root = ref.WriteLock();
			if (root) {
				if (!node || node->Id() != e->nid) node = root->FindNode(0, e->nid);
				if (node) {
					if (pt->Type() == GL_FLOAT_TYPE)
						node->Params().SetValue(pt->Key(), GlFloatWrap(_float_value(p, pt, v, 0)));
					else if (pt->Type() == GL_INT32_TYPE)
						node->Params().SetValue(pt->Key(), GlInt32Wrap(_int32_value(p, pt, v, 0)));
					else if (pt->Type() == GL_MENU_TYPE)
						node->Params().SetValue(pt->Key(), GlInt32Wrap(_menu_value(p, pt, v, 0)));
					err = B_OK;
				}
			}
		}
	}
	if (root) ref.WriteUnlock(root);
	
	return err;
}
#endif

void GlStrainedParamList::MakeEmpty()
{
	delete mData;
	mData = new _StrainedParamListData();
	mFlags = 0;
}

void GlStrainedParamList::Print() const
{
	printf("GlStrainedParamList size %ld\n", (mData) ? mData->entries.size() : 0);
	if (!mData) return;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		const _StrainedParamListEntry*	e = mData->entries[k];
		if (e) {
			printf("%ld: '%s' nid control %ld midi %ld nid %p ", k, e->Label(NO_DEFAULT_LABEL_F),
					e->control, e->midi, e->nid);
			if (e->param) e->param->Print();
			else if (e->paramType) e->paramType->Print();
			else printf("\n");
		}
	}
}

// #pragma mark -

/***************************************************************************
 * GL-STRAINED-PARAM
 ****************************************************************************/
GlStrainedParam::GlStrainedParam()
{
	Clear();
}

void GlStrainedParam::Clear()
{
	nid = 0;
	p = 0;
	index = 0;
	pt = 0;
	label = 0;
	midi = 0;
}
