#include <stdio.h>
#include <ArpCore/StlVector.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlParamStrainer.h>
#include <GlPublic/GlParamType.h>
#include <GlKernel/GlDefs.h>
#include <GlKernel/_GlStrainList.h>

static const char*				TYPE_ENTRY_STR	= "te";
static const char*				TYPE_STR		= "t";
static const char*				KEY_STR			= "k";
static const char*				INDEX_STR		= "i";
static const char*				ENABLED_STR		= "e";
static const char*				LABEL_STR		= "l";
static const char*				CONTROL_STR		= "c";
static const char*				MIDI_STR		= "m";

/*************************************************************************
 * _PARAM-STRAINER-DATA
 *************************************************************************/
class _ParamStrainerTypeEntry
{
public:
	int32					key, index;
	BString16				label;
	int32					control, midi;
	
	_ParamStrainerTypeEntry(int32 inKey, int32 inIndex,
							const BMessage* config = 0);
	
	/* Only actually write something if I have data to write,
	 * otherwise answer B_ERROR.
	 */
	status_t WriteTo(const GlParamType* pt, BMessage& config) const;
};


class _ParamStrainerNodeEntry
{
public:
	gl_node_id							nid;
	vector<_ParamStrainerTypeEntry*>	entries;
	_ParamStrainerNodeEntry(gl_node_id inNid) : nid(inNid)		{ }
	~_ParamStrainerNodeEntry();

	status_t			Load(	_GlStrainList& list, const GlParamList& params,
								const GlParamTypeList& paramTypes) const;
	status_t			Strain(	_GlStrainList& list) const;

	const _ParamStrainerTypeEntry*	EntryAt(int32 key, int32 index) const;
	_ParamStrainerTypeEntry*		EntryAt(int32 key, int32 index);
};

class _ParamStrainerData
{
public:
	vector<_ParamStrainerNodeEntry*>	entries;

	_ParamStrainerData()			{ }
	~_ParamStrainerData();

	bool							HasNode(gl_node_id nid);

	const _ParamStrainerNodeEntry*	EntryAt(gl_node_id nid) const;
	_ParamStrainerNodeEntry*		EntryAt(gl_node_id nid);
};

/*************************************************************************
 * GL-PARAM-STRAINER
 *************************************************************************/
GlParamStrainer::GlParamStrainer()
		: mData(new _ParamStrainerData())
{
}

GlParamStrainer::~GlParamStrainer()
{
	delete mData;
}

status_t GlParamStrainer::InitCheck() const
{
	if (!mData) return B_NO_MEMORY;
	return B_OK;
}

#if 0
// -- obsolete
	bool				Enabled(	gl_param_key key) const;
	status_t			SetEnabled(	gl_param_key key, bool enabled);
// -- end obsolete

bool GlParamStrainer::Enabled(gl_param_key key) const
{
	const _ParamStrainerTypeEntry*	te = FindAt(key);
	if (!te) return false;
	return te->enabled;
}

status_t GlParamStrainer::SetEnabled(gl_param_key key, bool enabled)
{
	_ParamStrainerTypeEntry*	te = MakeAt(key);
	if (!te) return B_NO_MEMORY;
	te->enabled = enabled;
	return B_OK;
}
#endif

const BString16* GlParamStrainer::Label(gl_param_key key) const
{
	const _ParamStrainerTypeEntry*	te = FindAt(key);
	if (!te) return 0;
	return &te->label;
}

status_t GlParamStrainer::SetLabel(gl_param_key key, const BString16* label)
{
	_ParamStrainerTypeEntry*	te = MakeAt(key);
	if (!te) return B_NO_MEMORY;
	if (!label) te->label = "";
	else te->label = label;
	return B_OK;
}

int32 GlParamStrainer::Control(gl_param_key key) const
{
	const _ParamStrainerTypeEntry*	te = FindAt(key);
	if (!te) return GL_CONTROL_OFF;
	return te->control;
}

status_t GlParamStrainer::SetControl(gl_param_key key, int32 control)
{
	_ParamStrainerTypeEntry*	te = MakeAt(key);
	if (!te) return B_NO_MEMORY;
	te->control = control;
	return B_OK;
}

int32 GlParamStrainer::Midi(gl_param_key key) const
{
	const _ParamStrainerTypeEntry*	te = FindAt(key);
	if (!te) return GL_UNSET_MIDI;
	return te->midi;
}

