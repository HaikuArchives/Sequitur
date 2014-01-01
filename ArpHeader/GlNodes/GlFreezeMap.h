/* GlFreezeMap.h
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
 * 2004.04.02				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLFREEZEAP_H
#define GLNODES_GLFREEZEMAP_H

#include <GlPublic/GlAlgoNodes.h>
class GlFreezeMapAddOn;

/***************************************************************************
 * GL-FREEZE-MAP
 * Turn off morphing and/or randomness of anyone I wrap.
 ***************************************************************************/
class GlFreezeMap : public GlNode1d
{
public:
	GlFreezeMap(const GlFreezeMapAddOn* addon, const BMessage* config);
	GlFreezeMap(const GlFreezeMap& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
	const GlFreezeMapAddOn*	mAddOn;
};

/***************************************************************************
 * GL-FREEZE-MAP-ADD-ON
 ***************************************************************************/
class GlFreezeMapAddOn : public GlNode1dAddOn
{
public:
	GlFreezeMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mMorph;
	const GlParamType*		mRandom;

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
