/* GlMirror.h
 * Copyright (c)2002 by Eric Hackborn.
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
 * 2002.08.22				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLMIRROR_H
#define GLNODES_GLMIRROR_H

#include <GlPublic/GlPlaneNode.h>

/***************************************************************************
 * GL-MIRROR
 * Take a source, mirror it horizontally and/or vertically, and blend it
 * into itself.
 *
 * Chains:
 *		2D - 0.			The horizontal blending.
 *		2D - 1.			The vertical blending.
 ***************************************************************************/
class GlMirror : public GlPlaneNode
{
public:
	GlMirror(const GlNodeAddOn* addon, const BMessage* config);
	GlMirror(const GlMirror& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode			inherited;
};

/***************************************************************************
 * GL-MIRROR-ADD-ON
 ***************************************************************************/
class GlMirrorAddOn : public GlNodeAddOn
{
public:
	GlMirrorAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
