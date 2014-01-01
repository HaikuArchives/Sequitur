#include <ArpCore/StlVector.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpBitmap.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlUtils.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlParamView.h>
#include <GlPublic/GlAlgo.h>
#include <GlPublic/GlProcessStatus.h>
#include <GlPublic/GlStrainedParamList.h>
#include <GlKernel/GlConfigView.h>
#include <GlKernel/GlDefs.h>
#include <GlKernel/_GlStrainList.h>

static const char*		CHAIN_STR				= "chain";
static const char*		STRAINER_STR			= "strn";
static const char*		TAGS_STR				= "tags";

static const char*		CHAIN_STRAIN_STR		= "chain_strn";
static const char*		STRAINER_I_STR			= "strn_i";

// GL-NODE-PRIVATE
class GlNodePrivate
{
public:
	vector<GlChain*>		chains;

	GlNodePrivate()			{ }
	~GlNodePrivate()		{ MakeEmpty(); }

	void			MakeEmpty()
	{
		for (uint32 k = 0; k < chains.size(); k++) {
			if (chains[k]) {
				chains[k]->MakeEmpty();
				chains[k]->DecRefs();
			}
		}
		chains.resize(0);
	}
};


/*************************************************************************
 * GL-ABSTRACT-NODE
 *************************************************************************/
GlAbstractNode::GlAbstractNode()
		: mData(new GlNodePrivate())
{
}

GlAbstractNode::GlAbstractNode(const BMessage* config)
		: mData(new GlNodePrivate())
{
	if (mData && config) ReadFrom(*config);
}

GlAbstractNode::GlAbstractNode(const GlAbstractNode& o)
		: mData(new GlNodePrivate())
{
	if (mData && o.mData) {
		for (uint32 k = 0; k < o.mData->chains.size(); k++) {
			GlChain*		chain = o.mData->chains[k]->Clone();
			if (chain) {
				chain->IncRefs();
				mData->chains.push_back(chain);
			}
		}
	}
}

GlAbstractNode::~GlAbstractNode()
{
	delete mData;
}

gl_node_id GlAbstractNode::Id() const
{
	return (void*)this;
}

status_t GlAbstractNode::GetProperty(int32 code, GlParamWrap& wrap) const
{
	return B_ERROR;
}

status_t GlAbstractNode::SetProperty(int32 code, const GlParamWrap& wrap)
{
	return B_ERROR;
}

uint32 GlAbstractNode::ChainSize() const
{
	ArpVALIDATE(mData, return 0);
	return uint32(mData->chains.size());
}

const GlChain* GlAbstractNode::ChainAt(uint32 index) const
{
	ArpVALIDATE(mData, return 0);
	if (index >= mData->chains.size()) return 0;
	return mData->chains[index];
}

GlChain* GlAbstractNode::ChainAt(uint32 index)
{
	ArpVALIDATE(mData, return 0);
	if (index >= mData->chains.size()) return 0;
	return mData->chains[index];
}

const GlChain* GlAbstractNode::FindChain(gl_chain_id id, bool recurse) const
{
	ArpVALIDATE(mData, return 0);
	ArpVALIDATE(id, return 0);
	uint32					k;
	for (k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k] && id == mData->chains[k]->Id()) return mData->chains[k];
	}
	if (!recurse) return 0;
	for (k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k]) {
			const GlChain*	c = mData->chains[k]->FindChain(id);
			if (c) return c;
		}
	}
	return 0;	
}

GlChain* GlAbstractNode::FindChain(gl_chain_id id, bool recurse)
{
	ArpVALIDATE(mData, return 0);
	ArpVALIDATE(id, return 0);
	uint32					k;
	for (k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k] && id == mData->chains[k]->Id()) return mData->chains[k];
	}
	if (!recurse) return 0;
	for (k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k]) {
			GlChain*		c = mData->chains[k]->FindChain(id);
			if (c) return c;
		}
	}
	return 0;	
}

const GlChain* GlAbstractNode::FindChain(int32 key) const
{
	ArpASSERT(mData);
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k] && key == mData->chains[k]->Key())
			return mData->chains[k];
	}
	return 0;
}

GlChain* GlAbstractNode::FindChain(int32 key)
{
	ArpASSERT(mData);
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k] && key == mData->chains[k]->Key())
			return mData->chains[k];
	}
	return 0;
}

