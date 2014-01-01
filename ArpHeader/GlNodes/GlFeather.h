/* GlFeather.h
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
 * 2004.03.04				hackborn@angryredplanet.com
 * Extracted from the (very old) GlConvolve node, parameterized it.
 */
#ifndef GLNODES_GLCONVOLVE_H
#define GLNODES_GLCONVOLVE_H

#include <GlPublic/GlFilterKernel.h>
#include <GlPublic/GlPlaneNode.h>
class GlFeatherAddOn;

/***************************************************************************
 * GL-FEATHER
 * Use a filter kernel to spread a pixel out to its neighbors.
 ***************************************************************************/
class GlFeather : public GlPlaneNode
{
public:
	GlFeather(const GlFeatherAddOn* addon, const BMessage* config);
	GlFeather(const GlFeather& o);
	
	virtual GlNode*				Clone() const;
	virtual GlNodeVisual*		NewVisual(const GlRootRef& ref) const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

protected:
	friend class _GlFeatherVisual;

private:
	typedef GlPlaneNode			inherited;
	const GlFeatherAddOn*		mAddOn;

	status_t					GetChainParams(	const gl_generate_args& args,
												float* lr, int32* la, float* tr, int32* ta,
												float* rr, int32* ra, float* br, int32* ba,
												GlAlgo** filter,
												GlAlgo** attenuation) const;
};

/***************************************************************************
 * GL-FEATHER-SUM-ADD-ON
 ***************************************************************************/
class GlFeatherAddOn : public GlNodeAddOn
{
public:
	GlFeatherAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

	const GlParamType*		mL;		// The sides of the filter kernel -- the
	const GlParamType*		mT;		// kernel will always have a center pixel,
	const GlParamType*		mR;		// then extend out by the left, top, right
	const GlParamType*		mB;		// and bottom values.

private:
	typedef GlNodeAddOn		inherited;
};


#endif
