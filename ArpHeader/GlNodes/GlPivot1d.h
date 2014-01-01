/* GlPivot1d.h
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
 * 2003.03.09				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLPIVOT1D_H
#define GLNODES_GLPIVOT1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlPivot1dAddOn;

/***************************************************************************
 * GL-PIVOT-1D
 * Generate a curve that's a simple line pivoted along a center point.
 * This is the basis for contrast when used in point processing.
 ***************************************************************************/
class GlPivot1d : public GlNode1d
{
public:
	GlPivot1d(const GlPivot1dAddOn* addon, const BMessage* config);
	GlPivot1d(const GlPivot1d& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
	const GlPivot1dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-PIVOT-1D-ADD-ON
 ***************************************************************************/
class GlPivot1dAddOn : public GlNode1dAddOn
{
public:
	GlPivot1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mAngle;		// The angle, 0 - 180.  0 flat, 90 straight up
	const GlParamType*		mLoc;		// The location of the pivot point

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
