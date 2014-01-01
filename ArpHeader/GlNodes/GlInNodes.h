/* GlInNodes.h
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
 * 2004.03.08				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLNODES_GLINNODES_H
#define GLNODES_GLINNODES_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-IN
 ***************************************************************************/
class GlIn : public GlNode
{
public:
	GlIn(const GlNodeAddOn* addon, const BMessage* config);
	GlIn(const GlIn& o);
	
	virtual GlNode*			Clone() const;
	virtual GlAlgo*			Generate(const gl_generate_args& args) const;

private:
	typedef GlNode			inherited;
};

/***************************************************************************
 * GL-IN-ADD-ON
 ***************************************************************************/
class GlInAddOn : public GlNodeAddOn
{
public:
	GlInAddOn(uint32 io);

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const;

protected:
//	virtual GlImage*		NewImage() const;

private:
	typedef GlNodeAddOn		inherited;
	uint32					mIo;
};

/***************************************************************************
 * GL-IN-NBR-ADD-ON
 ***************************************************************************/
class GlInNbrAddOn : public GlNodeAddOn
{
public:
	GlInNbrAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const	{ return GL_NUMBER_IO; }

private:
	typedef GlNodeAddOn		inherited;
};


#endif
