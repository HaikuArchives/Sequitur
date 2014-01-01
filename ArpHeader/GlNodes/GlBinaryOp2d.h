/* GlBinaryOp2d.h
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
 * 2004.03.09				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLBINARYOP2D_H
#define GLNODES_GLBINARYOP2D_H

#include <GlPublic/GlAlgoNodes.h>
class GlBinaryOp2dAddOn;

enum {
	GL_ADD_BINARY_SRF_KEY	= 'add_',
	GL_SUB_BINARY_SRF_KEY	= 'sub_',
	GL_MULT_BINARY_SRF_KEY	= 'mult',
	GL_DIV_BINARY_SRF_KEY	= 'div_',
	GL_MIN_BINARY_SRF_KEY	= 'min_',
	GL_MAX_BINARY_SRF_KEY	= 'max_'
};

/***************************************************************************
 * GL-BINARY-OP-2D
 * Combine two nodes based on the rule.
 ***************************************************************************/
class GlBinaryOp2d : public GlNode2d
{
public:
	GlBinaryOp2d(const GlBinaryOp2dAddOn* addon, const BMessage* config);
	GlBinaryOp2d(const GlBinaryOp2d& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode2d			inherited;
	const GlBinaryOp2dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-BINARY-OP-2D-ADD-ON
 ***************************************************************************/
class GlBinaryOp2dAddOn : public GlNode2dAddOn
{
public:
	GlBinaryOp2dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mOp;

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNode2dAddOn	inherited;
};


#endif
