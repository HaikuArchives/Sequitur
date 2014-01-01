/* GlConvolveSum.h
 * Copyright (c)2003-2004 by Eric Hackborn.
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
 * 2004.04.22				hackborn@angryredplanet.com
 * Ported to chaining framework.
 *
 * 2003.03.11				hackborn@angryredplanet.com
 * Extracted from the (very old) GlConvolve node, parameterized it.
 */
#ifndef GLNODES_GLCONVOLVESUM_H
#define GLNODES_GLCONVOLVESUM_H

#include <GlPublic/GlChainMacro.h>
#include <GlPublic/GlFilterKernel.h>
#include <GlPublic/GlPlaneNode.h>
class GlConvolveSumAddOn;

enum {
	GL_CONVOLVE_SUM_NONE	= 0,	
	GL_CONVOLVE_SUM_BLUR	= 1,
	GL_CONVOLVE_SUM_MEDIAN	= 2
};	

/***************************************************************************
 * GL-CONVOLVE-SUM
 * Apply a filter kernel to an image, summing the results on the center.
 ***************************************************************************/
class GlConvolveSum : public GlPlaneNode
{
public:
	GlConvolveSum(const GlConvolveSumAddOn* addon, const BMessage* config);
	GlConvolveSum(const GlConvolveSum& o);
	
	virtual GlNode*				Clone() const;
	virtual status_t			ParamChanged(gl_param_key key);
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;


//	virtual status_t			Preview(int32 w, int32 h, ArpBitmap** outBm);

protected:
#if 0
	status_t	MakePreview(GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
							float low, float high, int32 viewW, int32 viewH,
							ArpBitmap** outBm) const;
#endif
	status_t	MakeKernel(	GlAlgo2d* s, int32 l, int32 t, int32 r, int32 b,
							float low, float high, GlFilterKernel& outKernel) const;

private:
	typedef GlPlaneNode			inherited;
	const GlConvolveSumAddOn*	mAddOn;
	
	/* For the preview.
	 */
	bool		mChanged;
};

/***************************************************************************
 * GL-CONVOLVE-SUM-ADD-ON
 * INPUTS:
 *		Image - 0.		The images to be placed.
 *		2D - 0.			Any surface to add my surface to
 *		2D - 1.			The surface that creates the filter kernel.
 * OUTPUTS:
 *		Image - 0.		The placed images.
 ***************************************************************************/
class GlConvolveSumAddOn : public GlNodeAddOn
{
public:
	GlConvolveSumAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }
#if 0
	const GlParamType*		mMapLType;		// The low value in the int to float mapping
	const GlParamType*		mMapHType;		// The high value in the int to float mapping
	const GlParamType*		mLtType;		// Left and top edge of the kernel (from a center)
	const GlParamType*		mRbType;		// Right and bottom edge of the kernel (from a center)
#endif
	/* Answer GL_CONVOLVE_SUM_NONE or one of the macro codes.
	 */
	int32					ChainMacro(const GlChain* c) const;

	GlChainMacro			mBlur1;

private:
	typedef GlNodeAddOn		inherited;

};


#endif
