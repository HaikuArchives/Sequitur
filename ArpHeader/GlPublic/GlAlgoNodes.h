/* GlAlgoNodes.h
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
 * 2004.04.14				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLALGONODES_H
#define GLPUBLIC_GLALGONODES_H

#include <GlPublic/GlNode.h>
class GlAlgoIm;

/***************************************************************************
 * GL-NODE-1D
 * This convenience class handles the messy work of generating a preview
 * image from a curve.
 ***************************************************************************/
class GlNode1d : public GlNode
{
public:
	GlNode1d(const GlNodeAddOn* addon, const BMessage* config);
	GlNode1d(const GlNode1d& o);
	
	virtual BView*				NewView(gl_new_view_params& params) const;
	virtual GlNodeVisual*		NewVisual(const GlRootRef& ref) const;

private:
	typedef GlNode				inherited;
};

/***************************************************************************
 * GL-NODE-1D-ADD-ON
 ***************************************************************************/
class GlNode1dAddOn : public GlNodeAddOn
{
protected:
	GlNode1dAddOn(	const BString16& creator, int32 key,
					const BString16* category, const BString16* label,
					int32 majorVersion, int32 minorVersion);

	virtual uint32			Io() const	{ return GL_1D_IO; }

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
};


/***************************************************************************
 * GL-NODE-2D
 * This convenience class handles the messy work of generating a preview
 * image from a surface.
 ***************************************************************************/
class GlNode2d : public GlNode
{
public:
	GlNode2d(const GlNodeAddOn* addon, const BMessage* config);
	GlNode2d(const GlNode2d& o);
	
	virtual BView*				NewView(gl_new_view_params& params) const;
	virtual GlNodeVisual*		NewVisual(const GlRootRef& ref) const;

private:
	typedef GlNode				inherited;
};

/***************************************************************************
 * GL-NODE-2D-ADD-ON
 ***************************************************************************/
class GlNode2dAddOn : public GlNodeAddOn
{
protected:
	GlNode2dAddOn(	const BString16& creator, int32 key,
					const BString16* category, const BString16* label,
					int32 majorVersion, int32 minorVersion);

	virtual uint32			Io() const	{ return GL_2D_IO; }

protected:
	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
};


/***************************************************************************
 * GL-NODE-IM
 * A superclass for nodes that generate GlAlgoIm objects.
 ***************************************************************************/
class GlNodeIm : public GlNode
{
public:
	GlNodeIm(const GlNodeAddOn* addon, const BMessage* config);

private:
	typedef GlNode				inherited;
};

/***************************************************************************
 * GL-NODE-IM-ADD-ON
 ***************************************************************************/
class GlNodeImAddOn : public GlNodeAddOn
{
protected:
	GlNodeImAddOn(	const BString16& creator, int32 key,
					const BString16* category, const BString16* label,
					int32 majorVersion, int32 minorVersion);

	virtual uint32			Io() const		{ return GL_IMAGE_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
