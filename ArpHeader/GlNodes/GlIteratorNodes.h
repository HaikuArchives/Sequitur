/* GlIteratorNodes.h
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
 * 2004.04.27				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLITERATORNODES_H
#define GLNODES_GLITERATORNODES_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-ITERATOR-IM
 ***************************************************************************/
class GlIteratorIm : public GlNode
{
public:
	GlIteratorIm(const GlNodeAddOn* addon, const BMessage* config);
	GlIteratorIm(const GlIteratorIm& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;
	virtual BView*			NewView(gl_new_view_params& params) const;

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-ITERATOR-IM-ADD-ON
 ***************************************************************************/
class GlIteratorImAddOn : public GlNodeAddOn
{
public:
	GlIteratorImAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return GL_IMAGE_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
