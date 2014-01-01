/* GlBlend1d.h
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
 * 2003.02.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLBLEND1D_H
#define GLNODES_GLBLEND1D_H

#include <GlPublic/GlAlgoNodes.h>

/***************************************************************************
 * GL-SIN-1D
 * Generate a sine wave.
 ***************************************************************************/
class GlBlend1d : public GlNode1d
{
public:
	GlBlend1d(const GlNode1dAddOn* addon, const BMessage* config);
	GlBlend1d(const GlBlend1d& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
};

/***************************************************************************
 * GL-SIN-1D-ADD-ON
 ***************************************************************************/
class GlBlend1dAddOn : public GlNode1dAddOn
{
public:
	GlBlend1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
