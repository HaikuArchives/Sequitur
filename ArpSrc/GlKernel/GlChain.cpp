#include <ArpCore/StlVector.h>
#include <GlPublic/GlActions.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlNodeReaderWriter.h>
#include <GlPublic/GlAlgo.h>
#include <GlKernel/GlDefs.h>

static const char*		KEY_STR				= "key";
static const char*		TYPE_STR			= "type";
static const char*		DYNAMIC_STR			= "dyn";
static const char*		DYNAMIC_COUNT_STR	= "dync";
static const char*		NODE_MSG			= "node";

// GL-CHAIN-PRIVATE
class GlChainPrivate
{
public:
	vector<GlNode*>	nodes;

	GlChainPrivate();
	~GlChainPrivate();

	void		MakeEmpty();
};

/***************************************************************************
 * GL-CHAIN
 ****************************************************************************/
GlChain::GlChain(	int32 key, uint32 io, const BString16* label,
					GlNode* parent, int32 dynamic)
		: mData(new GlChainPrivate()), mStatus(B_OK),
		  mKey(key), mIo(io), mParent(0), mDynamic(dynamic),
		  mDynamicCount(0)
{
	if (label) mLabel = *label;
	SetParent(parent);
	if (!mData) mStatus = B_NO_MEMORY;
}

GlChain::GlChain(const BMessage* config)
		: mData(new GlChainPrivate()), mStatus(B_OK), mKey(0),
		  mIo(0), mParent(0), mDynamic(0), mDynamicCount(0)
{
	if (!mData) {
		mStatus = B_NO_MEMORY;
		return;
	}
	if (config) mStatus = ReadFrom(*config);
}

GlChain::GlChain(const GlChain& o)
		: mLabel(o.mLabel), mData(new GlChainPrivate()), mStatus(B_OK),
		  mKey(o.mKey), mIo(o.mIo), mParent(0), mDynamic(o.mDynamic),
		  mDynamicCount(o.mDynamicCount)
{
	if (!mData) mStatus = B_NO_MEMORY;
	if (mData && o.mData) {
		for (uint32 k = 0; k < o.mData->nodes.size(); k++) {
			GlNode*		n = o.mData->nodes[k]->Clone();
			if (n) {
				n->IncRefs();
				n->SetParent(this);
				mData->nodes.push_back(n);
			}
		}
	}
}

GlChain::~GlChain()
{
	SetParent(0);
	delete mData;
}

status_t GlChain::Status() const
{
	if (!mData) return B_NO_MEMORY;
	return mStatus;
}

GlChain* GlChain::Clone() const
{
	return new GlChain(*this);
}

int32 GlChain::Key() const
{
	return mKey;
}

gl_chain_id GlChain::Id() const
{
	return (void*)this;
}

uint32 GlChain::Io() const
{
	return mIo;
}

BString16 GlChain::Label(bool appendDynamicCount) const
{
	if (appendDynamicCount && mDynamic > 0) {
		BString16		l(mLabel);
		l << " " << int32(mDynamicCount + 1);
		return l;
	}
	return mLabel;
}

const GlNode* GlChain::Parent() const
{
	return mParent;
}

int32 GlChain::Dynamic() const
{
	return mDynamic;
}

void GlChain::SetDynamicCount(uint32 dynCount)
{
	mDynamicCount = dynCount;
}

uint32 GlChain::NodeCount() const
{
	ArpVALIDATE(mData, return 0);
	return uint32(mData->nodes.size());
}

const GlNode* GlChain::NodeAt(uint32 index) const
{
	ArpVALIDATE(mData, return 0);
	if (index < mData->nodes.size()) return mData->nodes[index];
	return 0;
}

GlNode* GlChain::NodeAt(uint32 index)
{
	ArpVALIDATE(mData, return 0);
	if (index < mData->nodes.size()) return mData->nodes[index];
	return 0;
}

