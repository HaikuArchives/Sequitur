/* GlBinaryOp1d.h
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
 * 2003.02.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLBINARYOP1D_H
#define GLNODES_GLBINARYOP1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlBinaryOp1dAddOn;

enum {
	GL_ADD_BINARY_1D_KEY	= 'add_',
	GL_SUB_BINARY_1D_KEY	= 'sub_',
	GL_MULT_BINARY_1D_KEY	= 'mult',
	GL_DIV_BINARY_1D_KEY	= 'div_',
	GL_MIN_BINARY_1D_KEY	= 'min_',
	GL_MAX_BINARY_1D_KEY	= 'max_'
};

/***************************************************************************
 * GL-BINARY-OP-1D
 * Combine two nodes based on the rule.
 ***************************************************************************/
class GlBinaryOp1d : public GlNode1d
{
public:
	GlBinaryOp1d(const GlBinaryOp1dAddOn* addon, const BMessage* config);
	GlBinaryOp1d(const GlBinaryOp1d& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlBinaryOp1dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-BINARY-OP-1D-ADD-ON
 * INPUTS:
 *		1D - 0. 		Any input curve to add to.
 * OUTPUTS:
 *		1D - 0.			My curve.
 ***************************************************************************/
class GlBinaryOp1dAddOn : public GlNode1dAddOn
{
public:
	GlBinaryOp1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mOp;

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
