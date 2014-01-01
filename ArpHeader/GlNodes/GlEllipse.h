/* GlEllipse.h
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
#ifndef GLNODES_GLELLIPSE_H
#define GLNODES_GLELLIPSE_H

#include <GlPublic/GlPlaneNode.h>
class GlEllipseAddOn;

/***************************************************************************
 * GL-ELLIPSE
 * Generate a shape based on an x radius and a y radius -- if no 1D input
 * exists, that shape is a simple ellipse.
 ***************************************************************************/
class GlEllipse : public GlPlaneNode
{
public:
	GlEllipse(const GlEllipseAddOn* addon, const BMessage* config);
	GlEllipse(const GlEllipse& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
	const GlEllipseAddOn*	mAddOn;

	status_t				GetChainParams(	const gl_generate_args& args,
											BPoint& outX, BPoint& outY,
											BPoint& outW, BPoint& outH,
											GlAlgo** outAX, GlAlgo** outAY,
											GlAlgo** outAZ) const;
};

/***************************************************************************
 * GL-ELLIPSE-ADD-ON
 ***************************************************************************/
class GlEllipseAddOn : public GlNodeAddOn
{
public:
	GlEllipseAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

	const GlParamType*		mX;			// X pos (a point, where x is rel and y is abs)
	const GlParamType*		mY;			// Y pos (a point, where x is rel and y is abs)
	const GlParamType*		mW;			// Width (a point, where x is rel and y is abs)
	const GlParamType*		mH;			// Height (a point, where x is rel and y is abs)

private:
	typedef GlNodeAddOn		inherited;
};


#endif