const GlNode* GlAbstractNode::FindNode(gl_chain_id cid, gl_node_id nid, bool recurse) const
{
	ArpVALIDATE(nid, return 0);
	if (cid) {
		const GlChain*		chain = FindChain(cid, recurse);
		if (chain) return chain->FindNode(nid, false);
		return 0;
	}
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		const GlNode*		node = mData->chains[k]->FindNode(nid, recurse);
		if (node) return node;
	}
	return 0;
}

GlNode* GlAbstractNode::FindNode(gl_chain_id cid, gl_node_id nid, bool recurse)
{
	ArpVALIDATE(nid, return 0);
	if (cid) {
		GlChain*			chain = FindChain(cid, recurse);
		if (chain) return chain->FindNode(nid, false);
		return 0;
	}
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		GlNode*				node = mData->chains[k]->FindNode(nid, recurse);
		if (node) return node;
	}
	return 0;
}

BView* GlAbstractNode::NewView(gl_new_view_params& params) const
{
	return 0;
}

status_t GlAbstractNode::ReadFrom(const BMessage& config)
{
	ArpVALIDATE(mData, return B_NO_MEMORY);
	mData->MakeEmpty();
	BMessage				msg;
	status_t				err;
	for (int32 k = 0; config.FindMessage(CHAIN_STR, k, &msg) == B_OK; k++) {
		GlChain*			chain = new GlChain(&msg);
		if (!chain) return B_NO_MEMORY;
		chain->IncRefs();
		if ((err = chain->Status()) == B_OK) mData->chains.push_back(chain);
		else chain->DecRefs();
		if (err != B_OK) return err;
			
		msg.MakeEmpty();
	}
	return B_OK;
}

status_t GlAbstractNode::WriteTo(BMessage& config) const
{
	ArpVALIDATE(mData, return B_NO_MEMORY);
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k]) {
			BMessage	msg;
			status_t	err = mData->chains[k]->WriteTo(msg);
			if (err != B_OK) return err;
			err = config.AddMessage(CHAIN_STR, &msg);
			if (err != B_OK) return err;
		}
	}
	return B_OK;
}

void GlAbstractNode::MakeEmpty()
{
	if (mData) mData->MakeEmpty();
}

GlAlgo* GlAbstractNode::GenerateChainAlgo(int32 key, const gl_generate_args& args) const
{
	const GlChain*		chain = FindChain(key);
	if (!chain) return 0;
	return chain->Generate(args);
}

GlAlgo1d* GlAbstractNode::GenerateChain1d(int32 key, const gl_generate_args& args) const
{
	GlAlgo*				a = GenerateChainAlgo(key, args);
	GlAlgo1d*			a1d = (a) ? a->As1d() : 0;
	if (a1d) return a1d;
	delete a;
	return 0;
}

GlAlgo2d* GlAbstractNode::GenerateChain2d(int32 key, const gl_generate_args& args) const
{
	GlAlgo*				a = GenerateChainAlgo(key, args);
	GlAlgo2d*			a2d = (a) ? a->As2d() : 0;
	if (a2d) return a2d;
	delete a;
	return 0;
}

GlChain* GlAbstractNode::VerifyChain(GlChain* chain, bool* added)
{
	ArpVALIDATE(chain, return 0);
	if (added) *added = false;
	chain->IncRefs();
	ArpASSERT(mData);
	if (!mData) {
		chain->DecRefs();
		return 0;
	}
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k] && chain->Key() == mData->chains[k]->Key()) {
			ArpASSERT(mData->chains[k]->Io() == chain->Io());
			mData->chains[k]->mLabel = chain->mLabel;
			chain->DecRefs();
			return mData->chains[k];
		}
	}
	mData->chains.push_back(chain);
	if (added) *added = true;
	return chain;	
}

status_t GlAbstractNode::AddChain(GlChain* chain)
{
	ArpVALIDATE(mData && chain, return B_ERROR);
	chain->IncRefs();
	mData->chains.push_back(chain);
	return B_OK;
}

status_t GlAbstractNode::RemoveChain(gl_chain_id cid)
{
	ArpVALIDATE(mData, return B_ERROR);
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k] && cid == mData->chains[k]->Id()) {
			mData->chains[k]->MakeEmpty();
			mData->chains[k]->DecRefs();
			mData->chains.erase(mData->chains.begin() + k);
		}
	}
	return B_OK;
}

void GlAbstractNode::SetParentHack(GlNode* node)
{
	ArpVALIDATE(mData, return);
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		if (mData->chains[k]) mData->chains[k]->SetParent(node);
	}
}