status_t GlParamStrainer::SetMidi(gl_param_key key, int32 midi)
{
	_ParamStrainerTypeEntry*	te = MakeAt(key);
	if (!te) return B_NO_MEMORY;
	te->midi = midi;
	return B_OK;
}

status_t GlParamStrainer::NewGetAt(gl_param_key key, int32* ptIndex,
								const BString16** outLabel, int32* outControl, 
								int32* outMidi) const
{
	const _ParamStrainerTypeEntry*	te = FindAt(key);
	if (!te) return B_ERROR;
	if (ptIndex) *ptIndex = te->index;
	if (outLabel) *outLabel = &te->label;
	if (outControl) *outControl = te->control;
	if (outMidi) *outMidi = te->midi;
	return B_OK;
}

status_t GlParamStrainer::GetAt(gl_param_key key, int32* ptIndex,
								bool* outEnabled, const BString16** outLabel,
								int32* outMidi) const
{
	const _ParamStrainerTypeEntry*	te = FindAt(key);
	if (!te) return B_ERROR;
	if (ptIndex) *ptIndex = te->index;
	if (outEnabled) *outEnabled = (te->control == GL_CONTROL_OFF) ? false : true;
	if (outLabel) *outLabel = new BString16(te->label);
	if (outMidi) *outMidi = te->midi;
	return B_OK;
}

status_t GlParamStrainer::GetAt(gl_node_id nid, uint32 paramTypeIndex, int32* outPtKey,
								int32* outPtIndex, bool* outEnabled, const BString16** outLabel,
								int32* outMidi) const
{
	ArpVALIDATE(mData, return B_ERROR);
	_ParamStrainerNodeEntry*	ne = mData->EntryAt(nid);
	if (!ne) return B_ERROR;
	if (paramTypeIndex >= ne->entries.size()) return B_ERROR;
	if (outPtKey) *outPtKey = ne->entries[paramTypeIndex]->key;
	if (outPtIndex) *outPtIndex = ne->entries[paramTypeIndex]->index;
	if (outEnabled) {
		bool		enabled = false;
		if (ne->entries[paramTypeIndex]->control == GL_CONTROL_ON) enabled = true;
		*outEnabled = enabled;
	}
	if (outLabel) *outLabel = &ne->entries[paramTypeIndex]->label;
	if (outMidi) *outMidi = ne->entries[paramTypeIndex]->midi;
	return B_OK;
}

uint32 GlParamStrainer::Size() const
{
	if (!mData) return 0;
	uint32			c = 0;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k]) c += uint32(mData->entries[k]->entries.size());
	}
	return c;
}

uint32 GlParamStrainer::SizeAt(gl_node_id nid) const
{
	ArpVALIDATE(mData, return 0);
	const _ParamStrainerNodeEntry*	ne = mData->EntryAt(nid);
	if (!ne) return 0;
	return uint32(ne->entries.size());
}

status_t GlParamStrainer::ReadNode(const GlNode* node, const BMessage& config)
{
	ArpVALIDATE(mData && node && node->AddOn(), return B_ERROR);
//printf("Strainer read node %s\n", node->AddOn()->Label().String());
	gl_node_id				nid = node->Id();
	ArpASSERT(mData->HasNode(nid) == false);
	_ParamStrainerNodeEntry*	e = 0;
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k]->nid == nid) {
			e = mData->entries[k];
			break;
		}
	}
	if (!e) {
		e = new _ParamStrainerNodeEntry(nid);
		if (e) mData->entries.push_back(e);
	}
	if (!e) return B_NO_MEMORY;
	BMessage				entryMsg;
	for (int32 i = 0; config.FindMessage(TYPE_ENTRY_STR, i, &entryMsg) == B_OK; i++) {
		int32				key;
		if (entryMsg.FindInt32(KEY_STR, &key) == B_OK) {
			const GlParamType*	pt = node->AddOn()->ParamTypes().Find(key);
			if (pt) {
				_ParamStrainerTypeEntry*	te = new _ParamStrainerTypeEntry(pt->Key(), 0, &entryMsg);
				if (te) e->entries.push_back(te);
			}
		}
		entryMsg.MakeEmpty();
	}
	return B_OK;
}

