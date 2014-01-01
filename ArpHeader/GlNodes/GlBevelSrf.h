/* GlBevelSrf.h
 * Copyright (c)2002-2004 by Eric Hackborn.
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
 * 2004.03.24				hackborn@angryredplanet.com
 * Updated to chaining framework.
 *
 * 2002.08.22				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLBEVEL_H
#define GLNODES_GLBEVEL_H

#include <GlPublic/GlAlgoNodes.h>
class GlBevelSrfAddOn;

/***************************************************************************
 * GL-BEVEL-SRF
 * Bevel the surface.
 ***************************************************************************/
class GlBevelSrf : public GlNode2d
{
public:
	GlBevelSrf(const GlBevelSrfAddOn* addon, const BMessage* config);
	GlBevelSrf(const GlBevelSrf& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode2d		inherited;
	const GlBevelSrfAddOn*	mAddOn;
};

/***************************************************************************
 * GL-BEVEL-SRF-ADD-ON
 ***************************************************************************/
class GlBevelSrfAddOn : public GlNode2dAddOn
{
public:
	GlBevelSrfAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mQuality;	// Different bevel algorithms
	const GlParamType*		mWidth;		// Width of bevel
	const GlParamType*		mSmoothing;	// How much to smooth -- is this necessary?

private:
	typedef GlNode2dAddOn	inherited;
};


#endif
