/* GlPromote.h
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
 * 2003.03.08				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLPROMOTE_H
#define GLNODES_GLPROMOTE_H

#include <GlPublic/GlPlaneNode.h>
class GlPromoteAddOn;

/***************************************************************************
 * GL-PROMOTE
 * Combine two map objects into one surface.
 *
 * Chain output:
 *		1D - 0.				The X filler.
 *		1D - 1.				The Y filler.
 *		1D - 2.				The tie breaker, for modes that require it.
 ***************************************************************************/
class GlPromote : public GlPlaneNode
{
public:
	GlPromote(const GlPromoteAddOn* addon, const BMessage* config);
	GlPromote(const GlPromote& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
	const GlPromoteAddOn*	mAddOn;
};

/***************************************************************************
 * GL-PROMOTE-ADD-ON
 ***************************************************************************/
class GlPromoteAddOn : public GlNodeAddOn
{
public:
	GlPromoteAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_PIXEL_TARGETS_F; }

	const GlParamType*		mOp;		// The operation -- average, min etc.

private:
	typedef GlNodeAddOn		inherited;
};


#endif
