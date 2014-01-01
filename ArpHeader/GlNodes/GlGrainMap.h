/* GlGrainMap.h
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
#ifndef GLNODES_GLGRAINMAP_H
#define GLNODES_GLGRAINMAP_H

#include <GlPublic/GlAlgoNodes.h>
class GlGrainMapAddOn;

/***************************************************************************
 * GL-GRAIN-MAP
 * Akin to granulary synthesis:  Take the input and replicate it at
 * various sizes.
 ***************************************************************************/
class GlGrainMap : public GlNode1d
{
public:
	GlGrainMap(const GlGrainMapAddOn* addon, const BMessage* config);
	GlGrainMap(const GlGrainMap& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d		inherited;
	const GlGrainMapAddOn*	mAddOn;
};

/***************************************************************************
 * GL-GRAIN-MAP-ADD-ON
 ***************************************************************************/
class GlGrainMapAddOn : public GlNode1dAddOn
{
public:
	GlGrainMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mMin;		// The smallest grain size
	const GlParamType*		mMax;		// The largest grain size

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