const GlChain* GlChain::FindChain(gl_chain_id id) const
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k]) {
			const GlChain*		c = mData->nodes[k]->FindChain(id);
			if (c) return c;
		}
	}
	return 0;
}

GlChain* GlChain::FindChain(gl_chain_id id)
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k]) {
			GlChain*		c = mData->nodes[k]->FindChain(id);
			if (c) return c;
		}
	}
	return 0;
}

const GlNode* GlChain::FindNode(gl_node_id nid, bool recurse) const
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k]) {
			if (nid == mData->nodes[k]->Id()) return mData->nodes[k];
			if (recurse) {
				const GlNode*	n = mData->nodes[k]->FindNode(0, nid, recurse);
				if (n) return n;
			}
		}
	}
	return 0;
}

GlNode* GlChain::FindNode(gl_node_id nid, bool recurse)
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k]) {
			if (nid == mData->nodes[k]->Id()) return mData->nodes[k];
			if (recurse) {
				GlNode*			n = mData->nodes[k]->FindNode(0, nid, recurse);
				if (n) return n;
			}
		}
	}
	return 0;
}

status_t GlChain::AddNode(GlNode* node)
{
	ArpVALIDATE(node, return B_ERROR);
	node->IncRefs();
	if (!mData) {
		node->DecRefs();
		return B_NO_MEMORY;
	}
	mData->nodes.push_back(node);
	node->SetParent(this);
	ChainChanged(Id(), mDynamic);
	return B_OK;
}

status_t GlChain::InsertNode(GlNode* node, int32 index)
{
	if (index < 0 || index >= int32(mData->nodes.size()))
		return AddNode(node);
	ArpVALIDATE(node, return B_ERROR);
	node->IncRefs();
	if (!mData) {
		node->DecRefs();
		return B_NO_MEMORY;
	}
	mData->nodes.insert(mData->nodes.begin() + index, node);
	node->SetParent(this);
	ChainChanged(Id(), mDynamic);
	return B_OK;
}

status_t GlChain::ReplaceNode(GlNode* node, int32 index)
{
	ArpVALIDATE(node, return B_ERROR);
	node->IncRefs();
	if (mData && index >= 0 && index < int32(mData->nodes.size())) {
		if (mData->nodes[index]) {
			mData->nodes[index]->MakeEmpty();
			mData->nodes[index]->DecRefs();
		}
		mData->nodes[index] = node;
		ChainChanged(Id(), mDynamic);
		return B_OK;
	}
	node->DecRefs();
	return B_ERROR;
}

status_t GlChain::RemoveNode(gl_node_id nid)
{
	ArpVALIDATE(mData, return 0);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k] && nid == mData->nodes[k]->Id()) {
			GlNode*		n = mData->nodes[k];
			mData->nodes.erase(mData->nodes.begin() + k);
			n->MakeEmpty();
			n->DecRefs();
			ChainChanged(Id(), mDynamic);
			return B_OK;
		}
	}
	return B_ERROR;	
}

void GlChain::MakeEmpty()
{
	SetParent(0);
	if (mData) mData->MakeEmpty();
	ChainChanged(Id(), mDynamic);
}

status_t GlChain::DeleteNodes()
{
	if (!mData) return B_OK;
	mData->MakeEmpty();
// I think this is a special method, and currently doesn't want to
// send notification.  An argument should be added.
//	ChainChanged(Id(), mDynamic);
	return B_OK;
}

int32 GlChain::Walk(GlNodeAction& action, bool recurse)
{
	ArpVALIDATE(mData, return GL_CONTINUE);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		GlNode*					n = mData->nodes[k];
		ArpASSERT(n);
		int32					ans = action.Perform(n);
		if (ans != GL_CONTINUE) return ans;
		if (recurse) {
			uint32				s = n->ChainSize();
			for (uint32 k2 = 0; k2 < s; k2++) {
				GlChain*		c = n->ChainAt(k2);
				if (c) {
					ans = c->Walk(action, recurse);
					if (ans != GL_CONTINUE) return ans;
				}
			}				
		}
	}
	return GL_CONTINUE;
}

