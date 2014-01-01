#include <be/interface/Window.h>
#include <ArpMath/ArpDefs.h>
#include <ArpInterface/ArpPrefs.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlRootNode.h>
#include <Glasslike/GlDefs.h>
#include <Glasslike/GlProjectList.h>

class _GlProjectEntry
{
public:
	GlRootRef		ref;
	
	_GlProjectEntry(const GlRootRef& r) : ref(r)	{ }
};

/*************************************************************************
 * GL-PROJECT-LIST
 *************************************************************************/
GlProjectList::GlProjectList()
		: mDirty(true), mCurrent(-1)
{
}

GlProjectList::~GlProjectList()
{
	FreeList();
	FreePath();
}

GlRootRef GlProjectList::At(uint32 index) const
{
	if (index >= mList.size() || !mList[index]) return GlRootRef();
	return mList[index]->ref;
}

GlRootRef GlProjectList::At(gl_id id) const
{
	for (uint32 k = 0; k < mList.size(); k++) {
		if (mList[k] && id == mList[k]->ref.NodeId()) {
			return mList[k]->ref;
		}
	}
	return GlRootRef();
}

uint32 GlProjectList::Size() const
{
	return uint32(mList.size());
}

GlRootRef GlProjectList::Current() const
{
	if (mCurrent < 0 || mCurrent >= int32(mList.size()) || !mList[mCurrent])
		return GlRootRef();
	return mList[mCurrent]->ref;
}

status_t GlProjectList::SetCurrent(gl_id id)
{
	if (mCurrent >= 0 && mCurrent < int32(mList.size())) {
		if (mList[mCurrent]) mList[mCurrent]->ref.RemoveAll();
	}
	FreePath();

	for (uint32 k = 0; k < mList.size(); k++) {
		if (mList[k] && id == mList[k]->ref.NodeId()) {
			mCurrent = k;
			return AddNode(mList[k]->ref.NodeId(), mList[k]->ref.ChainId(), 0);
		}
	}
	mCurrent = -1;
	ArpASSERT(false);
	return B_ERROR;
}

status_t GlProjectList::SetChain(gl_node_id nid, gl_chain_id cid)
{
	for (uint32 k = 0; k < mNodes.size(); k++) {
		if (mNodes[k] && mNodes[k]->mNid == nid) {
			mNodes[k]->mCid = cid;
			mDirty = true;
			return B_OK;
		}
	}
	return B_ERROR;
}

const GlChain* GlProjectList::Tail(const GlRootNode* root) const
{
	int32					index = int32(mNodes.size()) - 1;
	while (index >= 0) {
		if (mNodes[index] && mNodes[index]->mCid) {
			const GlChain*	chain = root->FindChain(mNodes[index]->mCid);
			if (chain) return chain;
		}
		index--;
	}
	return 0;
}

GlChain* GlProjectList::Tail(GlRootNode* root) const
{
	int32					index = int32(mNodes.size()) - 1;
	while (index >= 0) {
		if (mNodes[index] && mNodes[index]->mCid) {
			GlChain*		chain = root->FindChain(mNodes[index]->mCid);
			if (chain) return chain;
		}
		index--;
	}
	return 0;
}

status_t GlProjectList::GetTail(gl_node_id* outNid, gl_chain_id* outCid) const
{
	if (mNodes.size() < 1) return B_ERROR;
	if (outNid) *outNid = mNodes[mNodes.size() - 1]->mNid;
	if (outCid) *outCid = mNodes[mNodes.size() - 1]->mCid;
	if (*outNid || *outCid) return B_OK;
	return B_ERROR;
}

status_t GlProjectList::SetRef(const GlRootRef& ref)
{
	FreePath();
	if (ref.IsValid() == false) return B_OK;

	int32				index = -1;
	for (uint32 k = 0; k < mList.size(); k++) {
		if (mList[k] && ref.NodeId() == mList[k]->ref.NodeId()) {
			index = k;
			break;
		}
	}
	if (index < 0) {
		AddEntry(ref);
		ArpVALIDATE(mList.size() > 0
				&& mList[mList.size() - 1]->ref.NodeId() == ref.NodeId(), return B_ERROR);
		index = int32(mList.size() - 1);
	}
	ArpVALIDATE(index >= 0 && index < int32(mList.size()), return B_ERROR);
	mCurrent = index;	

	return AddNode(ref.NodeId(), ref.ChainId(), 0);
}

status_t GlProjectList::AddRef(const GlRootRef& ref)
{
	if (mList.size() < 1) return SetRef(ref);
	return AddEntry(ref);
}

