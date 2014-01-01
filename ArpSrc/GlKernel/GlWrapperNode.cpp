#include <GlPublic/GlChain.h>
#include <GlPublic/GlImage.h>
#include <GlPublic/GlUtils.h>
#include <GlPublic/GlParamType.h>
#include <GlKernel/GlDefs.h>
#include <GlKernel/GlWrapperNode.h>

/*************************************************************************
 * GL-WRAPPER-NODE
 *************************************************************************/
GlWrapperNode::GlWrapperNode(const GlNodeAddOn* addon)
		: inherited(addon, 0)
{
}

GlWrapperNode::GlWrapperNode(const GlWrapperNode& o)
		: inherited(o)
{
}

GlNode* GlWrapperNode::Clone() const
{
	return new GlWrapperNode(*this);
}

uint32 GlWrapperNode::Io() const
{
	const GlChain*		c = ChainAt(0);
	if (c) return c->Io();
	ArpASSERT(false);
	return 0;
}

BString16 GlWrapperNode::Creator() const
{
	GlTextWrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_CREATOR, w);
	return w.v;
}

int32 GlWrapperNode::Key() const
{
	GlInt32Wrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_KEY, w);
	return w.v;
}

BString16 GlWrapperNode::Category() const
{
	GlTextWrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_CATEGORY, w);
	return w.v;
}

BString16 GlWrapperNode::Label() const
{
	GlTextWrap		w;
	Params().GetValue(0, GL_PARAM_ROOT_LABEL, w);
	return w.v;
}

GlAlgo* GlWrapperNode::Generate(const gl_generate_args& args) const
{
	const GlChain*		c = ChainAt(0);
	if (c) return c->Generate(args);
	ArpASSERT(false);
	return 0;
}

const ArpBitmap* GlWrapperNode::Image() const
{
	if (mImage) return mImage;
	GlImage*		img = gl_new_node_image((GlNode*)this);
	if (!img) return 0;
	mImage = img->AsBitmap();
	delete img;
	return mImage;
}

float GlWrapperNode::GetMatch(	const BString16& creator, int32 key,
								int32* major, int32* minor) const
{
//printf("keys "); gl_print_key(Key()); printf(" to "); gl_print_key(key);
//printf(", creators %s to %s\n", Creator().String(), creator.String());
	if (Key() != key) return 0.;
	if (Creator() != creator) return 0.;
	return 1.;
}

void GlWrapperNode::Print(uint32 tabs) const
{
	for (uint32 t = 0; t < tabs; t++) printf("\t");
	printf("Wrapper node %p\n", Id());
	PrintChains(tabs + 1);
}

// #pragma mark -

/***************************************************************************
 * GL-WRAPPER-NODE-ADD-ON
 ***************************************************************************/
GlWrapperNodeAddOn::GlWrapperNodeAddOn()
		: inherited(SZI[SZI_arp], GL_WRAPPER_KEY, &SZI[SZI_arp], SZ(SZ_Wrapper), 1, 0)
{
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_CREATOR,		SZ(SZ_Creator),	GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlInt32ParamType(GL_PARAM_ROOT_KEY,		SZ(SZ_Key), LONG_MIN, LONG_MAX, 0,	GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_CATEGORY,	SZ(SZ_Category), GL_DEF_ROW, GL_ROOT_INFO_F));
	AddParamType(new GlTextParamType(GL_PARAM_ROOT_LABEL,		SZ(SZ_Label),	GL_DEF_ROW, GL_ROOT_INFO_F));
}

GlNode* GlWrapperNodeAddOn::NewInstance(const BMessage* config) const
{
	ArpASSERT(config);
	GlWrapperNode*		n = new GlWrapperNode(this);
	if (n && config && n->ReadFrom(*config) != B_OK) {
		n->MakeEmpty();
		n->DecRefs();
		n = 0;
	}
	return n;
}
