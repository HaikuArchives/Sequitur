/* GlSumNbr.h
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
 * 2003.03.28				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLSUMNBR_H
#define GLNODES_GLSUMNBR_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-SUM-NBR
 * Sum any number set I receive.
 ***************************************************************************/
class GlSumNbr : public GlNode
{
public:
	GlSumNbr(const GlNodeAddOn* addon, const BMessage* config);
	GlSumNbr(const GlSumNbr& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-SUM-NBR-ADD-ON
 ***************************************************************************/
class GlSumNbrAddOn : public GlNodeAddOn
{
public:
	GlSumNbrAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_NUMBER_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
