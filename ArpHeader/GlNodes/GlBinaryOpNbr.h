/* GlBinaryOpNbr.h
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
#ifndef GLNODES_GLBINARYOPNBR_H
#define GLNODES_GLBINARYOPNBR_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-BINARY-OP-NBR
 * Combine two nodes based on the rule.
 ***************************************************************************/
class GlBinaryOpNbr : public GlNode
{
public:
	GlBinaryOpNbr(const GlNodeAddOn* addon, const BMessage* config);
	GlBinaryOpNbr(const GlBinaryOpNbr& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-BINARY-OP-NBR-ADD-ON
 ***************************************************************************/
class GlBinaryOpNbrAddOn : public GlNodeAddOn
{
public:
	GlBinaryOpNbrAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_NUMBER_IO; }

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
};


#endif
