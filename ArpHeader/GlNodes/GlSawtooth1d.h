/* GlSawtooth1d.h
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
#ifndef GLNODES_GLSAWTOOTH1D_H
#define GLNODES_GLSAWTOOTH1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlSawtooth1dAddOn;

/***************************************************************************
 * GL-SAWTOOTH-1D
 * Generate a sawtooth wave.
 ***************************************************************************/
class GlSawtooth1d : public GlNode1d
{
public:
	GlSawtooth1d(const GlSawtooth1dAddOn* addon, const BMessage* config);
	GlSawtooth1d(const GlSawtooth1d& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlSawtooth1dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-SAWTOOTH-1D-ADD-ON
 ***************************************************************************/
class GlSawtooth1dAddOn : public GlNode1dAddOn
{
public:
	GlSawtooth1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mCycle;		// How rapidly the sawtooth oscillates
	const GlParamType*		mPhase;		// Where in the oscillation to start

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
