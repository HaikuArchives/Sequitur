/* GlConstant1d.h
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
 * 2004.02.23				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLCONSTANT1D_H
#define GLNODES_GLCONSTANT1D_H

#include <GlPublic/GlAlgoNodes.h>

/***************************************************************************
 * GL-CONSTANT-1D
 * Generate a single value.
 ***************************************************************************/
class GlConstant1d : public GlNode1d
{
public:
	GlConstant1d(const GlNode1dAddOn* addon, const BMessage* config);
	GlConstant1d(const GlConstant1d& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
};

/***************************************************************************
 * GL-CONSTANT-1D-ADD-ON
 ***************************************************************************/
class GlConstant1dAddOn : public GlNode1dAddOn
{
public:
	GlConstant1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
