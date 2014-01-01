/* GlResizeIm.h
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
 * 2003.03.11				hackborn@angryredplanet.com
 * Updated to new conventions, parameterized.
 */
#ifndef GLNODES_RESIZEIM_H
#define GLNODES_RESIZEIM_H

#include <GlPublic/GlNode.h>
#include <GlPublic/GlSupportTypes.h>
class GlResizeImAddOn;

/***************************************************************************
 * GL-RESIZE-IM
 * Add or subtract borders from all incoming bitmaps and surfaces.  This node
 * operates in too modes:  Absolute (i.e. the number of pixels specified is
 * added / subtracted) or Relative (i.e. in terms of percent).  If any nodes
 * are connected to the surfaces input, then only relative mode is allowed.
 ***************************************************************************/
class GlResizeIm : public GlNode
{
public:
	GlResizeIm(const GlResizeImAddOn* addon, const BMessage* config);
	GlResizeIm(const GlResizeIm& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode			inherited;
	const GlResizeImAddOn*	mAddOn;

	status_t				GetChainParams(	const gl_generate_args& args,
											GlRelAbs& left, GlRelAbs& top,
											GlRelAbs& right, GlRelAbs& bottom,
											GlAlgo** outIm) const;
};

/***************************************************************************
 * GL-RESIZE-IM-ADD-ON
 * Four params for the left, top, right, bottom sides.  Each param has
 * relative and absolute coords.
 ***************************************************************************/
class GlResizeImAddOn : public GlNodeAddOn
{
public:
	GlResizeImAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
