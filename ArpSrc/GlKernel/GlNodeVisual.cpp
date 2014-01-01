#include <GlPublic/GlNode.h>
#include <GlPublic/GlNodeVisual.h>
#include <GlPublic/GlRootNode.h>

/***************************************************************************
 * GL-NODE-VISUAL
 ****************************************************************************/
GlNodeVisual::~GlNodeVisual()
{
	ArpASSERT(!mRoot);
	if (mNode) mNode->DecRefs();
}

void GlNodeVisual::ParamChanged(gl_param_key key)
{
}

status_t GlNodeVisual::Visual(int32 w, int32 h, ArpBitmap** outBm, int32 index)
{
	GlNode*		n = LockNode();
	if (!n) return B_ERROR;
	status_t	err = PreVisual(n);
	if (err == B_OK) {
		err = LockedVisual(n, w, h, outBm, index);
		PostVisual(n);
	}
	UnlockNode(n);
	return err;
}

GlNodeVisual::GlNodeVisual(const GlRootRef& ref, gl_node_id nid)
		: mRef(ref), mNid(nid), mNode(0), mRoot(0)
{
}

status_t GlNodeVisual::PreVisual(GlNode* node)
{
	return B_OK;
}

status_t GlNodeVisual::LockedVisual(GlNode* node, int32 w, int32 h,
									ArpBitmap** outBm, int32 index)
{
	return B_ERROR;
}

void GlNodeVisual::PostVisual(GlNode* node)
{
}

GlNode* GlNodeVisual::LockNode()
{
	/* Make sure the previous lock call actuall unlocked.
	 */
	ArpASSERT(!mRoot);
	mRoot = mRef.WriteLock();
	if (!mRoot) return 0;
	if (mNode) return mNode;
	mNode = mRoot->FindNode(0, mNid);
	if (mNode) {
		mNode->IncRefs();
		return mNode;
	}
	mRef.WriteUnlock(mRoot);
	return 0;
}

void GlNodeVisual::UnlockNode(GlNode* n)
{
	ArpASSERT(n == mNode);
	ArpASSERT(mRoot);
	mRef.WriteUnlock(mRoot);
	mRoot = 0;
}