status_t GlProjectList::AddNode(gl_node_id nid, gl_chain_id cid, uint32 chains)
{
	GlPathNode*		node = new GlPathNode();
	if (!node) return B_NO_MEMORY;
	node->mNid = nid;
	node->mCid = cid;
	node->mChains = chains;
	return AddNode(node);
}

uint32 GlProjectList::PathSize() const
{
	return uint32(mNodes.size());
}

status_t GlProjectList::Unset()
{
	FreePath();
	mCurrent = -1;
	mDirty = true;
	return B_OK;
}

status_t GlProjectList::Pop()
{
	if (mNodes.size() < 1) return B_ERROR;
	return Truncate(uint32(mNodes.size() - 1));
}

status_t GlProjectList::PopTo(uint32 size)
{
	if (size >= mNodes.size()) return B_ERROR;
	return Truncate(size);
}

status_t GlProjectList::PopTo(gl_path_node_id pid)
{
	ArpVALIDATE(pid, return B_ERROR);
	for (uint32 k = 0; k < mNodes.size(); k++) {
		if (pid == mNodes[k]->Id()) {
			if (k == mNodes.size() - 1) return B_ERROR;
			return Truncate(k + 1);
		}
	}
	return B_ERROR;
}

status_t GlProjectList::MakeEmpty()
{
	FreePath();
	FreeList();
	mCurrent = -1;
	return B_OK;
}

status_t GlProjectList::Truncate(uint32 index)
{
	for (int32 k = index; k < int32(mNodes.size()); k++) {
		delete mNodes[k];
		mNodes[k] = 0;
	}
	mNodes.resize(index);
	mDirty = true;
	return B_OK;
}

status_t GlProjectList::AddEntry(const GlRootRef& ref)
{
	_GlProjectEntry*	e = new _GlProjectEntry(ref);
	if (!e) return B_NO_MEMORY;
	mList.push_back(e);
	return B_OK;
}

status_t GlProjectList::AddNode(GlPathNode* node)
{
	ArpVALIDATE(node, return B_ERROR);
	mNodes.push_back(node);
	mDirty = true;
	return B_OK;
}

void GlProjectList::FreeList()
{
	for (uint32 k = 0; k < mList.size(); k++) delete mList[k];
	mList.resize(0);
}

void GlProjectList::FreePath()
{
	for (uint32 k = 0; k < mNodes.size(); k++) delete mNodes[k];
	mNodes.resize(0);
	mDirty = true;
}

void GlProjectList::Print(uint32 tabs) const
{
	uint32			tab, k;
	for (tab = 0; tab < tabs; tab++) printf("\t");
	printf("GlProjectList size %ld (current index %ld)\n", mList.size(), mCurrent);
	for (k = 0; k < mList.size(); k++) {
		printf("%ld %p\n", k, mList[k]->ref.NodeId());
	}
	for (tab = 0; tab < tabs; tab++) printf("\t");
	printf("GlProjectList path size %ld\n", mNodes.size());
	for (k = 0; k < mNodes.size(); k++) {
		printf("%ld: ", k);
		mNodes[k]->Print(tabs + 1);
	}
}

// #pragma mark -

/*************************************************************************
 * GL-PATH-NODE
 *************************************************************************/
GlPathNode::GlPathNode()
		: editor(0), mNid(0), mCid(0), mChains(0)
{
}

GlPathNode::GlPathNode(int32 inEditor)
		: editor(inEditor), mNid(0), mCid(0),
		  mChains(0)
{
}

GlPathNode::~GlPathNode()
{
}

GlPathNode& GlPathNode::operator=(const GlPathNode& node)
{
	/* If I'm being set to the same node, don't wipe out
	 * my editor.
	 */
	if (mNid && mNid == node.mNid) ;
	else if (mCid && mCid == node.mCid) ;
	else editor = node.editor;
	mNid = node.mNid;
	mCid = node.mCid;
	mChains = node.mChains;
	return *this;
}

gl_path_node_id GlPathNode::Id() const
{
	return (void*)this;
}

bool GlPathNode::IsRoot() const
{
	return editor == GL_PROJECT_EDITOR;
}

#if 0
	status_t			Set(int32 editor);

status_t GlPathNode::Set(int32 inEditor)
{
	editor = inEditor;
	mCid = mNid = 0;
	mChains = 0;
	return B_OK;
}
#endif

void GlPathNode::Print(uint32 tabs) const
{
	for (uint32 tab = 0; tab < tabs; tab++) printf("\t");
	printf("GlPathNode nid %p cid %p - editor %ld\n", mNid, mCid, editor);
}
