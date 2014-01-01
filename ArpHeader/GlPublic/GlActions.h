/* GlActions.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * To Do
 * ~~~~~~~~~~
 *
 *	- Nothing!
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2003.06.05			hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLACTIONS_H
#define GLPUBLIC_GLACTIONS_H

#include <GlPublic/GlDefs.h>
class GlAlgo;
class GlAlgo1d;
class GlAlgo2d;
class GlAlgoIm;
class GlAlgoNbr;
class GlNode;

/***************************************************************************
 * GL-ALGO-ACTION
 * Perform an action on a parse node.
 ***************************************************************************/
class GlAlgoAction
{
public:
	virtual int32		Perform(GlAlgo* a)			{ ArpASSERT(false); return GL_CONTINUE; }
	virtual int32		Perform(GlAlgo1d* a)		{ ArpASSERT(false); return GL_CONTINUE; }
	virtual int32		Perform(GlAlgo2d* a)		{ ArpASSERT(false); return GL_CONTINUE; }
	virtual int32		Perform(GlAlgoIm* a)		{ ArpASSERT(false); return GL_CONTINUE; }
	virtual int32		Perform(GlAlgoNbr* a)		{ ArpASSERT(false); return GL_CONTINUE; }

	int32				PerformSwitch(GlAlgo* a);
};

/***************************************************************************
 * GL-NODE-ACTION
 * Perform an action on a node.
 ***************************************************************************/
class GlNodeAction
{
public:
	virtual int32		Perform(GlNode* n)			{ ArpASSERT(false); return GL_STOP_OPERATION; }
	virtual int32		Perform(const GlNode* n)	{ ArpASSERT(false); return GL_STOP_OPERATION; }
};




/***************************************************************************
 * Some standard, common actions
 ***************************************************************************/


/***************************************************************************
 * GL-PARSE-FROM-NODE-ACTION
 * Find a parse node for the given node ID.
 ***************************************************************************/
class GlParseFromNodeAction : public GlAlgoAction
{
public:
	gl_node_id			nid;
	GlAlgo*				node;
	
	GlParseFromNodeAction(gl_node_id nid = 0);

	virtual int32		Perform(GlAlgo* n);
};


#endif
