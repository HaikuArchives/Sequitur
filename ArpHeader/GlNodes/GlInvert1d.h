/* GlInvert1d.h
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
 * 2004.20.21				hackborn@angryredplanet.com
 * Ported to chaining framework.
 *
 * 2003.03.12				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLINVERT1D_H
#define GLNODES_GLINVERT1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlInvert1dAddOn;

/***************************************************************************
 * GL-INVERT-1D
 * Invert whatever passes through me.
 ***************************************************************************/
class GlInvert1d : public GlNode1d
{
public:
	GlInvert1d(const GlNode1dAddOn* addon, const BMessage* config);
	GlInvert1d(const GlInvert1d& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
};

/***************************************************************************
 * GL-INVERT-1D-ADD-ON
 ***************************************************************************/
class GlInvert1dAddOn : public GlNode1dAddOn
{
public:
	GlInvert1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
