/* GlOutlineSrf.h
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
 * 2003.03.08				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLOUTLINESRF_H
#define GLNODES_GLOUTLINESRF_H

#include <GlPublic/GlPlaneNode.h>
class GlOutlineSrfAddOn;

/***************************************************************************
 * GL-OUTLINE-SRF
 ***************************************************************************/
class GlOutlineSrf : public GlPlaneNode
{
public:
	GlOutlineSrf(const GlOutlineSrfAddOn* addon, const BMessage* config);
	GlOutlineSrf(const GlOutlineSrf& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
	
private:
	typedef GlPlaneNode			inherited;
	const GlOutlineSrfAddOn*	mAddOn;
};

/***************************************************************************
 * GL-OUTLINE-SRF-ADD-ON
 ***************************************************************************/
class GlOutlineSrfAddOn : public GlNodeAddOn
{
public:
	GlOutlineSrfAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

	const GlParamType*		mRadiusRel;		// Radius of the dot
	const GlParamType*		mRadiusAbs;		// Radius of the dot
	const GlParamType*		mEraseBg;		// Erase the background

private:
	typedef GlNodeAddOn		inherited;
};


#endif
