/* GlAlgoIm.h
 * Copyright (c)2004 by Eric Hackborn.
 * All rights reserved.
 *
 * This code is not public domain, nor freely distributable.
 * Please direct any questions or requests to Eric Hackborn,
 * at <hackborn@angryredplanet.com>.
 *
 * ----------------------------------------------------------------------
 *
 * Known Bugs
 * ~~~~~~~~~~
 *	- None!  Ha ha!
 *
 * ----------------------------------------------------------------------
 *
 * History
 * ~~~~~~~
 * 2004.04.14			hackborn@angryredplanet.com
 * Created this file
 */
#ifndef GLPUBLIC_GLALGOIM_H
#define GLPUBLIC_GLALGOIM_H

#include <GlPublic/GlAlgo.h>

/***************************************************************************
 * GL-SURFACE
 * I represent an unbounded, generated area.  Subclasses fill the area as
 * appropriate, which can either be one (a path) or two (a mask) dimensional.
 ****************************************************************************/
class GlAlgoIm : public GlAlgo
{
public:
	virtual ~GlAlgoIm();

	/* ACCESSING
	   ---------- */
	virtual uint32			Io() const		{ return GL_IMAGE_IO; }

	/* CONVERTING
	   --------- */
	virtual GlAlgoIm*		AsIm();
	virtual const GlAlgoIm*	AsIm() const;

protected:
	GlAlgoIm(gl_node_id nid, int32 token = GL_NO_TOKEN);
	GlAlgoIm(const GlAlgoIm& o);
	
private:
	typedef GlAlgo			inherited;
};

#endif