status_t GlParamStrainer::WriteNode(const GlNode* node, BMessage& config) const
{
	ArpVALIDATE(mData && node && node->AddOn(), return B_ERROR);
	gl_node_id						nid = node->Id();
	const _ParamStrainerNodeEntry*	ne = mData->EntryAt(nid);
	if (!ne) return B_OK;
	uint32							size = uint32(ne->entries.size());
	for (uint32 k = 0; k < size; k++) {
		const _ParamStrainerTypeEntry*	te = ne->entries[k];
		if (te) {
			BMessage				entryMsg;
			if (te->WriteTo(node->AddOn()->ParamTypes().Find(te->key), entryMsg) == B_OK) {
				if (config.AddMessage(TYPE_ENTRY_STR, &entryMsg) != B_OK) return B_ERROR;
			}
		}
	}
	return B_OK;
}

#if 0
	status_t			Strain(_GlStrainList* list) const;

status_t GlParamStrainer::Strain(_GlStrainList* list) const
{
	ArpVALIDATE(mData, return 0);
	ArpASSERT(list);
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k]) mData->entries[k]->Strain(list);
	}
	return B_OK;
}
#endif

status_t GlParamStrainer::Load(	gl_node_id nid, _GlStrainList& list,
								const GlParamList& params,
								const GlParamTypeList& paramTypes) const
{
	ArpVALIDATE(mData, return B_NO_MEMORY);
	const _ParamStrainerNodeEntry*	e = mData->EntryAt(nid);
	if (!e) return B_OK;
	status_t						err = e->Load(list, params, paramTypes);
	if (err != B_OK) return err;
	/* After loading in the params for the given node, I then
	 * run through all other params and set any modifications.
	 */
	for (uint32 k = 0; k < mData->entries.size(); k++) {
		_ParamStrainerNodeEntry*	e = mData->entries[k];
		if (e && nid != e->nid) {
			e->Strain(list);
		}
	}
	return B_OK;
}

const _ParamStrainerTypeEntry* GlParamStrainer::FindAt(const gl_param_key& key) const
{
	ArpVALIDATE(mData, return 0);
	_ParamStrainerNodeEntry*	ne = mData->EntryAt(key.nid);
	if (!ne) return 0;
	return ne->EntryAt(key.key, key.index);
}

_ParamStrainerTypeEntry* GlParamStrainer::MakeAt(const gl_param_key& key)
{
	ArpVALIDATE(mData, return 0);
	_ParamStrainerNodeEntry*	ne = mData->EntryAt(key.nid);
	if (!ne) {
		ne = new _ParamStrainerNodeEntry(key.nid);
		if (!ne) return 0;
		mData->entries.push_back(ne);
	}
	_ParamStrainerTypeEntry*	te = ne->EntryAt(key.key, key.index);
	if (!te) {
		te = new _ParamStrainerTypeEntry(key.key, key.index);
		if (!te) return 0;
		ne->entries.push_back(te);
	}
	return te;
}

void GlParamStrainer::Print() const
{
	printf("GlParamStrainer\n");
	if (!mData) {
		printf("\tno data\n");
		return;
	}
	uint32			k, k2;
	for (k = 0; k < mData->entries.size(); k++) {
		if (mData->entries[k]) {
			printf("\tnode id: %p\n", mData->entries[k]->nid);
			for (k2 = 0; k2 < mData->entries[k]->entries.size(); k2++) {
				printf("\t\tparam type key: %ld index: %ld label: %s control: %ld midi: %ld\n",
						mData->entries[k]->entries[k2]->key,
						mData->entries[k]->entries[k2]->index,
						mData->entries[k]->entries[k2]->label.String(),
						mData->entries[k]->entries[k2]->control,
						mData->entries[k]->entries[k2]->midi);
			}
		}
	}
}

// #pragma mark -

/*************************************************************************
 * _PARAM-STRAINER-TYPE-ENTRY
 *************************************************************************/
_ParamStrainerTypeEntry::_ParamStrainerTypeEntry(	int32 inKey, int32 inIndex,
													const BMessage* config)
		: key(inKey), index(inIndex), control(GL_CONTROL_OFF), midi(GL_MIDI_OFF)
{
	if (config) {
		bool		b;
		int32		i32;
		BString16	s;
		if (config->FindInt32(INDEX_STR, &i32) == B_OK) index = i32;
// BW
		if (config->FindBool(ENABLED_STR, &b) == B_OK) control = (b) ? GL_CONTROL_ON : GL_CONTROL_OFF;
		if (config->FindString(LABEL_STR, &s) == B_OK) label = s;
		if (config->FindInt32(CONTROL_STR, &i32) == B_OK) control = i32;
		if (config->FindInt32(MIDI_STR, &i32) == B_OK) midi = i32;
	}
}

