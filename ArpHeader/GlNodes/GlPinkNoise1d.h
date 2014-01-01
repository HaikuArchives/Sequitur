/* GlPinkNoise1d.h
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
 * 2004.04.01				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLPINKNOISE1D_H
#define GLNODES_GLPINKNOISE1D_H

#include <GlPublic/GlAlgoNodes.h>

/***************************************************************************
 * GL-PINK-NOISE-1D
 * Draw pink noise.
 * Params:
 *	Amount		float	0 -1	The amount of noise
 ***************************************************************************/
class GlPinkNoise1d : public GlNode1d
{
public:
	GlPinkNoise1d(const GlNode1dAddOn* addon, const BMessage* config);
	GlPinkNoise1d(const GlPinkNoise1d& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
};

/***************************************************************************
 * GL-PINK-NOISE-1D-ADD-ON
 ***************************************************************************/
class GlPinkNoise1dAddOn : public GlNode1dAddOn
{
public:
	GlPinkNoise1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
