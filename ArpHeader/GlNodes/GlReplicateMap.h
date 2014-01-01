/* GlReplicateMap.h
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
 * 2003.02.20				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLREPLICATEMAP_H
#define GLNODES_GLREPLICATEMAP_H

#include <GlPublic/GlAlgoNodes.h>
class GlReplicateMapAddOn;

/***************************************************************************
 * GL-REPLICATE-MAP
 * Take an existing map and repeat it on top of itself.
 ***************************************************************************/
class GlReplicateMap : public GlNode1d
{
public:
	GlReplicateMap(const GlReplicateMapAddOn* addon, const BMessage* config);
	GlReplicateMap(const GlReplicateMap& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlReplicateMapAddOn*	mAddOn;
};

/***************************************************************************
 * GL-REPLICATE-MAP-ADD-ON
 ***************************************************************************/
class GlReplicateMapAddOn : public GlNode1dAddOn
{
public:
	GlReplicateMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mDepth;		// Number of replications

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
