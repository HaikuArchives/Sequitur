/* GlUserNodeAddOn.h
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
 * 2004.03.10				hackborn@angryredplanet.com
 * Created this file.
 */
#ifndef GLKERNEL_GLUSERNODEADDON_H
#define GLKERNEL_GLUSERNODEADDON_H

#include <GlPublic/GlNode.h>

/***************************************************************************
 * GL-USER-NODE-ADD-ON
 * A special constructor addon.  I instantiate new instances of root nodes
 * (although this should turn into standard nodes as soon as the parameters
 * are moved to the param list).  I'm a pass-through addon -- the new nodes
 * are given the GlRootNodeAddOn as their addon.
 ***************************************************************************/
class GlUserNodeAddOn : public GlNodeAddOn
{
public:
	GlUserNodeAddOn(const BMessage& msg);

	virtual GlNode*			NewInstance(const BMessage* config) const;
	virtual uint32			Io() const;

	status_t				InitCheck() const;

private:
	typedef GlNodeAddOn		inherited;
	status_t				mStatus;
	BMessage				mConfig;
	uint32					mIo;
};

#endif
