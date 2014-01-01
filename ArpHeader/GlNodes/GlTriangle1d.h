/* GlTriangle1d.h
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
 * 2003.02.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLTRIANGLE1D_H
#define GLNODES_GLTRIANGLE1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlTriangle1dAddOn;

/***************************************************************************
 * GL-TRIANGLE-1D
 * Generate a triangle wave.
 ***************************************************************************/
class GlTriangle1d : public GlNode1d
{
public:
	GlTriangle1d(const GlTriangle1dAddOn* addon, const BMessage* config);
	GlTriangle1d(const GlTriangle1d& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlTriangle1dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-TRIANGLE-1D-ADD-ON
 ***************************************************************************/
class GlTriangle1dAddOn : public GlNode1dAddOn
{
public:
	GlTriangle1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mCycle;		// How rapidly the triangle oscillates
	const GlParamType*		mPhase;		// Where in the oscillation to start

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
