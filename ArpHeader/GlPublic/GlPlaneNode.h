/* GlPlaneNode.h
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
 * 2004.01.31				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLPLANENODE2_H
#define GLNODES_GLPLANENODE2_H

#include <GlPublic/GlAlgo2d.h>
#include <GlPublic/GlNode.h>

enum {
	GL_PIXEL_TARGET_VIEW	= GL_NODE_VIEWS
};

enum {
	GL_PIXEL_TARGET_PROP	= GL_NODE_PROPS
};

/***************************************************************************
 * GL-PLANE-NODE
 * Invert whatever passes through me.
 ***************************************************************************/
class GlPlaneNode : public GlNode
{
public:
	GlPlaneNode(const GlNodeAddOn* addon, const BMessage* config);
	GlPlaneNode(const GlPlaneNode& o);

	/* Answer a mask of GL_PIXEL_R_MASK etc. that defines which pixels I should
	 * be processing.  Only valid for nodes with the GL_PIXEL_TARGETS_F flag.
	 */
	uint32					PixelTargets() const;
	void					SetPixelTargets(uint32 targets);

	virtual status_t		GetProperty(int32 code, GlParamWrap& wrap) const;
	virtual status_t		SetProperty(int32 code, const GlParamWrap& wrap);
	virtual status_t		ReadFrom(const BMessage& config);
	virtual status_t		WriteTo(BMessage& config) const;
	virtual BView*			NewView(gl_new_view_params& params) const;
	virtual GlNodeVisual*	NewVisual(const GlRootRef& ref) const;

private:
	typedef GlNode			inherited;
	uint32					mPixelTargets;
};


#endif