void GlAbstractNode::PrintChains(uint32 tabs) const
{
	if (!mData) {
		for (uint32 t = 0; t < tabs; t++) printf("\t");
		printf("<error> no chains\n");
	}
	for (uint32 k = 0; k < mData->chains.size(); k++) {
		mData->chains[k]->Print(tabs);
	}
}

// #pragma mark -

/*************************************************************************
 * GL-NODE
 *************************************************************************/
GlNode::GlNode(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(0), mParent(0), mParams(Id(), &(addon->ParamTypes())),
		  mImage(0), mBaseAddon(addon), mTags(0)
		 
{
	if (config) ReadFrom(*config);
}

GlNode::GlNode(const GlNode& o)
		: inherited(o), mParent(0), mParams(o.mParams, Id()),
		  mImage(0), mBaseAddon(o.mBaseAddon), mTags(o.mTags)
{
	SetParentHack(this);
}

GlNode::~GlNode()
{
	SetParent(0);
	delete mImage;
}

const GlNodeAddOn* GlNode::AddOn() const
{
	return mBaseAddon;
}

uint32 GlNode::Io() const
{
	if (mBaseAddon) return mBaseAddon->Io();
	ArpASSERT(false);
	return GL_IMAGE_IO;
}

uint32 GlNode::Flags() const
{
	if (mBaseAddon) return mBaseAddon->Flags();
	return 0;
}

uint32 GlNode::Tags() const
{
	return mTags;
}

void GlNode::SetTags(uint32 tags)
{
	mTags = tags;
}

const GlChain* GlNode::Parent() const
{
	return mParent;
}

GlChain* GlNode::Parent()
{
	return mParent;
}

const GlParamList& GlNode::Params() const
{
	return mParams;
}

GlParamList& GlNode::Params()
{
	return mParams;
}

status_t GlNode::SetMapParam(	const GlAlgo1d* map, int32 index,
								const char* name)
{
	return B_ERROR;
}

const GlParamStrainer& GlNode::ParamStrainer() const
{
	return mStrainer;
}

GlParamStrainer& GlNode::ParamStrainer()
{
	return mStrainer;
}

status_t GlNode::ReadFrom(const BMessage& config)
{
	status_t		err = inherited::ReadFrom(config);
	SetParentHack(this);

	mParams.ReadFrom(config);

	int32				i32;
	if (config.FindInt32(TAGS_STR, &i32) == B_OK) mTags = i32;
	/* Read the strainer
	 */
	BMessage			sm;
	if (config.FindMessage(STRAINER_STR, &sm) == B_OK) mStrainer.ReadNode(this, sm);
	sm.MakeEmpty();
	int32				msgIndex = 0;
	while (config.FindMessage(CHAIN_STRAIN_STR, msgIndex, &sm) == B_OK) {
		if (sm.FindInt32(STRAINER_I_STR, &i32) == B_OK) {
			GlChain*	c = ChainAt(i32);
			if (c) c->ReadFrom(sm, mStrainer, true);
		}
		sm.MakeEmpty();
		msgIndex++;
	}
	
	return err;
}

status_t GlNode::WriteTo(BMessage& config) const
{
	ArpVALIDATE(mBaseAddon, return B_ERROR);
	if (mBaseAddon->mCreator.Length() < 1) return B_ERROR;
	status_t		err = inherited::WriteTo(config);
	if (err != B_OK) return err;
	if ((err = config.AddString16(GL_NODE_CREATOR_STR, mBaseAddon->mCreator)) != B_OK) return err;
	if ((err = config.AddInt32(GL_NODE_KEY_STR, mBaseAddon->mKey)) != B_OK) return err;
/* FIX: This is really handy for debugging, but I don't know if I want it
 * cluttering things up when it ships.  On the flip side, it's a good way
 * to communicate to the user when something goes wrong and a node can't
 * be found.
 */
if (mBaseAddon->mLabel.String()) config.AddString16("label", mBaseAddon->mLabel);
//debugger("sds");
	if ((err = mParams.WriteTo(config)) != B_OK) return err;

	if ((err = config.AddInt32(TAGS_STR, mTags)) != B_OK) return err;
	/* Write the strainer
	 */
	if (mStrainer.Size() > 0) {
		BMessage			sm;
		if (mStrainer.WriteNode(this, sm) == B_OK) config.AddMessage(STRAINER_STR, &sm);
		uint32				s = ChainSize();
		for (uint32 k = 0; k < s; k++) {
			const GlChain*	c = ChainAt(k);
			if (c) {
				sm.MakeEmpty();
				if (c->WriteTo(sm, mStrainer, true) == B_OK) {
					sm.AddInt32(STRAINER_I_STR, k);
					config.AddMessage(CHAIN_STRAIN_STR, &sm);
				}
			}
		}
	}
	return B_OK;
}

BView* GlNode::NewView(gl_new_view_params& params) const
{
	if (params.viewType == GL_INSPECTOR_VIEW)
		return NewInspectorView(params);
	else if (params.viewType == GL_CONFIG_VIEW)
		return NewConfigView(params);
	return inherited::NewView(params);
}

status_t GlNode::GetParams(GlParamListI& list, uint32 flags) const
{
	if (flags&GL_STRAIN_PARAMS) GetConfigParams(list);
	else GetInspectParams(list);
	/* Add in all params from nodes I own.
	 */
	_GlStrainList		strainList;
	/* Add in any params from my chains.
	 */
	uint32				s = ChainSize();
	for (uint32 k = 0; k < s; k++) {
		const GlChain*	c = ChainAt(k);
		if (c) c->StrainParams(&strainList);
	}
	status_t			err = strainList.Install(list);
	if (err != B_OK) return err;

	return B_OK;
}

status_t GlNode::ParamChanged(gl_param_key key)
{
	if (mParent) mParent->ParamChanged(key);
	return B_OK;
}

status_t GlNode::ChainChanged(gl_chain_id id, int32 dynamic)
{
	if (mParent) mParent->ChainChanged(id, 0);
	/* The dynamic value is a convenience supplied by the
	 * chain that changed -- it's just letting us know the
	 * change was to a dynamic chain, so we can run through
	 * and see if any chains need to be added or removed.
	 * The rule is to always keep 1 empty chain for the given
	 * type.
	 */
	if (dynamic > 0) {
		uint32			size = ChainSize(), count = 0, emptyCount = 0;
		GlChain*		first = 0;
		GlChain*		lastEmpty = 0;
		for (uint32 k = 0; k < size; k++) {
			GlChain*	c = ChainAt(k);
			if (c && dynamic == c->Dynamic()) {
				if (!first) first = c;
				c->SetDynamicCount(count);
				count++;
				if (c->NodeAt(0) == 0) {
					emptyCount++;
					lastEmpty = c;
				}
			}
		}
		/* There must always be at least one, since that's the
		 * pattern I'll base any new ones on.
		 */
		ArpASSERT(first);
		if (emptyCount == 0) {
			BString16		lbl(first->Label());
			GlChain*		c = new GlChain(first->Key(), first->Io(),
											&lbl, this, dynamic);
			if (c) {
				c->SetDynamicCount(count);
				AddChain(c);			
				ChainChanged(c->Id(), 0);
			}
		} else if (emptyCount > 1 && lastEmpty) {
			gl_chain_id		cid = lastEmpty->Id();
			RemoveChain(cid);
			ChainChanged(cid, 0);
		}
	}
	return B_OK;
}

GlNodeVisual* GlNode::NewVisual(const GlRootRef& ref) const
{
	return 0;
}

GlRecorder* GlNode::NewRecorder(const GlRootRef& ref)
{
	return 0;
}

void GlNode::MakeEmpty()
{
	SetParent(0);
	inherited::MakeEmpty();
}

const ArpBitmap* GlNode::Image() const
{
	if (mImage) return mImage;
	if (mBaseAddon) return mBaseAddon->Image();
	
	mImage = GlGlobals().CloneBitmap(GL_NODE_IMAGE_ID);
	return mImage;
}

float GlNode::GetMatch(	const BString16& creator, int32 key,
						int32* major, int32* minor) const
{
	if (mBaseAddon) return mBaseAddon->GetMatch(creator, key, major, minor);
	return 0.0;
}

status_t GlNode::GetInspectParams(GlParamListI& list) const
{
	const GlNodeAddOn*		addon = AddOn();
	ArpVALIDATE(addon, return B_ERROR);

	const GlParamTypeList&	paramTypes = addon->ParamTypes();
	const GlParamType*		pt;
	gl_param_key			key(Id(), 0, 0);
	for (uint32 k = 0; (pt = paramTypes.At(k)) != 0; k++) {
		key.key = pt->Key();
		bool				added = false;
		const GlParam*		p = 0;
		int32				midi;
		for (uint32 pk = 0; (p = mParams.Find(key.key, pk)) != 0; pk++) {
			key.index = pk;
			if (mStrainer.NewGetAt(key, 0, 0, 0, &midi) == B_OK) {
				list.Add(key.nid, p, key.index, 0, GL_CONTROL_ON, midi);
			} else list.Add(key.nid, p, key.index, 0, GL_CONTROL_ON, GL_MIDI_OFF);
			added = true;
		}
		if (!added) list.Add(key.nid, pt, 0, GL_CONTROL_ON, GL_MIDI_OFF);
	}
	return B_OK;
}

status_t GlNode::GetConfigParams(GlParamListI& list) const
{
	const GlNodeAddOn*		addon = AddOn();
	ArpVALIDATE(addon, return B_ERROR);

	const GlParamTypeList&	paramTypes = addon->ParamTypes();
	const GlParamType*		pt;
	gl_param_key			key(Id(), 0, 0);
	for (uint32 k = 0; (pt = paramTypes.At(k)) != 0; k++) {
		key.key = pt->Key();
		bool				added = false;
		const GlParam*		p = 0;
		const BString16*	label = NULL;
		int32				control, midi;
		for (uint32 pk = 0; (p = mParams.Find(key.key, pk)) != 0; pk++) {
			key.index = pk;
			if (mStrainer.NewGetAt(key, 0, &label, &control, &midi) == B_OK) {
				list.Add(key.nid, p, key.index, label, control, midi);
			} else list.Add(key.nid, p, key.index, 0, GL_CONTROL_OFF, GL_MIDI_OFF);
			added = true;
		}
		if (!added) list.Add(key.nid, pt, 0, GL_CONTROL_OFF, GL_MIDI_OFF);
	}
	return B_OK;
}

status_t GlNode::StrainParams(_GlStrainList* strainer) const
{
	ArpVALIDATE(strainer, return B_ERROR);
	const GlNodeAddOn*		addon = AddOn();
	ArpVALIDATE(addon, return B_ERROR);
	/* Add in any params from my chains.
	 */
	uint32						s = ChainSize();
	for (uint32 k = 0; k < s; k++) {
		const GlChain*			c = ChainAt(k);
		if (c) c->StrainParams(strainer);
	}
	/* Run the strainer -- this will load any params from my
	 * param list, and turn off any from other nodes that the
	 * strainer says are off.
	 */
	if (!strainer) return B_ERROR;
	return mStrainer.Load(Id(), *strainer, mParams, addon->ParamTypes());
}

void GlNode::SetParent(GlChain* chain)
{
	if (mParent) mParent->DecRefs();
	mParent = chain;
	if (mParent) mParent->IncRefs();
}

BView* GlNode::NewInspectorView(const gl_new_view_params& params) const
{
	ArpVALIDATE(mParent, return 0);
	GlStrainedParamList		list;
	if (GetParams(list) != B_OK) return 0;

	GlParamView*			v = new GlParamView(params, mParent->Id(), Id());
	if (!v) return 0;

	BRect					r(2, 2, 150, 2);
	GlNodeVisual*			visual = NewVisual(params.ref);
	if (visual) {
		r.bottom = r.top + 50;
		v->AddVisualView(r, "vis", visual);
		r.top = r.bottom + 2;
	}
	v->AddParamControls(list, &r);
	return v;
}

BView* GlNode::NewConfigView(const gl_new_view_params& params) const
{
	GlStrainedParamList		list;
	list.SetFlags(list.NO_DEFAULT_LABEL_F);
	if (GetParams(list, GL_STRAIN_PARAMS) != B_OK) return 0;

	if (list.Size() < 1) return 0;
	return new GlConfigView(params.frame, params.ref, *this, list);
}

void GlNode::Print(uint32 tabs) const
{
	for (uint32 t = 0; t < tabs; t++) printf("\t");
	const GlNodeAddOn*	addon = AddOn();
	if (!addon) printf("Node <no addon>\n");
	else printf("Node %s:%ld %s\n", addon->Creator().String(),
									addon->Key(),
									addon->Label().String());
	PrintChains(tabs + 1);
}

// #pragma mark -

/*************************************************************************
 * GL-NODE-ADD-ON
 *************************************************************************/
GlNodeAddOn::GlNodeAddOn(	const BString16& creator, int32 key,
							const BString16* category, const BString16* label,
							int32 majorVersion, int32 minorVersion)
		: mKey(key), mMajor(majorVersion), mMinor(minorVersion),
		  mImage(0)
{
	mCreator = creator;
	if (category) mCategory = *category;
	if (label) mLabel = *label;
}

GlNodeAddOn::~GlNodeAddOn()
{
	delete mImage;
}

#if 0
status_t GlNodeAddOn::Archive(BMessage& config) const
{
	if (mCreator.Length() < 1 || mName.Length() < 1) return B_ERROR;
	config.AddString("creator", mCreator);
	config.AddInt32(GL_KEY_STR, mKey);
	config.AddString("name", mName);
	config.AddInt32("Mv", mMajor);
	config.AddInt32("mv", mMinor);
	return B_OK;	
}
#endif

gl_node_add_on_id GlNodeAddOn::Id() const
{
	return (void*)this;
}

uint32 GlNodeAddOn::Flags() const
{
	return 0;
}

BString16 GlNodeAddOn::Creator() const
{
	return mCreator;
}

int32 GlNodeAddOn::Key() const
{
	return mKey;
}

BString16 GlNodeAddOn::Category() const
{
	return mCategory;
}

BString16 GlNodeAddOn::Label() const
{
	return mLabel;
}

void GlNodeAddOn::GetVersion(int32* major, int32* minor) const
{
	if (major) *major = mMajor;
	if (minor) *minor = mMinor;
}

const GlParamTypeList& GlNodeAddOn::ParamTypes() const
{
	return mParamTypes;
}

const ArpBitmap* GlNodeAddOn::Image() const
{
	if (mImage) return mImage;
	GlImage*		img = NewImage();
	if (!img) return 0;
	mImage = img->AsBitmap();
	delete img;
	return mImage;
}

GlAlgo* GlNodeAddOn::Generate(const gl_generate_args& args) const
{
	GlNode*				n = NewInstance(0);
	if (!n) return 0;
	n->IncRefs();
	GlAlgo*				a = n->Generate(args);
	n->MakeEmpty();
	n->DecRefs();
	return a;
}

float GlNodeAddOn::GetMatch(const BString16& creator,
							int32 key, int32* major, int32* minor) const
{
	if (mCreator != creator) return 0.0f;
	if (mKey != key) return 0.0f;
	return 1.0f;
}

const GlParamType* GlNodeAddOn::AddParamType(GlParamType* paramType)
{
	ArpVALIDATE(paramType, return 0);
#if 0
if (mParamTypes.Find(paramType->Key()) != 0) {
	printf("Dup keys for %s\n", mLabel.String());
	printf("Existing: ");
		mParamTypes.Find(paramType->Key())->Print();
	printf("Duplicate: "); paramType->Print();
}
#endif
	ArpASSERT(mParamTypes.Find(paramType->Key()) == 0);
	mParamTypes.Add(paramType);
	return paramType;
}

GlImage* GlNodeAddOn::NewImage() const
{
	GlNode*		n = NewInstance(0);
	if (!n) return GlGlobals().CloneImage(GL_NODE_IMAGE_ID);
#if 0
if (mLabel == "Outline") {
	n->Print();
//debugger("sdsd");
}
#endif

	n->IncRefs();
	GlImage*	img = gl_new_node_image(n);
	n->MakeEmpty();
	n->DecRefs();
#if 0
if (mLabel == "Outline") {
	ArpBitmap*	bm = (img) ? img->AsBitmap() : 0;
	if (bm) bm->Save("/boot/home/tmp/ol.png", ARP_PNG_FORMAT);
	delete bm;
}
#endif
	return img;
}

// #pragma mark -

/*************************************************************************
 * GL-NEW-VIEW-PARAMS
 *************************************************************************/

gl_new_view_params::gl_new_view_params()
	: size(sizeof(gl_new_view_params)), flags(0), viewType(0),
	  frame(0, 0, 0, 0), channel(0)
{
}

gl_new_view_params::gl_new_view_params(const gl_new_view_params& o)
	: size(sizeof(gl_new_view_params)), flags(o.flags),
	  viewType(o.viewType), frame(o.frame), ref(o.ref),
	  channel(o.channel)
{
}

gl_new_view_params::~gl_new_view_params()
{
}

gl_new_view_params& gl_new_view_params::operator=(const gl_new_view_params& o)
{
	if (this != &o) {
		size = sizeof(gl_new_view_params);
		flags = o.flags;
		viewType = o.viewType;
		frame = o.frame;
		ref = o.ref;
		channel = o.channel;
	}
	return *this;
}