int32 GlChain::Walk(GlNodeAction& action, bool recurse) const
{
	ArpVALIDATE(mData, return GL_CONTINUE);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		const GlNode*			n = mData->nodes[k];
		ArpASSERT(n);
		int32					ans = action.Perform(n);
		if (ans != GL_CONTINUE) return ans;
		if (recurse) {
			uint32				s = n->ChainSize();
			for (uint32 k2 = 0; k2 < s; k2++) {
				const GlChain*	c = n->ChainAt(k2);
				if (c) {
					ans = c->Walk(action, recurse);
					if (ans != GL_CONTINUE) return ans;
				}
			}				
		}
	}
	return GL_CONTINUE;
}

status_t GlChain::StrainParams(_GlStrainList* strainer) const
{
	ArpVALIDATE(mData, return B_ERROR);
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		status_t	err = mData->nodes[k]->StrainParams(strainer);
		if (err != B_OK) return err;
	}
	return B_OK;
}

status_t GlChain::ParamChanged(gl_param_key key)
{
	ArpASSERT(mParent);
	if (mParent) mParent->ParamChanged(key);
	return B_OK;
}

status_t GlChain::ChainChanged(gl_chain_id id, int32 dynamic)
{
	if (mParent) mParent->ChainChanged(id, dynamic);
	return B_OK;
}

GlAlgo* GlChain::Generate(const gl_generate_args& args) const
{
	ArpVALIDATE(mData, return 0);
	GlAlgo*			head = 0;
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		GlAlgo*		a = mData->nodes[k]->Generate(args);
		if (a) {
			if (!head) head = a;
			else head->AddTail(a);
		}
	}
	if (!head) return 0;
	return head->Parse();
}

status_t GlChain::ReadFrom(const BMessage& config)
{
	ArpVALIDATE(mData, return 0);
	status_t				err;
	int32					i32;
	if (config.FindInt32(KEY_STR, &i32) == B_OK) mKey = i32;
	else mKey = 0;
	if (config.FindInt32(TYPE_STR, &i32) == B_OK) mIo = i32;
	else mIo = 0;
	if (config.FindInt32(DYNAMIC_STR, &i32) == B_OK) mDynamic = i32;
	else mDynamic = 0;
	if (config.FindInt32(DYNAMIC_COUNT_STR, &i32) == B_OK) mDynamicCount = i32;
	else mDynamicCount = 0;
	
	BMessage				msg;
	for (int32 k = 0; config.FindMessage(NODE_MSG, k, &msg) == B_OK; k++) {
		BString16			creator;
		int32				key;
		if ((err = msg.FindString(GL_NODE_CREATOR_STR, &creator)) != B_OK) return err;
		if ((err = msg.FindInt32(GL_NODE_KEY_STR, &key)) != B_OK) return err;
		const GlNodeAddOn*	addon = GlGlobals().GetAddOn(creator, key);
		/* FIX:  Actually, this should pull label info out and add
		 * an error node.
		 */
		if (!addon) {
			printf("**ERROR no addon %s:%ld\n", creator.String(), key);
			return B_ERROR;
		}
		GlNode*			node = addon->NewInstance(&msg);
		if (!node) return B_NO_MEMORY;
		node->IncRefs();
		err = AddNode(node);
		node->DecRefs();
		if (err != B_OK) return err;

		msg.MakeEmpty();
	}
	return B_OK;
}

