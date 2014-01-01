/* GlValue.h
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
 * 2004.02.23				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLVALUE_H
#define GLNODES_GLVALUE_H

#include <GlPublic/GlPlaneNode.h>

/***************************************************************************
 * GL-VALUE
 * A surface that takes a pixel component (RGBA, depth, etc) and translates
 * it to a 2D.
 ***************************************************************************/
class GlValue : public GlPlaneNode
{
public:
	GlValue(const GlNodeAddOn* addon, const BMessage* config);
	GlValue(const GlValue& o);

	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
};

/***************************************************************************
 * GL-VALUE-ADD-ON
 ***************************************************************************/
class GlValueAddOn : public GlNodeAddOn
{
public:
	GlValueAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

protected:
	/* Stolen from GlAlgo2dNode -- that's the behaviour I want, not
	 * the default when I process an image.
	 */
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
};

#endif
