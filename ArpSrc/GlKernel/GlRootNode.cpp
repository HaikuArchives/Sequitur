#include <GlPublic/GlChain.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlRootNode.h>
#include <GlKernel/GlDefs.h>

static const int32		ROOT_CHAIN_KEY			= 'root';

/*************************************************************************
 * GL-ROOT-NODE
 *************************************************************************/
GlRootNode::GlRootNode(const GlNodeAddOn* addon, int32 type)
		: inherited(addon, 0)
{
	FlushChanges(false);
	ArpASSERT(addon && addon->Key() == GL_ROOT_KEY);
	if (type != 0) VerifyChain(new GlChain(ROOT_CHAIN_KEY, type, SZ(SZ_Root), this));
}

GlRootNode::GlRootNode(const GlNodeAddOn* addon, const BMessage* config)
		: inherited(addon, config)
{
	FlushChanges(false);
}

GlRootNode::GlRootNode(const GlRootNode& o)
		: inherited(o)
{
}

bool GlRootNode::ReadLock() const
{
	return mReadLock.Lock();
}

bool GlRootNode::WriteLock(const char* name)
{
	return mWriteLock.Lock();
}

bool GlRootNode::ReadUnlock() const
{
	mReadLock.Unlock();
	return true;
}

bool GlRootNode::WriteUnlock()
{
	FlushChanges();
	mWriteLock.Unlock();
	return true;
}

GlNode* GlRootNode::Clone() const
{
	/* This won't really work -- the param strainer isn't cloned,
	 * and even if it was, a lot of work would need to be put into
	 * making sure the nid in the orig get turned into the nids in
	 * the clone.
	 */
//	ArpASSERT(false);
	return new GlRootNode(*this);
}

uint32 GlRootNode::Io() const
{
	const GlChain*		c = ChainAt(0);
	if (c) return c->Io();
	ArpASSERT(false);
	return 0;
}

BString16 GlRootNode::Creator() const
{
	GlTextWrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_CREATOR, w);
	return w.v;
}

void GlRootNode::SetCreator(const BString16& str)
{
	GlTextWrap		w(str);
	Params().SetValue(GL_PARAM_ROOT_CREATOR, w);	

	mInfoChanges.what = INFO_CODE;
	if (mInfoChanges.ReplaceString(GL_NODE_CREATOR_STR, str) != B_OK)
		mInfoChanges.AddString(GL_NODE_CREATOR_STR, str);
}

int32 GlRootNode::Key() const
{
	GlInt32Wrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_KEY, w);
	return w.v;
}

void GlRootNode::SetKey(int32 k)
{
	GlInt32Wrap		w(k);
	Params().SetValue(GL_PARAM_ROOT_KEY, w);

	mInfoChanges.what = INFO_CODE;
	if (mInfoChanges.ReplaceInt32(GL_NODE_KEY_STR, k) != B_OK)
		mInfoChanges.AddInt32(GL_NODE_KEY_STR, k);
}

BString16 GlRootNode::Category() const
{
	GlTextWrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_CATEGORY, w);
	return w.v;
}

void GlRootNode::SetCategory(const BString16& str)
{
	GlTextWrap		w(str);
	Params().SetValue(GL_PARAM_ROOT_CATEGORY, w);

	mInfoChanges.what = INFO_CODE;
	if (mInfoChanges.ReplaceString(GL_NODE_CATEGORY_STR, str) != B_OK)
		mInfoChanges.AddString(GL_NODE_CATEGORY_STR, str);
}

BString16 GlRootNode::Label() const
{
	GlTextWrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_LABEL, w);
	return w.v;
}

void GlRootNode::SetLabel(const BString16& str)
{
	GlTextWrap		w(str);
	Params().SetValue(GL_PARAM_ROOT_LABEL, w);

	mInfoChanges.what = INFO_CODE;
	if (mInfoChanges.ReplaceString(GL_NODE_LABEL_STR, str) != B_OK)
		mInfoChanges.AddString(GL_NODE_LABEL_STR, str);
}