status_t GlChain::WriteTo(BMessage& config) const
{
	ArpVALIDATE(mData, return 0);
	status_t			err;
	if ((err = config.AddInt32(KEY_STR, mKey)) != B_OK) return err;
	if ((err = config.AddInt32(TYPE_STR, mIo)) != B_OK) return err;
	if (mDynamic > 0) {
		if ((err = config.AddInt32(DYNAMIC_STR, mDynamic)) != B_OK) return err;
		if ((err = config.AddInt32(DYNAMIC_COUNT_STR, mDynamicCount)) != B_OK) return err;
	}
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k]) {
			BMessage	msg;
			err = mData->nodes[k]->WriteTo(msg);
			if (err != B_OK) return err;
			err = config.AddMessage(NODE_MSG, &msg);
			if (err != B_OK) return err;
		}
	}
	return B_OK;
}

class GlChainNodeReader : public GlNodeAction
{
public:
	const BMessage&				msg;
	GlNodeReaderWriter&			rw;
	int32						targetIndex;
	
	GlChainNodeReader(BMessage& inMsg, GlNodeReaderWriter& inRw, int32 inTargetIndex)
			: msg(inMsg), rw(inRw), targetIndex(inTargetIndex), mCurIndex(-1)	{ }

	virtual int32		Perform(GlNode* n)
	{
		mCurIndex++;
		if (mCurIndex != targetIndex) return GL_CONTINUE;
		rw.ReadNode(n, msg);
		return GL_STOP_OPERATION;
	}

private:
	int32						mCurIndex;
};

status_t GlChain::ReadFrom(	const BMessage& config, GlNodeReaderWriter& rw,
							bool recurse)
{
	BMessage					msg;
	int32						msgIndex = 0;
	while (config.FindMessage(GL_NMSG_STR, msgIndex, &msg) == B_OK) {
		int32					targetIndex;
		if (msg.FindInt32(GL_NI_STR, &targetIndex) == B_OK) {
			ArpASSERT(targetIndex >= 0);
			GlChainNodeReader	action(msg, rw, targetIndex);
			Walk(action, recurse);
		}
		msg.MakeEmpty();
		msgIndex++;
	}
	return B_OK;
}

class GlChainNodeWriter : public GlNodeAction
{
public:
	BMessage&					config;
	const GlNodeReaderWriter&	rw;
	int32						curIndex;
	
	GlChainNodeWriter(BMessage& inConfig, const GlNodeReaderWriter& inRw)
			: config(inConfig), rw(inRw), curIndex(-1)	{ }

	virtual int32		Perform(const GlNode* n)
	{
		curIndex++;
		BMessage		msg;
		if (rw.WriteNode(n, msg) == B_OK) {
			msg.AddInt32(GL_NI_STR, curIndex);
			config.AddMessage(GL_NMSG_STR, &msg);
		}
		return GL_CONTINUE;
	}
};

status_t GlChain::WriteTo(	BMessage& config, const GlNodeReaderWriter& rw,
							bool recurse) const
{
	GlChainNodeWriter	action(config, rw);
	Walk(action, recurse);
	return B_OK;
}

void GlChain::SetParent(GlNode* node)
{
	if (mParent) mParent->DecRefs();
	mParent = node;
	if (mParent) mParent->IncRefs();
}

void GlChain::Print(uint32 tabs) const
{
	uint32		t;
	for (t = 0; t < tabs; t++) printf("\t");
	printf("GlChain '%s' type %ld %p ", mLabel.String(), mIo, this);
	if (!mData) {
		printf("<empty>\n");
		return;
	} else printf("size %ld\n", mData->nodes.size());
	for (uint32 k = 0; k < mData->nodes.size(); k++) {
		if (mData->nodes[k]) mData->nodes[k]->Print(tabs + 1);
	}	
}

// #pragma mark -

/***************************************************************************
 * GL-CHAIN-PRIVATE
 ****************************************************************************/
GlChainPrivate::GlChainPrivate()
{
}

GlChainPrivate::~GlChainPrivate()
{
	MakeEmpty();
}

void GlChainPrivate::MakeEmpty()
{
	for (uint32 k = 0; k < nodes.size(); k++) {
		if (nodes[k]) {
			nodes[k]->MakeEmpty();
			nodes[k]->DecRefs();
		}
	}
	nodes.resize(0);
}
