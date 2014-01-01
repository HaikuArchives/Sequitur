#include <GlPublic/GlChain.h>
#include <GlPublic/GlGlobalsI.h>
#include <GlPublic/GlParamType.h>
#include <GlPublic/GlRootNode.h>
#include <GlKernel/GlDefs.h>
#include <GlKernel/GlUserNodeAddOn.h>
#include <GlKernel/GlWrapperNode.h>

/***************************************************************************
 * GL-USER-NODE-ADD-ON
 ***************************************************************************/
GlUserNodeAddOn::GlUserNodeAddOn(const BMessage& msg)
		: inherited(SZI[SZI_arp], 0, 0, 0, 1, 0), mStatus(B_ERROR),
		  mConfig(msg), mIo(0)
{
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_CREATOR,		SZ(SZ_Creator),	GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlInt32ParamType(GL_PARAM_ROOT_KEY,		SZ(SZ_Key), LONG_MIN, LONG_MAX, 0,	GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_CATEGORY,	SZ(SZ_Category), GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_LABEL,		SZ(SZ_Label),	GL_DEF_ROW, GL_ROOT_INFO_F));

	const GlNodeAddOn*		addon = GlGlobals().GetAddOn(GL_ROOT_KEY);
	if (!addon) return;

	GlRootNode*				root = new GlRootNode(addon, (const BMessage*)0);
	if (!root) return;
	root->IncRefs();
	mStatus = root->ReadFrom(msg);
	if (mStatus != B_OK) {
		root->MakeEmpty();
		root->DecRefs();
		return;
	}
	mCreator = root->Creator();
	mKey = root->Key();
	mCategory = root->Category();
	mLabel = root->Label();
	mIo = root->Io();
	
	root->MakeEmpty();
	root->DecRefs();
}

GlNode* GlUserNodeAddOn::NewInstance(const BMessage* config) const
{
	const BMessage*		c = (config) ? config : &mConfig;
	GlWrapperNode*		n = new GlWrapperNode(this);
	if (n && c && n->ReadFrom(*c) != B_OK) {
		n->MakeEmpty();
		n->DecRefs();
		n = 0;
	}
	return n;
}

uint32 GlUserNodeAddOn::Io() const
{
	return mIo;
}

status_t GlUserNodeAddOn::InitCheck() const
{
	return mStatus;
}
