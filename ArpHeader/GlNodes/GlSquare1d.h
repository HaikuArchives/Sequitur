/* GlSquare1d.h
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
#ifndef GLNODES_GLSQUARE1D_H
#define GLNODES_GLSQUARE1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlSquare1dAddOn;

/***************************************************************************
 * GL-SQUARE-1D
 * Generate a square wave.
 ***************************************************************************/
class GlSquare1d : public GlNode1d
{
public:
	GlSquare1d(const GlSquare1dAddOn* addon, const BMessage* config);
	GlSquare1d(const GlSquare1d& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
	const GlSquare1dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-SQUARE-1D-ADD-ON
 ***************************************************************************/
class GlSquare1dAddOn : public GlNode1dAddOn
{
public:
	GlSquare1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mCycle;		// The number of repetitions
	const GlParamType*		mPhase;		// Where to start in the wave
	const GlParamType*		mWidth;		// The width of each square

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
