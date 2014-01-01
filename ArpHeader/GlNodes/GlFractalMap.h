/* GlFractalMap.h
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
 * 2003.03.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLFRACTALMAP_H
#define GLNODES_GLFRACTALMAP_H

#include <GlPublic/GlAlgoNodes.h>
class GlFractalMapAddOn;

/***************************************************************************
 * GL-FRACTAL-MAP
 * Generate a map that's just a fractal line, using midpoint displacement.
 ***************************************************************************/
class GlFractalMap : public GlNode1d
{
public:
	GlFractalMap(const GlFractalMapAddOn* addon, const BMessage* config);
	GlFractalMap(const GlFractalMap& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlFractalMapAddOn*	mAddOn;
};

/***************************************************************************
 * GL-FRACTAL-MAP-ADD-ON
 ***************************************************************************/
class GlFractalMapAddOn : public GlNode1dAddOn
{
public:
	GlFractalMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mStart;			// 0 - 1, the value at 0
	const GlParamType*		mEnd;			// 0 - 1, the value at 1
	const GlParamType*		mRug;			// How rugged the line is
	const GlParamType*		mRnd;			// Randomness
	const GlParamType*		mDetail;		// How much precision to use for
											// clients that wind up in AlgoAt()

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
