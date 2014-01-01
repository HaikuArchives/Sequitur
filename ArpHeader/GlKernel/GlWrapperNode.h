/* GlWrapperNode.h
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
 * 2004.03.03				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLPUBLIC_GLWRAPPERNODE_H
#define GLPUBLIC_GLWRAPPERNODE_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-WRAPPER-NODE
 * A node that wraps a chain (typically this chain came from a root
 * node, which always has one and only one chain).  Since I'm used to
 * load root nodes from disk and insert them into a chain, I keep track
 * of the original root's param info, like creator, key, etc.
 ***************************************************************************/
class GlWrapperNode : public GlNode
{
public:
	GlWrapperNode(const GlNodeAddOn* addon);
	GlWrapperNode(const GlWrapperNode& o);
	
	GlNode*						Clone() const;

	virtual uint32				Io() const;
	BString16					Creator() const;
	int32						Key() const;
	BString16					Category() const;
	BString16					Label() const;

	virtual GlAlgo*				Generate(const gl_generate_args& args) const;
	virtual const ArpBitmap*	Image() const;
	virtual float				GetMatch(	const BString16& creator, int32 key,
											int32* major = 0, int32* minor = 0) const;

private:
	typedef GlNode				inherited;
	
public:
	void						Print(uint32 tabs = 0) const;
};

/***************************************************************************
 * GL-ROOT-NODE-ADD-ON
 * This isn't used with any of the system nodes -- this is an alternative
 * addon for when the root is loaded from a file.
 ***************************************************************************/
class GlWrapperNodeAddOn : public GlNodeAddOn
{
public:
	GlWrapperNodeAddOn();

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const		{ return 0; }

private:
	typedef GlNodeAddOn		inherited;
};

#endif
