/* GlCompositeImg.h
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
 * 2003.02.26			hackborn@angryredplanet.com
 * Combined composite color and composite material
 */
#ifndef GLNODES_GLCOMPOSITE_H
#define GLNODES_GLCOMPOSITE_H

#include "GlPublic/GlImage.h"
#include "GlPublic/GlMask.h"
#include "GlPublic/GlNode.h"
class GlCompositeImgAddOn;

/***************************************************************************
 * GL-COMPOSITE
 * Composite all images on port 1 into all images on port 0.  Respect
 * the surfaces, if any.
 *
 * Param matrix outputs:
 *		1D - 0.			A straight value replacement.
 *		2D - 0.			A mask for the destination images.
 *		2D - 1.			A mask for the source images.
 ***************************************************************************/
class GlCompositeImg : public GlNode
{
public:
	GlCompositeImg(const GlCompositeImgAddOn* addon, const BMessage* config);
	GlCompositeImg(const GlCompositeImg& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
	virtual status_t			Process(GlNodeDataList& list,
										const gl_process_args* args);

private:
	typedef GlNode			inherited;
	const GlCompositeImgAddOn*	mAddOn;
};

/***************************************************************************
 * GL-COMPOSITE-ADD-ON
 * INPUTS:
 *		Image - 0.		The destination image(s).  If none, one is generated
 						to accomodate all the source images.
 *		Image - 1.		The sources -- each one is composited to all destinations.
 * OUTPUTS:
 *		Image - 0.		The destination images, freshly composited.
 ***************************************************************************/
class GlCompositeImgAddOn : public GlNodeAddOn
{
public:
	GlCompositeImgAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const	{ return GL_IMAGE_IO; }

	const GlParamType*		mMode;
	// Color
	const GlParamType*		mROn;		// R
	const GlParamType*		mRSrc;
	const GlParamType*		mGOn;		// G
	const GlParamType*		mGSrc;
	const GlParamType*		mBOn;		// B
	const GlParamType*		mBSrc;
	const GlParamType*		mAOn;		// A
	const GlParamType*		mASrc;
	// Material
	const GlParamType*		mZOn;		// Depth
	const GlParamType*		mZSrc;
	const GlParamType*		mDiffOn;	// Diffusion
	const GlParamType*		mDiffSrc;
	const GlParamType*		mSpecOn;	// Specularity
	const GlParamType*		mSpecSrc;
	const GlParamType*		mDOn;		// Density
	const GlParamType*		mDSrc;
	const GlParamType*		mCOn;		// Cohesion
	const GlParamType*		mCSrc;
	const GlParamType*		mFOn;		// Fluidity
	const GlParamType*		mFSrc;

	const GlParamType*		mAlignSources;	// If there's no destination image
											// and multiple sources, align them to 0, 0.
											
private:
	typedef GlNodeAddOn		inherited;
};


#endif
