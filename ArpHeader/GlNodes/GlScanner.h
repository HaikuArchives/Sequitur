/* GlScanner.h
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
 * 2004.03.17				hackborn@angryredplanet.com
 * Moved to new (final for 1.0?) framework -- chaining model.
 *
 * 2003.03.08				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLSCANNER_H
#define GLNODES_GLSCANNER_H

#include <GlPublic/GlPlaneNode.h>
class GlScannerAddOn;

/***************************************************************************
 * GL-SCANNER
 * Run a line of any angle through a plane.  The line is warped based
 * on a map.
 ***************************************************************************/
class GlScanner : public GlPlaneNode
{
public:
	GlScanner(const GlScannerAddOn* addon, const BMessage* config);
	GlScanner(const GlScanner& o);
	
	virtual GlNode*			Clone() const;
	virtual GlRecorder*		NewRecorder(const GlRootRef& ref);
	virtual BView*			NewView(gl_new_view_params& params) const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlPlaneNode		inherited;
	const GlScannerAddOn*	mAddOn;
};

/***************************************************************************
 * GL-SCANNER-ADD-ON
 ***************************************************************************/
class GlScannerAddOn : public GlNodeAddOn
{
public:
	GlScannerAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO | GL_2D_IO; }
	virtual uint32			Flags() const	{ return GL_RECORDER_F | GL_PIXEL_TARGETS_F; }

	const GlParamType*		mMode;			// Scanning mode
	const GlParamType*		mAngle;			// The angle of the scan
	const GlParamType*		mLengthAbs;		// Absolute length of a scan
	const GlParamType*		mLengthRel;		// Relative length of a scan
	const GlParamType*		mResolution;	// The resolution of the sequence

private:
	typedef GlNodeAddOn		inherited;
};


#endif
