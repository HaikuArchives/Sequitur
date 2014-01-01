/* GlSequenceMap.h
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
 * 2004.03.30				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLSEQUENCEMAP_H
#define GLNODES_GLSEQUENCEMAP_H

#include <GlPublic/GlMapList.h>
#include <GlPublic/GlAlgoNodes.h>
class GlSequenceMapAddOn;

/***************************************************************************
 * GL-SEQUENCE-MAP
 * I store a series of maps and interpolate between them.
 ***************************************************************************/
class GlSequenceMap : public GlNode1d
{
public:
	GlSequenceMap(const GlSequenceMapAddOn* addon, const BMessage* config);
	GlSequenceMap(const GlSequenceMap& o);
	
	virtual GlNode*				Clone() const;
	virtual status_t			SetMapParam(const GlAlgo1d* map, int32 index,
											const char* name);
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;

private:
	typedef GlNode1d			inherited;
	const GlSequenceMapAddOn*	mAddOn;
	mutable GlMapList			mMaps;
};

/***************************************************************************
 * GL-SEQUENCE-MAP-ADD-ON
 ***************************************************************************/
class GlSequenceMapAddOn : public GlNode1dAddOn
{
public:
	GlSequenceMapAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mFrames;	// How many frames to store

private:
	typedef GlNode1dAddOn	inherited;
};


#endif
