/* GlEnvelope1d.h
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
 * 2003.02.21				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLENVELOPE1D_H
#define GLNODES_GLENVELOPE1D_H

#include <GlPublic/GlAlgoNodes.h>
class GlEnvelope1dAddOn;

/* A convenience for a part of the UI that uses envelopes.
 */
GlAlgo1d*	gl_new_linear_envelope(float start = 0, float end = 1);

/***************************************************************************
 * GL-ENVELOPE-1D
 * A generic envelope with user-assigned points.
 ***************************************************************************/
class GlEnvelope1d : public GlNode1d
{
public:
	GlEnvelope1d(const GlEnvelope1dAddOn* addon, const BMessage* config);
	GlEnvelope1d(const GlEnvelope1d& o);
	
	virtual GlNode*				Clone() const;
	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
	virtual BView*				NewView(gl_new_view_params& params) const;

private:
	typedef GlNode1d			inherited;
	const GlEnvelope1dAddOn*	mAddOn;
};

/***************************************************************************
 * GL-ENVELOPE-1D-ADD-ON
 * INPUTS:
 *		1D - 0. 		Any input curve to add to.
 * OUTPUTS:
 *		1D - 0.			My curve.
 ***************************************************************************/
class GlEnvelope1dAddOn : public GlNode1dAddOn
{
public:
	GlEnvelope1dAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;

	const GlParamType*		mStart;		// The starting value
	const GlParamType*		mEnd;		// The ending value
	const GlParamType*		mStage;		// Multiple stage params are allowed
	const GlParamType*		mCurve;		// The curve of the current stage
	
private:
	typedef GlNode1dAddOn	inherited;
};


#endif
