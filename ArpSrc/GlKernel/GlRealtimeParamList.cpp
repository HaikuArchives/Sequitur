#include <ArpCore/StlVector.h>
#include <GlPublic/GlControlState.h>
#include <GlPublic/GlMidiEvent.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParam.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamWrap.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlRealtimeParamList.h>
#include <GlPublic/GlAlgo2d.h>

/***************************************************************************
 * _GL-RTPL-DATA
 ****************************************************************************/
class _GlRtplData
{
public:
	vector<GlAlgo*>		redirectors;

	_GlRtplData();

	void	AddRedirector(GlAlgo* n);
	void	UpdateSource(GlNode* n);
};

/***************************************************************************
 * GL-REALTIME-ENTRY
 ****************************************************************************/
class GlRealtimeEntry
{
public:
	GlAlgo*					target;
	gl_param_key			key;

	virtual ~GlRealtimeEntry() { }
	
	virtual status_t		SetValue(	float v, GlAlgo* root,
										GlParseFromNodeAction& action) = 0;
	virtual status_t		UpdateSource(GlNode* node) = 0;

	void					Build(GlAlgo* n);

protected:
	GlRealtimeEntry(gl_node_id nid, int32 key, int32 index);

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-REALTIME-FLOAT
 ****************************************************************************/
class GlRealtimeFloat : public GlRealtimeEntry
{
public:
	GlRealtimeFloat(gl_node_id nid, int32 key, int32 index, float min, float max);
	
	virtual status_t		SetValue(	float v, GlAlgo* root,
										GlParseFromNodeAction& action);
	virtual status_t		UpdateSource(GlNode* node);

private:
	typedef GlRealtimeEntry	inherited;
	float					mMin, mMax;

public:
	virtual void			Print() const;
};

/***************************************************************************
 * GL-REALTIME-ENTRY-LIST
 ****************************************************************************/
class GlRealtimeEntryList
{
public:
	GlRealtimeEntryList();
	~GlRealtimeEntryList();

	uint32			Size() const;
	
	status_t		SetValue(	const gl_param_key& key, const GlParamWrap& wrap,
								GlAlgo* root, GlParseFromNodeAction& action);
	status_t		SetValue(	float v, GlAlgo* root,
								GlParseFromNodeAction& action);

	status_t		Add(GlRealtimeEntry* e);
	void			Build(GlAlgo* n);
	status_t		UpdateSource(GlNode* node);

private:
	vector<GlRealtimeEntry*>	mEntries;

public:
	void			Print(uint32 tabs) const;
};

/***************************************************************************
 * GL-REALTIME-PARAM-LIST
 ****************************************************************************/
GlRealtimeParamList::GlRealtimeParamList()
		: mData(new _GlRtplData()), mParams(0), mRoot(0)
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) mMidi[k] = 0;
}

GlRealtimeParamList::~GlRealtimeParamList()
{
	delete mData;
	delete mParams;
	mParams = 0;
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) delete mMidi[k];
}

status_t GlRealtimeParamList::Add(	gl_node_id nid, const GlParam* param, int32 index,
									const BString16* label, int32 control, int32 midi)
{
	if (!param) return B_ERROR;
	const GlParamType*		pt = param->ParamType();
	if (!pt) return B_ERROR;

	GlRealtimeEntryList*	list = 0;
	if (midi >= 0 && midi < GL_MIDI_SIZE) list = MidiListAt(midi, true);
	else list = ParamList(true);
	if (!list) return B_NO_MEMORY;

	if (pt->Type() == GL_FLOAT_TYPE) {
		float				min, max;
		if (((GlFloatParam*)param)->GetRange(&min, &max) != B_OK) {
			min = ((const GlFloatParamType*)pt)->Min();
			max = ((const GlFloatParamType*)pt)->Max();
		}
		return list->Add(new GlRealtimeFloat(nid, pt->Key(), index, min, max));
	}
	return B_ERROR;
}

status_t GlRealtimeParamList::Add(	gl_node_id nid, const GlParamType* pt,
									const BString16* label, int32 control, int32 midi)
{
	if (!pt) return B_ERROR;

	GlRealtimeEntryList*	list = 0;
	if (midi >= 0 && midi < GL_MIDI_SIZE) list = MidiListAt(midi, true);
	else list = ParamList(true);
	if (!list) return B_NO_MEMORY;

	if (pt->Type() == GL_FLOAT_TYPE) {
		float				min = ((const GlFloatParamType*)pt)->Min(),
							max = ((const GlFloatParamType*)pt)->Max();
		/* FIX:  I'm passing in a 0 where index is -- param types have
		 * no index so how best to deal with this?  Should there be
		 * an index value that means 'all indexes'?
		 */
		return list->Add(new GlRealtimeFloat(nid, pt->Key(), 0, min, max));
	}
	return B_ERROR;
}