GlAlgo* GlRootNode::Generate(const gl_generate_args& args) const
{
	const GlChain*		c = FindChain(ROOT_CHAIN_KEY);
	if (!c) return 0;
	return c->Generate(args);
}

status_t GlRootNode::ParamChanged(gl_param_key key)
{
	mNodeChanges.what = NODE_CODE;
/* No one actually uses this and it's not a very good way to
 * do it anyway (should check to see if the pointer is already there),
 * so don't for now.
 */
//	if (key.nid) mNodeChanges.AddPointer(GL_NODE_ID_STR, key.nid);
	return B_OK;
}

status_t GlRootNode::ChainChanged(gl_chain_id id, int32 dynamic)
{
	/* Roots can't have dynamic chains.
	 */
	ArpASSERT(dynamic < 1);
	mChainChanges.what = CHAIN_CODE;
	void*				p;
	for (int32 i = 0; mChainChanges.FindPointer(GL_CHAIN_ID_STR, i, &p) == B_OK; i++) {
		if (p == id) return B_OK;
	}
	mChainChanges.AddPointer(GL_CHAIN_ID_STR, id);
	return B_OK;
}

bool GlRootNode::HasCreator() const
{
	GlTextWrap		w;
	if (Params().GetValueNoInit(0, GL_PARAM_ROOT_CREATOR, w) != B_OK) return false;
	return w.v.Length() > 0;
}

bool GlRootNode::HasKey() const
{
	GlInt32Wrap		w;
	if (Params().GetValueNoInit(0, GL_PARAM_ROOT_KEY, w) == B_OK) return true;
	return false;
}

bool GlRootNode::HasLabel() const
{
	GlTextWrap		w;
	if (Params().GetValueNoInit(0, GL_PARAM_ROOT_LABEL, w) != B_OK) return false;
	return w.v.Length() > 0;
}

void GlRootNode::ClearChanges()
{
	FlushChanges(false);
}

void GlRootNode::FlushChanges(bool send)
{
	if (send) {
		BMessenger			m;
		BMessage			changes(CHANGE_MSG);
		uint32				code = 0;
		if (mInfoChanges.what == INFO_CODE) {
			code |= INFO_CODE;
			changes.AddMessage("m", &mInfoChanges);
		}
		if (mNodeChanges.what == NODE_CODE) {
			code |= NODE_CODE;
			changes.AddMessage("m", &mNodeChanges);
		}
		if (mChainChanges.what == CHAIN_CODE) {
			code |= CHAIN_CODE;
			changes.AddMessage("m", &mChainChanges);
		}
		if (code != 0) ReportMsgChange(code, &changes, m);
	}
	
	SetSender(0);
	mInfoChanges.what = 0;
	mInfoChanges.MakeEmpty();
	mNodeChanges.what = 0;
	mNodeChanges.MakeEmpty();
	mChainChanges.what = 0;
	mChainChanges.MakeEmpty();
}

void GlRootNode::Print(uint32 tabs) const
{
	for (uint32 t = 0; t < tabs; t++) printf("\t");
	printf("GlRootNode %p\n", Id());
	PrintChains(tabs + 1);
}

// #pragma mark -

/***************************************************************************
 * GL-ROOT-NODE-ADD-ON
 ***************************************************************************/
GlRootNodeAddOn::GlRootNodeAddOn()
		: inherited(SZI[SZI_arp], GL_ROOT_KEY, &SZI[SZI_arp], SZ(SZ_Root), 1, 0)
{
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_CREATOR,		SZ(SZ_Creator),	GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlInt32ParamType(GL_PARAM_ROOT_KEY,		SZ(SZ_Key), LONG_MIN, LONG_MAX, 0,	GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_CATEGORY,	SZ(SZ_Category), GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_LABEL,		SZ(SZ_Label),	GL_DEF_ROW, GL_ROOT_INFO_F));
}

GlNode* GlRootNodeAddOn::NewInstance(const BMessage* config) const
{
	return new GlRootNode(this, config);
}
