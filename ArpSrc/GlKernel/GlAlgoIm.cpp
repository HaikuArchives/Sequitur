#include <typeinfo>
#include <GlPublic/GlAlgoIm.h>

/***************************************************************************
 * GL-ALGO-IM
 ****************************************************************************/
GlAlgoIm::~GlAlgoIm()
{
}

GlAlgoIm* GlAlgoIm::AsIm()
{
	return this;
}

const GlAlgoIm* GlAlgoIm::AsIm() const
{
	return this;
}

GlAlgoIm::GlAlgoIm(gl_node_id nid, int32 token)
		: inherited(nid, token)
{
}

GlAlgoIm::GlAlgoIm(const GlAlgoIm& o)
		: inherited(o)
{
}
