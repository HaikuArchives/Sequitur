/* GlStairMap.h
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
 * 2003.02.21				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLSTAIRMAP_H
#define GLNODES_GLSTAIRMAP_H

#include <GlPublic/GlAlgoNodes.h>
class GlStairMapAddOn;

/***************************************************************************
 * GL-STAIR-MAP
 * Quantize
 ***************************************************************************/
class GlStairMap : public GlNode1d
{
public:
	GlStairMap(const GlStairMapAddOn* addon, const BMessage* config);
	GlStairMap(const GlStairMap& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlStairMapAddOn*		mAddOn;
};

/***************************************************************************
 * GL-STAIR-MAP-ADD-ON
 ***************************************************************************/
class GlStairMapAddOn : public GlNode1dAddOn
{
public:
	GlStairMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mSteps;		// Number of steps, 1 to 255

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
