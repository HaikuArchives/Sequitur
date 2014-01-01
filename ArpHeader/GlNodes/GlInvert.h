/* GlInvert.h
 * Copyright (c)2003 by Eric Hackborn.
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
 * 2003.03.13				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLINVERT_H
#define GLNODES_GLINVERT_H

#include <GlPublic/GlPlaneNode.h>

/***************************************************************************
 * GL-INVERT
 * Process a 2D or image with a 1D.
 *
 * The chain generates a 1D, to be used for the application.
 ***************************************************************************/
class GlInvert : public GlPlaneNode
{
public:
	GlInvert(const GlNodeAddOn* addon, const BMessage* config);
	GlInvert(const GlInvert& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
};

/***************************************************************************
 * GL-INVERT-ADD-ON
 ***************************************************************************/
class GlInvertAddOn : public GlNodeAddOn
{
public:
	GlInvertAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

private:
	typedef GlNodeAddOn		inherited;
};

#endif
