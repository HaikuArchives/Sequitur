/* GlReplicate.h
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
 * 2004.02.19				hackborn@angryredplanet.com
 * Created this file.
 */
 
#ifndef GLNODES_GLTILE_H
#define GLNODES_GLTILE_H

#include <GlPublic/GlPlaneNode.h>
class GlReplicateAddOn;

/***************************************************************************
 * GL-REPLICATE
 * Take the image, subdivide it, and overlay that on the original image.
 * Keep doing this up to the depth limit.
 ***************************************************************************/
class GlReplicate : public GlPlaneNode
{
public:
	GlReplicate(const GlReplicateAddOn* addon, const BMessage* config);
	GlReplicate(const GlReplicate& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
	const GlReplicateAddOn*	mAddOn;
};

/***************************************************************************
 * GL-REPLICATE-ADD-ON
 ***************************************************************************/
class GlReplicateAddOn : public GlNodeAddOn
{
public:
	GlReplicateAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