status_t _ParamStrainerTypeEntry::WriteTo(const GlParamType* pt, BMessage& config) const
{
	if (!pt) return B_ERROR;
	/* Eh, write nothing if there's nothing to write.
	 */
//	if (!enabled && (!(label.String()) || label.Length() < 1)) return B_ERROR;
	if (control == GL_CONTROL_OFF && midi < GL_MIDI_A
			&& (!(label.String()) || label.Length() < 1) )
		return B_ERROR;

	if (config.AddInt32(TYPE_STR, pt->Type()) != B_OK) return B_ERROR;
	if (config.AddInt32(KEY_STR, pt->Key()) != B_OK) return B_ERROR;
	if (config.AddInt32(INDEX_STR, index) != B_OK) return B_ERROR;
	if (label.String() && label.Length() > 0)
		if (config.AddString(LABEL_STR, label) != B_OK) return B_ERROR;
	if (config.AddInt32(CONTROL_STR, control) != B_OK) return B_ERROR;
	if (config.AddInt32(MIDI_STR, midi) != B_OK) return B_ERROR;
	return B_OK;
}

// #pragma mark -

/*************************************************************************
 * _PARAM-STRAINER-NODE-ENTRY
 *************************************************************************/
_ParamStrainerNodeEntry::~_ParamStrainerNodeEntry()
{
	uint32		size = uint32(entries.size());
	for (uint32 k = 0; k < size; k++) delete entries[k];
	entries.resize(0);
}

status_t _ParamStrainerNodeEntry::Load(	_GlStrainList& list, const GlParamList& params,
										const GlParamTypeList& paramTypes) const
{
	for (uint32 k = 0; k < entries.size(); k++) {
		const _ParamStrainerTypeEntry*	e = entries[k];
		if (e && e->control == GL_CONTROL_ON || e->midi >= GL_MIDI_A) {
			gl_param_key			key(nid, e->key, e->index);
			const BString16*		lbl = NULL;
			if (e->label.Length() > 0) lbl = &e->label;
			
			const GlParam*			p = params.Find(e->key, e->index);
			if (p) list.Load(key, p, lbl, e->control, e->midi);
			else {
				const GlParamType*	pt = paramTypes.Find(e->key);
				if (pt) list.Load(key, pt, lbl, e->control, e->midi);
			}
		}
	}
	return B_OK;
}

status_t _ParamStrainerNodeEntry::Strain(_GlStrainList& list) const
{
	for (uint32 k = 0; k < entries.size(); k++) {
		const _ParamStrainerTypeEntry*	e = entries[k];
		if (e) {
			gl_param_key			key(nid, e->key, e->index);
			const BString16*		lbl = 0;
			if (e->label.Length() > 0) lbl = &e->label;

			list.Strain(key, lbl, e->control, e->midi);
		}
	}
	return B_OK;
}

const _ParamStrainerTypeEntry* _ParamStrainerNodeEntry::EntryAt(int32 ptKey,
																int32 ptIndex) const
{
	uint32		size = uint32(entries.size());
	for (uint32 k = 0; k < size; k++) {
		if (ptKey == entries[k]->key && ptIndex == entries[k]->index)
			return entries[k];
	}
	return 0;
}

_ParamStrainerTypeEntry* _ParamStrainerNodeEntry::EntryAt(	int32 ptKey,
															int32 ptIndex)
{
	uint32		size = uint32(entries.size());
	for (uint32 k = 0; k < size; k++) {
		if (ptKey == entries[k]->key && ptIndex == entries[k]->index)
			return entries[k];
	}
	return 0;
}

// #pragma mark -

/*************************************************************************
 * _PARAM-STRAINER-DATA
 *************************************************************************/
_ParamStrainerData::~_ParamStrainerData()
{
	for (uint32 k = 0; k < entries.size(); k++) delete entries[k];
}

bool _ParamStrainerData::HasNode(gl_node_id nid)
{
	for (uint32 k = 0; k < entries.size(); k++) {
		if (entries[k]->nid == nid) return true;
	}
	return false;
}

const _ParamStrainerNodeEntry* _ParamStrainerData::EntryAt(gl_node_id nid) const
{
	uint32		size = uint32(entries.size());
	for (uint32 k = 0; k < size; k++) {
		if (nid == entries[k]->nid) return entries[k];
	}
	return 0;
}

_ParamStrainerNodeEntry* _ParamStrainerData::EntryAt(gl_node_id nid)
{
	uint32		size = uint32(entries.size());
	for (uint32 k = 0; k < size; k++) {
		if (nid == entries[k]->nid) return entries[k];
	}
	return 0;
}
