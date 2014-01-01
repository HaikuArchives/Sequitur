#include <stdio.h>
#include <GlPublic/GlRootRef.h>
#include <GlPublic/GlChain.h>
#include <GlPublic/GlNode.h>
#include <GlPublic/GlRootNode.h>

//static int32		gCount = 0;

/*************************************************************************
 * GL-ROOT-REF
 *************************************************************************/
GlRootRef::GlRootRef()
		: mRoot(NULL)
{
//	gCount++;
}

GlRootRef::GlRootRef(const GlRootNode* node)
		: mRoot(const_cast<GlRootNode*>(node))
{
//	gCount++;
	if (mRoot) mRoot->IncRefs();
}

GlRootRef::GlRootRef(const GlRootRef& ref)
		: mRoot(0)
{
//	gCount++;
	mRoot = ref.mRoot;
	if (mRoot) mRoot->IncRefs();
}

GlRootRef::~GlRootRef()
{
//	gCount--;
//if (gCount == 0 && mRoot) mRoot->Print();
//printf("GlRootRef::~ %ld\n", gCount);
	if (mRoot) mRoot->DecRefs();
}

GlRootRef& GlRootRef::operator=(const GlRootRef& ref)
{
	SetTo(ref.mRoot);
	return *this;
}

bool GlRootRef::SetTo(const GlRootNode* node)
{
	if (mRoot) mRoot->DecRefs();
	mRoot = const_cast<GlRootNode*>(node);
	if (mRoot) mRoot->IncRefs();
	
	return IsValid();
}

bool GlRootRef::SetTo(const GlRootRef& ref)
{
	SetTo(ref.mRoot);
	return IsValid();
}

bool GlRootRef::IsValid() const
{
	return mRoot != 0;
}

gl_node_id GlRootRef::NodeId() const
{
	if (!IsValid()) return 0;
	return mRoot->Id();
}

gl_chain_id GlRootRef::ChainId() const
{
	if (!mRoot) return 0;
	const GlChain*		chain = mRoot->ChainAt(0);
	if (!chain) return 0;
	return chain->Id();
}

const GlRootNode* GlRootRef::ReadLock() const
{
	if (!mRoot) {
		return NULL;
	}
	if (!mRoot->ReadLock() ) return NULL;
	return mRoot;
}

void GlRootRef::ReadUnlock(const GlRootNode* node) const
{
	if (node) {
		if (node != mRoot) ArpASSERT(false);	// bad node returned to rootref
		else node->ReadUnlock();
	}
}

GlRootNode* GlRootRef::WriteLock(const char* name)
{
	if (!mRoot) {
		return 0;
	}
	if (!mRoot->WriteLock(name) ) return NULL;
	return mRoot;
}

void GlRootRef::WriteUnlock(GlRootNode* node)
{
	if (node) {
		if (node != mRoot) ArpASSERT(false);	// bad node returned to rootref
		else node->WriteUnlock();
	}
}

status_t GlRootRef::AddObserver(uint32 code, const BMessenger& m)
{
	if (!mRoot) return B_NO_INIT;
	return mRoot->AddObserver(code, m);
}

status_t GlRootRef::RemoveObserver(uint32 code, const BMessenger& m)
{
	if (!mRoot) return B_NO_INIT;
	return mRoot->RemoveObserver(code, m);
}

status_t GlRootRef::RemoveAll()
{
	if (!mRoot) return B_NO_INIT;
	mRoot->RemoveAll();
	return B_OK;
}

status_t GlRootRef::ReportMsgChange(uint32 code, BMessage* msg, BMessenger sender)
{
	if (!mRoot) return B_NO_INIT;
	mRoot->ReportMsgChange(code, msg, sender);
	return B_OK;
}
