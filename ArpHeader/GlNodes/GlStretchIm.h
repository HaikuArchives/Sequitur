/* GlStretchIm.h
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
 * 2003.04.11				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLSTRETCHIM_H
#define GLNODES_GLSTRETCHIM_H

#include <GlPublic/GlAlgoIm.h>
#include <GlPublic/GlNode.h>
class GlStretchImAddOn;

/***************************************************************************
 * GL-STRETCH-IM
 * Stretch an image according to 1D rules for each axis.  Each axis
 * is given a range, and the 1Ds are translated into a pixel number
 * based on the value at each pixel.  A range of from 1 to 1 would cause
 * no change, a range of 0.5 to 0.5 would halve the image (along the given
 * axis).
 *
 * Param matrix output:
 *		1D - 0.			The x axis stretching.
 *		1D - 1.			The y axis stretching.
 ***************************************************************************/
class GlStretchIm : public GlNode
{
public:
	GlStretchIm(const GlNodeAddOn* addon, const BMessage* config);
	GlStretchIm(const GlStretchIm& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-STRETCH-IM-ADD-ON
 ***************************************************************************/
class GlStretchImAddOn : public GlNodeAddOn
{
public:
	GlStretchImAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO; }
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNodeAddOn		inherited;
};


#endif
