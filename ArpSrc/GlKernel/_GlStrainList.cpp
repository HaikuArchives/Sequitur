#include <GlKernel/GlDefs.h>
#include <GlKernel/_GlStrainList.h>

// _GL-STRAIN-LIST-PARAM
class _GlStrainListParam
{
public:
	int32				key, index, control, midi;
	const GlParam*		param;
	const GlParamType*	paramType;
	const BString16*	label;
		
	_GlStrainListParam(int32 k, int32 i) : key(k), index(i),
			control(GL_CONTROL_OFF), midi(GL_MIDI_OFF), param(0),
			paramType(0), label(0)	{ }

public:
	void			Print() const
	{
		if (label) printf(" %s", label);
		printf(" key %ld index %ld control %ld midi %ld", key, index, control, midi);
//		if (param) param->Print();
//		else if (paramType) paramType->Print();
//		else printf("\n");
		printf("\n");
	}
};

// _GL-STRAIN-LIST-NODE
class _GlStrainListNode
{
public:
	gl_node_id			nid;

	_GlStrainListNode(gl_node_id n);
	~_GlStrainListNode();

	status_t				Install(GlParamListI& target);
	_GlStrainListParam*		Param(int32 key, int32 index, bool create);

private:
	vector<_GlStrainListParam*>	mParams;

public:
	void			Print() const;
};

// #pragma mark -

/*************************************************************************
 * _GL-STRAIN-LIST
 *************************************************************************/
_GlStrainList::_GlStrainList()
{
}

_GlStrainList::~_GlStrainList()
{
	for (uint32 k = 0; k < mNodes.size(); k++) delete mNodes[k];
}

status_t _GlStrainList::Load(	const gl_param_key& key, const GlParam* param,
								const BString16* label, int32 control, int32 midi)
{
	ArpASSERT(Param(key) == 0);
	_GlStrainListParam*		p = Param(key, true);
	if (!p) return B_NO_MEMORY;
	
	p->param = param;
	p->paramType = 0;
	p->label = label;
	p->control = control;
	p->midi = midi;
	return B_OK;
}

status_t _GlStrainList::Load(	const gl_param_key& key, const GlParamType* paramType,
								const BString16* label, int32 control, int32 midi)
{
	ArpASSERT(Param(key) == 0);
	_GlStrainListParam*		p = Param(key, true);
	if (!p) return B_NO_MEMORY;
	
	p->param = 0;
	p->paramType = paramType;
	p->label = label;
	p->control = control;
	p->midi = midi;
	return B_OK;
}

status_t _GlStrainList::Strain(	const gl_param_key& key, const BString16* label,
								int32 control, int32 midi)
{
	_GlStrainListParam*		p = Param(key, false);
	if (!p) return B_OK;

	if (label) p->label = label;
	if (p->control == GL_CONTROL_ON) p->control = control;
	if (p->midi >= GL_MIDI_A) p->midi = midi;
	return B_OK;
}

status_t _GlStrainList::Install(GlParamListI& target)
{
	for (uint32 k = 0; k < mNodes.size(); k++) mNodes[k]->Install(target);
	return B_OK;
}

	
#if 0
	status_t		Set(const gl_param_key& key, const WCHAR* label,
						int32 control, int32 midi);
	status_t		Sync(	gl_node_id nid, const GlParamList* params,
							const GlParamTypeList* paramTypes);
#endif

#if 0
status_t _GlStrainList::Set(const gl_param_key& key, const WCHAR* label,
							int32 control, int32 midi)
{
	_GlStrainListParam*		p = MakeParam(key);
	if (!p) return B_ERROR;
	if (p->control != GL_CONTROL_OFF) p->control = control;
	if (p->midi == GL_UNSET_MIDI) p->midi = midi;
	if (p->label == 0) p->label = label;
	return B_OK;
}

status_t _GlStrainList::Sync(	gl_node_id nid, const GlParamList* params,
								const GlParamTypeList* paramTypes)
{
	ArpVALIDATE(params && paramTypes, return B_ERROR);
	for (uint32 k = 0; k < mNodes.size(); k++) {
		if (nid == mNodes[k]->nid)
			return mNodes[k]->Sync(params, paramTypes, target);	
	}
	return B_OK;
}
#endif

_GlStrainListParam* _GlStrainList::Param(const gl_param_key& key, bool create)
{
	_GlStrainListNode*		n = Node(key.nid, create);
	if (!n) return 0;
	return n->Param(key.key, key.index, create);
}

_GlStrainListNode* _GlStrainList::Node(gl_node_id nid, bool create)
{
	for (uint32 k = 0; k < mNodes.size(); k++) {
		if (nid == mNodes[k]->nid) return mNodes[k];
	}
	if (!create) return 0;
	_GlStrainListNode*	n = new _GlStrainListNode(nid);
	if (!n) return 0;
	mNodes.push_back(n);
	return n;
}

void _GlStrainList::Print() const
{
	printf("_GlStrainList size %ld\n", mNodes.size());
	for (uint32 k = 0; k < mNodes.size(); k++) {
		printf("\t%ld: ", k);
		mNodes[k]->Print();
	}
}

// #pragma mark -

/*************************************************************************
 * _GL-STRAIN-LIST-NODE
 *************************************************************************/
_GlStrainListNode::_GlStrainListNode(gl_node_id n)
		: nid(n)
{
}

_GlStrainListNode::~_GlStrainListNode()
{
	for (uint32 k = 0; k < mParams.size(); k++) delete mParams[k];
}

status_t _GlStrainListNode::Install(GlParamListI& target)
{
	for (uint32 k = 0; k < mParams.size(); k++) {
		_GlStrainListParam*	e = mParams[k];
		ArpASSERT(e);
		if (e->control == GL_CONTROL_ON || e->midi >= GL_MIDI_A) {
			if (e->param) target.Add(nid, e->param, e->index, e->label, e->control, e->midi);
			else if (e->paramType) target.Add(nid, e->paramType, e->label, e->control, e->midi);
		}
	}
	return B_OK;
}

_GlStrainListParam* _GlStrainListNode::Param(int32 key, int32 index, bool create)
{
	for (uint32 k = 0; k < mParams.size(); k++) {
		if (key == mParams[k]->key && index == mParams[k]->index) return mParams[k];
	}
	if (!create) return 0;
	_GlStrainListParam*		p = new _GlStrainListParam(key, index);
	if (!p) return 0;
	mParams.push_back(p);
	return p;
}

void _GlStrainListNode::Print() const
{
	printf("_GlStrainListNode %p size %ld\n", nid, mParams.size());
	for (uint32 k = 0; k < mParams.size(); k++) {
		printf("\t\t%ld: ", k);
		mParams[k]->Print();
	}
}
