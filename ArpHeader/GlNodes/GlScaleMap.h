/* GlScaleMap.h
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
#ifndef GLNODES_GLSCALEMAP_H
#define GLNODES_GLSCALEMAP_H

#include <GlPublic/GlAlgoNodes.h>
class GlScaleMapAddOn;

/***************************************************************************
 * GL-SCALE-MAP
 * Scale the value down to my range
 ***************************************************************************/
class GlScaleMap : public GlNode1d
{
public:
	GlScaleMap(const GlScaleMapAddOn* addon, const BMessage* config);
	GlScaleMap(const GlScaleMap& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
	const GlScaleMapAddOn*	mAddOn;
};

/***************************************************************************
 * GL-SCALE-MAP-ADD-ON
 ***************************************************************************/
class GlScaleMapAddOn : public GlNode1dAddOn
{
public:
	GlScaleMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mLow;
	const GlParamType*		mHigh;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