status_t GlRealtimeParamList::SetValue(const gl_param_key& key, const GlParamWrap& wrap)
{
	if (!mParams) return B_ERROR;
	return mParams->SetValue(key, wrap, mRoot, mRootAction);
}

status_t GlRealtimeParamList::SetValue(int32 midi, const GlMidiEvent& e)
{
	ArpVALIDATE(midi >= 0 && midi < GL_MIDI_SIZE, return B_ERROR);
	if (!mMidi[midi]) return B_BAD_INDEX;

	float			v = 0;
	if (e.type == e.CONTROL_CHANGE) v = e.value2 / 127.0f;
	else return B_ERROR;

	return mMidi[midi]->SetValue(v, mRoot, mRootAction);
}

class _BuildListAction : public GlAlgoAction
{
public:
	_BuildListAction(	_GlRtplData* data,
						GlRealtimeEntryList* params,
						GlRealtimeEntryList** midi)
			: mData(data), mParams(params), mMidi(midi)	{ }
	
	virtual int32		Perform(GlAlgo* n)
	{
		if (mParams) mParams->Build(n);
		for (uint32 k = 0; k < GL_MIDI_SIZE; k++) {
			if (mMidi[k]) mMidi[k]->Build(n);
		}
		if (mData && n->Flags()&n->REDIRECTOR_F) mData->AddRedirector(n);
		return GL_CONTINUE;
	}

private:
	_GlRtplData*			mData;
	GlRealtimeEntryList*	mParams;
	GlRealtimeEntryList**	mMidi;
};

status_t GlRealtimeParamList::Build(GlAlgo2d* s)
{
	ArpVALIDATE(s, return B_ERROR);
	_BuildListAction		action(mData, mParams, mMidi);
	s->Walk(action, 0);
	/* If there are any redirectors, then set a root, so
	 * the assigned parse node for each param is bypassed and
	 * it's found from walking the tree.  This is necessary
	 * because redirectors alter the branches of the tree.
	 */
	if (mData && mData->redirectors.size() > 0) mRoot = s;

	return B_OK;		
}

status_t GlRealtimeParamList::UpdateSource(GlNode* node)
{
	ArpVALIDATE(node, return B_ERROR);

	if (mParams) mParams->UpdateSource(node);
	
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) {
		if (mMidi[k]) mMidi[k]->UpdateSource(node);
	}

	if (mData) mData->UpdateSource(node);
	
	return B_OK;
}

void GlRealtimeParamList::SetState(GlControlState& s) const
{
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) {
		if (mMidi[k] && mMidi[k]->Size() > 0)
			s.SetMidi(k);
	}
}

GlRealtimeEntryList* GlRealtimeParamList::ParamList(bool create)
{
	if (!mParams && create) mParams = new GlRealtimeEntryList();
	return mParams;
}

GlRealtimeEntryList* GlRealtimeParamList::MidiListAt(int32 midi, bool create)
{
	ArpASSERT(midi >= 0 && midi < GL_MIDI_SIZE);

	if (!mMidi[midi] && create) mMidi[midi] = new GlRealtimeEntryList();
	return mMidi[midi];
}

void GlRealtimeParamList::Print(uint32 tabs) const
{
	for (uint32 t = 0; t < tabs; t++) printf("\t");
	printf("RealtimeParamList\n");

	for (uint32 t = 0; t < tabs; t++) printf("\t");
	printf("RealtimeParamList - midi\n");
	for (uint32 k = 0; k < GL_MIDI_SIZE; k++) {
		if (mMidi[k]) {
			for (uint32 t = 0; t < tabs + 1; t++) printf("\t");
			printf("%ld: ", k);
			mMidi[k]->Print(tabs + 1);
		}
	}
}

// #pragma mark -

/***************************************************************************
 * _GL-RTPL-DATA
 ****************************************************************************/
_GlRtplData::_GlRtplData()
{
}

void _GlRtplData::AddRedirector(GlAlgo* node)
{
	ArpASSERT(node);
	redirectors.push_back(node);
}

