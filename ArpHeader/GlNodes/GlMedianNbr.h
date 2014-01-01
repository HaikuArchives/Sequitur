/* GlMedianNbr.h
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
 * 2004.04.23				hackborn@angryredplanet.com
 * Ported to chaining framework.
 *
 * 2003.03.28				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLMEDIANNBR_H
#define GLNODES_GLMEDIANNBR_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-MEDIAN-NBR
 * Find the median of the number set.
 ***************************************************************************/
class GlMedianNbr : public GlNode
{
public:
	GlMedianNbr(const GlNodeAddOn* addon, const BMessage* config);
	GlMedianNbr(const GlMedianNbr& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-MEDIAN-NBR-ADD-ON
 ***************************************************************************/
class GlMedianNbrAddOn : public GlNodeAddOn
{
public:
	GlMedianNbrAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_NUMBER_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
