/* GlEdge.h
 * Copyright (c)2002-2004 by Eric Hackborn.
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
 * 2004.04.21				hackborn@angryredplanet.com
 * Ported to chaining framework.
 *
 * 2002.08.22				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLEDGE_H
#define GLNODES_GLEDGE_H

#include <GlPublic/GlPlaneNode.h>

/***************************************************************************
 * GL-EDGE
 ***************************************************************************/
class GlEdge : public GlPlaneNode
{
public:
	GlEdge(const GlNodeAddOn* addon, const BMessage* config);
	GlEdge(const GlEdge& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
};

/***************************************************************************
 * GL-EDGE-ADD-ON
 ***************************************************************************/
class GlEdgeAddOn : public GlNodeAddOn
{
public:
	GlEdgeAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