void _GlRtplData::UpdateSource(GlNode* node)
{
	ArpASSERT(node);
	for (uint32 k = 0; k < redirectors.size(); k++) {
		ArpASSERT(redirectors[k]);
		GlNode*		n = node->FindNode(0, redirectors[k]->NodeId());
		if (n) redirectors[k]->UpdateSource(n);
	}
}

// #pragma mark -

/***************************************************************************
 * GL-REALTIME-ENTRY-LIST
 ****************************************************************************/
GlRealtimeEntryList::GlRealtimeEntryList()
{
}

GlRealtimeEntryList::~GlRealtimeEntryList()
{
	for (uint32 k = 0; k < mEntries.size(); k++) delete mEntries[k];
	mEntries.resize(0);
}

uint32 GlRealtimeEntryList::Size() const
{
	return uint32(mEntries.size());
}

status_t GlRealtimeEntryList::SetValue(	const gl_param_key& key,
										const GlParamWrap& wrap,
										GlAlgo* root,
										GlParseFromNodeAction& action)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->target && mEntries[k]->key == key) {
			if (!root) mEntries[k]->target->SetParam(key, wrap);
			else {
				action.nid = mEntries[k]->key.nid;
				root->Walk(action);
				if (action.node) action.node->SetParam(key, wrap);
			}
		}
	}
	return B_OK;
}

status_t GlRealtimeEntryList::SetValue(	float v, GlAlgo* root,
										GlParseFromNodeAction& action)
{
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k]) mEntries[k]->SetValue(v, root, action);
	}
	return B_OK;
}

status_t GlRealtimeEntryList::Add(GlRealtimeEntry* e)
{
	if (!e) return B_ERROR;
	mEntries.push_back(e);
	return B_OK;
}

void GlRealtimeEntryList::Build(GlAlgo* n)
{
	ArpASSERT(n);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->target == 0)
			mEntries[k]->Build(n);
	}
}

status_t GlRealtimeEntryList::UpdateSource(GlNode* node)
{
	ArpASSERT(node);
	for (uint32 k = 0; k < mEntries.size(); k++) {
		if (mEntries[k] && mEntries[k]->key.nid) {
			GlNode*		n = node->FindNode(0, mEntries[k]->key.nid);
			if (n) mEntries[k]->UpdateSource(n);
		}
	}
	return B_OK;	
}

void GlRealtimeEntryList::Print(uint32 tabs) const
{
	printf("size (%ld)\n", mEntries.size());
	for (uint32 k = 0; k < mEntries.size(); k++) {
		for (uint32 t = 0; t < tabs + 1; t++) printf("\t");
		printf("%ld: ", k);
		mEntries[k]->Print();
	}
}

// #pragma mark -

/***************************************************************************
 * GL-REALTIME-ENTRY
 ****************************************************************************/
GlRealtimeEntry::GlRealtimeEntry(gl_node_id n, int32 k, int32 i)
		: target(0), key(n, k, i)
{
}

void GlRealtimeEntry::Build(GlAlgo* n)
{
	ArpASSERT(n);
	if (key.nid == n->NodeId()) target = n;
}

void GlRealtimeEntry::Print() const
{
	printf("GlRealtimeEntry <error>\n");
}

// #pragma mark -

/***************************************************************************
 * GL-REALTIME-FLOAT
 ****************************************************************************/
GlRealtimeFloat::GlRealtimeFloat(	gl_node_id nid, int32 k, int32 i,
									float min, float max)
		: inherited(nid, k, i), mMin(min), mMax(max)
{
}

status_t GlRealtimeFloat::SetValue(	float v, GlAlgo* root,
									GlParseFromNodeAction& action)
{
	float			r = mMax - mMin;
//	if (a) v = mMin + (a->At(v) * r);
	v = mMin + (r * v);
	GlFloatWrap		w(v);

	if (root) {
		action.nid = key.nid;
		root->Walk(action);
		if (action.node) return action.node->SetParam(key, w);
	} else if (target) return target->SetParam(key, w);
	return B_ERROR;
}

status_t GlRealtimeFloat::UpdateSource(GlNode* node)
{
	ArpASSERT(node);
	if (!target) return B_ERROR;
	GlFloatWrap		wrap;
	if (target->GetParam(key, wrap) != B_OK) return B_ERROR;
	status_t	err = node->Params().SetValue(key.key, wrap, key.index);
	if (err == B_OK) node->ParamChanged(key);
	return err;
}

void GlRealtimeFloat::Print() const
{
	printf("GlRealtimeFloat min %f max %f target %p (nid %p key %ld index %ld)\n",
			mMin, mMax, target, key.nid, key.key, key.index);
}
