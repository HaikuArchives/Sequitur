#include <GlPublic/GlActions.h>
#include <GlPublic/GlAlgo.h>

/***************************************************************************
 * GL-ALGO-ACTION
 ***************************************************************************/
int32 GlAlgoAction::PerformSwitch(GlAlgo* a)
{
	ArpASSERT(a);
	uint32				io = a->Io();
	if (io&GL_1D_IO) {
		GlAlgo1d*		a1d = a->As1d();
		if (a1d) return Perform(a1d);
	} else if (io&GL_2D_IO) {
		GlAlgo2d*		a2d = a->As2d();
		if (a2d) return Perform(a2d);
	} else if (io&GL_IMAGE_IO) {
		GlAlgoIm*		aIm = a->AsIm();
		if (aIm) return Perform(aIm);
	} else if (io&GL_NUMBER_IO) {
		GlAlgoNbr*		aNbr = a->AsNbr();
		if (aNbr) return Perform(aNbr);
	}
	return Perform(a);
}

/***************************************************************************
 * GL-PARSE-FROM-NODE-ACTION
 ****************************************************************************/
GlParseFromNodeAction::GlParseFromNodeAction(gl_node_id n)
		: nid(n), node(0)
{
}

int32 GlParseFromNodeAction::Perform(GlAlgo* n)
{
	node = 0;
	if (!nid) return GL_STOP_OPERATION;
	ArpASSERT(n);
	if (nid == n->NodeId()) {
		node = n;
		return GL_STOP_OPERATION;
	}
	return GL_CONTINUE;
}